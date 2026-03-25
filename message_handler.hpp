#pragma once

#include "im_protocol.hpp"
#include "user_manager.hpp"
#include "connection_manager.hpp"
#include "message_storage.hpp"
#include "connection_monitor.hpp"
#include "group_manager.hpp"
#include <memory>
#include <functional>

namespace im {

// 业务逻辑处理
class MessageHandler {
public:
  MessageHandler(std::shared_ptr<UserManager> user_mgr,
                 std::shared_ptr<ConnectionManager> conn_mgr,
                 std::shared_ptr<MessageStorage> msg_storage = nullptr,
                 std::shared_ptr<ConnectionMonitor> conn_monitor = nullptr,
                 std::shared_ptr<GroupManager> group_mgr = nullptr);

  // 处理消息（返回是否需要回包）
  struct HandleResult {
    bool should_reply = false;
    Packet reply_pkt;
    std::vector<int> notify_targets;  // 需要通知的其他连接 fd
  };

  HandleResult handle_message(int fd, const Packet& pkt);

private:
  std::shared_ptr<UserManager> user_mgr_;
  std::shared_ptr<ConnectionManager> conn_mgr_;
  std::shared_ptr<MessageStorage> msg_storage_;
  std::shared_ptr<ConnectionMonitor> conn_monitor_;
  std::shared_ptr<GroupManager> group_mgr_;

  // 各类消息的处理
  Packet handle_login(int fd, const Packet& pkt);
  Packet handle_logout(int fd, const Packet& pkt);
  Packet handle_chat(int fd, const Packet& pkt);
  Packet handle_chat_to(int fd, const Packet& pkt);
  Packet handle_heartbeat(int fd, const Packet& pkt);
  Packet handle_create_group(int fd, const Packet& pkt);
  Packet handle_join_group(int fd, const Packet& pkt);
  Packet handle_leave_group(int fd, const Packet& pkt);
  Packet handle_chat_group(int fd, const Packet& pkt);
  Packet handle_list_groups(int fd, const Packet& pkt);
  Packet handle_group_members(int fd, const Packet& pkt);

  // 辅助：创建错误回包
  Packet make_error_reply(const std::string& error_msg);

  // 辅助：创建成功回包
  Packet make_success_reply(const std::string& data);

  // 辅助：创建登录回包
  Packet make_login_reply(bool success, const std::string& data_or_msg);
};

} // namespace im
