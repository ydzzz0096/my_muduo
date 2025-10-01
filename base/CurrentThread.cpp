// base/CurrentThread.cpp (新增这个文件)

#include "CurrentThread.h"

namespace CurrentThread
{
    __thread int t_cachedTid = 0;

    void cacheTid()
    {
        if (t_cachedTid == 0)
        {
            // 通过 Linux 系统调用获取当前线程的 tid
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}