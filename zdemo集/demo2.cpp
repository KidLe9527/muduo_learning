// 测试派生类和基类的析构函数顺序
#include <iostream>
using namespace std;

class Base
{
public:
    Base(){
        cout << "Base constructor" << endl;
    }
    virtual ~Base(){
        cout << "Base destructor" << endl;
    }
};

class Derived : public Base
{
public:
    Derived(){
        cout << "Derived constructor" << endl;
    }
    ~Derived(){
        cout << "Derived destructor" << endl;
    }
};

int main()
{
    //Derived d;  // 创建Derived对象，首先调用Base的构造函数，然后调用Derived的构造函数，然后先析构Derived再析构Base
    Base *p = new Derived();  // 创建Derived对象，首先调用Base的构造函数，然后调用Derived的构造函数，但是由于Base的析构函数不是虚函数，所以只会调用Base的析构函数，而不会调用Derived的析构函数，导致资源泄漏等问题，因此在基类的析构函数前面加上virtual关键字，声明为虚函数，才能保证通过基类指针删除派生类对象时，能够正确调用派生类的析构函数，避免资源泄漏等问题。
    delete p;   

    return 0;
}

