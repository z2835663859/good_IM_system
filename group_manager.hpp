#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

namespace im {

// 分组信息
struct GroupInfo {
  uint32_t group_id;
  std::string group_name;
  std::string description;
  uint32_t creator_id;
  std::unordered_set<uint32_t> members;
  std::chrono::system_clock::time_point created_time;
};

// 分组管理器
class GroupManager {
public:
  GroupManager();
  ~GroupManager();

  // 创建分组
  uint32_t create_group(uint32_t creator_id, const std::string& name,
                        const std::string& description = "");

  // 删除分组
  bool delete_group(uint32_t group_id);

  // 添加成员到分组
  bool add_member(uint32_t group_id, uint32_t user_id);

  // 从分组移除成员
  bool remove_member(uint32_t group_id, uint32_t user_id);

  // 获取用户加入的所有分组
  std::vector<uint32_t> get_user_groups(uint32_t user_id) const;

  // 检查用户是否在分组中
  bool is_member(uint32_t group_id, uint32_t user_id) const;

  // 获取分组的所有成员
  std::vector<uint32_t> get_group_members(uint32_t group_id) const;

  // 获取分组信息
  const GroupInfo* get_group_info(uint32_t group_id) const;

  // 获取分组数量
  int get_group_count() const;

  // 清空所有分组
  void clear_all();

private:
  std::unordered_map<uint32_t, GroupInfo> groups_;
  uint32_t next_group_id_;
};

} // namespace im
