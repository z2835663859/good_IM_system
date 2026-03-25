#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <mutex>
#include <chrono>
#include <ctime>

namespace im {

// 日志级别
class Logger {
public:
  enum class Level {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
  };

  // 单例模式
  static Logger& get_instance() {
    static Logger instance;
    return instance;
  }

  // 设置日志级别
  void set_level(Level level) {
    level_ = level;
  }

  // 启用文件输出
  void enable_file_output(const std::string& filename) {
    file_output_enabled_ = true;
    log_file_ = filename;
  }

  // 禁用文件输出
  void disable_file_output() {
    file_output_enabled_ = false;
  }

  // 记录日志
  void log(Level level, const std::string& message) {
    if (level < level_) {
      return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    std::string level_str;
    switch (level) {
      case Level::DEBUG:
        level_str = "DEBUG";
        break;
      case Level::INFO:
        level_str = "INFO";
        break;
      case Level::WARN:
        level_str = "WARN";
        break;
      case Level::ERROR:
        level_str = "ERROR";
        break;
    }

    std::string formatted = format_message(level_str, message);

    // 输出到 stdout
    std::cout << formatted;
    std::cout.flush();

    // 输出到文件
    if (file_output_enabled_) {
      std::ofstream file(log_file_, std::ios::app);
      if (file.is_open()) {
        file << formatted;
        file.close();
      }
    }
  }

private:
  Logger() : level_(Level::INFO), file_output_enabled_(false) {}

  ~Logger() = default;

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  std::string format_message(const std::string& level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto tm = std::localtime(&time);

    std::ostringstream oss;
    oss << "[" << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << "] ["
        << level << "] " << message << "\n";

    return oss.str();
  }

  Level level_;
  bool file_output_enabled_;
  std::string log_file_;
  std::mutex mutex_;
};

// 宏定义便捷日志接口
#define LOG_DEBUG(msg) \
  im::Logger::get_instance().log(im::Logger::Level::DEBUG, msg)

#define LOG_INFO(msg) \
  im::Logger::get_instance().log(im::Logger::Level::INFO, msg)

#define LOG_WARN(msg) \
  im::Logger::get_instance().log(im::Logger::Level::WARN, msg)

#define LOG_ERROR(msg) \
  im::Logger::get_instance().log(im::Logger::Level::ERROR, msg)

} // namespace im
