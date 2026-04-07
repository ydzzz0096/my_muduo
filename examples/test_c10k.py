import socket
import time
import threading
import os

# ==========================================
# 🔻🔻🔻 压测参数配置区 🔻🔻🔻
# ==========================================
SERVER_IP = '127.0.0.1'
SERVER_PORT = 8000
THREAD_COUNT = 20         # 【修改这里】客户端线程数量改为 20
CONN_PER_THREAD = 1000    # 每个线程负责 1000 个，总计 20 * 1000 = 20,000 个连接
# ==========================================

connections = []

def connect_batch(thread_name, count):
    local_conns = []
    for i in range(count):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # 开启 REUSEADDR 防止端口耗尽
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            sock.connect((SERVER_IP, SERVER_PORT))
            local_conns.append(sock)
            
            if (i + 1) % 200 == 0:
                print(f"Thread {thread_name}: Created {i + 1} connections...")
        except Exception as e:
            print(f"Thread {thread_name} Failed at {i}: {e}")
            break
            
    connections.extend(local_conns)
    print(f"--- Thread {thread_name} Finished: {len(local_conns)} connections established ---")
    
    # 保持连接存活 (死循环挂起)
    while True:
        time.sleep(10)

# 启动多线程进行压测
threads = []
for i in range(THREAD_COUNT):
    t_name = f"T{i}"
    t = threading.Thread(target=connect_batch, args=(t_name, CONN_PER_THREAD), name=t_name)
    t.daemon = True  # 设置为守护线程，确保 Ctrl+C 能瞬间杀掉
    t.start()
    threads.append(t)

# 主线程优雅捕获 Ctrl+C
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("\n[Client] 收到停止信号，强行退出...")
    os._exit(0)