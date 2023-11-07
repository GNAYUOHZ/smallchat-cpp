#pragma once

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "epoll.h"

// This structure represents a connected client.
class Client {
 public:
  explicit Client(int fd, sockaddr_in addr)
      : fd_(fd), nick_("user:" + std::to_string(fd)), addr_(addr) {
    std::cout << "accept client conn: " << GetAddrStr() << std::endl;
  }
  ~Client() {
    std::cout << "disconnected client conn: " << GetAddrStr() << std::endl;
    close(fd_);
  }

  int GetFd() const { return fd_; }
  const std::string &GetNick() const { return nick_; }
  sockaddr_in GetAddr() const { return addr_; }
  std::string GetAddrStr() const {
    return std::string(inet_ntoa(addr_.sin_addr)) + ":" +
           std::to_string(ntohs(addr_.sin_port));
  }

 private:
  int fd_;            // Client socket.
  std::string nick_;  // Nickname of the client.
  sockaddr_in addr_;  // Client addr
};

class ChatServer {
 public:
  ChatServer(int port);

  ~ChatServer();

  void Run();

 private:
  // Create a TCP socket listening to 'port' ready to accept connections.
  int CreateTCPServer(int port);

  // If the listening socket is "readable", it actually means
  // there are new clients connections pending to accept.
  void AcceptClient(int fd);

  // ReadAndBroadcast a well formed message from one client.
  void ReadAndBroadcast(int fd);

  void AddClient(std::shared_ptr<Client> client);

  void RemoveClient(std::shared_ptr<Client> client);

  // Send the specified string to all connected clients but the one
  // having as socket descriptor 'excluded'.
  void SendMsgToAllClientsBut(int excluded, const std::string &msg);

 private:
  Epoll epoll_;
  std::unordered_map<int, std::shared_ptr<Client>> clients_;
};
