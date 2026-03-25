// 完整的 IM 服务器实现（包含 P2 特性）
// 编译: clang++ -std=c++17 -o server_im im_protocol.cpp user_manager.cpp connection_manager.cpp \
//        message_handler.cpp message_storage.cpp logger.cpp connection_monitor.cpp group_manager.cpp server_im.cpp

#include "im_protocol.hpp"
#include "user_manager.hpp"
#include "connection_manager.hpp"
#include "message_handler.hpp"
#include "message_storage.hpp"
#include "logger.hpp"
#include "connection_monitor.hpp"
#include "group_manager.hpp"

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
#include <chrono>
#include <thread>

using namespace im;

// 连接的收发缓冲
struct ConnectionData {
  int fd;
  ByteBuffer in_buf;
  std::vector<std::vector<uint8_t>> out_queue;
  std::chrono::system_clock::time_point last_activity;
};

int main(int argc, char* argv[]) {
  int port = 9999;
  if (argc > 1) {
    port = std::atoi(argv[1]);
  }

  // 初始化日志系统
  Logger& logger = Logger::get_instance();
  logger.set_level(Logger::Level::DEBUG);
  logger.enable_file_output("im_server.log");

  LOG_INFO("========== IM Server Starting ==========");
  LOG_INFO("Port: " + std::to_string(port));

  // 创建各个管理器和存储
  auto user_mgr = std::make_shared<UserManager>();
  auto conn_mgr = std::make_shared<ConnectionManager>(user_mgr);
  auto msg_storage = std::make_shared<MessageStorage>("messages.db");
  auto conn_monitor = std::make_shared<ConnectionMonitor>(300);  // 5分钟超时
  auto group_mgr = std::make_shared<GroupManager>();
  auto msg_handler = std::make_shared<MessageHandler>(user_mgr, conn_mgr, msg_storage, 
                                                       conn_monitor, group_mgr);

  // 预注册一些测试用户
  user_mgr->register_user("alice", "alice123");
  user_mgr->register_user("bob", "bob123");
  user_mgr->register_user("charlie", "charlie123");

  LOG_INFO("Initialized 3 test users: alice, bob, charlie");

  // 创建监听 socket
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    perror("socket");
    LOG_ERROR("Failed to create socket");
    return 1;
  }

  int yes = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("bind");
    LOG_ERROR("Failed to bind to port " + std::to_string(port));
    return 1;
  }

  if (listen(listen_fd, 128) < 0) {
    perror("listen");
    LOG_ERROR("Failed to listen");
    return 1;
  }

  LOG_INFO("Listening on port " + std::to_string(port));
  fcntl(listen_fd, F_SETFL, fcntl(listen_fd, F_GETFL, 0) | O_NONBLOCK);

  std::map<int, ConnectionData> conns;
  auto last_cleanup = std::chrono::system_clock::now();

  while (true) {
    fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    FD_SET(listen_fd, &readfds);
    int max_fd = listen_fd;

    for (auto& [fd, conn_data] : conns) {
      FD_SET(fd, &readfds);
      if (!conn_data.out_queue.empty()) {
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

        ConnectionData conn_data;
        conn_data.fd = client_fd;
        conn_data.last_activity = std::chrono::system_clock::now();

        conns[client_fd] = conn_data;
        conn_mgr->add_connection(client_fd);
        conn_monitor->update_activity(client_fd);

        LOG_INFO("New connection from " + std::string(inet_ntoa(client_addr.sin_addr)) + 
                 ":" + std::to_string(ntohs(client_addr.sin_port)) + " (fd=" + 
                 std::to_string(client_fd) + ")");
      }
    }

    // 处理读事件
    std::vector<int> to_close;
    for (auto& [fd, conn_data] : conns) {
      if (FD_ISSET(fd, &readfds)) {
        uint8_t buf[4096];
        ssize_t n = read(fd, buf, sizeof(buf));

        if (n > 0) {
          conn_data.last_activity = std::chrono::system_clock::now();
          conn_data.in_buf.append(buf, (size_t)n);

          // 尝试解包和处理
          Packet pkt;
          while (Codec::try_decode(conn_data.in_buf, pkt)) {
            auto result = msg_handler->handle_message(fd, pkt);
            
            // 更新连接活动时间
            conn_monitor->update_activity(fd);

            // 回复消息
            if (result.should_reply) {
              auto reply_bytes = Codec::encode(result.reply_pkt);
              conn_data.out_queue.push_back(reply_bytes);
              LOG_DEBUG("Sent reply to fd=" + std::to_string(fd));
            }

            // 如果是登录消息，推送离线消息
            if ((MsgType)pkt.type == MsgType::LOGIN && msg_storage) {
              uint32_t user_id = conn_mgr->get_user_by_connection(fd);
              if (user_id > 0) {
                auto offline_msgs = msg_storage->get_offline_messages(user_id);
                if (!offline_msgs.empty()) {
                  LOG_INFO("Pushing " + std::to_string(offline_msgs.size()) + 
                           " offline messages to user " + std::to_string(user_id));
                  for (const auto& msg : offline_msgs) {
                    Packet offline_pkt;
                    offline_pkt.type = (uint16_t)MsgType::CHAT_TO;
                    offline_pkt.body = "{\"from_user_id\":" + std::to_string(msg.from_id) + 
                                       ",\"text\":\"" + msg.content + "\",\"offline\":true}";
                    auto offline_bytes = Codec::encode(offline_pkt);
                    conn_data.out_queue.push_back(offline_bytes);
                    LOG_DEBUG("Queued offline message from user " + std::to_string(msg.from_id));
                  }
                }
              }
            }

            // 推送给其他连接
            if (!result.notify_targets.empty()) {
              auto notify_bytes = Codec::encode(result.reply_pkt);
              for (int target_fd : result.notify_targets) {
                if (target_fd != fd && conns.count(target_fd)) {
                  conns[target_fd].out_queue.push_back(notify_bytes);
                  LOG_DEBUG("Routed message to fd=" + std::to_string(target_fd));
                }
              }
            }
          }
        } else if (n == 0) {
          LOG_INFO("Connection closed by peer (fd=" + std::to_string(fd) + ")");
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
    for (auto& [fd, conn_data] : conns) {
      if (FD_ISSET(fd, &writefds)) {
        while (!conn_data.out_queue.empty()) {
          auto& data = conn_data.out_queue.front();
          ssize_t n = write(fd, data.data(), data.size());

          if (n > 0) {
            data.erase(data.begin(), data.begin() + n);
            if (data.empty()) {
              conn_data.out_queue.erase(conn_data.out_queue.begin());
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

    // 关闭连接
    for (int fd : to_close) {
      close(fd);
      conn_mgr->remove_connection(fd);
      conn_monitor->remove_connection(fd);
      conns.erase(fd);
      LOG_DEBUG("Connection closed, remaining: " + std::to_string(conns.size()));
    }

    // 定期检查和清理超时连接（每30秒一次）
    auto now = std::chrono::system_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_cleanup).count() >= 30) {
      auto timeout_fds = conn_monitor->check_timeout();
      if (!timeout_fds.empty()) {
        LOG_WARN("Found " + std::to_string(timeout_fds.size()) + " timeout connections");
        for (int timeout_fd : timeout_fds) {
          if (conns.count(timeout_fd)) {
            LOG_INFO("Closing timeout connection fd=" + std::to_string(timeout_fd));
            close(timeout_fd);
            conn_mgr->remove_connection(timeout_fd);
            conns.erase(timeout_fd);
          }
        }
      }
      
      // 也清理超时用户
      user_mgr->cleanup_timeout_users(60);
      
      int active_conns = conn_monitor->get_active_count();
      LOG_DEBUG("Active connections: " + std::to_string(active_conns));
      
      last_cleanup = now;
    }
  }

  LOG_INFO("========== IM Server Stopped ==========");
  close(listen_fd);
  return 0;
}
