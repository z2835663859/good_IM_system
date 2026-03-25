#!/bin/bash

# 快速启动脚本 - 一键编译并启动服务器

PROJECT_DIR="/Users/z2835663859/Qt_study/my_project"
cd "$PROJECT_DIR"

echo "================================"
echo "  IM 项目 - 快速启动"
echo "================================"
echo ""

# 检查二进制文件是否存在
if [ ! -f "server_im" ] || [ ! -f "im_client" ]; then
    echo "📦 首次启动，进行编译..."
    ./build.sh all
    echo ""
fi

echo "📊 系统信息"
echo "  项目路径: $PROJECT_DIR"
echo "  服务器包: server_im"
echo "  客户端包: im_client"
echo ""

echo "🚀 启动选项："
echo "  1) 启动服务器 (server_im)"
echo "  2) 启动客户端 (im_client)"
echo "  3) 启动服务器 + 客户端 (后台)"
echo "  4) 运行测试 (test_codec)"
echo "  5) 清理并重新编译"
echo "  6) 查看帮助文档"
echo ""

read -p "选择 (1-6): " choice

case $choice in
    1)
        echo "📡 启动服务器..."
        echo "按 Ctrl+C 停止服务器"
        ./server_im
        ;;
    2)
        echo "💬 启动客户端..."
        echo "输入 'help' 获取命令列表"
        ./im_client
        ;;
    3)
        echo "🚀 启动服务器 (后台)..."
        ./server_im > /tmp/im_server.log 2>&1 &
        SERVER_PID=$!
        echo "✅ 服务器已启动 (PID=$SERVER_PID)"
        echo "📝 日志: tail -f /tmp/im_server.log"
        sleep 1
        echo ""
        echo "💬 启动客户端..."
        ./im_client
        echo ""
        echo "🛑 停止服务器..."
        kill $SERVER_PID 2>/dev/null
        echo "✅ 已停止服务器"
        ;;
    4)
        echo "🧪 运行协议测试..."
        ./build.sh codec
        ./test_codec
        ;;
    5)
        echo "🧹 清理并重新编译..."
        ./build.sh clean
        ./build.sh all
        ;;
    6)
        echo "📚 查看 README.md"
        cat README.md | head -100
        echo ""
        echo "更多文档请查看:"
        echo "  - README.md (详细说明)"
        echo "  - QUICKSTART.md (快速参考)"
        echo "  - BEST_PRACTICES.md (最佳实践)"
        ;;
    *)
        echo "❌ 无效选择"
        exit 1
        ;;
esac
