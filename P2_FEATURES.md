# P2 产品特性（高级功能模块）

IM 系统的第二阶段特性，提升系统可靠性和用户体验。

---

## 📦 核心模块

### 1. 离线消息存储 (message_storage)

**功能说明**
- 当目标用户离线时，自动保存消息
- 用户重新登录时，推送所有离线消息
- 支持消息已读状态追踪

**存储格式**
```
messages.db （文本文件）
from_id|to_id|content|timestamp|read_flag
2|1|Hi alice!|1711428896000|0
3|1|Another message|1711428897000|1
```

**API 接口**
```cpp
// 保存离线消息
void save_offline_message(uint32_t from_id, uint32_t to_id, 
                          const std::string& content);

// 获取用户的所有离线消息
std::vector<OfflineMessage> get_offline_messages(uint32_t to_id);

// 标记消息为已读
void mark_as_read(uint32_t to_id, uint32_t from_id);

// 获取未读消息数
int get_unread_count(uint32_t to_id) const;
```

**技术亮点**
- ✅ 文件 I/O 操作管理
- ✅ 字符串解析与序列化
- ✅ 线程安全的消息管理

---

### 2. 结构化日志系统 (logger)

**功能说明**
- 四个日志级别：DEBUG, INFO, WARN, ERROR
- 支持文件和标准输出双重输出
- 自动时间戳和日志级别标记

**日志格式**
```
[2026-03-25 16:36:09] [INFO] User alice (id=1) logged in
[2026-03-25 16:36:09] [DEBUG] Sent reply to fd=4
[2026-03-25 16:36:36] [WARN] Found 2 timeout connections
[2026-03-25 16:36:36] [ERROR] Failed to bind to port 9999
```

**使用示例**
```cpp
// 初始化
Logger& logger = Logger::get_instance();
logger.set_level(Logger::Level::DEBUG);
logger.enable_file_output("im_server.log");

// 记录日志
LOG_INFO("Server started on port 9999");
LOG_WARN("Connection timeout detected");
LOG_ERROR("Socket bind failed");
```

**技术亮点**
- ✅ 单例模式（线程安全）
- ✅ 时间戳格式化
- ✅ 日志级别过滤

---

### 3. 连接监控器 (connection_monitor)

**功能说明**
- 跟踪每个活跃连接的最后活动时间
- 定期检测超时连接
- 自动清理空闲连接

**超时策略**
```
连接超时时间：300 秒（可配置）
检查间隔：30 秒
```

**API 接口**
```cpp
// 更新连接活动时间
void update_activity(int fd);

// 检查超时连接（返回超时fd列表）
std::vector<int> check_timeout();

// 获取活跃连接数
int get_active_count() const;

// 获取最后活动时间（单位：秒）
int64_t get_last_activity(int fd) const;
```

**日志示例**
```
[WARN] Found 3 timeout connections
[INFO] Closing timeout connection fd=4
[DEBUG] Active connections: 12
```

**技术亮点**
- ✅ 时间戳对比和计时
- ✅ 迭代器安全删除
- ✅ 后台定期清理

---

### 4. 分组管理器 (group_manager)

**功能说明**
- 支持创建/删除聊天分组
- 管理分组成员
- 查询用户所属分组

**为未来特性做准备**
- 分组消息广播（后续实现）
- 分组权限管理（后续实现）
- 分组邀请系统（后续实现）

**API 接口**
```cpp
// 创建分组，返回分组ID
uint32_t create_group(uint32_t creator_id, const std::string& name,
                      const std::string& description = "");

// 添加/移除成员
bool add_member(uint32_t group_id, uint32_t user_id);
bool remove_member(uint32_t group_id, uint32_t user_id);

// 查询分组信息
std::vector<uint32_t> get_user_groups(uint32_t user_id) const;
std::vector<uint32_t> get_group_members(uint32_t group_id) const;
bool is_member(uint32_t group_id, uint32_t user_id) const;
```

**技术亮点**
- ✅ 泛型容器使用（map, set）
- ✅ 快速成员查询（O(1) 时间复杂度）
- ✅ 灵活的分组架构

---

## 🔄 整合方式

### P2 模块在服务器中的生命周期

```
1. 初始化阶段
   server_im.cpp main()
   ├─ 创建 MessageStorage 实例
   ├─ 创建 Logger 实例（启用文件输出）
   ├─ 创建 ConnectionMonitor 实例
   └─ 创建 GroupManager 实例

2. 消息处理阶段
   message_handler.cpp
   ├─ handle_login():  调用 conn_monitor->update_activity()
   ├─ handle_chat_to(): 调用 msg_storage->save_offline_message()
   └─ 各处理函数:  调用 LOG_*(msg)

3. 定期维护阶段 (30秒一次)
   server_im.cpp main loop
   ├─ 检查超时连接: conn_monitor->check_timeout()
   ├─ 关闭超时连接
   ├─ 清理超时用户
   └─ 记录统计日志

4. 关闭阶段
   server_im.cpp global cleanup
   └─ 自动析构所有模块
```

---

## 📊 性能特性

| 特性 | 实现 | 性能 |
|------|------|------|
| 离线消息查询 | 文本扫描 | O(n) |
| 连接超时检查 | 哈希表遍历 | O(n) |
| 分组成员查询 | 集合查询 | O(1) |
| 日志输出 | 无锁 + 互斥锁 | 低开销 |

---

## 🧪 测试

### 离线消息功能测试
```bash
python3 test_offline.py
```

**测试场景**
1. alice 登录
2. alice 登出
3. bob 发送消息给离线的 alice
4. alice 重新登录
5. ✅ alice 接收到 bob 的离线消息

**成功标志**
```
✓ 成功接收离线消息！
消息内容: {"from_user_id":2,"text":"Hi alice, this is offline message from bob","offline":true}
```

---

## 🚀 编译指令

```bash
# 编译包含 P2 特性的完整服务器
clang++ -std=c++17 -o server_im \
  im_protocol.cpp user_manager.cpp connection_manager.cpp \
  message_handler.cpp message_storage.cpp logger.cpp \
  connection_monitor.cpp group_manager.cpp server_im.cpp

# 启动服务器（可选指定端口，默认9999）
./server_im 9999

# 查看日志
tail -f im_server.log
```

---

## 🔮 后续增强方向

### P3 特性规划
- [ ] SQLite 数据库持久化（替代文本文件）
- [ ] 分组消息广播（GROUP_CHAT 消息类型）
- [ ] 消息加密（TLS/SSL）
- [ ] 用户头像和昵称管理
- [ ] 消息撤回和编辑
- [ ] 消息搜索和过滤
- [ ] 链接预览功能
- [ ] 语音消息支媑持

### 扩展方向
- [ ] 移动客户端（iOS/Android）
- [ ] Web 客户端（WebSocket）
- [ ] 系统通知
- [ ] 防洪水安全
- [ ] 用户禁言/黑名单
- [ ] 消息审核系统

