#pragma once

#include <functional>
#include <map>
#include <memory>
#include <sys/types.h>
#include <sys/event.h>

namespace im {

class Conn;

// 通用事件处理器接口
class EventHandler {
public:
  virtual ~EventHandler() = default;
  virtual void handle_read() = 0;
  virtual void handle_write() = 0;
  virtual void handle_error() = 0;
  virtual int get_fd() const = 0;
};

// 事件循环：封装 kqueue/epoll/select
class EventLoop : public std::enable_shared_from_this<EventLoop> {
public:
  EventLoop();
  ~EventLoop();

  // 添加事件处理器
  void add_handler(int fd, std::shared_ptr<EventHandler> handler, int events);

  // 修改监听事件
  void mod_handler(int fd, int events);

  // 移除事件处理器
  void del_handler(int fd);

  // 事件循环主函数（阻塞，直到 stop() 被调用）
  void run();

  // 停止事件循环
  void stop();

  // 获取当前是否运行中
  bool is_running() const { return running_; }

  // 获取 kqueue fd（仅供内部使用）
  int get_kq_fd() const { return kq_fd_; }

private:
  int kq_fd_;
  bool running_;

  // fd -> Handler 映射
  std::map<int, std::shared_ptr<EventHandler>> fd_to_handler_;
};

} // namespace im
