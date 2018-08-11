//
// Created by mars on 11.08.18.
//

#include <iostream>
#include "SignalingManager.h"

SignalingManager::~SignalingManager() {
  isTerminating = true;

  if (thread.joinable()) {
    thread.join();
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

  thread = std::thread(&SignalingManager::threadPolling, this);
}

void SignalingManager::threadPolling() {
  while (!isTerminating) {
    mg_mgr_poll(context.get(), 1000);
  }
}

void SignalingManager::requestHandler(struct mg_connection *nc, int ev, void *ev_data) {
  struct websocket_message *wm = (struct websocket_message *) ev_data;

  {
    auto eventName = std::to_string(ev);
    try {
      eventName = mg_event_names.at(ev);
    } catch (std::out_of_range &e) {

    }
    std::cout << "> [" << eventName << "]" << std::endl;
  }

  switch (ev) {
    case MG_EV_WEBSOCKET_FRAME : {
      std::cout << "- - -" << std::endl
                << std::string(reinterpret_cast<const char *>(wm->data), wm->size) << std::endl
                << "- - -" << std::endl;
    }
  }
}


