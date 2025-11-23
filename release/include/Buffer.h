// net/Buffer.h

#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>
#include <algorithm> // for std::swap
#include <cstddef>   // for size_t
#include <sys/types.h> // for ssize_t

// 网络库底层的缓冲区类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : m_buffer(kCheapPrepend + initialSize),
          m_readerIndex(kCheapPrepend),
          m_writerIndex(kCheapPrepend)
    {}

    size_t readableBytes() const { return m_writerIndex - m_readerIndex; }
    size_t writableBytes() const { return m_buffer.size() - m_writerIndex; }
    size_t prependableBytes() const { return m_readerIndex; }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const { return begin() + m_readerIndex; }
    
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            m_readerIndex += len; // 应用层只读取了 readableBytes_ 数据的一部分
        }
        else // len == readableBytes()
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        m_readerIndex = kCheapPrepend;
        m_writerIndex = kCheapPrepend;
    }

    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
    }

    // 把 data 写入 Writable 缓冲区
    void append(const char* data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        m_writerIndex += len;
    }

    char* beginWrite() { return begin() + m_writerIndex; }
    const char* beginWrite() const { return begin() + m_writerIndex; }

    // 内核缓冲区和应用层缓冲区的通信
    // 从 fd 读取数据到缓冲区
    ssize_t readFd(int fd, int* savedErrno);
    // 通过 fd 发送数据
    ssize_t writeFd(int fd, int* savedErrno);

private:
    char* begin() { return &*m_buffer.begin(); }
    const char* begin() const { return &*m_buffer.begin(); }

    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            m_buffer.resize(m_writerIndex + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + m_readerIndex,// 源头
                      begin() + m_writerIndex,// 结尾
                      begin() + kCheapPrepend);// 目标位置的起点
            m_readerIndex = kCheapPrepend;
            m_writerIndex = m_readerIndex + readable;
        }
    }

    std::vector<char> m_buffer;
    size_t m_readerIndex;
    size_t m_writerIndex;
};

#endif