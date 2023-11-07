#include "chatserver.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

extern bool g_shutdown_flag;

ChatServer::ChatServer(int port) {
  int server_fd = CreateTCPServer(port);
  if (server_fd == -1) {
    throw std::runtime_error("creating listening socket failed.");
  }

  int ret = epoll_.Add(server_fd, [this](int fd) { this->AcceptClient(fd); });
  if (ret != 0) {
    throw std::runtime_error("epoll add server_fd failed.");
  }
  std::cout << "ChatServer started on port: " << port
            << ", epoll fd: " << server_fd << std::endl;
}

ChatServer::~ChatServer() {
  // Send a server close msg to all client.
  std::string close_msg = "Server is closing.\n";
  SendMsgToAllClientsBut(-1, close_msg);

  // Auto close client conn.
  clients_.clear();

  std::this_thread::sleep_for(std::chrono::seconds(5));
}

void ChatServer::Run() {
  while (!g_shutdown_flag) {
    epoll_.Wait();
  }
}

int ChatServer::CreateTCPServer(int port) {
  int server = socket(AF_INET, SOCK_STREAM, 0);
  if (server == -1) {
    return -1;
  }

  int yes = 1;
  setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);
  if (bind(server, reinterpret_cast<sockaddr *>(&server_addr),
           sizeof(server_addr)) == -1 ||
      listen(server, 511) == -1) {
    close(server);
    return -1;
  }
  return server;
}

void ChatServer::AcceptClient(int fd) {
  int client_fd;
  sockaddr_in client_addr;
  while (true) {
    socklen_t client_addr_len = sizeof(client_addr);
    client_fd = accept(fd, reinterpret_cast<sockaddr *>(&client_addr),
                       &client_addr_len);
    if (client_fd == -1) {
      if (errno == EINTR) {
        return;  // Try again.
      } else {
        std::cout << "accept client fail." << std::endl;
        return;
      }
    }
    break;
  }

  std::shared_ptr<Client> client =
      std::make_shared<Client>(client_fd, client_addr);

  // Set the specified socket in non-blocking mode, with no delay flag.
  int flags, yes = 1;
  if ((flags = fcntl(fd, F_GETFL)) == -1) return;
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));

  AddClient(client);
}

void ChatServer::ReadAndBroadcast(int fd) {
  char readbuf[256];
  int nread = read(fd, readbuf, sizeof(readbuf) - 1);
  // Error or short read means that the socket was closed.
  if (nread <= 0) {
    RemoveClient(clients_[fd]);
    return;
  }
  readbuf[nread] = 0;

  // Create a message to send everybody (and show on the server console)
  // in the form: nick> some message.
  std::string msg = clients_[fd]->GetNick() + "> " + std::string(readbuf);
  SendMsgToAllClientsBut(fd, msg);
}

void ChatServer::AddClient(std::shared_ptr<Client> client) {
  int client_fd = client->GetFd();

  int ret =
      epoll_.Add(client_fd, [this](int fd) { this->ReadAndBroadcast(fd); });
  if (ret != 0) {
    return;
  }
  clients_[client_fd] = client;
  // Send a welcome msg to all.
  std::string join_msg = "Welcome " + client->GetNick() + " join chat.\n";
  SendMsgToAllClientsBut(-1, join_msg);
}

void ChatServer::RemoveClient(std::shared_ptr<Client> client) {
  int client_fd = client->GetFd();
  int ret = epoll_.Remove(client_fd);
  if (ret != 0) {
    return;
  }
  clients_.erase(client_fd);
  // Send a leave msg to all.
  std::string leave_msg = client->GetNick() + " leave chat.\n";
  SendMsgToAllClientsBut(client_fd, leave_msg);
}

void ChatServer::SendMsgToAllClientsBut(int excluded, const std::string &msg) {
  std::cout << msg;
  for (const auto &client : clients_) {
    if (client.first == excluded) {
      continue;
    }
    write(client.first, msg.c_str(), msg.size());
  }
}