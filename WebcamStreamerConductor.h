//
// Created by mars on 12.08.18.
//

#ifndef WEBRTC_CLIENT_1_WEBCAMSTREAMERCONDUCTOR_H
#define WEBRTC_CLIENT_1_WEBCAMSTREAMERCONDUCTOR_H


#include <rtc_base/scoped_ref_ptr.h>
#include <api/peerconnectioninterface.h>

class WebcamStreamerConductor : public webrtc::PeerConnectionObserver,
                                public webrtc::CreateSessionDescriptionObserver {
public:
  void connect();

  //
  // PeerConnectionObserver implementation.
  //
  void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;

  void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;

  void OnRenegotiationNeeded() override;

  void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;

  void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

  void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override;

  //
  // CreateSessionDescriptionObserver implementation.
  //
  void OnSuccess(webrtc::SessionDescriptionInterface *desc) override;

  void OnFailure(webrtc::RTCError error) override;

private:
  bool initializePeerConnection();

  bool createPeerConnection(bool dtls);

  void deletePeerConnection();

  void addTracks();


  std::unique_ptr<cricket::VideoCapturer> openVideoCaptureDevice();

  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peerConnection;
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peerConnectionFactory;
};


#endif //WEBRTC_CLIENT_1_WEBCAMSTREAMERCONDUCTOR_H
