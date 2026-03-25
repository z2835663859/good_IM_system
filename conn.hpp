#pragma once

#include "im_protocol.hpp"
#include "event_loop.hpp"
#include <deque>
#include <functional>
#include <memory>
#include <string>

namespace im {

class EventLoop;

// 单个连接的处理器
class Conn : public EventHandler, public std::enable_shared_from_this<Conn> {
public:
  // 连接状态
  enum State {
    CONNECTING = 0,
    CONNECTED = 1,
    CLOSING = 2,
    CLOSED = 3,
  };

  Conn(int fd, std::shared_ptr<EventLoop> loop);
  ~Conn();

  int get_fd() const override { return fd_; }
  State get_state() const { return state_; }

  // EventHandler 接口
  void handle_read() override;
  void handle_write() override;
  void handle_error() override;

  // 设置连接相关的回调函数
  using MessageCallback = std::function<void(std::shared_ptr<Conn>, const Packet&)>;
  using CloseCallback = std::function<void(std::shared_ptr<Conn>)>;

  void set_message_callback(const MessageCallback& cb) { on_message_ = cb; }
  void set_close_callback(const CloseCallback& cb) { on_close_ = cb; }
  void set_connection_callback(const std::function<void(std::shared_ptr<Conn>)>& cb) {
    on_connect_ = cb;
  }

  // 发送消息（会入队到发缓冲）
  void send(const Packet& pkt);

  // 启用读/写事件监听
  void enable_reading();
  void enable_writing();
  void disable_writing();

  // 用户数据关联
  void set_context(void* ctx) { context_ = ctx; }
  void* get_context() const { return context_; }

private:
  int fd_;
  State state_;
  std::shared_ptr<EventLoop> loop_;

  // 收缓冲和发缓冲
  ByteBuffer in_buf_;
  std::deque<std::vector<uint8_t>> out_q_;

  // 写缓冲偏移（用于处理部分写）
  size_t write_offset_ = 0;

  // 回调函数
  MessageCallback on_message_;
  CloseCallback on_close_;
  std::function<void(std::shared_ptr<Conn>)> on_connect_;

  // 用户数据
  void* context_ = nullptr;

  // 内部辅助
  void close_internal();
};

} // namespace im
