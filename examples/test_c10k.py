import socket
import time
import threading

# 目标配置
SERVER_IP = '127.0.0.1'
SERVER_PORT = 8000
TARGET_CONNECTIONS = 10000 # 目标：1万个连接

connections = []

def connect_batch(start, count):
    local_conns = []
    for i in range(count):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            sock.connect((SERVER_IP, SERVER_PORT))
            local_conns.append(sock)
            if i % 100 == 0:
                print(f"Thread {threading.current_thread().name}: Created {i} connections...")
        except Exception as e:
            print(f"Failed: {e}")
            break
    # 保持连接存活
    connections.extend(local_conns)
    while True:
        time.sleep(10)

# 启动 10 个线程，每个负责 1000 个连接
threads = []
for i in range(10):
    t = threading.Thread(target=connect_batch, args=(0, 1000), name=f"T{i}")
    t.start()
    threads.append(t)

# 主线程等待
for t in threads:
    t.join()