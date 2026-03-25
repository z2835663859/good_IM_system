#pragma once

#include "conn.hpp"
#include "event_loop.hpp"
#include <map>
#include <memory>
#include <string>

namespace im {

// TCP 服务器
class Server : public EventHandler, public std::enable_shared_from_this<Server> {
public:
  Server(std::shared_ptr<EventLoop> loop, const std::string& host, int port);
  ~Server();

  // EventHandler 接口
  int get_fd() const override { return listen_fd_; }
  void handle_read() override;
  void handle_write() override {}
  void handle_error() override {}

  // 启动服务器（开始监听）
  bool start();

  // 停止服务器
  void stop();

  // 设置连接建立/关闭的回调
  using ConnectionCallback = std::function<void(std::shared_ptr<Conn>)>;
  using MessageCallback = Conn::MessageCallback;

  void set_connection_callback(const ConnectionCallback& cb) { on_connect_ = cb; }
  void set_message_callback(const MessageCallback& cb) { on_message_ = cb; }
  void set_close_callback(const std::function<void(std::shared_ptr<Conn>)>& cb) {
    on_close_ = cb;
  }

  // 获取事件循环
  std::shared_ptr<EventLoop> get_loop() const { return loop_; }

  // 获取已连接数量
  size_t get_connection_count() const { return connections_.size(); }

private:
  std::shared_ptr<EventLoop> loop_;
  std::string host_;
  int port_;
  int listen_fd_;

  std::map<int, std::shared_ptr<Conn>> connections_;

  // 回调
  ConnectionCallback on_connect_;
  MessageCallback on_message_;
  std::function<void(std::shared_ptr<Conn>)> on_close_;

  // 处理新连接
  void handle_connection_close(std::shared_ptr<Conn> conn);
};

} // namespace im
