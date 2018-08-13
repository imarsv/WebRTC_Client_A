//
// Created by mars on 11.08.18.
//

#include <iostream>
#include "SignalingManager.h"

using namespace std::chrono_literals;

SignalingManager::~SignalingManager() {
  isTerminating = true;

  if (observerThread.joinable()) {
    observerThread.join();
  }

  if (webSocketThread.joinable()) {
    webSocketThread.join();
  }

  if (context) {
    mg_mgr_free(context.get());
    context.reset();
  }
}

void SignalingManager::setURL(const std::string &url) {
  this->url = url;
}

void SignalingManager::run() {

  context = std::make_unique<mg_mgr>();

  mg_mgr_init(context.get(), nullptr);

  struct mg_connect_opts opts{};
  opts.user_data = this;

  struct mg_connection *nc;
  nc = mg_connect_ws_opt(context.get(), requestHandler, opts, url.c_str(), nullptr, nullptr);
  if (nc == nullptr) {
    throw std::runtime_error("Invalid address");
  }

  webSocketThread = std::thread(&SignalingManager::webSocketThreadPooling, this);
  observerThread = std::thread(&SignalingManager::observerThreadPooling, this);
}

void SignalingManager::webSocketThreadPooling() {
  while (!isTerminating) {
    mg_mgr_poll(context.get(), 1000);
  }
}

void SignalingManager::observerThreadPooling() {
  while (!isTerminating) {
    std::string message;

    {
      std::lock_guard<std::mutex> lock(ingoingMessagesMutex);
      if (!ingoingMessages.empty()) {
        message = ingoingMessages.front();
        ingoingMessages.pop();
      }
    }

    if (message.empty() || (callback == nullptr)) {
      std::this_thread::sleep_for(10ms);
    } else {
      callback->OnMessage(message);
    }
  }
}

void SignalingManager::requestHandler(struct mg_connection *nc, int ev, void *ev_data) {
  struct websocket_message *wm = (struct websocket_message *) ev_data;

  switch (ev) {
    case MG_EV_POLL: {
      auto *manager = reinterpret_cast<SignalingManager *>(nc->user_data);
      std::string message;
      {
        std::lock_guard<std::mutex> lock(manager->outgoingMessagesMutex);
        if (!manager->outgoingMessages.empty()) {
          message = manager->outgoingMessages.front();
          manager->outgoingMessages.pop();
        }
      }

      if (!message.empty()) {
        std::cout << "SignalingManager outgoing" << std::endl;
//        std::cout << message << std::endl << std::endl;
        std::cout << std::flush;

        mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, message.c_str(), message.length());
      }
    }
      break;

    case MG_EV_WEBSOCKET_FRAME : {
      auto *manager = reinterpret_cast<SignalingManager *>(nc->user_data);

      auto message = std::string(reinterpret_cast<const char *>(wm->data), wm->size);
      std::cout << "SignalingManager ingoing" << std::endl;
//      std::cout << message << std::endl << std::endl;
      std::cout << std::flush;

      {
        std::lock_guard<std::mutex> lock(manager->ingoingMessagesMutex);
        manager->ingoingMessages.push(message);
      }
    }
      break;
    case MG_EV_WEBSOCKET_CONTROL_FRAME: {
      auto *manager = reinterpret_cast<SignalingManager *>(nc->user_data);
      auto message = std::string(reinterpret_cast<const char *>(wm->data), wm->size);

      if (wm->flags & WEBSOCKET_OP_PONG) {
        break;
      }

      std::cout << "MG_EV_WEBSOCKET_CONTROL_FRAME [ " << wm->size << " bytes]" << std::endl;
      std::cout << "Message:" << message << std::endl;
      std::cout << "Flags: " << std::hex << wm->flags << std::endl;
      std::cout << "MG_F_LISTENING: " << ((wm->flags & MG_F_LISTENING) > 0) << std::endl;
      std::cout << "MG_F_UDP: " << ((wm->flags & MG_F_UDP) > 0) << std::endl;
      std::cout << "MG_F_RESOLVING: " << ((wm->flags & MG_F_RESOLVING) > 0) << std::endl;
      std::cout << "MG_F_CONNECTING: " << ((wm->flags & MG_F_CONNECTING) > 0) << std::endl;
      std::cout << "MG_F_SSL: " << ((wm->flags & MG_F_SSL) > 0) << std::endl;
      std::cout << "MG_F_SSL_HANDSHAKE_DONE: " << ((wm->flags & MG_F_SSL_HANDSHAKE_DONE) > 0) << std::endl;
      std::cout << "MG_F_WANT_READ: " << ((wm->flags & MG_F_WANT_READ) > 0) << std::endl;
      std::cout << "MG_F_WANT_WRITE: " << ((wm->flags & MG_F_WANT_WRITE) > 0) << std::endl;
      std::cout << "MG_F_SEND_AND_CLOSE: " << ((wm->flags & MG_F_SEND_AND_CLOSE) > 0) << std::endl;
      std::cout << "MG_F_CLOSE_IMMEDIATELY: " << ((wm->flags & MG_F_CLOSE_IMMEDIATELY) > 0) << std::endl;
      std::cout << "MG_F_WEBSOCKET_NO_DEFRAG: " << ((wm->flags & MG_F_WEBSOCKET_NO_DEFRAG) > 0) << std::endl;
      std::cout << "MG_F_DELETE_CHUNK: " << ((wm->flags & MG_F_DELETE_CHUNK) > 0) << std::endl;
      std::cout << "MG_F_ENABLE_BROADCAST: " << ((wm->flags & MG_F_ENABLE_BROADCAST) > 0) << std::endl;
      std::cout << "WEBSOCKET_OP_PONG: " << ((wm->flags & WEBSOCKET_OP_PONG) > 0) << std::endl;
    }
      break;

    case MG_EV_SEND:
    case MG_EV_RECV:
      break;
    default: {
      auto eventName = std::to_string(ev);
      try {
        eventName = mg_event_names.at(ev);
      } catch (std::out_of_range &e) {

      }
      std::cout << "> [" << eventName << "]" << std::endl;
    }
      break;
  }
}

void SignalingManager::send(const std::string &message) {
  std::lock_guard<std::mutex> lock(outgoingMessagesMutex);
  outgoingMessages.push(message);
}

void SignalingManager::registerObserver(PeerConnectionObserver *callback) {
  this->callback = callback;
}
