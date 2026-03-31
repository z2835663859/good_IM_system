#include "event_loop.hpp"
#include <sys/time.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <iostream>
namespace im {
EventLoop::EventLoop() : running_(false) {
  poll_fd_ = epoll_create1(0);
  if (poll_fd_ < 0) { throw std::runtime_error("epoll failed"); }
}
EventLoop::~EventLoop() { if (poll_fd_ >= 0) close(poll_fd_); }
void EventLoop::add_handler(int fd, std::shared_ptr<EventHandler> handler, int events) {
  if (!handler) return; fd_to_handler_[fd] = handler;
  struct epoll_event ev; ev.data.fd = fd; ev.events = 0;
  if (events & EVENT_READ) ev.events |= EPOLLIN;
  if (events & EVENT_WRITE) ev.events |= EPOLLOUT;
  epoll_ctl(poll_fd_, EPOLL_CTL_ADD, fd, &ev);
}
void EventLoop::mod_handler(int fd, int events) {
  struct epoll_event ev; ev.data.fd = fd; ev.events = 0;
  if (events & EVENT_READ) ev.events |= EPOLLIN;
  if (events & EVENT_WRITE) ev.events |= EPOLLOUT;
  epoll_ctl(poll_fd_, EPOLL_CTL_MOD, fd, &ev);
}
void EventLoop::del_handler(int fd) {
  epoll_ctl(poll_fd_, EPOLL_CTL_DEL, fd, nullptr);
  fd_to_handler_.erase(fd);
}
void EventLoop::run() {
  running_ = true; const int MAX_EVENTS = 64; struct epoll_event events[MAX_EVENTS];
  while (running_) {
    int n = epoll_wait(poll_fd_, events, MAX_EVENTS, -1);
    if (n < 0) { if (errno == EINTR) continue; break; }
    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;
      auto it = fd_to_handler_.find(fd);
      if (it == fd_to_handler_.end()) continue;
      auto handler = it->second; if (!handler) continue;
      if (events[i].events & (EPOLLERR | EPOLLHUP)) { handler->handle_error(); continue; }
      if (events[i].events & EPOLLIN) handler->handle_read();
      if (events[i].events & EPOLLOUT) handler->handle_write();
    }
  }
}
void EventLoop::stop() { running_ = false; }
} // namespace im
