#include "consolemanager.hh"
#include "messagebus.hh"
#include "settingsstore.hh"
#include "socketserver.hh"
#include "socketclient.hh"
#include "consoleoutputmessage.hh"
#include "socketoutputmessage.hh"
#include "protocolhandlerfactory.hh"
#include <signal.h>
#include <iostream>
#include <string>
#include <cstdlib>

using namespace op2p;

ConsoleManager::ConsoleManager()
  : service_(), 
    work_(service_), 
    signals_(service_), 
    worker_([=] { service_.run(); }) {
}

ConsoleManager::~ConsoleManager() {
}

void ConsoleManager::receive(const std::shared_ptr<Message>& pMessage) {
  service_.post([=] {
    std::cout << pMessage->data() << std::endl;
  });
}

void ConsoleManager::run() {
  registerSignalHandler();

  MessageBus::instance()->subscribe<ConsoleOutputMessage>(this);

  SettingsStore ss;
  ss.load();

  std::string line;
  if (!std::getline(std::cin, line)) return;
  
  auto handler = ProtocolHandlerFactory::instance()->handler(line);
  if (!handler) {
    // TODO: Print error message and exit
  }

  auto settings = ss.settings();
  SocketServer server{ settings, service_ };
  SocketClient client{ settings, service_ };

  if (settings.remoteHost == "") {
    server.start();
  } else {
    client.connect();
  }
  
  while (std::getline(std::cin, line)) {
    handler->exec(line);
  }
}

void ConsoleManager::registerSignalHandler() {
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
#if defined(SIGQUIT)
  signals_.add(SIGQUIT);
#endif

  signals_.async_wait(
    [](std::error_code, int) {
      std::exit(EXIT_SUCCESS);
    });
}
