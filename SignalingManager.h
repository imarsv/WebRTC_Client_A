//
// Created by mars on 11.08.18.
//

#ifndef WEBRTC_CLIENT_1_SIGNALINGMANAGER_H
#define WEBRTC_CLIENT_1_SIGNALINGMANAGER_H

#include <thread>
#include <unordered_map>
#include <queue>
#include <mutex>
#include "mongoose.h"

static std::unordered_map<unsigned, std::string> mg_event_names = {
    {MG_EV_POLL,                        "MG_EV_POLL"},
    {MG_EV_ACCEPT,                      "MG_EV_ACCEPT"},
    {MG_EV_CONNECT,                     "MG_EV_CONNECT"},
    {MG_EV_RECV,                        "MG_EV_RECV"},
    {MG_EV_SEND,                        "MG_EV_SEND"},
    {MG_EV_CLOSE,                       "MG_EV_CLOSE"},
    {MG_EV_TIMER,                       "MG_EV_TIMER"},

    {MG_EV_HTTP_REQUEST,                "MG_EV_HTTP_REQUEST"},
    {MG_EV_HTTP_REPLY,                  "MG_EV_HTTP_REPLY"},
    {MG_EV_HTTP_CHUNK,                  "MG_EV_HTTP_CHUNK"},
    {MG_EV_SSI_CALL,                    "MG_EV_SSI_CALL"},
    {MG_EV_SSI_CALL_CTX,                "MG_EV_SSI_CALL_CTX"},

    {MG_EV_WEBSOCKET_HANDSHAKE_REQUEST, "MG_EV_WEBSOCKET_HANDSHAKE_REQUEST"},
    {MG_EV_WEBSOCKET_HANDSHAKE_DONE,    "MG_EV_WEBSOCKET_HANDSHAKE_DONE"},
    {MG_EV_WEBSOCKET_FRAME,             "MG_EV_WEBSOCKET_FRAME"},
    {MG_EV_WEBSOCKET_CONTROL_FRAME,     "MG_EV_WEBSOCKET_CONTROL_FRAME"}
};

struct PeerConnectionObserver {
  virtual void OnMessage(const std::string &message) = 0;

protected:
  virtual ~PeerConnectionObserver() {}
};

class SignalingManager {
public:
//  SignalingManager() noexcept = default;

  virtual ~SignalingManager() noexcept;

//  SignalingManager(const SignalingManager &) = delete;
//
//  SignalingManager &operator=(const SignalingManager &) = delete;

  void registerObserver(PeerConnectionObserver *callback);

  void setURL(const std::string &url);

  void send(const std::string &message);

  void run();

private:
  void webSocketThreadPooling();
  void observerThreadPooling();

  static void requestHandler(struct mg_connection *nc, int ev, void *ev_data);

  std::thread webSocketThread;
  std::thread observerThread;
  bool isTerminating = false;

  std::unique_ptr<mg_mgr> context;

  std::string url;

  PeerConnectionObserver *callback = nullptr;

  mutable std::mutex outgoingMessagesMutex;
  std::queue<std::string> outgoingMessages;

  mutable std::mutex ingoingMessagesMutex;
  std::queue<std::string> ingoingMessages;
};


#endif //WEBRTC_CLIENT_1_SIGNALINGMANAGER_H
