#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace im {

class UserManager;

// 连接信息
struct ConnectionInfo {
  int fd;
  uint32_t user_id = 0;  // 0 表示未登录
  bool is_authenticated = false;
};

// 连接管理器：维护 fd <-> user_id 的映射
class ConnectionManager {
public:
  ConnectionManager(std::shared_ptr<UserManager> user_mgr);

  // 添加新连接
  void add_connection(int fd);

  // 移除连接
  void remove_connection(int fd);

  // 认证连接（关联到 user_id）
  void authenticate_connection(int fd, uint32_t user_id);

  // 反认证连接（用户注销）
  void deauthenticate_connection(int fd);

  // 获取连接信息
  std::shared_ptr<ConnectionInfo> get_connection(int fd) const;

  // 获取用户对应的连接
  int get_connection_by_user(uint32_t user_id) const;

  // 获取连接对应的 user_id
  uint32_t get_user_by_connection(int fd) const;

  // 获取是否已认证
  bool is_authenticated(int fd) const;

  // 获取所有已认证的连接
  std::vector<int> get_authenticated_connections() const;

private:
  std::shared_ptr<UserManager> user_mgr_;
  std::map<int, std::shared_ptr<ConnectionInfo>> connections_;  // fd -> ConnectionInfo
  std::map<uint32_t, int> user_to_conn_;                        // user_id -> fd
};

} // namespace im
