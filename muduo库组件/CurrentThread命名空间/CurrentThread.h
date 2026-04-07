#pragma once  // 防止头文件重复包含，替代传统的#ifndef...#define...#endif

#include <unistd.h>       // 系统调用相关头文件
#include <sys/syscall.h>  // 包含SYS_gettid等系统调用宏定义

namespace CurrentThread
{
    // __thread 是GCC内置的线程局部存储(TLS)关键字，每个线程有独立的t_cachedTid副本
    // extern 声明该变量，在其他文件中定义，这里仅做声明
    extern __thread int t_cachedTid; // 保存tid缓存 因为系统调用非常耗时 拿到tid后将其保存

    void cacheTid();  // 声明获取并缓存tid的函数

    inline int tid() // 高效获取当前线程的内核态 ID（tid），通过缓存 + 内联 + 分支预测优化，把高频调用的性能损耗降到最低。
    {
        // __builtin_expect 是GCC的优化指令，格式：__builtin_expect(表达式, 预期值)
        // 这里表示：预期 t_cachedTid == 0 为false(0)，告诉编译器优化分支预测
        // 意思是：如果缓存的tid为空（还未获取），则调用cacheTid()获取
        if (__builtin_expect(t_cachedTid == 0, 0)) 
        {
            cacheTid();
        }
        return t_cachedTid; // 返回缓存的tid
    }
}

/* 这两段代码实现了一个名为 CurrentThread 的命名空间，核心功能是在 Linux 系统下高效获取当前线程的内核态 ID（tid）🔥
在 Linux 系统中，pthread_self() 获取的是 pthread 库维护的线程 ID，
而 syscall(SYS_gettid) 获取的是内核层面的线程 ID，
这段代码就是为了高效获取后者。*/
 
/*
__thread：线程局部存储，每个线程拥有该变量的独立副本，互不干扰，且生命周期与线程一致。
inline：内联函数，编译器会尝试将函数体直接嵌入调用处，减少函数调用的栈开销，适合频繁调用的小函数（如 tid ()）。
__builtin_expect：GCC 提供的分支预测优化，__builtin_expect(t_cachedTid == 0, 0) 表示「预期 t_cachedTid == 0 为假」，编译器会据此优化指令布局，减少分支跳转的性能损耗（因为 tid 只会第一次获取，后续都是直接返回缓存值，所以该分支大概率不执行）。
*/

/*
extern 是 C/C++ 中的存储类说明符，核心作用有两个：
声明变量 / 函数的 “外部定义”：告诉编译器 “这个变量 / 函数的实际定义不在当前编译单元（.cpp/.c 文件），而是在其他编译单元中”；
扩展变量 / 函数的作用域：让当前编译单元能访问到其他编译单元中定义的全局变量 / 函数。*/