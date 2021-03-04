# 下面显示一个动态大小的结构：variant struct in C++

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

## std::string的大小

```
class A{
    std::string s_;
};
```

执行结果是: ```32```

std::basic_string的内部，是四个8字节的，分别是capacity, size, pointer to heap of char* buffer, pointer of SSO buffer
当然，如果string内部比较小，会不分配heap内存，而是用这32个字节存储字符串（当然需要size信息）

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

## 8字节的正常指针
```
class A{
    int a_;
    int* is_;
};
```

执行结果是：```16```

is_是一个指针，8字节，a_是为了对齐，也占用8字节，因此是16字节。
我们可以让is_指向一个分配的内存，所以，is_可以说指向一个动态分配的int数组。

## 不占内存的数组

```
class A{
    int a_;
    int is_[];
};
```

执行结果是：```4```

这个很有意思，is_还是一个数组（和a_地址连在一起），但不占内存

## 固定大小的数组

```
class A{
    int a_;
    int is_[1];
};
```

执行结果是：```8```

is_还是一个数组大小为1的数组（和a_地址连在一起），因此总长度是8字节。一共是2个int，a_和is_[0]

## 动态大小的数组

```
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
    for (int i = 0; i < sz; ++i) {
        p->is_[i] = i;  // if is_是其他类，可以用new (&(p->is_[i])) ctor();来初始化
    }
    return p;
}

template <typename T>
void clear_variable_struct(A* p) {
    for (int i = 0; i < p->sz_; ++i) {
        p->is_[i].~T();
    }
}

int main() {
    A* my_variable_a = create_variable_struct(1024);
    // use my_variable_a
    clear_variable_struct<int> (my_variable_a);
    delete my_variable_a;

    return 0;
}
```

这个你可以用内存监测，如下，运行后，没有内存泄漏
```
g++ -std=c++17 -fsanitize=leak test.cc
```

clear_variable_struct()，用了template，因为对于is_，如果是int数组，是不用析构函数~T()。否则，如果is_是其他类型的结构，里面有动态的内存分配，如果不调用析构函数~T()，会导致内存泄漏。

我们可以看到上面的代码，有下面这些特征：

1. 从上面的main()可以看出，我们将sz_和is_在内存上做了连续，这样对于CPU的cache非常好。

2. is_是动态大小的数组，即每个对象可以长度不一样

3. 用clear_varialble_struct()后，再delete这个对象，没有内存泄漏。上面的代码，可以编写到类A的dtor里

4. 如果数组长度为0，相比指向heap动态分配的一个指针，我们节省了一个指针的大小，更不用说1中的cache friendly
