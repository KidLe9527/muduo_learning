#pragma once    // 防止头文件被重复包含

#include <iostream> // for std::cout
#include <string>   // for std::string

class Timestamp
{
public:
    Timestamp();    // 默认构造函数，时间戳为0
    explicit Timestamp(int64_t microSecondsSinceEpoch); // 用微秒数构造时间戳
    static Timestamp now(); // 获取当前时间戳
    std::string toString() const; // 返回时间戳的字符串表示

private:
    int64_t microSecondsSinceEpoch_;    // 自纪元以来的微秒数
};