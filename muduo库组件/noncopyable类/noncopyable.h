#pragma once // 防止头文件重复包含，避免重复定义

/**
 * noncopyable被继承后 派生类对象可正常构造和析构 但派生类对象无法进行拷贝构造和赋值构造
 **/
class noncopyable
{
public:
    noncopyable(const noncopyable &other) = delete;  // 禁止拷贝构造
    noncopyable &operator=(const noncopyable &) = delete;  // 禁止赋值构造
    // void operator=(const noncopyable &) = delete;    // muduo将返回值变为void 这其实无可厚非 // 但C++标准库中并没有这么做，所以这里不做修改
protected:
    noncopyable() = default;    // 允许默认构造
    ~noncopyable() = default;   // 允许默认析构
};

// ⭕️ noncopyable类的作用是禁止派生类对象进行拷贝和赋值操作，从而避免了因误操作导致的资源管理问题，确保了对象的唯一性和资源的正确释放。 

// other 是 “原对象的引用”，代表用来拷贝的那个已存在的对象。省略了参数名 other，但参数类型 const noncopyable & 依然代表 “原对象的引用”
// 编译器不关心参数名，只关心参数类型是否符合拷贝构造函数的语法。

// 本头文件没有实现的原因是 noncopyable 类不需要任何成员函数的具体实现，因为它的主要作用是通过删除拷贝构造函数和赋值运算符来禁止对象的拷贝和赋值操作。