# Virtual Inheritance

引子：知乎上看到一个帖子[“C++关于虚继承的问题,为什么是这样的?”](https://www.zhihu.com/question/453103568)

[先看一下参考](https://isocpp.org/wiki/faq/multiple-inheritance#virtual-inheritance-where)

## 不用Virtual Inheritance的代码范例

```
#include <iostream>

struct A 
{
    A(int a)
    {
        std::cout << "A ctor = " << a << '\n';
    }
};

struct B1 : public A
{
    B1(int b1) : A(1)
    {
        std::cout << "B1 ctor = " << b1 << '\n';
    }
};

struct B2 : public A
{
    B2(int b2) : A(2)
    {
        std::cout << "B2 ctor = " << b2 << '\n';
    }
};

struct C : public B1, B2
{
    C(int c) : B1(10), B2(20)
    {
        std::cout << "C ctor = " << c << '\n';
    }
};

int main()
{
    C c(333);

    return 0;
}
```

执行结果是：
```
A ctor = 1
B1 ctor = 10
A ctor = 2
B2 ctor = 20
C ctor = 333
```

可以看出，在创建C里，有两个A，一个A是创建B1时同时创建的，所以```A ctor = 1```，另外一个A是创建B2时同时创建的。

这就带来一个问题，假设A里有一个virtual方法，叫foo()，那么C.foo()时，该调用哪一个，这就是Diamond问题

解决方法，就是用virtual继承，这时，编译器就保证只有一个A，代码如下

## 用virtual inheritance的范例

```
#include <iostream>

struct A 
{
    A(int a)
    {
        std::cout << "A ctor = " << a << '\n';
    }
};

struct B1 : public virtual A
{
    B1(int b1) : A(1)
    {
        std::cout << "B1 ctor = " << b1 << '\n';
    }
};

struct B2 : public virtual A
{
    B2(int b2) : A(2)
    {
        std::cout << "B2 ctor = " << b2 << '\n';
    }
};

struct C : public B1, B2
{
    C(int c) : B1(10), B2(20), A(0)
    {
        std::cout << "C ctor = " << c << '\n';
    }
};

int main()
{
    C c(333);

    return 0;
}
```

执行结果如下：
```
A ctor = 0
B1 ctor = 10
B2 ctor = 20
C ctor = 333
```

我们发现，在创建C时，调用B1(10)和B2(20)时，并没有调用其构造函数里对应的A(1)和A(2)，因为当前只有一个A，不能再由B1和B2同时来创建，所以，需要在D()的创建函数里，明确写明```A(0)```，否则，就会出下面的错误
```
error: no matching function for call to ‘A::A()’
     C(int c) : B1(10), B2(20)
```

## 有趣的是：只有一个单线继承，但用了virtual，编译器也是按Diamond法则去处理的

代码如下:

```
#include <iostream>

struct A 
{
    A(int a)
    {
        std::cout << "A ctor = " << a << '\n';
    }
};

struct B1 : public virtual A
{
    B1(int b1) : A(1)
    {
        std::cout << "B1 ctor = " << b1 << '\n';
    }
};

struct C : public B1
{
    C(int c) : B1(10)
    {
        std::cout << "C ctor = " << c << '\n';
    }
};

int main()
{
    C c(333);

    return 0;
}
```

上面代码编译会出粗，错误信息如下：
```
error: no matching function for call to ‘A::A()’
     C(int c) : B1(10)
```

但如果修改一行代码，如下：
```
C(int c) : B1(10), A(0)
```

那么编译通过，执行结果是：
```
A ctor = 0
B1 ctor = 10
C ctor = 333
```
