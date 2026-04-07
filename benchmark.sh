#!/bin/bash

echo "=== MyMuduo 自动化压测脚本 ==="

# ==========================================
# 🔻🔻🔻 压测参数配置区 (在这里修改测试参数) 🔻🔻🔻
# ==========================================
TARGET_IP="127.0.0.1"
TARGET_PORT=8000
CLIENT_THREADS=12    # 测试 QPS 时的客户端线程数 (建议 8 或 12)
CLIENT_CONNECTIONS=1000 # 测试 QPS 时的总连接数
# ==========================================

BUILD_DIR="./build/examples"
SERVER_BIN="$BUILD_DIR/echoserver"
QPS_CLIENT_BIN="$BUILD_DIR/test_qps"
PYTHON_C10K_SCRIPT="./examples/test_c10k.py"
SERVER_PID=""

# === 清理函数 (捕获 Ctrl+C) ===
cleanup() {
    echo -e "\n[!] 收到中断信号，正在清理环境..."
    if [ -n "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null
    fi
    pkill -f test_qps 2>/dev/null
    echo "测试已安全终止。"
    exit 0
}

trap cleanup SIGINT SIGTERM

if [ ! -f "$SERVER_BIN" ] || [ ! -f "$QPS_CLIENT_BIN" ]; then
    echo "错误: 找不到可执行文件。请确认你已经成功编译了项目。"
    exit 1
fi

pkill -f echoserver 2>/dev/null
pkill -f test_qps 2>/dev/null

echo "请选择测试模式:"
echo "1) QPS 吞吐量极限测试 (使用 C++ 客户端)"
echo "2) 海量并发连接测试 (使用 Python 客户端)"
read -p "输入数字 (1 或 2): " choice

if [ "$choice" == "1" ]; then
    echo "[1] 准备进行 QPS 测试..."

    $SERVER_BIN > "$BUILD_DIR/server.log" 2>&1 &
    SERVER_PID=$!
    sleep 2 

    echo "------------------------------------------------"
    echo "🎯 压测已启动 (PID: $SERVER_PID)"
    echo "------------------------------------------------"
    echo "💡 [高级监控指令] 请在新终端执行以下命令观察上下文切换："
    echo "   pidstat -wt -p $SERVER_PID 1"
    echo ""
    echo "📊 [字段解释]："
    echo "   cswch/s:  自愿上下文切换（通常因 readv 优化而减少）"
    echo "   nvcswch/s:非自愿上下文切换（因 CPU 抢占产生）"
    echo "------------------------------------------------"
    
    # 这里的 read -p 让你有时间去另一个窗口开启 pidstat
    read -p "⏸️ 监控已就绪？按【回车键】正式开始高频 Ping-Pong 压测..."
    
    $QPS_CLIENT_BIN $TARGET_IP $TARGET_PORT $CLIENT_THREADS $CLIENT_CONNECTIONS

    cleanup

elif [ "$choice" == "2" ]; then
    echo "[2] 准备进行并发连接测试..."

    ulimit -n 65535 2>/dev/null

    $SERVER_BIN > "$BUILD_DIR/server.log" 2>&1 &
    SERVER_PID=$!
    sleep 2

echo "------------------------------------------------"
    echo "🚀 正在启动 Python 并发客户端: python3 $PYTHON_C10K_SCRIPT"
    echo "【重要提示】请打开新终端，输入以下命令查看服务端真实内存占用:"
    echo "  top -p $SERVER_PID"
    echo "按 Ctrl+C 安全退出。"
    echo "------------------------------------------------"
    
    # 👇👇👇 加入这一行，让脚本在这里卡住等你的指令 👇👇👇
    read -p "⏸️ 服务端已就绪。请先去查看【空载内存】，看完后按【回车键】正式发射并发连接..."
    
    python3 $PYTHON_C10K_SCRIPT

    cleanup
else
    echo "输入无效，退出测试。"
fi