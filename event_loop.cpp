#include "event_loop.hpp"
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace im {

EventLoop::EventLoop() : running_(false) {
  kq_fd_ = kqueue();
  if (kq_fd_ < 0) {
    perror("kqueue");
    throw std::runtime_error("Failed to create kqueue");
  }
}

EventLoop::~EventLoop() {
  if (kq_fd_ >= 0) {
    close(kq_fd_);
  }
}

void EventLoop::add_handler(int fd, std::shared_ptr<EventHandler> handler, int events) {
  if (!handler) return;

  fd_to_handler_[fd] = handler;

  struct kevent kev[2];
  int cnt = 0;

  // events 可以包含 EVFILT_READ 或 EVFILT_WRITE
  if (events & EVFILT_READ) {
    memset(&kev[cnt], 0, sizeof(struct kevent));
    kev[cnt].ident = fd;
    kev[cnt].filter = EVFILT_READ;
    kev[cnt].flags = EV_ADD;
    kev[cnt].udata = (void*)(uintptr_t)fd;
    cnt++;
  }

  if (events & EVFILT_WRITE) {
    memset(&kev[cnt], 0, sizeof(struct kevent));
    kev[cnt].ident = fd;
    kev[cnt].filter = EVFILT_WRITE;
    kev[cnt].flags = EV_ADD;
    kev[cnt].udata = (void*)(uintptr_t)fd;
    cnt++;
  }

  if (cnt > 0 && kevent(kq_fd_, kev, cnt, nullptr, 0, nullptr) == -1) {
    perror("kevent add");
  }
}

void EventLoop::mod_handler(int fd, int events) {
  struct kevent kev[2];
  int cnt = 0;

  // 先删除旧事件
  struct kevent rm_read, rm_write;
  memset(&rm_read, 0, sizeof(struct kevent));
  rm_read.ident = fd;
  rm_read.filter = EVFILT_READ;
  rm_read.flags = EV_DELETE;

  memset(&rm_write, 0, sizeof(struct kevent));
  rm_write.ident = fd;
  rm_write.filter = EVFILT_WRITE;
  rm_write.flags = EV_DELETE;

  kevent(kq_fd_, &rm_read, 1, nullptr, 0, nullptr);
  kevent(kq_fd_, &rm_write, 1, nullptr, 0, nullptr);

  // 添加新事件
  if (events & EVFILT_READ) {
    memset(&kev[cnt], 0, sizeof(struct kevent));
    kev[cnt].ident = fd;
    kev[cnt].filter = EVFILT_READ;
    kev[cnt].flags = EV_ADD;
    kev[cnt].udata = (void*)(uintptr_t)fd;
    cnt++;
  }

  if (events & EVFILT_WRITE) {
    memset(&kev[cnt], 0, sizeof(struct kevent));
    kev[cnt].ident = fd;
    kev[cnt].filter = EVFILT_WRITE;
    kev[cnt].flags = EV_ADD;
    kev[cnt].udata = (void*)(uintptr_t)fd;
    cnt++;
  }

  if (cnt > 0 && kevent(kq_fd_, kev, cnt, nullptr, 0, nullptr) == -1) {
    perror("kevent mod");
  }
}

void EventLoop::del_handler(int fd) {
  auto it = fd_to_handler_.find(fd);
  if (it != fd_to_handler_.end()) {
    fd_to_handler_.erase(it);
  }

  struct kevent kev[2];
  memset(&kev[0], 0, sizeof(struct kevent));
  kev[0].ident = fd;
  kev[0].filter = EVFILT_READ;
  kev[0].flags = EV_DELETE;

  memset(&kev[1], 0, sizeof(struct kevent));
  kev[1].ident = fd;
  kev[1].filter = EVFILT_WRITE;
  kev[1].flags = EV_DELETE;

  kevent(kq_fd_, kev, 2, nullptr, 0, nullptr);
}

void EventLoop::run() {
  running_ = true;

  struct kevent events[64];
  struct timespec timeout;
  timeout.tv_sec = 1;
  timeout.tv_nsec = 0;

  while (running_) {
    int n = kevent(kq_fd_, nullptr, 0, events, 64, &timeout);

    if (n < 0) {
      perror("kevent");
      break;
    }

    for (int i = 0; i < n; i++) {
      int fd = (int)(uintptr_t)events[i].ident;
      auto it = fd_to_handler_.find(fd);

      if (it == fd_to_handler_.end()) continue;

      auto handler = it->second;

      if (events[i].filter == EVFILT_READ) {
        if (events[i].flags & EV_ERROR) {
          handler->handle_error();
        } else {
          handler->handle_read();
        }
      } else if (events[i].filter == EVFILT_WRITE) {
        if (events[i].flags & EV_ERROR) {
          handler->handle_error();
        } else {
          handler->handle_write();
        }
      }
    }
  }
}

void EventLoop::stop() {
  running_ = false;
}

} // namespace im
