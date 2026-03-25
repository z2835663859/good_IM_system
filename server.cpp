#include "server.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/event.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace im {

Server::Server(std::shared_ptr<EventLoop> loop, const std::string& host, int port)
    : loop_(loop), host_(host), port_(port), listen_fd_(-1) {
}

Server::~Server() {
  if (listen_fd_ >= 0) {
    close(listen_fd_);
  }
}

bool Server::start() {
  // 创建 listen socket
  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ < 0) {
    perror("socket");
    return false;
  }

  // 设置 SO_REUSEADDR
  int yes = 1;
  if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    perror("setsockopt");
    return false;
  }

  // 绑定
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port_);

  if (host_ == "*") {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    inet_pton(AF_INET, host_.c_str(), &addr.sin_addr);
  }

  if (bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return false;
  }

  // 监听
  if (listen(listen_fd_, 128) < 0) {
    perror("listen");
    return false;
  }

  printf("[Server] Listening on %s:%d\n", host_.c_str(), port_);

  // 设置非阻塞
  int flags = fcntl(listen_fd_, F_GETFL, 0);
  fcntl(listen_fd_, F_SETFL, flags | O_NONBLOCK);

  // 注册 listen socket 到事件循环
  loop_->add_handler(listen_fd_, shared_from_this(), EVFILT_READ);

  return true;
}

void Server::stop() {
  if (listen_fd_ >= 0) {
    loop_->del_handler(listen_fd_);
    close(listen_fd_);
    listen_fd_ = -1;
  }
  loop_->stop();
}

void Server::handle_read() {
  // 这个方法被事件循环调用，说明 listen socket 有新连接

  sockaddr_in client_addr{};
  socklen_t addr_len = sizeof(client_addr);

  while (true) {
    int client_fd = accept(listen_fd_, (sockaddr*)&client_addr, &addr_len);

    if (client_fd < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("accept");
      }
      break;
    }

    printf("[Server] New connection from %s:%d (fd=%d)\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);

    auto conn = std::make_shared<Conn>(client_fd, loop_);

    conn->set_message_callback(on_message_);
    conn->set_close_callback([this](auto c) {
      handle_connection_close(c);
    });

    if (on_connect_) {
      on_connect_(conn);
    }

    connections_[client_fd] = conn;

    // 注册到事件循环
    loop_->add_handler(client_fd, conn, EVFILT_READ);

    // 启用读事件
    conn->enable_reading();
  }
}

void Server::handle_connection_close(std::shared_ptr<Conn> conn) {
  int fd = conn->get_fd();

  auto it = connections_.find(fd);
  if (it != connections_.end()) {
    connections_.erase(it);
  }

  printf("[Server] Connection closed (fd=%d), remaining: %zu\n", fd, connections_.size());
}

} // namespace im
