// IM 演示客户端
// 编译: clang++ -std=c++17 -o im_client im_protocol.cpp im_client.cpp

#include "im_protocol.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>
#include <string>

using namespace im;

int main(int argc, char* argv[]) {
  std::string username = "alice";
  std::string password = "alice123";

  if (argc > 1) {
    username = argv[1];
  }
  if (argc > 2) {
    password = argv[2];
  }

  // 连接到服务器
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return 1;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(9000);
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

  if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("connect");
    return 1;
  }

  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

  printf("[Client] Connected to server\n");
  printf("[Client] Logging in as: %s\n", username.c_str());

  ByteBuffer in_buf;

  // 1. 发送登录请求
  {
    Packet login_req;
    login_req.type = (uint16_t)MsgType::LOGIN;
    login_req.body = "{\"username\":\"" + username + "\",\"password\":\"" + password + "\"}";

    auto bytes = Codec::encode(login_req);
    send(fd, bytes.data(), bytes.size(), 0);
    printf("[Send] LOGIN request\n");
  }

  // 等待登录响应
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // 接收响应
  uint8_t buf[4096];
  while (true) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0) {
      in_buf.append(buf, (size_t)n);
      Packet resp;
      while (Codec::try_decode(in_buf, resp)) {
        printf("[Recv] Type=%u Body=%s\n", resp.type, resp.body.c_str());
      }
    } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      break;
    } else {
      break;
    }
  }

  // 2. 发送心跳
  printf("\n[Test] Sending heartbeat...\n");
  {
    Packet hb;
    hb.type = (uint16_t)MsgType::HEARTBEAT;
    hb.body = "ping";
    auto bytes = Codec::encode(hb);
    send(fd, bytes.data(), bytes.size(), 0);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // 接收心跳响应
  while (true) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0) {
      in_buf.append(buf, (size_t)n);
      Packet resp;
      while (Codec::try_decode(in_buf, resp)) {
        printf("[Recv] Type=%u Body=%s\n", resp.type, resp.body.c_str());
      }
    } else {
      break;
    }
  }

  // 3. 发送单聊消息（alice -> bob）
  if (username == "alice") {
    printf("\n[Test] Sending chat message to bob (user 2)...\n");
    Packet chat;
    chat.type = (uint16_t)MsgType::CHAT_TO;
    chat.body = "{\"to_user_id\":2,\"text\":\"Hello bob!\"}";
    auto bytes = Codec::encode(chat);
    send(fd, bytes.data(), bytes.size(), 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    while (true) {
      ssize_t n = read(fd, buf, sizeof(buf));
      if (n > 0) {
        in_buf.append(buf, (size_t)n);
        Packet resp;
        while (Codec::try_decode(in_buf, resp)) {
          printf("[Recv] Type=%u Body=%s\n", resp.type, resp.body.c_str());
        }
      } else {
        break;
      }
    }
  }

  // 保持连接一段时间观察消息
  printf("\n[Client] Keep connected for 5 seconds...\n");
  for (int i = 0; i < 5; i++) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 定期接收消息
    while (true) {
      ssize_t n = read(fd, buf, sizeof(buf));
      if (n > 0) {
        in_buf.append(buf, (size_t)n);
        Packet resp;
        while (Codec::try_decode(in_buf, resp)) {
          printf("[Recv] Type=%u Body=%s\n", resp.type, resp.body.c_str());
        }
      } else {
        break;
      }
    }
  }

  // 注销
  printf("\n[Test] Logging out...\n");
  {
    Packet logout;
    logout.type = (uint16_t)MsgType::LOGOUT;
    logout.body = "{}";
    auto bytes = Codec::encode(logout);
    send(fd, bytes.data(), bytes.size(), 0);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  close(fd);
  printf("[Client] Disconnected\n");

  return 0;
}
