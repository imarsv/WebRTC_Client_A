//
// Created by mars on 12.08.18.
//

#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <modules/video_capture/video_capture.h>
#include "api/create_peerconnection_factory.h"
#include <modules/video_capture/video_capture_factory.h>
#include <iostream>
#include <json/json.h>
#include "pc/video_track_source.h"
#include "test/vcm_capturer.h"

#include "WebcamStreamerConductor.h"
#include "MediaConstraints.h"

const auto audioLabel = "audio_label";
const auto videoLabel = "video_label";
const auto streamId = "stream_id";
const auto sessionDescriptionTypeName = "type";
const auto sessionDescriptionSdpName = "sdp";

class WebcamCapturerTrackSource : public webrtc::VideoTrackSource {
public:
  static rtc::scoped_refptr<WebcamCapturerTrackSource> Create() {
    const size_t kWidth = 1280;
    const size_t kHeight = 720;
    const size_t kFps = 30;
    std::unique_ptr<webrtc::test::VcmCapturer> capturer;
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
        webrtc::VideoCaptureFactory::CreateDeviceInfo());
    if (!info) {
      return nullptr;
    }
    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i) {
      capturer = std::unique_ptr<webrtc::test::VcmCapturer>(webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
      if (capturer) {
        return new rtc::RefCountedObject<WebcamCapturerTrackSource>(
            std::move(capturer));
      }
    }

    return nullptr;
  }

protected:
  explicit WebcamCapturerTrackSource(
      std::unique_ptr<webrtc::test::VcmCapturer> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capturer_.get();
  }
  std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
};


class DummySetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
public:
  static DummySetSessionDescriptionObserver *Create() {
    return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
  }

  virtual void OnSuccess() { RTC_LOG(INFO) << __FUNCTION__; }

  virtual void OnFailure(webrtc::RTCError error) {
    RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
                  << error.message();
  }
};


WebcamStreamerConductor::WebcamStreamerConductor(SignalingManager *signalingManager) :
    signalingManager(signalingManager) {
  this->signalingManager->registerObserver(this);
}

void WebcamStreamerConductor::connect() {
  RTC_LOG(INFO) << "WebcamStreamerConductor::connect()";

  if (peerConnection.get()) {
    throw std::runtime_error("We only support connecting to one peer at a time");
  }

  if (initializePeerConnection()) {
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions();
    options.offer_to_receive_audio = 0;
    options.offer_to_receive_video = 0;
    peerConnection->CreateOffer(this, options);
  } else {
    RTC_LOG(LS_ERROR) << "Failed to initialize PeerConnection";
  }
}

bool WebcamStreamerConductor::initializePeerConnection() {
  RTC_LOG(INFO) << "WebcamStreamerConductor::initializePeerConnection()";

  RTC_DCHECK(!peerConnectionFactory);
  RTC_DCHECK(!peerConnection);

  peerConnectionFactory = webrtc::CreatePeerConnectionFactory(
      nullptr /* network_thread */, nullptr /* worker_thread */,
      nullptr /* signaling_thread */, nullptr /* default_adm */,
      webrtc::CreateBuiltinAudioEncoderFactory(),
      webrtc::CreateBuiltinAudioDecoderFactory(),
      webrtc::CreateBuiltinVideoEncoderFactory(),
      webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
      nullptr /* audio_processing */);

  if (!peerConnectionFactory) {
    RTC_LOG(LS_ERROR) << "Failed to initialize PeerConnectionFactory";
    deletePeerConnection();
    return false;
  }

  if (!createPeerConnection(/*dtls=*/true)) {
    RTC_LOG(LS_ERROR) << "CreatePeerConnection failed";
    deletePeerConnection();
  }

  addTracks();

  return peerConnection != nullptr;
}

bool WebcamStreamerConductor::createPeerConnection(bool dtls) {
  RTC_LOG(INFO) << "WebcamStreamerConductor::createPeerConnection(bool dtls = " << dtls << ")";

  RTC_DCHECK(peerConnectionFactory);
  RTC_DCHECK(!peerConnection);

  webrtc::PeerConnectionInterface::RTCConfiguration config;
  config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
  config.enable_dtls_srtp = dtls;

//  config.set_experiment_cpu_load_estimator(true);

//  webrtc::PeerConnectionInterface::IceServer server;
//  server.uri = GetPeerConnectionString();
//  config.servers.push_back(server);

  peerConnection = peerConnectionFactory->CreatePeerConnection(config, nullptr, nullptr, this);

  return peerConnection != nullptr;
}

void WebcamStreamerConductor::deletePeerConnection() {
  RTC_LOG(INFO) << "WebcamStreamerConductor::deletePeerConnection()";

  peerConnection = nullptr;
  peerConnectionFactory = nullptr;
}

void WebcamStreamerConductor::addTracks() {
  RTC_LOG(INFO) << "WebcamStreamerConductor::addTracks()";

  if (!peerConnection->GetSenders().empty()) {
    return;  // Already added tracks.
  }

  rtc::scoped_refptr<webrtc::AudioTrackInterface> audioTrack(
      peerConnectionFactory->CreateAudioTrack(audioLabel, peerConnectionFactory->CreateAudioSource(cricket::AudioOptions())));

  auto resultOrError = peerConnection->AddTrack(audioTrack, {streamId});
  if (!resultOrError.ok()) {
    RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
                      << resultOrError.error().message();
  }

  rtc::scoped_refptr<WebcamCapturerTrackSource> video_device = WebcamCapturerTrackSource::Create();
  if (video_device) {
    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(peerConnectionFactory->CreateVideoTrack(videoLabel, video_device));
//    main_wnd_->StartLocalRenderer(video_track_);

    resultOrError = peerConnection->AddTrack(video_track_, {streamId});
    if (!resultOrError.ok()) {
      RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
                        << resultOrError.error().message();
    }
  } else {
    RTC_LOG(LS_ERROR) << "OpenVideoCaptureDevice failed";
  }
}

/*
 * PeerConnectionObserver
 */
void WebcamStreamerConductor::OnMessage(const std::string &message) {
//  RTC_LOG(INFO)
//    << "WebcamStreamerConductor::OnMessage(const std::string &message = " << message << ")";

  Json::Reader reader;

  Json::Value jmessage;
  if (!reader.parse(message, jmessage)) {
    RTC_LOG(WARNING) << "Received unknown message. " << message;
    return;
  }

  if (!jmessage.isMember("type")) {
    RTC_LOG(WARNING) << "Received unknown message. " << message;
    return;
  }

  auto type = jmessage["type"].asString();

  if (type == "on_answer_sdp") {
    if (!jmessage.isMember("answer_sdp")) {
      RTC_LOG(WARNING) << "Invalid [ " << type << " ] format (" << message << ")";
      return;
    }

    auto sdp = jmessage["answer_sdp"].asString();

    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::SessionDescriptionInterface> sessionDescription =
        webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp, &error);

    if (!sessionDescription) {
      RTC_LOG(WARNING) << "Can't parse received session description message. "
                       << "SdpParseError was: " << error.description;
      return;
    }

    RTC_LOG(INFO) << " Received session description :" << sdp;

    peerConnection->SetRemoteDescription(DummySetSessionDescriptionObserver::Create(), sessionDescription.release());
  } else if (type == "on_video_bitrate") {
    if (!jmessage.isMember("video_bitrate")) {
      RTC_LOG(WARNING) << "Invalid [ " << type << " ] format (" << message << ")";
      return;
    }

    RTC_LOG(INFO) << " Video bitrate: " << jmessage["video_bitrate"].asLargestInt();
  } else if (type == "on_error") {
    if (!jmessage.isMember("message")) {
      RTC_LOG(WARNING) << "Invalid [ " << type << " ] format (" << message << ")";
      return;
    }

    RTC_LOG(LS_ERROR) << jmessage["message"];
  } else if (type == "on_media_receive") {
    RTC_LOG(INFO) << "Server media: " << message;
  } else {
    RTC_LOG(INFO) << "Received [ " << type << " ] unprocessed (" << message << ")";
  }
}

/*
 * webrtc::PeerConnectionObserver
 */
void WebcamStreamerConductor::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
  RTC_LOG(INFO)
    << "WebcamStreamerConductor::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state = "
    << new_state << ")";

}

void WebcamStreamerConductor::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {
  RTC_LOG(INFO)
    << "WebcamStreamerConductor::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel)";
}

void WebcamStreamerConductor::OnRenegotiationNeeded() {
  RTC_LOG(INFO) << "WebcamStreamerConductor::OnRenegotiationNeeded()";
}

void WebcamStreamerConductor::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  RTC_LOG(INFO)
    << "WebcamStreamerConductor::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state = "
    << new_state << ")";
}

void WebcamStreamerConductor::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {
  RTC_LOG(INFO)
    << "WebcamStreamerConductor::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)";
}

void WebcamStreamerConductor::OnIceCandidate(const webrtc::IceCandidateInterface *candidate) {
  RTC_LOG(INFO) << "WebcamStreamerConductor::OnIceCandidate(const webrtc::IceCandidateInterface *candidate)";
}

/*
 * public webrtc::CreateSessionDescriptionObserver
 */
void WebcamStreamerConductor::OnSuccess(webrtc::SessionDescriptionInterface *desc) {
  RTC_LOG(INFO) << "WebcamStreamerConductor::OnSuccess(webrtc::SessionDescriptionInterface *desc)";

  peerConnection->SetLocalDescription(DummySetSessionDescriptionObserver::Create(), desc);

  std::string sdp;
  desc->ToString(&sdp);

//  // For loopback test. To save some connecting delay.
//  if (loopback_) {
//    // Replace message type from "offer" to "answer"
//    std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
//        webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp);
//    peerConnection->SetRemoteDescription(
//        DummySetSessionDescriptionObserver::Create(),
//        session_description.release());
//    return;
//  }

  RTC_LOG(INFO) << " SDP Offer :" << sdp;

  Json::StyledWriter writer;
  {
    Json::Value jmessage;
    jmessage["type"] = "video_bitrate";
    jmessage["video_bitrate"] = 2000000;

    signalingManager->send(writer.write(jmessage));
  }

  {
    Json::Value jmessage;
    jmessage["type"] = "offer_sdp";
    jmessage["offer_sdp"] = sdp;

    signalingManager->send(writer.write(jmessage));
  }
}

void WebcamStreamerConductor::OnFailure(webrtc::RTCError error) {
  RTC_LOG(INFO) << "WebcamStreamerConductor::OnFailure(webrtc::RTCError error)";
  RTC_LOG(LERROR) << ToString(error.type()) << ": " << error.message();
}