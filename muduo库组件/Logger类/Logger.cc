#include <iostream> // 包含输入输出流库，用于在控制台输出日志信息

#include "Logger.h"
#include "Timestamp.h"

// 获取日志唯一的实例对象 单例模式，永远禁止拷贝、禁止复制、禁止产生新对象。
Logger &Logger::instance()
{
    static Logger logger;   // 局部静态变量，第一次调用函数时创建实例进行初始化，之后直接返回该实例
    return logger;
}

// 设置日志级别
void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}

// 写日志 [级别信息]time : msg 输出到控制台
void Logger::log(std::string msg)
{
    std::string pre = "";   // 日志前缀，根据日志级别设置不同的前缀
    switch (logLevel_)
    {
    case INFO:
        pre = "[INFO]";
        break;
    case ERROR:
        pre = "[ERROR]";
        break;
    case FATAL:
        pre = "[FATAL]";
        break;
    case DEBUG:
        pre = "[DEBUG]";
        break;
    default:
        break;
    }

    // 打印时间和msg    eg：[INFO]2026-02-25 15:30:20.123 : 服务器启动成功
    std::cout << pre + Timestamp::now().toString() << " : " << msg << std::endl;
}