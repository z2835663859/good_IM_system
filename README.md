# 基于Reactor模型的C++高性能即时通讯系统

一个成熟的即时通讯后端系统，演示 TCP 网络编程、协议设计、多连接管理、用户认证等核心技术。

---

## 📋 项目概述

### 功能特性

✅ **网络层**
- TCP 多连接管理（非阻塞 IO）
- 自定义应用层协议（长度前缀）
- 粘包/半包自动处理
- 部分写缓冲机制

✅ **业务层** (P1)
- 用户注册/登录（密码哈希）
- 心跳保活（连接活性检测）
- 单聊消息路由
- 在线状态管理

✅ **高级特性** (P2)
- 离线消息存储与推送
- 结构化日志系统（支持文件输出）
- 连接超时检测与清理
- 群组管理与实时消息广播

✅ **架构**
- Reactor 模式（事件驱动）
- 连接管理器（fd <-> user_id 映射）
- 消息处理器（业务逻辑解耦）
- 用户管理器（认证 + 状态）

---

## 🏗️ 文件结构

```
my_project/
├── 协议层
│   ├── im_protocol.hpp          # 协议定义、编解码器
│   └── im_protocol.cpp
│
├── 网络层 (Reactor)
│   ├── event_loop.hpp           # 事件循环（kqueue）
│   ├── event_loop.cpp
│   ├── conn.hpp                 # 连接类（收发、缓冲）
│   ├── conn.cpp
│   ├── server.hpp               # 服务器（监听、accept）
│   └── server.cpp
│
├── 业务层
│   ├── user_manager.hpp         # 用户管理（注册登录认证）
│   ├── user_manager.cpp
│   ├── connection_manager.hpp   # 连接-用户映射
│   ├── connection_manager.cpp
│   ├── message_handler.hpp      # 消息分发处理
│   └── message_handler.cpp
│
├── P2 高级特性
│   ├── message_storage.hpp      # 离线消息存储
│   ├── message_storage.cpp      # （基于文本文件的持久化）
│   ├── logger.hpp               # 结构化日志系统
│   ├── logger.cpp
│   ├── connection_monitor.hpp   # 连接超时监控
│   ├── connection_monitor.cpp
│   ├── group_manager.hpp        # 分组管理
│   └── group_manager.cpp
│
├── 服务端程序
│   ├── server_im.cpp            # 完整 IM 服务器（包含P2特性）
│   ├── server_simple.cpp        # 简化版（select + echo）
│   ├── server_min.cpp           # 最小版（两客户端 demo）
│   └── server_im               # 编译产物
│
├── 客户端程序
│   ├── im_client.cpp            # 交互式客户端
│   ├── test_client.cpp          # 自动化测试客户端
│   ├── test_offline.py          # 离线消息功能测试
│   └── im_client               # 编译产物
│
└── 测试程序
    ├── test_codec.cpp           # 协议编解码测试
    ├── test_multi.cpp           # 多连接测试
    └── test_codec              # 编译产物
```

---

## 🔧 编译说明

### 系统要求
- **macOS** (使用 kqueue)
- Clang/GCC C++17
- 无外部依赖

### 编译命令

#### 1️⃣ 编译完整 IM 服务器（推荐，包含P2特性）
```bash
cd /Users/z2835663859/Qt_study/my_project

clang++ -std=c++17 -Wall -Wextra -o server_im \
  im_protocol.cpp event_loop.cpp conn.cpp server.cpp \
  user_manager.cpp connection_manager.cpp message_handler.cpp \
  message_storage.cpp logger.cpp connection_monitor.cpp group_manager.cpp \
  server_im.cpp
```

或使用编译脚本：
```bash
./build.sh server
```

*P2 模块说明：*
- `message_storage.cpp` - 离线消息存储
- `logger.cpp` - 结构化日志输出
- `connection_monitor.cpp` - 连接超时监控
- `group_manager.cpp` - 群组管理

#### 2️⃣ 编译交互式客户端
```bash
clang++ -std=c++17 -Wall -Wextra -o im_client \
  im_protocol.cpp im_client.cpp
```

#### 3️⃣ 编译自动化测试客户端
```bash
clang++ -std=c++17 -o test_client im_protocol.cpp test_client.cpp
```

#### 4️⃣ 编译协议测试
```bash
clang++ -std=c++17 -o test_codec im_protocol.cpp test_codec.cpp
```

#### 5️⃣ 编译简化版 echo 服务器
```bash
clang++ -std=c++17 -o server_simple im_protocol.cpp server_simple.cpp
```

---

## 🚀 快速开始

### 场景 1：完整通讯系统演示

#### 启动服务器
```bash
./server_im
```

输出示例：
```
[Server] IM Server started on 127.0.0.1:9000
```

#### 启动客户端 #1 (Alice)
```bash
./im_client
```

交互示例：
```
> register alice 123456
Register OK, your user ID is: 1

> login alice 123456
Login OK

> send 2 Hello Bob!
Message sent to user 2

> heartbeat
Pong

> quit
Goodbye
```

#### 启动客户端 #2 (Bob) - 另一个终端
```bash
./im_client
```

互动示例：
```
> register bob 123456
Register OK, your user ID is: 1

> login bob 123456
Login OK

> [Incoming message from user 1]: Hello Bob!
> send 1 Hi Alice!
Message sent to user 1

> quit
```

---

### 场景 2：自动化测试

#### 测试协议编解码
```bash
./test_codec
```

输出：
```
decoded #1 type=2 body={"text":"hello"}
decoded #2 type=2 body={"text":"world"}
```

#### 测试多客户端连接 (3个客户端同时通信)
```bash
# 终端 1: 启动测试服务器
./server_simple

# 终端 2
./test_client 1

# 终端 3
./test_client 2

# 终端 4
./test_client 3
```

---

### 场景 3：手动连接测试 (nc 或 telnet)

```bash
# 启动服务器
./server_im &

# 另一个终端用 nc 连接
nc -u 127.0.0.1 9000
```

然后发送 JSON 命令（需要手动构造协议字节）。

---

### 场景 3：群组聊天功能测试

```bash
# 终端 1: 启动服务器
./server_im 9999

# 终端 2: 运行群组聊天测试
python3 test_group_chat.py
```

**测试包括：**
- 多用户同时连接
- 创建群组
- 成员加入/离开群组
- 实时群组消息广播
- 群组信息查询

详见 [GROUP_CHAT_FEATURES.md](GROUP_CHAT_FEATURES.md)

---

## 📖 协议说明

### 协议格式

```
[Magic: 4 bytes][Type: 2 bytes][Body Length: 4 bytes][Body: Variable]
```

- **Magic**: `0x494D3031` ("IM01")
- **Type**: 消息类型（2字节，大端序）
  - `1` = HEARTBEAT （心跳）
  - `2` = CHAT （单聊）
  - `3` = REGISTER （注册）
  - `4` = LOGIN （登录）
  - `5` = LOGOUT （登出）
  - `6` = GET_ONLINE_USERS （获取在线用户）

- **Body**: JSON 格式

### 消息示例

#### 登录请求
```json
{
  "username": "alice",
  "password": "123456"
}
```

#### 登录响应
```json
{
  "status": "ok",
  "data": "1"
}
```

#### 单聊消息
```json
{
  "to_user_id": 2,
  "content": "Hello"
}
```

#### 心跳
```
Request body: ""
Response body: "pong"
```

---

## 💡 使用示例

### 示例 1：两个客户端互相发送消息

**自动化方式** (Python脚本示例)
```python
import socket
import json
import time

def send_message(sock, msg_type, body):
    # 构造协议包
    magic = 0x494D3031
    body_bytes = body.encode() if isinstance(body, str) else json.dumps(body).encode()
    
    packet = bytearray()
    packet.extend(magic.to_bytes(4, 'big'))
    packet.extend(msg_type.to_bytes(2, 'big'))
    packet.extend(len(body_bytes).to_bytes(4, 'big'))
    packet.extend(body_bytes)
    
    sock.send(packet)

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('127.0.0.1', 9000))

# 注册
send_message(sock, 3, json.dumps({"username": "alice", "password": "pass123"}))
print(sock.recv(1024).decode())

# 登录
send_message(sock, 4, json.dumps({"username": "alice", "password": "pass123"}))
print(sock.recv(1024).decode())

# 发送消息給 user 2
send_message(sock, 2, json.dumps({"to_user_id": 2, "content": "Hello"}))

sock.close()
```

---

## 🧪 测试结果

### 已验证的功能

| 功能 | 状态 | 验证方式 |
|------|------|--------|
| 协议编解码 | ✅ | test_codec (粘包+半包) |
| 多连接管理 | ✅ | test_client (并发3客户端) |
| 用户注册 | ✅ | im_client |
| 用户登录 | ✅ | im_client |
| 单聊消息 | ✅ | im_client 互相通信 |
| 心跳保活 | ✅ | im_client heartbeat 命令 |
| 在线状态 | ✅ | ConnectionManager |
| 错误处理 | ✅ | 离线用户提示、认证失败 |

---

## 🔍 调试技巧

### 查看服务器日志

服务器启动时会输出：
```
[Server] IM Server started on 127.0.0.1:9000
[Accept] New connection from 127.0.0.1:xxxxx (fd=4)
[Auth] User alice (ID=1) logged in
[Message] from user 1 to user 2: Hello
[Connection closed] fd=4
```

### 启用转储日志到文件

```bash
./server_im > /tmp/im_server.log 2>&1 &
```

然后查看：
```bash
tail -f /tmp/im_server.log
```

### 用 netstat 查看连接

```bash
netstat -an | grep 9000
```

---

## 📝 常见问题

### Q1: 如何修改服务器端口？
编辑 `server_im.cpp`，修改主函数中的端口号（默认 9000）：
```cpp
auto server = std::make_shared<Server>(loop, "127.0.0.1", 9000);  // <- 改这里
```

### Q2: 如何添加新的消息类型？
1. 在 `im_protocol.hpp` 的 `MsgType` enum 中添加
2. 在 `message_handler.cpp` 中添加对应的处理函数

### Q3: 消息为什么没有收到？
- 检查目标用户是否在线（`get_online_users` 命令）
- 检查接收端是否成功登录
- 查看服务器日志中的错误信息

### Q4: 如何支持群聊？
在单聊消息处理基础上，将消息转发给多个用户即可。参考 `message_handler.cpp` 中的 `handle_chat()` 函数。

---

## 📚 核心源文件说明

### `im_protocol.hpp/cpp` - 协议层
- `ByteBuffer`: 接收缓冲区（处理粘包/半包）
- `Codec`: 编解码器（encode/try_decode）
- `Packet`: 逻辑消息包
- `MsgType`: 消息类型枚举

### `event_loop.hpp/cpp` - 事件循环
- 基于 macOS kqueue
- `add_handler()` / `mod_handler()` / `del_handler()` 管理事件
- `run()` 事件循环主函数

### `server.hpp/cpp` - 服务器
- 监听 socket、accept 新连接
- 管理所有活跃连接
- 注册消息回调

### `conn.hpp/cpp` - 连接对象
- 非阻塞读写
- 收缓冲 + 发缓冲
- 部分写偏移处理

### `user_manager.hpp/cpp` - 用户管理
- 用户注册（用户名去重、密码哈希）
- 登录认证
- 在线状态查询

### `connection_manager.hpp/cpp` - 连接管理
- fd <-> user_id 映射
- 连接生命周期管理

### `message_handler.hpp/cpp` - 消息处理
- 分发不同类型消息
- 调用对应的业务处理函数

---

## � 详细文档

- [**P2 高级特性总览**](P2_FEATURES.md) - 离线消息、日志系统、连接监控
- [**群组聊天功能**](GROUP_CHAT_FEATURES.md) - 群组管理和实时消息广播
- [**快速开始指南**](QUICKSTART.md)
- [**最佳实践**](BEST_PRACTICES.md)

---

## �📞 支持

如有问题，请检查：
1. 编译错误：确保使用 C++17
2. 运行错误：查看标准输出/标准错误
3. 协议问题：参考 `im_protocol.hpp` 中的注释
4. 业务逻辑：查看 `message_handler.cpp` 中的处理流程

---

**最后更新**: 2026年3月25日 (P2 群组聊天功能完成)
