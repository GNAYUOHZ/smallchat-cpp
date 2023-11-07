#include "epoll.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <stdexcept>

Epoll::Epoll() {
  epollfd_ = epoll_create1(0);
  if (epollfd_ == -1) {
    throw std::runtime_error("epoll_create1 failed.");
  }
}

Epoll::~Epoll() {
  if (epollfd_ != -1) {
    close(epollfd_);
  }
}

int Epoll::Add(int fd, std::function<void(int)> read_callback) {
  if (callbacks_.size() >= kMaxEvents) {
    std::cerr << "too many file descriptors to monitor." << std::endl;
    return -1;
  }

  epoll_event event;
  event.data.fd = fd;
  event.events = EPOLLIN;
  if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event) == -1) {
    std::cerr << "epoll_ctl: add failed." << std::endl;
    return -1;
  }

  callbacks_[fd] = std::move(read_callback);
  return 0;
}

int Epoll::Remove(int fd) {
  if (epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, NULL) == -1) {
    std::cerr << "epoll_ctl: del failed." << std::endl;
    return -1;
  }
  callbacks_.erase(fd);
  return 0;
}

void Epoll::Wait() {
  int size = callbacks_.size();
  epoll_event events[size];
  int num_events = epoll_wait(epollfd_, events, size, 1000);
  if (num_events == -1) {
    if (errno == EINTR) {
      return;  // Try again.
    } else {
      std::cerr << "epoll_wait failed." << std::endl;
      return;
    }
  }

  for (int i = 0; i < num_events; ++i) {
    int fd = events[i].data.fd;
    if (callbacks_.count(fd) && events[i].events & EPOLLIN &&
        callbacks_[fd] != nullptr) {
      callbacks_[fd](fd);
    }
  }
}