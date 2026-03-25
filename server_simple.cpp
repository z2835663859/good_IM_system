// 简化版多连接服务器（同步单线程）
// 用 select 测试，避免 kqueue 的复杂性

#include "im_protocol.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

using namespace im;

// 连接状态
struct Connection {
  int fd;
  ByteBuffer in_buf;
  std::vector<std::vector<uint8_t>> out_queue;
};

int main() {
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    perror("socket");
    return 1;
  }

  int yes = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(9000);

  if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return 1;
  }

  if (listen(listen_fd, 128) < 0) {
    perror("listen");
    return 1;
  }

  printf("[Server] Listening on port 9000\n");

  fcntl(listen_fd, F_SETFL, fcntl(listen_fd, F_GETFL, 0) | O_NONBLOCK);

  std::map<int, Connection> conns;

  while (true) {
    fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    FD_SET(listen_fd, &readfds);
    int max_fd = listen_fd;

    // 为所有已连接的 fd 设置读事件，有待发数据则设置写事件
    for (auto& [fd, conn] : conns) {
      FD_SET(fd, &readfds);
      if (!conn.out_queue.empty()) {
        FD_SET(fd, &writefds);
      }
      max_fd = std::max(max_fd, fd);
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int ret = select(max_fd + 1, &readfds, &writefds, nullptr, &timeout);
    if (ret < 0) {
      perror("select");
      break;
    }

    // 处理新连接
    if (FD_ISSET(listen_fd, &readfds)) {
      sockaddr_in client_addr{};
      socklen_t addr_len = sizeof(client_addr);

      int client_fd = accept(listen_fd, (sockaddr*)&client_addr, &addr_len);
      if (client_fd >= 0) {
        fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK);
        printf("[Accept] New connection from %s:%d (fd=%d)\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);
        conns[client_fd] = Connection{client_fd, ByteBuffer(), {}};
      }
    }

    // 处理已有连接的读事件
    std::vector<int> to_close;
    for (auto& [fd, conn] : conns) {
      if (FD_ISSET(fd, &readfds)) {
        uint8_t buf[4096];
        ssize_t n = read(fd, buf, sizeof(buf));

        if (n > 0) {
          conn.in_buf.append(buf, (size_t)n);

          Packet pkt;
          while (Codec::try_decode(conn.in_buf, pkt)) {
            printf("[Recv] from fd=%d type=%u body=%s\n", fd, pkt.type, pkt.body.c_str());

            // Echo: 回送给所有连接
            Packet resp;
            resp.type = pkt.type;
            resp.body = std::string("Echo: ") + pkt.body;
            auto resp_bytes = Codec::encode(resp);

            // 发送给发送者
            conns[fd].out_queue.push_back(resp_bytes);

            // 广播给其他连接
            for (auto& [other_fd, _] : conns) {
              if (other_fd != fd) {
                conns[other_fd].out_queue.push_back(resp_bytes);
              }
            }
          }
        } else if (n == 0) {
          printf("[Close] fd=%d closed by peer\n", fd);
          to_close.push_back(fd);
        } else {
          if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("read");
            to_close.push_back(fd);
          }
        }
      }
    }

    // 处理写事件
    for (auto& [fd, conn] : conns) {
      if (FD_ISSET(fd, &writefds)) {
        while (!conn.out_queue.empty()) {
          auto& data = conn.out_queue.front();
          ssize_t n = write(fd, data.data(), data.size());

          if (n > 0) {
            data.erase(data.begin(), data.begin() + n);
            if (data.empty()) {
              conn.out_queue.erase(conn.out_queue.begin());
            }
          } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
              perror("write");
              to_close.push_back(fd);
            }
            break;
          }
        }
      }
    }

    // 关闭需要关闭的连接
    for (int fd : to_close) {
      close(fd);
      conns.erase(fd);
      printf("[Stats] Connection closed, remaining: %zu\n", conns.size());
    }
  }

  close(listen_fd);
  return 0;
}
