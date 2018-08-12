//
// Created by mars on 12.08.18.
//

#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <modules/video_capture/video_capture.h>
#include <media/engine/webrtcvideocapturerfactory.h>
#include <modules/video_capture/video_capture_factory.h>
#include <iostream>

#include "WebcamStreamerConductor.h"

const auto audioLabel = "audio_label";
const auto videoLabel = "video_label";
const auto streamId = "stream_id";

void WebcamStreamerConductor::connect() {
  RTC_LOG(INFO) << "WebcamStreamerConductor::connect()";

  if (peerConnection.get()) {
    throw std::runtime_error("We only support connecting to one peer at a time");
  }

  if (initializePeerConnection()) {
    peerConnection->CreateOffer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
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
      peerConnectionFactory->CreateAudioTrack(
          audioLabel, peerConnectionFactory->CreateAudioSource(cricket::AudioOptions())));

  auto resultOrError = peerConnection->AddTrack(audioTrack, {streamId});
  if (!resultOrError.ok()) {
    RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
                      << resultOrError.error().message();
  }

  std::unique_ptr<cricket::VideoCapturer> videoCaptureDevice = openVideoCaptureDevice();
  if (videoCaptureDevice) {
    rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack(
        peerConnectionFactory->CreateVideoTrack(
            videoLabel, peerConnectionFactory->CreateVideoSource(std::move(videoCaptureDevice), nullptr)));

    resultOrError = peerConnection->AddTrack(videoTrack, {streamId});
    if (!resultOrError.ok()) {
      RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
                        << resultOrError.error().message();
    }
  } else {
    RTC_LOG(LS_ERROR) << "openVideoCaptureDevice failed";
  }
}

std::unique_ptr<cricket::VideoCapturer> WebcamStreamerConductor::openVideoCaptureDevice() {
  RTC_LOG(INFO) << "WebcamStreamerConductor::openVideoCaptureDevice()";

  std::vector<std::string> device_names;

  {
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(webrtc::VideoCaptureFactory::CreateDeviceInfo());
    if (!info) {
      return nullptr;
    }

    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i) {
      const uint32_t kSize = 256;
      char name[kSize] = {0};
      char id[kSize] = {0};

      if (info->GetDeviceName(i, name, kSize, id, kSize) != -1) {
        device_names.push_back(name);
      }
    }
  }

  cricket::WebRtcVideoDeviceCapturerFactory factory;
  std::unique_ptr<cricket::VideoCapturer> capturer;
  for (const auto &name : device_names) {
    capturer = factory.Create(cricket::Device(name, 0));
    if (capturer) {
      break;
    }
  }

  return capturer;
}


/*
 * webrtc::PeerConnectionObserver
 */
void WebcamStreamerConductor::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
  RTC_LOG(INFO) << "WebcamStreamerConductor::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state)";

}

void WebcamStreamerConductor::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {
  RTC_LOG(INFO) << "WebcamStreamerConductor::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel)";
}

void WebcamStreamerConductor::OnRenegotiationNeeded() {
  RTC_LOG(INFO) << "WebcamStreamerConductor::OnRenegotiationNeeded()";
}

void WebcamStreamerConductor::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  RTC_LOG(INFO)
      << "WebcamStreamerConductor::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state)";
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

//  peerConnection->SetLocalDescription(DummySetSessionDescriptionObserver::Create(), desc);
//
//  std::string sdp;
//  desc->ToString(&sdp);
//
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
//
//  Json::StyledWriter writer;
//  Json::Value jmessage;
//  jmessage[kSessionDescriptionTypeName] = webrtc::SdpTypeToString(desc->GetType());
//  jmessage[kSessionDescriptionSdpName] = sdp;
//
//  std::cout << "--------------------------------------------------" << std::endl
//            << jmessage << std::endl
//            << "--------------------------------------------------" << std::endl;
//
//  SendMessage(writer.write(jmessage));
}

void WebcamStreamerConductor::OnFailure(webrtc::RTCError error) {
  RTC_LOG(INFO) << "WebcamStreamerConductor::OnFailure(webrtc::RTCError error)";
  RTC_LOG(LERROR) << ToString(error.type()) << ": " << error.message();
}