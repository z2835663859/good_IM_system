#include "message_handler.hpp"
#include "logger.hpp"
#include <cstdio>
#include <sstream>

namespace im {

MessageHandler::MessageHandler(std::shared_ptr<UserManager> user_mgr,
                               std::shared_ptr<ConnectionManager> conn_mgr,
                               std::shared_ptr<MessageStorage> msg_storage,
                               std::shared_ptr<ConnectionMonitor> conn_monitor,
                               std::shared_ptr<GroupManager> group_mgr)
    : user_mgr_(user_mgr), conn_mgr_(conn_mgr), msg_storage_(msg_storage),
      conn_monitor_(conn_monitor), group_mgr_(group_mgr) {
}

Packet MessageHandler::make_error_reply(const std::string& error_msg) {
  Packet reply;
  reply.type = (uint16_t)MsgType::NOTIFY;
  reply.body = "{\"status\":\"error\",\"message\":\"" + error_msg + "\"}";
  return reply;
}

Packet MessageHandler::make_success_reply(const std::string& data) {
  Packet reply;
  reply.type = (uint16_t)MsgType::NOTIFY;
  reply.body = "{\"status\":\"ok\",\"data\":\"" + data + "\"}";
  return reply;
}

// 返回LOGIN应答
Packet MessageHandler::make_login_reply(bool success, const std::string& data_or_msg) {
  Packet reply;
  reply.type = (uint16_t)MsgType::LOGIN;
  if (success) {
    reply.body = "{\"status\":\"ok\",\"data\":\"" + data_or_msg + "\"}";
  } else {
    reply.body = "{\"status\":\"error\",\"message\":\"" + data_or_msg + "\"}";
  }
  return reply;
}

Packet MessageHandler::handle_login(int fd, const Packet& pkt) {
  // 解析 JSON: {"username":"user1","password":"pass123"}
  // 简单的手工解析（实际应该用 JSON 库）

  // 提取 username 和 password
  std::string username, password;
  
  // 简单的正则查找
  size_t user_pos = pkt.body.find("\"username\":\"");
  size_t pass_pos = pkt.body.find("\"password\":\"");

  if (user_pos != std::string::npos && pass_pos != std::string::npos) {
    user_pos += 12;
    size_t user_end = pkt.body.find("\"", user_pos);
    username = pkt.body.substr(user_pos, user_end - user_pos);

    pass_pos += 12;
    size_t pass_end = pkt.body.find("\"", pass_pos);
    password = pkt.body.substr(pass_pos, pass_end - pass_pos);
  }

  if (username.empty() || password.empty()) {
    LOG_WARN("Login failed: missing username or password");
    return make_login_reply(false, "Missing username or password");
  }

  // 尝试登录
  auto [success, user_id_or_msg] = user_mgr_->login(username, password);

  if (!success) {
    LOG_WARN("Login failed: invalid credentials for " + username);
    return make_login_reply(false, "Invalid username or password");
  }

  uint32_t user_id = user_id_or_msg;

  // 认证连接
  conn_mgr_->authenticate_connection(fd, user_id);

  // 更新连接监控活动时间
  if (conn_monitor_) {
    conn_monitor_->update_activity(fd);
  }

  LOG_INFO("User " + username + " (id=" + std::to_string(user_id) + ") logged in");

  return make_login_reply(true, std::to_string(user_id));
}

Packet MessageHandler::handle_logout(int fd, const Packet& pkt) {
  uint32_t user_id = conn_mgr_->get_user_by_connection(fd);

  if (user_id == 0) {
    LOG_WARN("Logout failed: user not authenticated");
    return make_error_reply("Not logged in");
  }

  user_mgr_->logout(user_id);
  conn_mgr_->deauthenticate_connection(fd);

  LOG_INFO("User id=" + std::to_string(user_id) + " logged out");

  return make_success_reply("Logged out");
}

Packet MessageHandler::handle_heartbeat(int fd, const Packet& pkt) {
  uint32_t user_id = conn_mgr_->get_user_by_connection(fd);

  if (user_id > 0) {
    user_mgr_->update_last_active(user_id);
  }

  Packet reply;
  reply.type = (uint16_t)MsgType::HEARTBEAT;
  reply.body = "pong";

  return reply;
}

Packet MessageHandler::handle_chat(int fd, const Packet& pkt) {
  uint32_t user_id = conn_mgr_->get_user_by_connection(fd);

  if (user_id == 0) {
    return make_error_reply("Not logged in");
  }

  auto user = user_mgr_->get_user(user_id);
  if (!user) {
    return make_error_reply("User not found");
  }

  // 广播消息
  Packet broadcast;
  broadcast.type = (uint16_t)MsgType::CHAT;
  broadcast.body = "{\"from\":\"" + user->username + "\",\"text\":\"" + pkt.body + "\"}";

  printf("[Chat] from user %s (id=%u): %s\n", user->username.c_str(), user_id, pkt.body.c_str());

  return broadcast;
}

Packet MessageHandler::handle_chat_to(int fd, const Packet& pkt) {
  uint32_t sender_id = conn_mgr_->get_user_by_connection(fd);

  if (sender_id == 0) {
    LOG_WARN("ChatTo failed: sender not authenticated");
    return make_error_reply("Not logged in");
  }

  auto sender = user_mgr_->get_user(sender_id);
  if (!sender) {
    LOG_WARN("ChatTo failed: sender not found");
    return make_error_reply("Sender not found");
  }

  // 解析 {"to_user_id":2,"text":"hello"}
  size_t to_pos = pkt.body.find("\"to_user_id\":");
  size_t text_pos = pkt.body.find("\"text\":\"");

  if (to_pos == std::string::npos || text_pos == std::string::npos) {
    LOG_WARN("ChatTo failed: invalid format");
    return make_error_reply("Invalid format");
  }

  to_pos += 13;
  uint32_t to_user_id = std::stoul(pkt.body.substr(to_pos));

  text_pos += 8;
  size_t text_end = pkt.body.find("\"", text_pos);
  std::string text = pkt.body.substr(text_pos, text_end - text_pos);

  LOG_INFO("ChatTo: " + sender->username + " -> user " + std::to_string(to_user_id) + 
           ": " + text);

  Packet reply;
  reply.type = (uint16_t)MsgType::CHAT_TO;
  reply.body = "{\"from_user_id\":" + std::to_string(sender_id) + 
               ",\"from_username\":\"" + sender->username + 
               "\",\"text\":\"" + text + "\"}";

  // 检查目标用户是否在线
  int target_fd = conn_mgr_->get_connection_by_user(to_user_id);
  if (target_fd <= 0 && msg_storage_) {
    // 目标离线，保存离线消息
    msg_storage_->save_offline_message(sender_id, to_user_id, text);
    LOG_DEBUG("Offline message saved: " + std::to_string(sender_id) + " -> " + 
              std::to_string(to_user_id));
  }

  return reply;
}

MessageHandler::HandleResult MessageHandler::handle_message(int fd, const Packet& pkt) {
  HandleResult result;

  switch ((MsgType)pkt.type) {
    case MsgType::LOGIN:
      result.should_reply = true;
      result.reply_pkt = handle_login(fd, pkt);
      break;

    case MsgType::LOGOUT:
      result.should_reply = true;
      result.reply_pkt = handle_logout(fd, pkt);
      break;

    case MsgType::HEARTBEAT:
      result.should_reply = true;
      result.reply_pkt = handle_heartbeat(fd, pkt);
      break;

    case MsgType::CHAT:
      // 广播给所有已登录用户
      result.should_reply = false;
      {
        auto broadcast = handle_chat(fd, pkt);
        auto conns = conn_mgr_->get_authenticated_connections();
        result.notify_targets = conns;
        result.reply_pkt = broadcast;
      }
      break;

    case MsgType::CHAT_TO:
      // 单聊：路由到目标用户
      result.should_reply = false;
      {
        auto reply_pkt = handle_chat_to(fd, pkt);
        // 提取目标 user_id
        size_t to_pos = pkt.body.find("\"to_user_id\":");
        uint32_t to_user_id = std::stoul(pkt.body.substr(to_pos + 13));

        int target_fd = conn_mgr_->get_connection_by_user(to_user_id);
        if (target_fd > 0) {
          result.notify_targets.push_back(target_fd);
          result.reply_pkt = reply_pkt;
        } else {
          // 目标不在线
          result.should_reply = true;
          result.reply_pkt = make_error_reply("User offline");
        }
      }
      break;

    case MsgType::CREATE_GROUP:
      result.should_reply = true;
      result.reply_pkt = handle_create_group(fd, pkt);
      break;

    case MsgType::JOIN_GROUP:
      result.should_reply = true;
      result.reply_pkt = handle_join_group(fd, pkt);
      break;

    case MsgType::LEAVE_GROUP:
      result.should_reply = true;
      result.reply_pkt = handle_leave_group(fd, pkt);
      break;

    case MsgType::CHAT_GROUP:
      // 群组聊天：广播给群组内所有成员
      result.should_reply = true;  // 先回复发送者确认
      {
        auto reply_pkt = handle_chat_group(fd, pkt);
        // 提取 group_id
        size_t gid_pos = pkt.body.find("\"group_id\":");
        if (gid_pos != std::string::npos) {
          uint32_t group_id = std::stoul(pkt.body.substr(gid_pos + 11));
          
          if (group_mgr_) {
            auto members = group_mgr_->get_group_members(group_id);
            for (uint32_t member_id : members) {
              int member_fd = conn_mgr_->get_connection_by_user(member_id);
              if (member_fd > 0) {
                result.notify_targets.push_back(member_fd);
              }
            }
          }
        }
        result.reply_pkt = reply_pkt;
      }
      break;

    case MsgType::LIST_GROUPS:
      result.should_reply = true;
      result.reply_pkt = handle_list_groups(fd, pkt);
      break;

    case MsgType::GROUP_MEMBERS:
      result.should_reply = true;
      result.reply_pkt = handle_group_members(fd, pkt);
      break;

    default:
      result.should_reply = true;
      result.reply_pkt = make_error_reply("Unknown message type");
      break;
  }

  return result;
}

Packet MessageHandler::handle_create_group(int fd, const Packet& pkt) {
  uint32_t creator_id = conn_mgr_->get_user_by_connection(fd);

  if (creator_id == 0) {
    LOG_WARN("CreateGroup failed: user not authenticated");
    return make_error_reply("Not logged in");
  }

  auto creator = user_mgr_->get_user(creator_id);
  if (!creator) {
    LOG_WARN("CreateGroup failed: creator not found");
    return make_error_reply("Creator not found");
  }

  // 解析: {"group_name":"Programming Club","description":"We code together"}
  size_t name_pos = pkt.body.find("\"group_name\":\"");
  if (name_pos == std::string::npos) {
    LOG_WARN("CreateGroup failed: missing group_name");
    return make_error_reply("Missing group_name");
  }

  name_pos += 14;
  size_t name_end = pkt.body.find("\"", name_pos);
  std::string group_name = pkt.body.substr(name_pos, name_end - name_pos);

  // 提取 description（可选）
  std::string description = "";
  size_t desc_pos = pkt.body.find("\"description\":\"");
  if (desc_pos != std::string::npos) {
    desc_pos += 15;
    size_t desc_end = pkt.body.find("\"", desc_pos);
    description = pkt.body.substr(desc_pos, desc_end - desc_pos);
  }

  if (!group_mgr_) {
    LOG_WARN("CreateGroup failed: group manager not initialized");
    return make_error_reply("Group feature unavailable");
  }

  uint32_t group_id = group_mgr_->create_group(creator_id, group_name, description);
  // 创建者自动加入群组
  group_mgr_->add_member(group_id, creator_id);

  LOG_INFO("Group created: id=" + std::to_string(group_id) + " name=" + group_name + 
           " creator=" + creator->username);

  Packet reply;
  reply.type = (uint16_t)MsgType::CREATE_GROUP;
  reply.body = "{\"status\":\"ok\",\"group_id\":" + std::to_string(group_id) + 
               ",\"group_name\":\"" + group_name + "\"}";

  return reply;
}

Packet MessageHandler::handle_join_group(int fd, const Packet& pkt) {
  uint32_t user_id = conn_mgr_->get_user_by_connection(fd);

  if (user_id == 0) {
    LOG_WARN("JoinGroup failed: user not authenticated");
    return make_error_reply("Not logged in");
  }

  auto user = user_mgr_->get_user(user_id);
  if (!user) {
    LOG_WARN("JoinGroup failed: user not found");
    return make_error_reply("User not found");
  }

  // 解析: {"group_id":123}
  size_t gid_pos = pkt.body.find("\"group_id\":");
  if (gid_pos == std::string::npos) {
    LOG_WARN("JoinGroup failed: missing group_id");
    return make_error_reply("Missing group_id");
  }

  gid_pos += 11;
  uint32_t group_id = std::stoul(pkt.body.substr(gid_pos));

  if (!group_mgr_) {
    return make_error_reply("Group feature unavailable");
  }

  const GroupInfo* group = group_mgr_->get_group_info(group_id);
  if (!group) {
    LOG_WARN("JoinGroup failed: group not found: " + std::to_string(group_id));
    return make_error_reply("Group not found");
  }

  if (group_mgr_->is_member(group_id, user_id)) {
    LOG_INFO("JoinGroup: user " + user->username + " already in group " + group->group_name);
    return make_error_reply("Already in group");
  }

  group_mgr_->add_member(group_id, user_id);

  LOG_INFO("User " + user->username + " (id=" + std::to_string(user_id) + 
           ") joined group: " + group->group_name);

  Packet reply;
  reply.type = (uint16_t)MsgType::JOIN_GROUP;
  reply.body = "{\"status\":\"ok\",\"group_id\":" + std::to_string(group_id) + 
               ",\"group_name\":\"" + group->group_name + "\"}";

  return reply;
}

Packet MessageHandler::handle_leave_group(int fd, const Packet& pkt) {
  uint32_t user_id = conn_mgr_->get_user_by_connection(fd);

  if (user_id == 0) {
    LOG_WARN("LeaveGroup failed: user not authenticated");
    return make_error_reply("Not logged in");
  }

  auto user = user_mgr_->get_user(user_id);
  if (!user) {
    return make_error_reply("User not found");
  }

  // 解析: {"group_id":123}
  size_t gid_pos = pkt.body.find("\"group_id\":");
  if (gid_pos == std::string::npos) {
    return make_error_reply("Missing group_id");
  }

  gid_pos += 11;
  uint32_t group_id = std::stoul(pkt.body.substr(gid_pos));

  if (!group_mgr_) {
    return make_error_reply("Group feature unavailable");
  }

  if (!group_mgr_->is_member(group_id, user_id)) {
    return make_error_reply("Not in group");
  }

  group_mgr_->remove_member(group_id, user_id);

  LOG_INFO("User " + user->username + " (id=" + std::to_string(user_id) + 
           ") left group: " + std::to_string(group_id));

  Packet reply;
  reply.type = (uint16_t)MsgType::NOTIFY;
  reply.body = "{\"status\":\"ok\",\"message\":\"Left group\"}";

  return reply;
}

Packet MessageHandler::handle_chat_group(int fd, const Packet& pkt) {
  uint32_t sender_id = conn_mgr_->get_user_by_connection(fd);

  if (sender_id == 0) {
    LOG_WARN("ChatGroup failed: sender not authenticated");
    return make_error_reply("Not logged in");
  }

  auto sender = user_mgr_->get_user(sender_id);
  if (!sender) {
    return make_error_reply("Sender not found");
  }

  // 解析: {"group_id":123,"text":"Hello group"}
  size_t gid_pos = pkt.body.find("\"group_id\":");
  size_t text_pos = pkt.body.find("\"text\":\"");

  if (gid_pos == std::string::npos || text_pos == std::string::npos) {
    LOG_WARN("ChatGroup failed: invalid format");
    return make_error_reply("Invalid format");
  }

  gid_pos += 11;
  uint32_t group_id = std::stoul(pkt.body.substr(gid_pos));

  text_pos += 8;
  size_t text_end = pkt.body.find("\"", text_pos);
  std::string text = pkt.body.substr(text_pos, text_end - text_pos);

  if (!group_mgr_) {
    return make_error_reply("Group feature unavailable");
  }

  if (!group_mgr_->is_member(group_id, sender_id)) {
    LOG_WARN("ChatGroup failed: user not in group");
    return make_error_reply("Not in group");
  }

  LOG_INFO("GroupChat: " + sender->username + " -> group " + std::to_string(group_id) + 
           ": " + text);

  Packet reply;
  reply.type = (uint16_t)MsgType::CHAT_GROUP;
  reply.body = "{\"group_id\":" + std::to_string(group_id) + 
               ",\"from_user_id\":" + std::to_string(sender_id) +
               ",\"from_username\":\"" + sender->username + 
               "\",\"text\":\"" + text + "\"}";

  return reply;
}

Packet MessageHandler::handle_list_groups(int fd, const Packet& pkt) {
  uint32_t user_id = conn_mgr_->get_user_by_connection(fd);

  if (user_id == 0) {
    return make_error_reply("Not logged in");
  }

  if (!group_mgr_) {
    return make_error_reply("Group feature unavailable");
  }

  auto groups = group_mgr_->get_user_groups(user_id);

  std::string groups_json = "[";
  for (size_t i = 0; i < groups.size(); ++i) {
    uint32_t group_id = groups[i];
    const GroupInfo* group = group_mgr_->get_group_info(group_id);
    if (!group) continue;

    if (i > 0) groups_json += ",";
    groups_json += "{\"group_id\":" + std::to_string(group_id) + 
                   ",\"group_name\":\"" + group->group_name + 
                   "\",\"member_count\":" + std::to_string(group->members.size()) + "}";
  }
  groups_json += "]";

  Packet reply;
  reply.type = (uint16_t)MsgType::LIST_GROUPS;
  reply.body = "{\"status\":\"ok\",\"groups\":" + groups_json + "}";

  return reply;
}

Packet MessageHandler::handle_group_members(int fd, const Packet& pkt) {
  uint32_t user_id = conn_mgr_->get_user_by_connection(fd);

  if (user_id == 0) {
    return make_error_reply("Not logged in");
  }

  // 解析: {"group_id":123}
  size_t gid_pos = pkt.body.find("\"group_id\":");
  if (gid_pos == std::string::npos) {
    return make_error_reply("Missing group_id");
  }

  gid_pos += 11;
  uint32_t group_id = std::stoul(pkt.body.substr(gid_pos));

  if (!group_mgr_) {
    return make_error_reply("Group feature unavailable");
  }

  auto members = group_mgr_->get_group_members(group_id);

  std::string members_json = "[";
  for (size_t i = 0; i < members.size(); ++i) {
    if (i > 0) members_json += ",";
    members_json += std::to_string(members[i]);
  }
  members_json += "]";

  Packet reply;
  reply.type = (uint16_t)MsgType::NOTIFY;
  reply.body = "{\"status\":\"ok\",\"members\":" + members_json + "}";

  return reply;
}

} // namespace im
