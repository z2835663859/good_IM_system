#include "message_storage.hpp"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>

namespace im {

MessageStorage::MessageStorage(const std::string& storage_file)
    : storage_file_(storage_file), total_messages_(0) {
  // 如果文件不存在，创建空文件
  std::ifstream file(storage_file_);
  if (!file.good()) {
    std::ofstream new_file(storage_file_);
    new_file.close();
  }
  
  // 加载现有消息统计
  auto messages = load_all_messages();
  total_messages_ = messages.size();
}

MessageStorage::~MessageStorage() = default;

void MessageStorage::save_offline_message(uint32_t from_id, uint32_t to_id, const std::string& content) {
  auto messages = load_all_messages();
  
  OfflineMessage msg;
  msg.from_id = from_id;
  msg.to_id = to_id;
  msg.content = content;
  msg.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
  msg.read = false;
  
  messages.push_back(msg);
  total_messages_++;
  
  save_all_messages(messages);
}

std::vector<OfflineMessage> MessageStorage::get_offline_messages(uint32_t to_id) {
  auto all_messages = load_all_messages();
  std::vector<OfflineMessage> result;
  
  for (auto& msg : all_messages) {
    if (msg.to_id == to_id && !msg.read) {
      result.push_back(msg);
      msg.read = true;
    }
  }
  
  // 保存已标记为已读的消息
  if (!result.empty()) {
    save_all_messages(all_messages);
  }
  
  return result;
}

void MessageStorage::mark_as_read(uint32_t to_id, uint32_t from_id) {
  auto messages = load_all_messages();
  
  for (auto& msg : messages) {
    if (msg.to_id == to_id && msg.from_id == from_id) {
      msg.read = true;
    }
  }
  
  save_all_messages(messages);
}

int MessageStorage::get_unread_count(uint32_t to_id) const {
  auto messages = load_all_messages();
  int count = 0;
  
  for (const auto& msg : messages) {
    if (msg.to_id == to_id && !msg.read) {
      count++;
    }
  }
  
  return count;
}

void MessageStorage::clear_user_messages(uint32_t user_id) {
  auto messages = load_all_messages();
  
  messages.erase(
    std::remove_if(messages.begin(), messages.end(),
      [user_id](const OfflineMessage& msg) {
        return msg.to_id == user_id || msg.from_id == user_id;
      }),
    messages.end()
  );
  
  total_messages_ = messages.size();
  save_all_messages(messages);
}

int MessageStorage::get_total_messages() const {
  return total_messages_;
}

std::vector<OfflineMessage> MessageStorage::load_all_messages() const {
  std::vector<OfflineMessage> messages;
  std::ifstream file(storage_file_);
  
  if (!file.is_open()) {
    return messages;
  }
  
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty()) continue;
    
    // 格式: from_id|to_id|content|timestamp|read
    std::stringstream ss(line);
    std::string token;
    
    OfflineMessage msg;
    
    // from_id
    if (std::getline(ss, token, '|')) {
      msg.from_id = std::stoul(token);
    }
    // to_id
    if (std::getline(ss, token, '|')) {
      msg.to_id = std::stoul(token);
    }
    // content
    if (std::getline(ss, token, '|')) {
      msg.content = token;
    }
    // timestamp
    if (std::getline(ss, token, '|')) {
      msg.timestamp = std::stoull(token);
    }
    // read
    if (std::getline(ss, token, '|')) {
      msg.read = (token == "1");
    }
    
    messages.push_back(msg);
  }
  
  file.close();
  return messages;
}

void MessageStorage::save_all_messages(const std::vector<OfflineMessage>& messages) const {
  std::ofstream file(storage_file_);
  
  if (!file.is_open()) {
    return;
  }
  
  for (const auto& msg : messages) {
    file << msg.from_id << "|"
         << msg.to_id << "|"
         << msg.content << "|"
         << msg.timestamp << "|"
         << (msg.read ? "1" : "0") << "\n";
  }
  
  file.close();
}

} // namespace im
