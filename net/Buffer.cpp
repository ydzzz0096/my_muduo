// net/Buffer.cpp

#include "Buffer.h"
#include <cerrno>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从 fd 读取数据到缓冲区，使用了 readv 优化
 * readv 可以一次性读取到多个缓冲区，减少了系统调用次数
 */
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    // 预留一个 64k 的栈上空间，用于当 Buffer 内部空间不足时，临时存放数据
    char extrabuf[65536] = {0}; 
    
    /*
        iovec 的意思是 “I/O Vector”
        
        它内部主要有两个成员：
            void* iov_base: 指向内存缓冲区的起始地址。
            size_t iov_len: 这块内存缓冲区的长度。
    */
    struct iovec vec[2];
    const size_t writable = writableBytes();

    // 第一块缓冲区：Buffer 内部的可写空间
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writable;

    // 第二块缓冲区：栈上的临时空间
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // 当 Buffer 内部空间足够大时，只使用第一块缓冲区即可,经验主义判断
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    /*
        而 ::readv 这个函数，就是那个**“中间过程”：它发起一次系统调用**，让 CPU 切换到内核态，由内核亲自执行“从内核缓冲区拷贝数据到用户缓冲区”这个动作，然后再切换回用户态，将结果返回给您。
    */
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0)
    {
        *savedErrno = errno;
    }
    else if (n <= writable) // Buffer 内部空间足够容纳所有读出的数据
    {
        m_writerIndex += n;
    }
    else // Buffer 内部空间不够，数据有一部分读到了栈上的 extrabuf
    {
        m_writerIndex = m_buffer.size();
        append(extrabuf, n - writable); // 将栈上的数据追加到 Buffer (可能会触发扩容)
    }
    return n;
}

// 这里没有移动readindex指针是因为职责分离,需要交给调用者tcpconnection移动
ssize_t Buffer::writeFd(int fd, int* savedErrно)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *savedErrно = errno;
    }
    return n;
}