#include "group_manager.hpp"
#include <algorithm>

namespace im {

GroupManager::GroupManager() : next_group_id_(1000) {
}

GroupManager::~GroupManager() = default;

uint32_t GroupManager::create_group(uint32_t creator_id, const std::string& name,
                                     const std::string& description) {
  uint32_t group_id = next_group_id_++;

  GroupInfo group;
  group.group_id = group_id;
  group.group_name = name;
  group.description = description;
  group.creator_id = creator_id;
  group.members.insert(creator_id);  // 创建者自动加入
  group.created_time = std::chrono::system_clock::now();

  groups_[group_id] = group;

  return group_id;
}

bool GroupManager::delete_group(uint32_t group_id) {
  return groups_.erase(group_id) > 0;
}

bool GroupManager::add_member(uint32_t group_id, uint32_t user_id) {
  auto it = groups_.find(group_id);
  if (it == groups_.end()) {
    return false;
  }

  it->second.members.insert(user_id);
  return true;
}

bool GroupManager::remove_member(uint32_t group_id, uint32_t user_id) {
  auto it = groups_.find(group_id);
  if (it == groups_.end()) {
    return false;
  }

  return it->second.members.erase(user_id) > 0;
}

std::vector<uint32_t> GroupManager::get_user_groups(uint32_t user_id) const {
  std::vector<uint32_t> result;

  for (const auto& [group_id, group] : groups_) {
    if (group.members.count(user_id) > 0) {
      result.push_back(group_id);
    }
  }

  return result;
}

bool GroupManager::is_member(uint32_t group_id, uint32_t user_id) const {
  auto it = groups_.find(group_id);
  if (it == groups_.end()) {
    return false;
  }

  return it->second.members.count(user_id) > 0;
}

std::vector<uint32_t> GroupManager::get_group_members(uint32_t group_id) const {
  std::vector<uint32_t> result;

  auto it = groups_.find(group_id);
  if (it != groups_.end()) {
    result.insert(result.end(), it->second.members.begin(),
                  it->second.members.end());
  }

  return result;
}

const GroupInfo* GroupManager::get_group_info(uint32_t group_id) const {
  auto it = groups_.find(group_id);
  if (it == groups_.end()) {
    return nullptr;
  }

  return &it->second;
}

int GroupManager::get_group_count() const {
  return groups_.size();
}

void GroupManager::clear_all() {
  groups_.clear();
  next_group_id_ = 1000;
}

} // namespace im
