#include "user_manager.hpp"
#include <functional>
#include <algorithm>

namespace im {

std::string UserManager::hash_password(const std::string& password) {
  // 简单演示用的哈希（实际应该用 bcrypt）
  std::hash<std::string> hasher;
  return std::to_string(hasher(password + "salt"));  // 加盐
}

std::pair<bool, std::string> UserManager::register_user(const std::string& username,
                                                         const std::string& password) {
  // 检查用户是否已存在
  if (username_to_id_.count(username)) {
    return {false, "Username already registered"};
  }

  if (username.empty() || password.empty()) {
    return {false, "Username or password cannot be empty"};
  }

  // 创建新用户
  uint32_t user_id = next_user_id_++;
  auto user = std::make_shared<User>();
  user->user_id = user_id;
  user->username = username;
  user->password_hash = hash_password(password);
  user->is_online = false;

  users_[user_id] = user;
  username_to_id_[username] = user_id;

  return {true, std::to_string(user_id)};
}

std::pair<bool, uint32_t> UserManager::login(const std::string& username,
                                             const std::string& password) {
  auto it = username_to_id_.find(username);
  if (it == username_to_id_.end()) {
    return {false, 0};  // 用户不存在
  }

  uint32_t user_id = it->second;
  auto user = users_[user_id];

  // 验证密码
  if (user->password_hash != hash_password(password)) {
    return {false, 0};  // 密码错误
  }

  // 标记为在线
  user->is_online = true;
  user->last_active = std::chrono::system_clock::now();
  online_users_[user_id] = username;

  return {true, user_id};
}

void UserManager::logout(uint32_t user_id) {
  auto it = users_.find(user_id);
  if (it != users_.end()) {
    it->second->is_online = false;
  }

  online_users_.erase(user_id);
}

std::shared_ptr<User> UserManager::get_user(uint32_t user_id) const {
  auto it = users_.find(user_id);
  if (it != users_.end()) {
    return it->second;
  }
  return nullptr;
}

std::shared_ptr<User> UserManager::get_user_by_name(const std::string& username) const {
  auto it = username_to_id_.find(username);
  if (it != username_to_id_.end()) {
    return get_user(it->second);
  }
  return nullptr;
}

bool UserManager::is_online(uint32_t user_id) const {
  return online_users_.count(user_id) > 0;
}

std::vector<std::shared_ptr<User>> UserManager::get_online_users() const {
  std::vector<std::shared_ptr<User>> result;
  for (const auto& [user_id, _] : online_users_) {
    auto user = get_user(user_id);
    if (user) {
      result.push_back(user);
    }
  }
  return result;
}

void UserManager::update_last_active(uint32_t user_id) {
  auto it = users_.find(user_id);
  if (it != users_.end()) {
    it->second->last_active = std::chrono::system_clock::now();
  }
}

void UserManager::cleanup_timeout_users(int timeout_sec) {
  auto now = std::chrono::system_clock::now();
  std::vector<uint32_t> to_logout;

  for (const auto& [user_id, username] : online_users_) {
    auto user = users_[user_id];
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - user->last_active).count();

    if (elapsed > timeout_sec) {
      to_logout.push_back(user_id);
    }
  }

  for (uint32_t user_id : to_logout) {
    logout(user_id);
  }
}

} // namespace im
