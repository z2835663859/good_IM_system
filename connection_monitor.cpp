#include "connection_monitor.hpp"

namespace im {

ConnectionMonitor::ConnectionMonitor(int timeout_seconds)
    : timeout_seconds_(timeout_seconds) {
}

ConnectionMonitor::~ConnectionMonitor() = default;

void ConnectionMonitor::update_activity(int fd) {
  activities_[fd].last_activity = std::chrono::system_clock::now();
}

std::vector<int> ConnectionMonitor::check_timeout() {
  std::vector<int> timeout_fds;
  auto now = std::chrono::system_clock::now();

  for (auto it = activities_.begin(); it != activities_.end(); ) {
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        now - it->second.last_activity);

    if (duration.count() >= timeout_seconds_) {
      timeout_fds.push_back(it->first);
      it = activities_.erase(it);
    } else {
      ++it;
    }
  }

  return timeout_fds;
}

int ConnectionMonitor::get_active_count() const {
  return activities_.size();
}

void ConnectionMonitor::remove_connection(int fd) {
  activities_.erase(fd);
}

int64_t ConnectionMonitor::get_last_activity(int fd) const {
  auto it = activities_.find(fd);
  if (it == activities_.end()) {
    return -1;
  }

  auto duration = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now() - it->second.last_activity);
  
  return duration.count();
}

} // namespace im
