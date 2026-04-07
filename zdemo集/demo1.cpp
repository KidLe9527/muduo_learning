#include <iostream>
#include <vector>
#include <span>  // C++17及以上支持

using namespace std;

int main() {
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    int start_idx = 2;
    int length = 3;  // 提取3个元素（和之前的[2,5)等价）

    // 核心：创建span视图，不拷贝任何元素
    span<int> sub_span(vec.data() + start_idx, length);
    // 也可以用迭代器构造（C++20及以上）：
    // span<int> sub_span(vec.begin() + start_idx, vec.begin() + 5);

    // 打印视图（和vector遍历方式一致）
    for (int num : sub_span) {
        cout << num << " ";  // 输出：3 4 5
    }
    cout << endl;

    // 修改视图中的元素（会直接修改原vector，因为是视图）
    sub_span[0] = 30;

    // 打印原vector，验证修改
    for (int num : vec) {
        cout << num << " ";  // 输出：1 2 30 4 5 6 7 8 9
    }

    return 0;
}