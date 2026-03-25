#include "conn.hpp"
#include "event_loop.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <cstdio>
#include <cstring>

namespace im {

Conn::Conn(int fd, std::shared_ptr<EventLoop> loop)
    : fd_(fd), state_(CONNECTING), loop_(loop) {
  // 设置非阻塞
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  // macOS: 避免 SIGPIPE
#ifdef __APPLE__
  int on = 1;
  setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif
}

Conn::~Conn() {
  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}

void Conn::enable_reading() {
  if (state_ == CONNECTING) {
    state_ = CONNECTED;
  }

  if (loop_) {
    loop_->mod_handler(fd_, EVFILT_READ);
  }
}

void Conn::enable_writing() {
  if (state_ != CONNECTED && state_ != CLOSING) return;

  if (loop_) {
    loop_->mod_handler(fd_, EVFILT_READ | EVFILT_WRITE);
  }
}

void Conn::disable_writing() {
  if (loop_) {
    loop_->mod_handler(fd_, EVFILT_READ);
  }
}

void Conn::send(const Packet& pkt) {
  auto bytes = Codec::encode(pkt);

  // 先检查是否有待发数据
  bool need_write = out_q_.empty();

  out_q_.push_back(bytes);

  // 如果发缓冲是空的，立即尝试写
  if (need_write) {
    enable_writing();
    handle_write();
  }
}

void Conn::handle_read() {
  uint8_t buf[4096];

  while (true) {
    ssize_t n = ::read(fd_, buf, sizeof(buf));

    if (n > 0) {
      in_buf_.append(buf, (size_t)n);

      // 解包
      Packet pkt;
      while (Codec::try_decode(in_buf_, pkt)) {
        if (on_message_) {
          on_message_(shared_from_this(), pkt);
        }
      }
    } else if (n == 0) {
      // 对端关闭
      handle_error();
      break;
    } else {
      // n < 0
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("read");
        handle_error();
      }
      break;
    }
  }
}

void Conn::handle_write() {
  while (!out_q_.empty()) {
    auto& data = out_q_.front();

    // 计算还需要写的数据
    size_t to_write = data.size() - write_offset_;
    if (to_write == 0) {
      out_q_.pop_front();
      write_offset_ = 0;
      continue;
    }

    ssize_t n = ::write(fd_, data.data() + write_offset_, to_write);

    if (n > 0) {
      write_offset_ += (size_t)n;
      if (write_offset_ == data.size()) {
        out_q_.pop_front();
        write_offset_ = 0;
      }
    } else {
      // n <= 0
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("write");
        handle_error();
      }
      break;
    }
  }

  // 如果发完了，关闭写事件
  if (out_q_.empty()) {
    disable_writing();

    if (state_ == CLOSING) {
      handle_error();
    }
  }
}

void Conn::handle_error() {
  if (state_ == CLOSED) return;

  state_ = CLOSED;

  if (loop_) {
    loop_->del_handler(fd_);
  }

  if (on_close_) {
    on_close_(shared_from_this());
  }
}

void Conn::close_internal() {
  if (state_ == CLOSED) return;
  state_ = CLOSING;
  if (out_q_.empty()) {
    handle_error();
  }
}

} // namespace im
