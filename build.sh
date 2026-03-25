#!/bin/bash

# IM 项目编译脚本
# 用法: ./build.sh [all|server|client|clean|test]

set -e

PROJECT_DIR="/Users/z2835663859/Qt_study/my_project"
cd "$PROJECT_DIR"

CXX="clang++"
CXXFLAGS="-std=c++17 -Wall -Wextra"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

echo_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_usage() {
    cat << EOF
用法: $0 [命令]

命令:
  all       - 编译所有程序 (默认)
  server    - 仅编译 server_im
  client    - 仅编译 im_client
  test      - 编译所有测试程序
  codec     - 编译协议测试 (test_codec)
  clean     - 清理编译产物
  help      - 显示此帮助信息

示例:
  $0              # 编译所有
  $0 server       # 编译服务器
  $0 clean        # 清理
EOF
}

build_server() {
    echo_info "编译 server_im (包含 P2 特性)..."
    $CXX $CXXFLAGS -o server_im \
        im_protocol.cpp event_loop.cpp conn.cpp server.cpp \
        user_manager.cpp connection_manager.cpp message_handler.cpp \
        message_storage.cpp logger.cpp connection_monitor.cpp group_manager.cpp \
        server_im.cpp
    echo_info "✅ server_im 编译完成 (包含 P0/P1/P2 特性)"
}

build_client() {
    echo_info "编译 im_client..."
    $CXX $CXXFLAGS -o im_client im_protocol.cpp im_client.cpp
    echo_info "✅ im_client 编译完成"
}

build_test_codec() {
    echo_info "编译 test_codec..."
    $CXX $CXXFLAGS -o test_codec im_protocol.cpp test_codec.cpp
    echo_info "✅ test_codec 编译完成"
}

build_test_client() {
    echo_info "编译 test_client..."
    $CXX $CXXFLAGS -o test_client im_protocol.cpp test_client.cpp
    echo_info "✅ test_client 编译完成"
}

build_server_simple() {
    echo_info "编译 server_simple..."
    $CXX $CXXFLAGS -o server_simple im_protocol.cpp server_simple.cpp
    echo_info "✅ server_simple 编译完成"
}

build_all() {
    echo_info "编译所有程序..."
    build_server
    build_client
    build_test_codec
}

build_tests() {
    echo_info "编译所有测试程序..."
    build_test_codec
    build_test_client
    build_server_simple
}

clean() {
    echo_info "清理编译产物..."
    rm -f server_im im_client test_codec test_client test_multi server_simple
    echo_info "✅ 清理完成"
}

list_binaries() {
    echo_info "已编译的程序:"
    ls -lh server_im im_client test_codec test_client server_simple 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
}

# 主逻辑
CMD="${1:-all}"

case "$CMD" in
    all)
        build_all
        list_binaries
        ;;
    server)
        build_server
        ;;
    client)
        build_client
        ;;
    test)
        build_tests
        ;;
    codec)
        build_test_codec
        ;;
    clean)
        clean
        ;;
    help)
        print_usage
        ;;
    *)
        echo_error "未知命令: $CMD"
        print_usage
        exit 1
        ;;
esac

echo ""
echo_info "完成！"
