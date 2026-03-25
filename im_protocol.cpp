#include "im_protocol.hpp"

namespace im {

void ByteBuffer::append(const uint8_t* data, size_t n) {
  buf_.insert(buf_.end(), data, data + n);
}

size_t ByteBuffer::readable() const {
  return buf_.size() - rpos_;
}

const uint8_t* ByteBuffer::peek() const {
  return buf_.data() + rpos_;
}

void ByteBuffer::consume(size_t n) {
  rpos_ += n;
  if (rpos_ == buf_.size()) {
    buf_.clear();
    rpos_ = 0;
  }
}

uint16_t Codec::read_u16_be(const uint8_t* p) {
  return (uint16_t(p[0]) << 8) | uint16_t(p[1]);
}

uint32_t Codec::read_u32_be(const uint8_t* p) {
  return (uint32_t(p[0]) << 24) |
         (uint32_t(p[1]) << 16) |
         (uint32_t(p[2]) << 8)  |
         (uint32_t(p[3]));
}

void Codec::write_u16_be(std::vector<uint8_t>& out, uint16_t v) {
  out.push_back(uint8_t((v >> 8) & 0xFF));
  out.push_back(uint8_t(v & 0xFF));
}

void Codec::write_u32_be(std::vector<uint8_t>& out, uint32_t v) {
  out.push_back(uint8_t((v >> 24) & 0xFF));
  out.push_back(uint8_t((v >> 16) & 0xFF));
  out.push_back(uint8_t((v >> 8) & 0xFF));
  out.push_back(uint8_t(v & 0xFF));
}

// 组包：Packet -> 字节流
std::vector<uint8_t> Codec::encode(const Packet& p) {
  std::vector<uint8_t> out;
  out.reserve(kHeaderSize + p.body.size());

  write_u32_be(out, kMagic);
  write_u16_be(out, p.type);
  write_u32_be(out, (uint32_t)p.body.size());

  out.insert(out.end(), p.body.begin(), p.body.end());
  return out;
}

// 拆包：从 ByteBuffer 中尽量解出 1 个完整 Packet
bool Codec::try_decode(ByteBuffer& in, Packet& out) {
  if (in.readable() < kHeaderSize) return false;

  const uint8_t* p = in.peek();
  uint32_t magic = read_u32_be(p);

  // magic 不对：说明流里混入了垃圾字节，滑动 1 字节继续找
  if (magic != kMagic) {
    in.consume(1);
    return false;
  }

  uint16_t type = read_u16_be(p + 4);
  uint32_t len  = read_u32_be(p + 6);

  // 防御：限制最大 body（后面可以做成配置）
  constexpr uint32_t kMaxBody = 2 * 1024 * 1024;
  if (len > kMaxBody) {
    // 认为协议错误，丢弃 magic 继续找
    in.consume(4);
    return false;
  }

  if (in.readable() < kHeaderSize + len) return false; // 半包：等更多数据

  out.type = type;
  out.body.assign((const char*)(p + kHeaderSize), len);

  in.consume(kHeaderSize + len);
  return true;
}

} // namespace im