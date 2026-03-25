# 快速命令参考

## 编译所有程序

```bash
cd /Users/z2835663859/Qt_study/my_project

# 编译 IM 服务器（完整版，推荐）
clang++ -std=c++17 -Wall -Wextra -o server_im \
  im_protocol.cpp event_loop.cpp conn.cpp server.cpp \
  user_manager.cpp connection_manager.cpp message_handler.cpp \
  server_im.cpp

# 编译交互式客户端
clang++ -std=c++17 -Wall -Wextra -o im_client im_protocol.cpp im_client.cpp

# 编译自动化测试客户端
clang++ -std=c++17 -o test_client im_protocol.cpp test_client.cpp

# 编译协议测试
clang++ -std=c++17 -o test_codec im_protocol.cpp test_codec.cpp

# 编译简化版 echo 服务器
clang++ -std=c++17 -o server_simple im_protocol.cpp server_simple.cpp
```

## 运行程序

### 方案 A: 完整 IM 系统 (推荐)

**终端 1 - 启动服务器**
```bash
./server_im
```

**终端 2 - Alice 登录**
```bash
./im_client
> register alice 123456
> login alice 123456
> heartbeat
> send 2 Hello Bob
> quit
```

**终端 3 - Bob 登录**
```bash
./im_client
> register bob 123456
> login bob 123456
> [等待消息...]
> send 1 Hi Alice
> quit
```

### 方案 B: 简化版 echo 服务器（快速测试）

**终端 1**
```bash
./server_simple
```

**终端 2**
```bash
./test_client 1
```

**终端 3**
```bash
./test_client 2
```

### 方案 C: 协议测试（验证编解码）

```bash
./test_codec
```

---

## 客户端命令

在交互式客户端 (`./im_client`) 中可以使用以下命令：

| 命令 | 描述 | 示例 |
|------|------|------|
| `register <用户名> <密码>` | 注册新用户 | `register alice 123456` |
| `login <用户名> <密码>` | 登录 | `login alice 123456` |
| `send <用户ID> <消息内容>` | 发送单聊消息 | `send 2 Hello` |
| `heartbeat` | 发送心跳（保活） | `heartbeat` |
| `online` | 查看在线用户 | `online` |
| `quit` | 退出客户端 | `quit` |

---

## 一键编译并测试

```bash
#!/bin/bash
cd /Users/z2835663859/Qt_study/my_project

# 编译
echo "编译中..."
clang++ -std=c++17 -Wall -Wextra -o server_im \
  im_protocol.cpp event_loop.cpp conn.cpp server.cpp \
  user_manager.cpp connection_manager.cpp message_handler.cpp \
  server_im.cpp

clang++ -std=c++17 -Wall -Wextra -o im_client im_protocol.cpp im_client.cpp

echo "✅ 编译完成"

# 启动服务器
echo "启动服务器..."
./server_im &
SERVER_PID=$!
sleep 2

# 启动两个客户端进行测试（可选）
echo "服务器已启动 (PID=$SERVER_PID)， 端口 9000"
echo "后台启动客户端: ./im_client"
echo ""
echo "按 Ctrl+C 停止服务器"

wait $SERVER_PID
```

保存为 `build_and_run.sh`，然后执行：
```bash
chmod +x build_and_run.sh
./build_and_run.sh
```

---

## 查看程序文件大小

```bash
ls -lh server_im im_client test_codec server_simple
```

---

## 清理编译产物

```bash
rm -f server_im im_client test_client test_codec test_multi server_simple
```

---

## 查看服务器日志

```bash
# 启动服务器并保存日志
./server_im > /tmp/im_server.log 2>&1 &

# 实时查看日志
tail -f /tmp/im_server.log

# 查看最后 50 行
tail -50 /tmp/im_server.log
```

---

## 在线连接计数

查看服务器当前有多少客户端连接：

```bash
# 方式 1: 查看日志中的 [Stats] 输出
tail -f /tmp/im_server.log | grep "Active connections"

# 方式 2: 用 lsof 查看监听端口
lsof -i :9000

# 方式 3: 用 netstat 查看连接
netstat -an | grep 9000
```

---

## 常见错误处理

### 错误: `Address already in use`
```bash
# 端口 9000 被占用，杀死老进程
killall server_im
sleep 1
./server_im
```

### 错误: `Cannot compile - undefined reference`
```bash
# 确保所有 .cpp 文件都被编译了
# 检查命令行中是否包含所有必要的 .cpp 文件
```

### 错误: `Connect refused`
```bash
# 服务器没有启动，检查是否运行了 ./server_im
ps aux | grep server_im
```

---

## Makefile 方式编译 (可选)

创建 `Makefile`:

```makefile
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra

SOURCES = im_protocol.cpp event_loop.cpp conn.cpp server.cpp \
          user_manager.cpp connection_manager.cpp message_handler.cpp

SERVER_OBJS = $(SOURCES:.cpp=.o) server_im.cpp
CLIENT_OBJS = im_protocol.cpp im_client.cpp

all: server_im im_client test_codec

server_im: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

im_client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

test_codec: im_protocol.cpp test_codec.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *.o server_im im_client test_codec

.PHONY: all clean
```

然后编译：
```bash
make          # 编译所有
make clean    # 清理
```

---

**最后更新**: 2026年3月24日
