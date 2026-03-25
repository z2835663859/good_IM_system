#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace im {

// 离线消息结构
struct OfflineMessage {
  uint32_t from_id;
  uint32_t to_id;
  std::string content;
  uint64_t timestamp;
  bool read;
};

// 离线消息存储接口
class MessageStorage {
public:
  explicit MessageStorage(const std::string& storage_file = "messages.db");
  ~MessageStorage();

  // 保存离线消息
  void save_offline_message(uint32_t from_id, uint32_t to_id, const std::string& content);

  // 获取用户的所有离线消息
  std::vector<OfflineMessage> get_offline_messages(uint32_t to_id);

  // 标记消息为已读
  void mark_as_read(uint32_t to_id, uint32_t from_id);

  // 获取未读消息数
  int get_unread_count(uint32_t to_id) const;

  // 清空用户的所有消息
  void clear_user_messages(uint32_t user_id);

  // 获取统计信息
  int get_total_messages() const;

private:
  std::string storage_file_;
  mutable int total_messages_;

  // 读取所有消息到内存
  std::vector<OfflineMessage> load_all_messages() const;

  // 保存消息到文件
  void save_all_messages(const std::vector<OfflineMessage>& messages) const;
};

} // namespace im
