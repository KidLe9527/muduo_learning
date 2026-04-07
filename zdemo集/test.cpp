#include <stdio.h>  // for snprintf
#include <time.h>   // for time_t, struct tm, time, localtime
#include <vector>

int main(){

    printf("Hello, World!\n");
    int *a = new int(10);
    printf("a = %d\n", *a);
    int c = 12 > 3 ? (7 < 2 ? 9 : 3) : (5 < 2 ? 10 : 15);
    printf("c = %d\n", c);  // 3
    delete a;   
    a = nullptr;

    std::vector<int> vec = std::vector<int>(10); // vector<int> a(10) 也是一样的效果，都是创建一个包含10个元素的vector，每个元素默认初始化为0
    int b = int(5); // int b = 5; 也是一样的效果，都是创建一个int类型的变量b，并将其初始化为5
    vec.push_back(b); // 在vector末尾添加一个元素b，vector的大小会增加1，原来10个元素现在变成11个元素，前10个元素是默认初始化的0，第11个元素是5
    printf("vec.size() = %lu\n", vec.size()); // 11
    printf("vec[10] = %d\n", vec[10]); // 5
    printf("vec[0] = %d\n", vec[0]); // 0           

    return 0;
}