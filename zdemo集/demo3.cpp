#include <iostream>
#include <functional>
using namespace std;

// 验证std::bind绑定一个无参数的函数时，是否需要指定占位符，以及调用时传入参数是否会导致编译错误。

// 无参数的函数
void func(int a) {
    std::cout << a << endl;
    cout << "func() 被调用，未使用任何参数" << endl;
}

int main() {
    // 定义一个要求接收1个int参数的function
    function<void(int, int)> cb;
    
    // 绑定无参数的func，不指定占位符
    cb = bind(func, placeholders::_2); // 这里使用占位符占位，虽然func不使用参数，但cb的类型要求必须接受两个int参数，所以需要占位符来满足类型要求。
    
    // 调用cb，传入参数100
    cb(100, 3); // 输出：func() 被调用，未使用任何参数
    
    return 0;
}