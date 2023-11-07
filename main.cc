#include <signal.h>

#include <iostream>

#include "chatserver.h"

bool g_shutdown_flag = false;  // for graceful shutdown

void ProcessSignal(int signum) {
  std::cout << "Receive signal: " << signum << std::endl;
  switch (signum) {
    case SIGINT:
      g_shutdown_flag = true;
      break;
    default:
      return;
  }
}

int main() {
  // capture signal of ctrl + c
  signal(SIGINT, ProcessSignal);

  constexpr const int kPort = 7711;
  ChatServer server(kPort);
  server.Run();
  return 0;
}