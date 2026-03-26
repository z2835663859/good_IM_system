// IM 客户端程序：演示 TCP 粘包问题
// 功能：连接到服务器，发送两个消息包并将其合并发送，测试粘包处理

#include "im_protocol.hpp"
#include <arpa/inet.h>      // inet_pton() 函数
#include <netinet/in.h>     // sockaddr_in 结构体
#include <sys/socket.h>     // socket() 相关 API
#include <unistd.h>         // close() 函数
#include <cstdio>           // perror() 函数

int main() {
  // 创建 TCP 套接字（IPv4, 流式传输）
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  // 初始化服务器地址结构
  sockaddr_in addr{};
  addr.sin_family = AF_INET;                      // 使用 IPv4
  addr.sin_port = htons(9999);                    // 端口号 9999（主机字节序转网络字节序）
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);// IP 地址转换为二进制格式

  // 连接到服务器，连接失败时打印错误并返回
  if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("connect");
    return 1;
  }

  // 构造两个聊天消息包
  im::Packet a{ (uint16_t)im::MsgType::CHAT, R"({"text":"hello"})" };
  im::Packet b{ (uint16_t)im::MsgType::CHAT, R"({"text":"world"})" };

  // 将消息包编码为字节序列
  auto ba = im::Codec::encode(a);
  auto bb = im::Codec::encode(b);

  // ===== TCP 粘包演示 =====
  // 将两个编码后的消息合并到一个缓冲区
  // 这样两个消息在网络传输中会作为一个数据流发送
  // 用来测试服务器是否能正确解析和处理粘包问题
  ba.insert(ba.end(), bb.begin(), bb.end());

  // 一次性发送合并后的所有数据
  send(fd, ba.data(), ba.size(), 0);

  // 关闭套接字连接
  close(fd);
  return 0;
}