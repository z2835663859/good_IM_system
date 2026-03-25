#include "connection_manager.hpp"
#include "user_manager.hpp"

namespace im {

ConnectionManager::ConnectionManager(std::shared_ptr<UserManager> user_mgr)
    : user_mgr_(user_mgr) {
}

void ConnectionManager::add_connection(int fd) {
  auto conn = std::make_shared<ConnectionInfo>();
  conn->fd = fd;
  conn->user_id = 0;
  conn->is_authenticated = false;
  connections_[fd] = conn;
}

void ConnectionManager::remove_connection(int fd) {
  auto it = connections_.find(fd);
  if (it != connections_.end()) {
    auto conn = it->second;
    if (conn->is_authenticated) {
      user_to_conn_.erase(conn->user_id);
    }
    connections_.erase(it);
  }
}

void ConnectionManager::authenticate_connection(int fd, uint32_t user_id) {
  auto conn = get_connection(fd);
  if (conn && user_mgr_) {
    conn->user_id = user_id;
    conn->is_authenticated = true;
    user_to_conn_[user_id] = fd;
    user_mgr_->update_last_active(user_id);
  }
}

void ConnectionManager::deauthenticate_connection(int fd) {
  auto conn = get_connection(fd);
  if (conn && conn->is_authenticated) {
    user_to_conn_.erase(conn->user_id);
    conn->is_authenticated = false;
    conn->user_id = 0;
  }
}

std::shared_ptr<ConnectionInfo> ConnectionManager::get_connection(int fd) const {
  auto it = connections_.find(fd);
  if (it != connections_.end()) {
    return it->second;
  }
  return nullptr;
}

int ConnectionManager::get_connection_by_user(uint32_t user_id) const {
  auto it = user_to_conn_.find(user_id);
  if (it != user_to_conn_.end()) {
    return it->second;
  }
  return -1;
}

uint32_t ConnectionManager::get_user_by_connection(int fd) const {
  auto conn = get_connection(fd);
  if (conn && conn->is_authenticated) {
    return conn->user_id;
  }
  return 0;
}

bool ConnectionManager::is_authenticated(int fd) const {
  auto conn = get_connection(fd);
  if (conn) {
    return conn->is_authenticated;
  }
  return false;
}

std::vector<int> ConnectionManager::get_authenticated_connections() const {
  std::vector<int> result;
  for (const auto& [fd, conn] : connections_) {
    if (conn->is_authenticated) {
      result.push_back(fd);
    }
  }
  return result;
}

} // namespace im
