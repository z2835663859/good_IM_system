#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <chrono>
#include <functional>

namespace im {

// 用户信息
struct User {
  uint32_t user_id;
  std::string username;
  std::string password_hash;  // 实际应用会存哈希值
  bool is_online = false;
  std::chrono::system_clock::time_point last_active;
};

// 用户映射：用户名 -> User，以及在线用户映射
class UserManager {
public:
  // 用户注册
  // 返回: (成功, 错误消息或 user_id)
  std::pair<bool, std::string> register_user(const std::string& username, 
                                             const std::string& password);

  // 用户登录
  // 返回: (成功, user_id 或错误消息)
  std::pair<bool, uint32_t> login(const std::string& username, 
                                   const std::string& password);

  // 用户注销
  void logout(uint32_t user_id);

  // 获取用户信息
  std::shared_ptr<User> get_user(uint32_t user_id) const;
  std::shared_ptr<User> get_user_by_name(const std::string& username) const;

  // 检查用户在线状态
  bool is_online(uint32_t user_id) const;

  // 获取所有在线用户
  std::vector<std::shared_ptr<User>> get_online_users() const;

  // 更新用户最后活动时间
  void update_last_active(uint32_t user_id);

  // 清理超时用户（可选的超时断开）
  void cleanup_timeout_users(int timeout_sec);

private:
  uint32_t next_user_id_ = 1;
  std::map<uint32_t, std::shared_ptr<User>> users_;        // user_id -> User
  std::map<std::string, uint32_t> username_to_id_;         // username -> user_id
  std::map<uint32_t, std::string> online_users_;            // 在线用户：user_id -> username

  std::string hash_password(const std::string& password);
};

} // namespace im
