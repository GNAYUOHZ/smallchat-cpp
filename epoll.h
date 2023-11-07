#pragma once

#include <functional>
#include <unordered_map>

class Epoll {
 public:
  Epoll();

  ~Epoll();

  int Add(int fd, std::function<void(int)> read_callback);

  int Remove(int fd);

  void Wait();

 private:
  int epollfd_;
  std::unordered_map<int, std::function<void(int)>> callbacks_;
  const unsigned int kMaxEvents = 1000;
};