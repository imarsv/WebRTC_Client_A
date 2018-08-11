//
// Created by mars on 11.08.18.
//

#ifndef WEBRTC_CLIENT_1_SIGNALINGMANAGER_H
#define WEBRTC_CLIENT_1_SIGNALINGMANAGER_H

#include <thread>
#include <unordered_map>
#include "mongoose.h"

static std::unordered_map<unsigned, std::string> mg_event_names = {
    {MG_EV_POLL, "MG_EV_POLL"},
    {MG_EV_ACCEPT, "MG_EV_ACCEPT"},
    {MG_EV_CONNECT, "MG_EV_CONNECT"},
    {MG_EV_RECV, "MG_EV_RECV"},
    {MG_EV_SEND, "MG_EV_SEND"},
    {MG_EV_CLOSE, "MG_EV_CLOSE"},
    {MG_EV_TIMER, "MG_EV_TIMER"},

    {MG_EV_HTTP_REQUEST, "MG_EV_HTTP_REQUEST"},
    {MG_EV_HTTP_REPLY, "MG_EV_HTTP_REPLY"},
    {MG_EV_HTTP_CHUNK, "MG_EV_HTTP_CHUNK"},
    {MG_EV_SSI_CALL, "MG_EV_SSI_CALL"},
    {MG_EV_SSI_CALL_CTX, "MG_EV_SSI_CALL_CTX"},

    {MG_EV_WEBSOCKET_HANDSHAKE_REQUEST, "MG_EV_WEBSOCKET_HANDSHAKE_REQUEST"},
    {MG_EV_WEBSOCKET_HANDSHAKE_DONE, "MG_EV_WEBSOCKET_HANDSHAKE_DONE"},
    {MG_EV_WEBSOCKET_FRAME, "MG_EV_WEBSOCKET_FRAME"},
    {MG_EV_WEBSOCKET_CONTROL_FRAME, "MG_EV_WEBSOCKET_CONTROL_FRAME"}
};

class SignalingManager {
public:
  SignalingManager() noexcept = default;

  virtual ~SignalingManager() noexcept;

  SignalingManager(const SignalingManager &) = delete;

  SignalingManager &operator=(const SignalingManager &) = delete;

  void setURL(const std::string &url);

  void run();

private:
  void threadPolling();

  static void requestHandler(struct mg_connection *nc, int ev, void *ev_data);

  std::thread thread;
  bool isTerminating = false;

  std::unique_ptr<mg_mgr> context;

  std::string url;
};


#endif //WEBRTC_CLIENT_1_SIGNALINGMANAGER_H
