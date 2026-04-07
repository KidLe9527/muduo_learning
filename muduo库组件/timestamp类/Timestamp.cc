#include <time.h>   // for time_t, struct tm, time, localtime
#include <stdio.h>  // for snprintf

#include "Timestamp.h"

Timestamp::Timestamp() : microSecondsSinceEpoch_(0)
{
}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch)
{
}

Timestamp Timestamp::now()
{
    return Timestamp(time(NULL));   // 返回自纪元以来的秒数
}

std::string Timestamp::toString() const
{
    char buf[128] = {0};    // 初始化为0
    tm *tm_time = localtime(reinterpret_cast<const time_t*>(&microSecondsSinceEpoch_));
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
    return buf;
}

#include <iostream>
int main() {
    std::cout << Timestamp::now().toString() << std::endl;
    return 0;
}

// localtime函数将time_t类型的时间转换为tm结构体表示的本地时间
// snprintf函数将格式化的字符串写入字符数组中
// time函数获取当前时间，返回自纪元以来的秒数

// reinterpret_cast用于类型转换，将time_t指针转换为int64_t指针
// tm结构体包含了年、月、日、时、分、秒等时间信息
// 注意：上述代码假设time_t类型和int64_t类型在内存表示上是兼容的，这在大多数平台上是成立的，但并非所有平台都如此。在实际使用中应注意这一点。

// Timestamp::now().toString()用于获取当前时间的字符串表示，格式为"YYYY/MM/DD HH:MM:SS"