# 群组聊天功能文档 (P2 功能扩展)

## 功能概述

群组聊天模块为 IM 项目添加了完整的群组管理和消息广播功能，支持多用户实时群组通信。

## 核心功能

### 1. 群组管理

#### 创建群组 (CREATE_GROUP)
```cpp
请求: {"group_name": "编程俱乐部", "description": "我们一起写代码"}
响应: {"status": "ok", "group_id": 1000, "group_name": "编程俱乐部"}
```
- 创建者自动加入群组
- 群组 ID 从 1000 开始自动递增

#### 加入群组 (JOIN_GROUP)
```cpp
请求: {"group_id": 1000}
响应: {"status": "ok", "message": "Joined group"}
```
- 验证用户登陆状态
- 防止重复加入（已在群组则返回错误）

#### 离开群组 (LEAVE_GROUP)
```cpp
请求: {"group_id": 1000}
响应: {"status": "ok", "message": "Left group"}
```

#### 列出群组 (LIST_GROUPS)
```cpp
请求: {}
响应: {
  "status": "ok",
  "groups": [
    {"group_id": 1000, "group_name": "编程俱乐部", "member_count": 3},
    {"group_id": 1001, "group_name": "产品讨论", "member_count": 5}
  ]
}
```

#### 查询群组成员 (GROUP_MEMBERS)
```cpp
请求: {"group_id": 1000}
响应: {
  "status": "ok",
  "members": [1, 2, 3]  // 用户 ID 列表
}
```

### 2. 群组消息广播 (CHAT_GROUP)

```cpp
请求: {"group_id": 1000, "text": "大家好，今天的代码审查进行中！"}

// 群组内所有在线成员接收到:
响应: {
  "group_id": 1000,
  "from_user_id": 1,
  "from_username": "alice",
  "text": "大家好，今天的代码审查进行中！"
}
```

**关键特性：**
- 实时广播给所有群组内在线成员
- 发送者立即获得确认回复
- 离线成员不接收群组消息（可扩展为离线消息队列）

## 协议定义

### 消息类型 (im_protocol.hpp)

```cpp
enum class MsgType : uint16_t {
  CREATE_GROUP    = 7,   // 创建群组
  JOIN_GROUP      = 8,   // 加入群组
  LEAVE_GROUP     = 9,   // 离开群组
  CHAT_GROUP      = 10,  // 群组聊天消息
  LIST_GROUPS     = 11,  // 查询用户的群组
  GROUP_MEMBERS   = 12,  // 查询群组成员
};
```

## 核心实现

### GroupManager (group_manager.hpp/cpp)

**内存存储结构：**
```cpp
struct GroupInfo {
  uint32_t group_id;                    // 群组唯一 ID
  std::string group_name;               // 群组名称
  std::string description;              // 群组描述
  uint32_t creator_id;                  // 创建者 ID
  std::unordered_set<uint32_t> members; // 成员集合 → O(1) 查询
  std::chrono::system_clock::time_point created_time; // 创建时间
};
```

**关键方法：**
- `create_group()`: 创建新群组，返回 group_id
- `add_member()`: 添加成员到群组
- `remove_member()`: 从群组移除成员
- `is_member()`: O(1) 成员检查
- `get_group_members()`: 获取所有成员列表
- `get_user_groups()`: 获取用户所有群组

### MessageHandler 集成

**新增处理方法：**
```cpp
Packet handle_create_group(int fd, const Packet& pkt);
Packet handle_join_group(int fd, const Packet& pkt);
Packet handle_leave_group(int fd, const Packet& pkt);
Packet handle_chat_group(int fd, const Packet& pkt);
Packet handle_list_groups(int fd, const Packet& pkt);
Packet handle_group_members(int fd, const Packet& pkt);
```

**广播逻辑（handle_message）：**
```cpp
case MsgType::CHAT_GROUP:
  // 1. 验证用户登陆和群组成员身份
  // 2. 记录日志
  // 3. 遍历群组所有成员的连接 fd
  // 4. 将消息加入 notify_targets 列表
  // 5. 立即回复发送者确认
```

## 技术亮点

### 1. **高效的成员查询**
- 使用 `unordered_set` 存储群组成员
- 检查成员身份: O(1) 时间复杂度
- 获取所有成员: O(n) 线性复杂度

### 2. **安全的消息广播**
```cpp
// 验证用户身份 → 验证群组存在 → 验证成员身份 → 广播消息
```
三层验证机制防止非法消息

### 3. **灵活的日志记录**
```cpp
LOG_INFO("Group created: id=" + std::to_string(group_id) + " name=" + group_name);
LOG_INFO("GroupChat: " + sender->username + " -> group " + std::to_string(group_id));
```
所有操作都有完整日志跟踪

## 测试用例

参考 `test_group_chat.py`，完整演示以下场景：

### 测试场景 1: 基础群组操作
```
[1] Alice、Bob、Charlie 连接到服务器
[2] 三人登陆认证
[3] Alice 创建 "Programming Club" 群组
[4] Bob 和 Charlie 加入群组
[5] 查询群组成员 → 返回 [alice, bob, charlie]
[6] 列出群组 → 显示群组信息和成员数
```

### 测试场景 2: 群组消息广播
```
[1] Alice 发送: "大家好，欢迎加入编程俱乐部！"
    ✓ Bob 实时接收到消息
    ✓ Charlie 实时接收到消息

[2] Bob 发送: "Hello everyone! 很高兴认识你们"
    ✓ Alice 实时接收到消息
    ✓ Charlie 实时接收到消息

[3] Charlie 发送: "Hi team! Let's build something awesome"
    ✓ Alice 实时接收到消息
    ✓ Bob 实时接收到消息
```

## 运行测试

```bash
# 编译
./build.sh server

# 启动服务器（在一个终端）
./server_im 9999

# 运行群组聊天测试（在另一个终端）
python3 test_group_chat.py
```

**预期输出：**
```
========== 群组聊天功能测试 ==========
✓ Alice 连接成功
✓ Bob 连接成功
✓ Charlie 连接成功

✓ 群组创建成功，群组ID: 1000
✓ Bob 和 Charlie 加入群组

✓ 群组成员: [1, 2, 3]

✅ Alice 收到群组消息
✅ Bob 收到群组消息  
✅ Charlie 收到群组消息

✅ 群组聊天测试完成！
```

## 扩展功能（未来计划）

### 1. 群组消息持久化
```cpp
// 存储群组消息到 messages.db
// 支持群组历史消息查询
```

### 2. 离线群组消息队列
```cpp
// 离线用户重新登陆时推送离线群组消息
// 类似单聊离线消息机制
```

### 3. 群组管理权限
```cpp
// 群组管理员管理
// 消息内容审查
// 成员禁言/踢出
```

### 4. 群组通知
```cpp
// 成员加入/离开通知
// 群组信息变更通知
```

## 架构对比

### 单聊 vs 群组聊天

| 特性 | 单聊 (CHAT_TO) | 群组聊天 (CHAT_GROUP) |
|------|---|---|
| 消息路由 | 点对点 | 一对多广播 |
| 离线处理 | 保存离线消息 | 暂不保存（可扩展） |
| 成员验证 | 检查目标用户 | 遍历群组成员 |
| 广播范围 | 1 个 fd | N 个 fd |

## 面试亮点

1. **系统设计**：展示如何在现有架构中添加新功能而不破坏现有逻辑
2. **数据结构选择**：`unordered_set` 提供 O(1) 成员检查
3. **实时性**：消息立即广播，无延迟
4. **安全性**：三层验证（认证 → 存在 → 成员身份）
5. **可扩展性**：协议框架支持后续添加更多群组功能
6. **日志跟踪**：完整的操作日志便于调试和监控

## 总结

群组聊天功能完整实现了从创建、成员管理到实时消息广播的全套逻辑，展示了在现有 IM 系统基础上扩展功能的能力，是面试中**展现系统设计和工程能力的好例子**。
