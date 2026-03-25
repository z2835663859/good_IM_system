#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace im {

// 'I''M''0''1' 的 ASCII（你也可以换成别的）
constexpr uint32_t kMagic = 0x494D3031;
constexpr size_t kHeaderSize = 4 + 2 + 4; // magic + type + body_len = 10

enum class MsgType : uint16_t {
  HEARTBEAT       = 1,
  CHAT            = 2,
  LOGIN           = 3,
  LOGOUT          = 4,
  CHAT_TO         = 5,   // 单聊消息（含目标 user_id）
  NOTIFY          = 6,   // 通知消息（用户上线/下线等）
  CREATE_GROUP    = 7,   // 创建群组
  JOIN_GROUP      = 8,   // 加入群组
  LEAVE_GROUP     = 9,   // 离开群组
  CHAT_GROUP      = 10,  // 群组聊天消息
  LIST_GROUPS     = 11,  // 查询用户的群组
  GROUP_MEMBERS   = 12,  // 查询群组成员
};

// 一个“逻辑包”
struct Packet {
  uint16_t type = 0;
  std::string body; // 先用 JSON 字符串
};

// 缓冲区：负责把 recv 到的碎片拼起来
class ByteBuffer {
public:
  void append(const uint8_t* data, size_t n);
  size_t readable() const;
  const uint8_t* peek() const;
  void consume(size_t n);

private:
  std::vector<uint8_t> buf_;
  size_t rpos_ = 0; // 读指针
};

// 编解码器：负责“组包/拆包”
class Codec {
public:
  static std::vector<uint8_t> encode(const Packet& p);
  static bool try_decode(ByteBuffer& in, Packet& out);

private:
  static uint16_t read_u16_be(const uint8_t* p);
  static uint32_t read_u32_be(const uint8_t* p);
  static void write_u16_be(std::vector<uint8_t>& out, uint16_t v);
  static void write_u32_be(std::vector<uint8_t>& out, uint32_t v);
};

} // namespace im