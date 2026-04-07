#include "CurrentThread.h"

namespace CurrentThread
{
    // 定义线程局部变量t_cachedTid，初始值为0，为什么在这里定义？因为它是线程局部的，每个线程都有独立的副本，定义在这里可以确保每个线程都能正确访问和修改自己的t_cachedTid。
    __thread int t_cachedTid = 0;

    // 实现获取并缓存tid的函数
    void cacheTid()
    {
        if (t_cachedTid == 0) // 双重检查（虽然单线程下没必要，但保持逻辑一致）
        {
            // ::syscall(SYS_gettid)：调用Linux系统调用获取内核级线程ID
            // static_cast<pid_t>：将系统调用返回值转换为进程/线程ID类型
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));    // 这里为什么不转换成int？因为pid_t在某些系统上可能是long类型，直接转换成int可能会丢失信息，使用static_cast可以确保类型安全的转换。
        }
    }
}


/*
::syscall(SYS_gettid)：:: 表示调用全局命名空间的函数，syscall() 是 Linux 系统调用接口，SYS_gettid 是获取当前线程 ID 的系统调用号。
pid_t：Linux 中表示进程 / 线程 ID 的类型（本质是整型），static_cast<pid_t> 是类型安全的强制转换。
双重检查 t_cachedTid == 0：虽然 __thread 变量是线程私有，不存在多线程竞争，但保留该检查是为了防止重复调用系统调用（系统调用耗时，能避免则避免）。*/