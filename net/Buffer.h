// net/Buffer.h

#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm> // for std::swap
#include <cstddef>   // for size_t
#include <string>
#include <sys/types.h> // for ssize_t
#include <vector>

/**
 * 网络库底层的缓冲区类型定义
 * 设计思想：
 * 1. 双指针设计：读指针和写指针
 * 2. 内存布局：[prependable][readable][writable]
 * 3. 预留空间：kCheapPrepend = 8，用于添加协议头等信息
 * 4. 高效读取：readv + 栈上临时缓冲区，减少系统调用
 */
class Buffer {
public:
  /**
   * 预留空间大小（8字节），用于添加协议头
   */
  static const size_t kCheapPrepend = 8;

  /**
   * 初始缓冲区大小（1024字节）
   */
  static const size_t kInitialSize = 1024;

  /**
   * 构造函数
   * @param initialSize 初始缓冲区大小
   */
  explicit Buffer(size_t initialSize = kInitialSize)
      : m_buffer(kCheapPrepend + initialSize), // 总大小 = 预留 + 初始
        m_readerIndex(kCheapPrepend),          // 读指针从预留空间后开始
        m_writerIndex(kCheapPrepend)           // 写指针与读指针相同
  {}

  /**
   * 获取可读数据的大小
   * @return 可读数据的字节数
   */
  size_t readableBytes() const { return m_writerIndex - m_readerIndex; }

  /**
   * 获取可写空间的大小
   * @return 可写空间的字节数
   */
  size_t writableBytes() const { return m_buffer.size() - m_writerIndex; }

  /**
   * 获取预留空间的大小
   * @return 预留空间的字节数
   */
  size_t prependableBytes() const { return m_readerIndex; }

  /**
   * 返回缓冲区中可读数据的起始地址
   * @return 可读数据的起始地址
   */
  const char *peek() const { return begin() + m_readerIndex; }

  /**
   * 读取指定长度的数据（只移动读指针，零拷贝），标记数据已被处理
   * @param len 要读取的字节数
   */
  void retrieve(size_t len) {
    if (len < readableBytes()) {
      m_readerIndex += len; // 应用层只读取了部分数据，移动读指针
    } else                  // len >= readableBytes()，读取全部数据
    {
      retrieveAll();
    }
  }

  /**
   * 读取所有数据（重置读写指针）
   */
  void retrieveAll() {
    m_readerIndex = kCheapPrepend; // 重置读指针到预留空间后
    m_writerIndex = kCheapPrepend; // 重置写指针到预留空间后
  }

  /**
   * 读取所有数据并返回为字符串
   * @return 读取的数据字符串
   */
  std::string retrieveAllAsString() {
    return retrieveAsString(readableBytes());
  }

  /**
   * 读取指定长度的数据并返回为字符串
   * @param len 要读取的字节数
   * @return 读取的数据字符串
   */
  std::string retrieveAsString(size_t len) {
    std::string result(peek(), len); // 从可读区域构造字符串
    retrieve(len);                   // 移动读指针
    return result;
  }

  /**
   * 确保缓冲区有足够的可写空间
   * @param len 需要的可写空间大小
   */
  void ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
      makeSpace(len); // 空间不足，调整或扩容
    }
  }

  /**
   * 向可写区域写入数据
   * @param data 数据指针
   * @param len 数据长度
   */
  void append(const char *data, size_t len) {
    ensureWritableBytes(len);                  // 确保有足够空间
    std::copy(data, data + len, beginWrite()); // 拷贝数据
    m_writerIndex += len;                      // 移动写指针
  }

  /**
   * 获取可写区域的起始地址
   * @return 可写区域的起始地址
   */
  char *beginWrite() { return begin() + m_writerIndex; }

  /**
   * 获取可写区域的起始地址（常量版本）
   * @return 可写区域的起始地址
   */
  const char *beginWrite() const { return begin() + m_writerIndex; }

  /**
   * 从文件描述符读取数据到缓冲区
   * 使用 readv 系统调用，结合栈上临时缓冲区，减少系统调用次数
   * @param fd 文件描述符
   * @param savedErrno 保存错误码的指针
   * @return 读取的字节数，-1 表示错误
   */
  ssize_t readFd(int fd, int *savedErrno);

  /**
   * 向文件描述符写入数据
   * @param fd 文件描述符
   * @param savedErrno 保存错误码的指针
   * @return 写入的字节数，-1 表示错误
   */
  ssize_t writeFd(int fd, int *savedErrno);

private:
  /**
   * 获取缓冲区的起始地址
   * @return 缓冲区的起始地址
   */
  char *begin() { return &*m_buffer.begin(); }

  /**
   * 获取缓冲区的起始地址（常量版本）
   * @return 缓冲区的起始地址
   */
  const char *begin() const { return &*m_buffer.begin(); }

  /**
   * 调整缓冲区空间
   * 策略：
   * 1. 总空间不足：扩容
   * 2. 总空间足够：移动数据到前面，复用空间
   * @param len 需要的额外空间
   */
  void makeSpace(size_t len) {
    // 检查总空间是否足够
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
      // 总空间不足，直接扩容
      m_buffer.resize(m_writerIndex + len);
    } else {
      // 总空间足够，移动数据到前面
      size_t readable = readableBytes();
      // 将可读数据从 [m_readerIndex, m_writerIndex) 移动到 [kCheapPrepend,
      // kCheapPrepend + readable)
      std::copy(begin() + m_readerIndex,        // 源头
                begin() + m_writerIndex,        // 结尾
                begin() + kCheapPrepend);       // 目标
      m_readerIndex = kCheapPrepend;            // 重置读指针
      m_writerIndex = m_readerIndex + readable; // 重置写指针
    }
  }

  /**
   * 底层存储
   */
  std::vector<char> m_buffer;

  /**
   * 读指针：可读数据的起始位置
   */
  size_t m_readerIndex;

  /**
   * 写指针：可写空间的起始位置
   */
  size_t m_writerIndex;
};

#endif