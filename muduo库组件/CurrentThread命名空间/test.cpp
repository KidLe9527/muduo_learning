#include "CurrentThread.h"
#include <iostream>
#include <pthread.h>
// syscall(SYS_gettid)是Linux系统调用接口，用于获取当前线程的内核态线程ID（tid），而不是pthread库维护的线程ID。

void* threadFunc(void* arg)
{
    // 每个线程调用tid()，获取自己的内核线程ID
    std::cout << "Thread tid: " << CurrentThread::tid() << std::endl;
    return nullptr;
}

int main()
{
    std::cout << "Main thread tid: " << CurrentThread::tid() << std::endl;
    
    pthread_t t1, t2;
    pthread_create(&t1, nullptr, threadFunc, nullptr);
    pthread_create(&t2, nullptr, threadFunc, nullptr);
    
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
    return 0;
}