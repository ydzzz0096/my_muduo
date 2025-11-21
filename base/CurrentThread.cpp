// base/CurrentThread.cpp (新增这个文件)

#include "CurrentThread.h"

namespace CurrentThread
{
    __thread int t_cachedTid = 0;

    void cacheTid()
    {
        if (t_cachedTid == 0)
        {
            //在不损失const 限定的前提下进行各种合法的类型转换
            // 系统调用序号是SYS_gettid的命令,把返回值交给我
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}