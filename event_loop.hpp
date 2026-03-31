#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <sys/epoll.h>
#include <unistd.h>
namespace im {
class EventHandler;
class EventLoop {
public:
  static const int EVENT_READ = 1;
  static const int EVENT_WRITE = 2;
  EventLoop();
  ~EventLoop();
  void add_handler(int fd, std::shared_ptr<EventHandler> handler, int events);
  void mod_handler(int fd, int events);
  void del_handler(int fd);
  void run();
  void stop();
private:
  int poll_fd_;
  bool running_;
  std::unordered_map<int, std::shared_ptr<EventHandler>> fd_to_handler_;
};
} // namespace im
