#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <memory>

namespace im {

// 前向声明
class Conn;

// 连接监控器
class ConnectionMonitor {
public:
  explicit ConnectionMonitor(int timeout_seconds = 300);
  ~ConnectionMonitor();

  // 更新连接的活动时间
  void update_activity(int fd);

  // 检查超时的连接
  std::vector<int> check_timeout();

  // 获取活跃连接数
  int get_active_count() const;

  // 移除连接
  void remove_connection(int fd);

  // 获取最后活动时间（秒）
  int64_t get_last_activity(int fd) const;

private:
  struct ConnectionActivity {
    std::chrono::system_clock::time_point last_activity;
    std::shared_ptr<int> conn_ptr;  // 用于连接生命周期追踪
  };

  int timeout_seconds_;
  std::unordered_map<int, ConnectionActivity> activities_;
};

} // namespace im
