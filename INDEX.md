# 📚 项目文档索引

## 🎯 快速导航

根据你的目的选择相应的文档或命令：

### I want to...

| 目标 | 推荐操作 | 耗时 |
|------|--------|------|
| 快速上手，5分钟跑起来 | 👉 **[快速启动](#快速启动快手-⚡)** | 5min |
| 了解项目完整功能 | 👉 **[README.md](README.md)** | 20min |
| 学习使用命令 | 👉 **[QUICKSTART.md](QUICKSTART.md)** | 10min |
| 深入理解架构 | 👉 **[BEST_PRACTICES.md](BEST_PRACTICES.md)** | 30min |
| 编译运行程序 | 👉 **[编译章节](#编译)** | 5min |
| 修改代码、扩展功能 | 👉 **[架构说明](#项目结构)** | varies |

---

## ⚡ 快速启动（快手）

### 方法 1: 交互式快速启动 (推荐新手)

```bash
cd /Users/z2835663859/Qt_study/my_project
./start.sh
```

然后选择菜单选项即可。

### 方法 2: 一键编译所有

```bash
cd /Users/z2835663859/Qt_study/my_project
./build.sh        # 编译所有
```

### 方法 3: 命令行直接启动

```bash
# 终端1: 启动服务器
cd /Users/z2835663859/Qt_study/my_project
./build.sh server && ./server_im

# 终端2: 启动客户端 (Alice)
cd /Users/z2835663859/Qt_study/my_project
./build.sh client && ./im_client

# 在客户端中输入命令
> register alice 123456
> login alice 123456
> send 2 Hello Bob!
> heartbeat
> quit
```

---

## 📖 完整文档

### 1. **README.md** - 项目完整说明
   - ✅ 项目概述与功能特性
   - ✅ 文件结构详解
   - ✅ 编译命令 (多种方式)
   - ✅ 快速开始示例
   - ✅ 协议格式说明
   - ✅ 使用示例与演示
   - ✅ 测试结果总结
   - ✅ 常见问题 (FAQ)
   - ✅ 下一步开发规划

   **适合**: 想全面了解项目的用户

   **快速查看**:
   ```bash
   head -50 README.md      # 看前50行
   grep -A5 "快速开始" README.md  # 看特定部分
   ```

### 2. **QUICKSTART.md** - 快速命令参考
   - ✅ 编译命令集速查
   - ✅ 运行各种程序的命令
   - ✅ 客户端命令表
   - ✅ 一键编译脚本
   - ✅ 故障排查快速方案
   - ✅ Makefile 方式

   **适合**: 需要快速查命令的用户

   **快速查看**:
   ```bash
   cat QUICKSTART.md | head -30   # 看编译部分
   grep "^## " QUICKSTART.md       # 看章节列表
   ```

### 3. **BEST_PRACTICES.md** - 最佳实践与优化
   - ✅ 30秒速成指南
   - ✅ 系统架构图与流程图
   - ✅ 常见使用场景
   - ✅ 测试矩阵
   - ✅ 性能指标与瓶颈分析
   - ✅ 调试技巧 (4种)
   - ✅ 代码审查清单
   - ✅ 优化建议 (分优先级)
   - ✅ 新手检查清单

   **适合**: 想深入理解、优化代码、修改功能的用户

   **快速查看**:
   ```bash
   cat BEST_PRACTICES.md | head -100   # 看前几个场景
   grep "### " BEST_PRACTICES.md       # 看所有小标题
   ```

---

## 🔧 编译

### 编译工具

所有编译都在 `macOS` 上用 `clang++` (C++17) 进行。

#### 方式 1: 使用编译脚本 (推荐)

```bash
cd /Users/z2835663859/Qt_study/my_project

# 编译所有程序
./build.sh

# 编译特定程序
./build.sh server         # 仅服务器
./build.sh client         # 仅客户端
./build.sh codec          # 仅协议测试
./build.sh clean          # 清理
```

#### 方式 2: 手动编译

```bash
# 编译服务器
clang++ -std=c++17 -Wall -Wextra -o server_im \
  im_protocol.cpp event_loop.cpp conn.cpp server.cpp \
  user_manager.cpp connection_manager.cpp message_handler.cpp \
  server_im.cpp

# 编译客户端
clang++ -std=c++17 -Wall -Wextra -o im_client \
  im_protocol.cpp im_client.cpp

# 编译测试
clang++ -std=c++17 -o test_codec im_protocol.cpp test_codec.cpp
```

#### 方式 3: 使用 Makefile

参考 QUICKSTART.md 中的 Makefile 示例。

### 编译检查

```bash
# 查看编译产物
ls -lh server_im im_client test_codec

# 检查是否可执行
file server_im   # 应显示 "executable"
./server_im --help  # 如果有 help 选项
```

---

## 🚀 运行

### 启动方式概览

| 场景 | 启动方式 | 文档 |
|------|--------|------|
| 完整 IM 演示 (推荐) | `./start.sh` → 选择1 + 选择2 | 本文档 |
| 快速测试协议 | `./build.sh codec && ./test_codec` | README.md |
| 多客户端压测 | `./server_simple` + 多个 `./test_client` | QUICKSTART.md |
| 手动调试 | `./server_im` + `nc 127.0.0.1 9000` | BEST_PRACTICES.md |

### 实际运行示例

#### 示例 1: 让两个用户聊天

```bash
# 终端1
./start.sh  # 选择 3: 启动服务器+客户端

# 或手动:
# 终端1
./server_im

# 终端2
./im_client
> register alice 123456
> login alice 123456
> send 2 Hello Bob

# 终端3
./im_client
> register bob 123456
> login bob 123456
> [Wait, receive message...]
> send 1 Hi Alice
```

#### 示例 2: 测试协议层

```bash
./build.sh codec
./test_codec
```

输出：
```
decoded #1 type=2 body={"text":"hello"}
decoded #2 type=2 body={"text":"world"}
```

---

## 📁 项目结构

```
/Users/z2835663859/Qt_study/my_project/
│
├── 📖 文档
│   ├── README.md              # 项目完整说明 ← 首先看这个
│   ├── QUICKSTART.md          # 快速命令参考
│   ├── BEST_PRACTICES.md      # 最佳实践与架构深度讨论
│   └── INDEX.md               # 本文件
│
├── 🛠️ 编译脚本
│   ├── build.sh               # 编译脚本 (推荐使用)
│   └── start.sh               # 快速启动脚本 (交互式)
│
├── 🧩 核心代码
│   ├── im_protocol.hpp/cpp    # 协议编解码
│   ├── event_loop.hpp/cpp     # 事件循环 (kqueue)
│   ├── server.hpp/cpp         # TCP 服务器
│   ├── conn.hpp/cpp           # 连接对象
│   ├── user_manager.hpp/cpp   # 用户管理
│   ├── connection_manager.hpp/cpp # 连接管理
│   └── message_handler.hpp/cpp    # 消息处理
│
├── 📱 服务端程序
│   ├── server_im.cpp          # 完整 IM 服务器 ← 推荐
│   ├── server_simple.cpp      # 简化版 (echo)
│   └── server_min.cpp         # 最小版 (demo)
│
├── 💬 客户端程序
│   ├── im_client.cpp          # 交互式客户端 ← 推荐
│   ├── test_client.cpp        # 自动化测试
│   └── client_send.cpp        # 粘包测试
│
└── 🧪 测试程序
    ├── test_codec.cpp         # 协议测试
    └── test_multi.cpp         # 多连接测试
```

---

## 💡 常见任务

### 任务 1: 我想快速看到效果

```bash
./start.sh
# 选择 3 (启动服务器+客户端)
```

结束！

### 任务 2: 我想理解代码

1. 阅读 [README.md](README.md) - 了解全体架构
2. 阅读 [BEST_PRACTICES.md](BEST_PRACTICES.md) - 看流程图
3. 阅读源代码注释
4. 在 gdb 中单步执行

### 任务 3: 我想添加新功能

1. 决定功能属于哪层 (协议/网络/业务)
2. 查看 [BEST_PRACTICES.md](BEST_PRACTICES.md) 中的"代码审查清单"
3. 修改对应文件
4. 使用 `./build.sh server` 重新编译
5. 用 `./start.sh` 测试

### 任务 4: 我想优化性能

1. 查看 [BEST_PRACTICES.md](BEST_PRACTICES.md) 中的"性能指标"
2. 查看"优化建议"列表
3. 根据优先级实施改进

### 任务 5: 排除故障

查看 [QUICKSTART.md](QUICKSTART.md) 中的"常见错误处理"部分。

---

## 🎓 学习路线

### 初学者 (1-2小时)
1. 运行 `./start.sh`，两个客户端互相聊天
2. 阅读 README.md 前半部分（项目概述 + 快速开始）
3. 尝试修改一条消息的格式

### 中级用户 (2-4小时)
1. 阅读 README.md 完整版本
2. 阅读 BEST_PRACTICES.md 中的流程图部分
3. 阅读源代码中的注释
4. 试着添加一个新的消息类型

### 高级用户 (4+ 小时)
1. 深入阅读所有源代码
2. 理解 kqueue 事件循环的工作原理
3. 实施 P2 级功能 (线程池、数据库等)
4. 创建自己的扩展

---

## ❓ 获取帮助

1. **快速问题**: 查看 README.md 的 FAQ 部分
2. **编译问题**: 查看 QUICKSTART.md 的"常见错误处理"
3. **架构问题**: 查看 BEST_PRACTICES.md 中的流程图
4. **代码问题**: 包含带有注释的源代码解释

---

## 📞 快速链接

| 文件 | 用途 | 打开方式 |
|------|------|--------|
| README.md | 完整说明 | `cat README.md` 或 在编辑器打开 |
| QUICKSTART.md | 命令参考 | `cat QUICKSTART.md` \| head -50 |
| BEST_PRACTICES.md | 深度讨论 | 用编辑器打开查看流程图 |
| build.sh | 编译工具 | `./build.sh` 或 `./build.sh help` |
| start.sh | 快速启动 | `./start.sh` |

---

## ✅ 检查清单

完成这些步骤，你就完全掌握了项目：

- [ ] 运行 `./start.sh` 并成功两个客户端聊天
- [ ] 阅读 README.md
- [ ] 运行 `./test_codec` 理解协议层
- [ ] 修改 server_im.cpp 中的端口号并重新编译
- [ ] 用不同的编译方式编译一次
- [ ] 查看并理解 BEST_PRACTICES.md 的流程图
- [ ] 在代码中添加至少一条调试输出
- [ ] 用 `gdb` 调试一次程序

---

**项目更新**: 2026年3月24日

**快速开始**: `./start.sh` ⚡
