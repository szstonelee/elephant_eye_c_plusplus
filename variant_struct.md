# variant struct in C++

下面显示一个动态大小的结构在C++里如何实现。

我的编译环境都是64位，X86, Linux

## int的字节大小

先看下面的代码，很简单
```
class A{
    int a_;
};

int main() {
    std::cout << sizeof(A) << '\n';
    return 0;
}
```

执行结果是：```4```

很显然，int，虽然被包括在class A里，但A的结构大小就是4字节。

Deep in: 如果是空的Class，sizeof()是多大？答案是1字节。

## std::string的大小

```
class A{
    std::string s_;
};
```

执行结果是: ```32```

std::basic_string的内部，是四个8字节的，分别是capacity, size, pointer to heap of char* buffer, pointer of SSO buffer

当然，如果存储的字符串内部比较小，会不分配heap动态内存，而是用这32个字节存储字符串。这个被称之为SSO (Small String Optimaztion), 当然SSO需要size信息，所以不能存储到32字节这么多，各个硬件平台甚至不同的库下SSO都有不同。[细节大家上网查一下1。](https://stackoverflow.com/questions/10315041/meaning-of-acronym-sso-in-the-context-of-stdstring?noredirect=1)， [细节2](https://stackoverflow.com/questions/21694302/what-are-the-mechanics-of-short-string-optimization-in-libc?noredirect=1).

[Facebook的Folly string又进一步优化](https://github.com/facebook/folly/blob/master/folly/docs/FBString.md)

## int + std::string的大小

```
class A{
    int a_;
    std::string s_;
};
```

执行结果是：```40```

虽然int可以是4字节，但如果作为整个结构，编译器尽量用word（8字节）去对齐，因此整个结构会是8+32字节

## int + 8字节的正常指针
```
class A{
    int a_;
    int* is_;
};
```

执行结果是：```16```

is_是一个指针，8字节，a_是为了word对齐，也占用8字节，因此是16字节。

理论上，a_和is_是内存连续的，尽管a_和指针is_之间有4字节因为word对齐padding了多于的4字节。

我们可以让is_指向一个分配的内存，所以，is_可以说指向一个动态分配的int数组。

### 深入探讨1
```
class A{
public:
    int a_;
    char* p_;
    int b_;
};
```

这个答案是：```24```

因为b_也需要word对齐，和上面的一个道理，right?

### 深入探讨2
```
class A{
public:
    int a_;
    int b_;
    char* p_;
};
```

这个答案是：```16```

why? ，因为每个int其实只用4字节，放在一起，也不影响word对齐。

NOTE: c++里，member data是按你declare的顺序在内存安排的，同时，也是按这个顺序倍依次instantialized的，对于dtor，次序反之。

## int + 不占内存的数组

```
class A{
    int a_;
    int is_[];
};
```

执行结果是：```4```

这个很有意思，is_从代码上看，还是一个数组，但不占内存。

## int + 固定大小的数组

```
class A{
    int a_;
    int is_[1];
};
```

执行结果是：```8```

is_还是一个数组大小为1的数组（和a_地址连在一起），因此总长度是8字节。一共是2个int，a_和is_[0]

注意：没有padding 4字节了，因为is_并不是一个指针，而是一个内置数组，从物理地址而言，就紧贴着a_

## 动态大小的数组，我们的重心：variant struct

## 一个简单例子

```
#include <iostream>
#include <string>
#include <cassert>

class A{
public:
    int sz_;
    int is_[];
};

A* create_variable_struct(int sz) {
    assert(sz >= 0);
    void* mem = std::malloc(sizeof(int) + sz*sizeof(int));
    A* p = static_cast<A*>(mem);
    p->sz_ = sz;
    return p;
}

void clear_variable_struct(A* p) {
    // do nothing
}

int main() {
    A* my_variable_a = create_variable_struct(1024);
    // use my_variable_a
    clear_variable_struct (my_variable_a);
    delete my_variable_a;

    return 0;
}
```

这个你可以用内存监测，如下，运行后，没有内存泄漏
```
g++ -std=c++17 -fsanitize=leak test.cc
```

分析：

我们发现，is_[]现在是一个内存上和sz_紧密相连的长度可变的内置数组，注意：is_并不是再指向另外一块动态内存，虽然是在create_variable_struct()里通过malloc分配而来的，但malloc()分配的大小，包括了sz_和这个数组，是它们之和。

如果sizeof(A)，仍然是4字节，结构上，C++认为A只有sz_占了4字节。这个没毛病，和上面的代码的结果是一致的。

但对于malloc而言，上面的代码是分配了4+4*1024大小的heap块，当然，malloc()分配的内存块前面还有描述这个内存块的额外的8个字节。否则free()，或则delete，就不知道如何通过一个指针值，i.e., heap地址，而回收这块内存块。但我们不用理会这个分配的内存块所需要长度信息，i.e. 内存分配的额外的meta data消耗。

大家可能比较好奇，为什么要调用一个什么都不做的clear_variable_struct()。这是为下面的代码准备的，即我们要思考一个问题，如果is_并不是一个简单的int数组，而是一个类型数组，对于类型，我们要考虑其内部可能又动态分配了内存，比如：std::string，就32字节大小，但通过其内部的几个指针，完全可以分配更大内容的内存，从而可以使std::string像buffer一样被使用，用几十个G都没有问题。

所以，我们必须要能调用类型数组的每个对象的析构桉树dtor()，让每个对象先释放自己的资源，最后，才能free()掉整个my_variable_a内存块，i.e., delete my_variable_a，只free了malloc，以及析构了sz_。

换言之：delete my_variable_a没有对数组对象进行清理，而只是清理了内存块（当然，包括sz_的清理，因为sizeof(A)是知道有sz_这个member data对象的，并且清除sz_的类型）。上面的int类型，只是恰巧不需要清理，i.e., int没有析构函数，而避免了内存泄漏这个问题。

如果我们换掉数组的类型int，而采用其他可能用到其他动态内存的类型，比如用std::string，并让std::string的大小超过SSO的大小（如果SSO， std::string不分配heap了），你可以再试一把，就发现会有内存泄漏。

下面我们看一下数组是类型，如何解决这个问题。

### template下的variant struct

```
#include <iostream>
#include <string>
#include <cassert>

template <typename T>
class A{
public:
    int sz_;
    T var_array_[];
};

template <typename T>
A<T>* create_variable_struct(int sz) {
    assert(sz >= 0);
    void* mem = std::malloc(sizeof(int) + sz*sizeof(T));
    A<T>* p = static_cast<A<T>*>(mem);
    p->sz_ = sz;
    for (int i = 0; i < sz; ++i) {
        new (&(p->var_array_[i])) T();    // or user-defined ctor
    }
    return p;
}

template <typename T>
void clear_variable_struct(A<T>* p) {
    for (int i = 0; i < p->sz_; ++i) {
        p->var_array_[i].~T();
    }
}

int main() {
    A<int>* my_variable_a = create_variable_struct<int> (1024);
    // use my_variable_a
    clear_variable_struct<int> (my_variable_a);
    delete my_variable_a;

    return 0;
}
```


clear_variable_struct()，用了template，因为对于var_array_，如果是int数组，是不用析构函数\~T()。但如果是其他类型的结构，里面有动态的内存分配，比如：std::string，则必须调用析构函数\~T()，否则会导致内存泄漏。

还有一个要注意的是：create_variable_struct()里，还调用了ctor。这是因为malloc()只有内存分配，并没有初始化数组中所有的T对象，即只完成了new 不带地址参数的部分工作。

### 结论

1. 我们使用动态结构，是让其在内存上做了连续，这样对于CPU的cache非常好。

2. 动态结构里的数组的长度是变长的，即每个对象可以长度不一样

3. 用clear_varialble_struct()后，再delete这个对象，没有内存泄漏。可以优化这块代码，整合到类A的dtor里

4. 如果数组长度为0，相比指向heap动态分配的一个指针，我们节省了一个指针的8字节

一个使用了variant struct的范例可参考：[Skip List performance with different memory layouts](skip_list_performance_with_memory.md)
