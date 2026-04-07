#pragma once
//#define MUDEBUG

#include <string>

#include "noncopyable.h"

// LOG_INFO("%s %d", arg1, arg2)
// do-while(0)是一个常见的宏定义技巧，目的是为了让宏在使用时表现得像一个普通的函数调用，避免一些潜在的语法问题。
// 例如，如果直接定义为一个代码块，可能会导致在某些条件语句中使用时出现问题，如 if (condition) LOG_INFO(...); else ... 这种情况会引发编译错误。
#define LOG_INFO(logmsgFormat, ...)                       \
    do                                                    \  
    {                                                     \   
        Logger &logger = Logger::instance();              \ 
        logger.setLogLevel(INFO);                         \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_ERROR(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(ERROR);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_FATAL(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(FATAL);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
        exit(-1);                                         \
    } while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(DEBUG);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)
#else
#define LOG_DEBUG(logmsgFormat, ...)    // 如果没有定义MUDEBUG宏，LOG_DEBUG就被定义为空宏，这样在编译时就不会生成任何与LOG_DEBUG相关的代码，从而提高效率，避免在生产环境中输出调试信息。
#endif

// 定义日志的级别 INFO ERROR FATAL DEBUG
// enum是一个用户自定义的类型，允许你定义一组命名的整数常量。在这个例子中，LogLevel枚举定义了四个日志级别：INFO、ERROR、FATAL和DEBUG。每个级别都对应一个整数值，默认情况下从0开始递增。使用枚举可以提高代码的可读性和可维护性，因为它允许你使用有意义的名称来表示不同的日志级别，而不是直接使用整数值。
enum LogLevel
{
    INFO,  // 普通信息 0
    ERROR, // 错误信息 1
    FATAL, // core dump信息，程序会退出，一般用于不可恢复的错误 2
    DEBUG, // 调试信息 3
};

// 输出一个日志类

class Logger : noncopyable
{
public:
    // 获取日志唯一的实例对象 单例
    static Logger &instance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);

private:
    int logLevel_;  // 日志级别，默认是INFO。作用是控制输出哪些级别的日志
};