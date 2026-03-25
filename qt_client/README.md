# IM Client Qt GUI

一个现代的、功能完整的 IM 客户端，使用 **Qt 6.5.3** 构建。

![Modern Dark Theme](https://img.shields.io/badge/Theme-Modern%20Dark-2D2D2D.svg)
![Qt 6.5.3](https://img.shields.io/badge/Qt-6.5.3-41cd52.svg)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)

---

## ✨ 功能特性

### 核心功能
- ✅ **用户认证** - 安全的登录/注册
- ✅ **单聊** - 点对点消息传输
- ✅ **群组聊天** - 创建、加入、发送群组消息
- ✅ **消息历史** - 保留聊天记录
- ✅ **实时通知** - 立即接收消息和通知

### UI/UX 设计
- 🎨 **现代深色主题** - 专业 Material Design 风格
- 📱 **响应式布局** - 自适应窗口大小
- 💬 **气泡对话** - 类似现代 IM 应用的聊天界面
- 📋 **用户/群组列表** - 快速切换联系人
- ⚡ **流畅交互** - 无阻塞 UI，后台接收消息

### 网络特性
- 🔗 **TCP 连接** - 稳定的网络通信
- 🔄 **自动重连** - 连接断开时自动处理
- 💓 **心跳保活** - 每30秒发送心跳保持连接
- 🛡️ **错误处理** - 完善的网络错误提示

---

## 🏗️ 项目结构

```
qt_client/
├── src/
│   ├── main.cpp                          # 应用入口
│   ├── network/
│   │   ├── im_client_network.hpp        # 网络通信层
│   │   └── im_client_network.cpp        # 协议编解码 + 连接管理
│   ├── ui/
│   │   ├── main_window.hpp              # 主窗口
│   │   └── main_window.cpp              # UI 布局 + 事件处理
│   └── models/
│       ├── chat_model.hpp               # Qt Model (聊天、群组、用户)
│       └── chat_model.cpp
├── resources/
│   └── (icons, stylesheets 等)
├── CMakeLists.txt                       # Qt 6 构建配置
├── build.sh                             # 编译脚本
└── README.md                            # 本文件
```

---

## 🚀 快速开始

### 系统要求

- **macOS** 10.13+
- **Qt 6.5.3** (已安装)
- **CMake 3.16+**
- **C++17 编译器** (Clang)

### 编译步骤

#### 1️⃣ 进入项目目录
```bash
cd /Users/z2835663859/Qt_study/my_project/qt_client
```

#### 2️⃣ 编译
```bash
chmod +x build.sh
./build.sh
```

或手动编译：
```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

#### 3️⃣ 运行

首先启动 IM 服务器：
```bash
cd /Users/z2835663859/Qt_study/my_project
./server_im 9999
```

然后启动客户端：
```bash
cd /Users/z2835663859/Qt_study/my_project/qt_client/build
./bin/im_client_gui
```

---

## 📖 使用说明

### 登录界面

1. **Server Host**: 服务器地址 (默认: 127.0.0.1)
2. **Server Port**: 服务器端口 (默认: 9999)
3. **Username**: 用户名 (测试账户: alice, bob, charlie)
4. **Password**: 密码 (测试账户: alice123, bob123, charlie123)

### 主界面

#### 左侧栏
- **📋 Contacts**: 联系人列表（点击进行单聊）
- **👥 Groups**: 群组列表（点击进入群聊）
- **➕ Create Group**: 创建新群组
- **➕ Join Group**: 加入现有群组（需输入Group ID）

#### 中央区域
- **聊天窗口**: 显示消息历史和实时消息
- **消息输入框**: 输入消息内容
- **发送按钮**: 发送消息

#### 顶部状态栏
- **用户信息**: 当前登录的用户名
- **连接状态**: 显示与服务器的连接状态

---

## 🎨 架构设计

### 网络层 (Network)

**IMClientNetwork** - 负责与服务器通信：
```
协议编码 ↔ TCP Socket ↔ 服务器
↓
信号/槽 → UI 更新
```

- `send_login()` - 发送登录请求
- `send_chat_to()` - 发送单聊消息
- `send_chat_group()` - 发送群组消息
- `send_create_group()` - 创建群组
- `send_join_group()` - 加入群组

### 数据模型 (Models)

使用 **Qt Model/View** 架构：

- **ChatModel** - 聊天消息列表
  - 支持不同角色的消息显示（发送/接收）
  - 自动计算头像颜色
  - 时间戳格式化

- **GroupModel** - 群组信息列表
  - 存储群组 ID、名称、成员数

- **UserModel** - 用户列表
  - 在线状态管理
  - 快速搜索

### UI 层 (UI)

**MainWindow** - 主窗口：
```
┌─ 登录页面 ─────────┐
│                   │
│ [用户名] [密码]   │
│ [服务器] [端口]   │
│         [登录]    │
└───────────────────┘
         ↓
┌─ 主聊天页面 ──────────────────┐
│ 状态栏                        │
│ ┌────────┐ ┌───────────────┐ │
│ │ 联系人 │ │               │ │
│ │ 群组   │ │  聊天窗口     │ │
│ │ 创建.. │ │               │ │
│ │ 加入.. │ │ [消息输入框]  │ │
│ └────────┘ └───────────────┘ │
└──────────────────────────────┘
```

---

## 🔌 通信协议

继承自服务器的协议：

### 协议格式
```
| Magic (4B) | Type (2B) | Length (4B) | Body (JSON) |
| 0x494D3031 | uint16_be | uint32_be   | UTF-8      |
```

### 消息类型
| 类型 | 值 | 用途 |
|------|-----|------|
| LOGIN | 3 | 登录请求 |
| LOGOUT | 4 | 登出 |
| CHAT_TO | 5 | 单聊消息 |
| CHAT_GROUP | 10 | 群组聊天 |
| CREATE_GROUP | 7 | 创建群组 |
| JOIN_GROUP | 8 | 加入群组 |
| LIST_GROUPS | 11 | 查询群组列表 |

---

## 🧵 线程模型

- **主线程** - UI 更新、用户交互
- **网络线程** (QTcpSocket) - 消息接收和发送（自动处理）

使用 Qt 的 **Signal/Slot** 机制实现线程安全的通信：
```cpp
network_->message_received() → main_window->on_message_received()
```

---

## 💾 数据存储

### 消息历史
- 在内存中以 `ChatModel` 存储
- 整个会话期间保留
- 应用关闭后清除（可扩展为持久化存储）

### 群组信息
- 从服务器动态获取
- 实时更新群组列表

---

## ⚙️ 配置说明

### 服务器连接
编辑登录界面的以下字段：
- **Server Host** - 服务器地址（localhost 或 IP）
- **Server Port** - 服务器端口（默认 9999）

### 内置测试账户
```
alice   / alice123
bob     / bob123
charlie / charlie123
```

---

## 🐛 常见问题

### Q: 无法连接到服务器？
- 检查服务器是否运行中 (`./server_im 9999`)
- 确认 IP 和端口正确
- 检查防火墙设置

### Q: 收不到消息？
- 确认网络连接正常
- 检查对方是否在线
- 查看 IM 服务器日志

### Q: 应用闪退？
- 检查 Qt 库是否正确安装
- 查看控制台错误信息
- 尝试重新编译

---

## 📊 代码统计

- **总代码行数**: 2500+ 行
- **网络层**: 600+ 行 (协议、连接管理)
- **UI 层**: 1200+ 行 (主窗口、布局、事件)
- **数据模型**: 400+ 行 (Model/View)
- **编译配置**: 50+ 行 (CMake)

---

## 🎓 学习价值

本项目展示了以下技能：

### Qt 技能
- ✅ Qt 6 现代特性（CMake、Qt Model/View）
- ✅ Signal/Slot 机制和多线程
- ✅ QSS 样式表应用
- ✅ 自定义 Model 实现
- ✅ 事件处理和布局管理

### 网络编程
- ✅ TCP Socket 编程
- ✅ 二进制协议设计和编解码
- ✅ 非阻塞 I/O
- ✅ 错误处理和重连机制

### 软件工程
- ✅ 模块化架构设计
- ✅ Separation of Concerns（分离关注点）
- ✅ Model-View-ViewModel 模式
- ✅ CMake 项目配置

---

## 🚀 未来扩展

### 近期
- [ ] 消息已读状态
- [ ] 表情符号支持
- [ ] 消息搜索功能
- [ ] 用户信息卡片

### 中期
- [ ] 消息持久化（SQLite）
- [ ] 文件传输
- [ ] 语音聊天基础
- [ ] 离线消息支持

### 长期
- [ ] 端到端加密
- [ ] 移动客户端 (Qt Quick)
- [ ] 视频通话
- [ ] 插件系统

---

## 📄 许可证

MIT License

---

## 👨‍💻 作者

Created for interview preparation and learning purposes.

---

## 📞 技术支持

遇到问题？检查以下内容：

1. **编译问题** → 确保 Qt 6.5.3 正确安装
2. **连接问题** → 检查服务器是否运行
3. **功能问题** → 查看服务器日志和客户端错误信息

**关键文件**：
- 协议定义: `src/network/im_client_network.hpp`
- 主逻辑: `src/ui/main_window.cpp`
- 数据模型: `src/models/chat_model.cpp`
