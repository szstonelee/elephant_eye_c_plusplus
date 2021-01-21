# 引言

我们知道，C++里，如果有virtual函数，则destructor必须为virtual，why?

下面这些code也许能说明得更透彻。

请参考：[code/multi_inheritance_cast.cpp](code/multi_inheritance_cast.cpp)

# 第一个实验，没有virtual的多层继承

## 类说明

请参考两个Base类

```
class NoVirtualBase1
{
private:
  char name_[32] = "NoVirtualBase1";
  char buf_[1<<20];
};

class NoVirtualBase2
{
private:
  char name_[32] = "VirtualBase2";
  char buf_[1<<20];
}
```

和继承这两个子类的父亲类
```
class NoVirtualDerived : public NoVirtualBase1, public NoVirtualBase2
{
private:
  char name_[32] = "NoVirtualDerived";
  char buf_[1<<20];  
}
```

可以看到，每个类都有自己的内部静态数据 name_ 和 buf_。因此，NoVirtualDerived 就包含了三个内存区，i.e., 自己的name_和buf_，以及两个儿子的name_和buf_。

然后我们只测试no virtual的情况
```
int main()
{
  test_no_virtual();

  // test_virtual();

  return 0;
}
```

在test_no_virtual()里，我们分别用不同的release函数，去delete不同类型的指针，尝试释放资源，然后看内存是否泄漏或出错。

## 先调用release_from_pointer_no_vritual_1()

首先用
```
void test_no_virtual()
{  
  ...
  release_from_pointer_no_vritual_1(father);
  // release_from_pointer_no_vritual_2(father);
}
```

这样调用的是：delete pointer of class NoVirtualBase1
```
void release_from_pointer_no_vritual_1(NoVirtualBase1* p)
{
  std::cout << "Release no virtual base 1 adress = " << p << '\n';
  delete p;
}
```

我们编译和执行
```
g++ -std=c++17 multi_inheritance_cast.cpp -fsanitize=leak
./a.out
```

### 执行结果
```
NoVirtualDerived address = 0x7f15288ff000
get name from father = NoVirtualDerived
No virtual base 1 pointer address = 0x7f15288ff000
get name as NoVirtualBase1 = NoVirtualBase1
No virtual base 2 pointer address = 0x7f15289ff020
get name as NoVirtualBase2 = NoVirtualBase2
Release no virtual base 1 adress = 0x7f15288ff000
```

### 分析

在print_address_for_no_virutal_1()里，打印的指针地址和derived pointer是一样的，里面的get_name()，访问的是base1的name_。

在print_address_for_no_virutal_2()里，指针地址和derived pointer是不一样的，因为derived的内存分布是：
```
internal memory of base1,  <- *base1 and *derived
internal memory of base2,  <- *base2
internal memory of only derived (exclude base1 and base2)
```

而get_name()，也是每个指针对应对象（相当于做了static_cast）的get_name()。

然后delete *base1，没有内存泄漏，因为base1的指针地址和derived的指针地址一致。

### Dive deeper问题

加一个更深的问题给大家：如果base里或derived里存在动态资源，然后每个类的destructor都会释放这些动态资源，会如何，比如：
```
class AnyForBaseOrDerived {
  AnyForBaseOrDerived() {
    dynamic_buf_ = new char[1<<10];
  }
  
  ~AnyForBaseOrDerived() {
    delete dynamic_buf_;
  }

private:
  char* dynamic_buf_ = nullptr;
};
```

## 然后切换到release_from_pointer_no_vritual_2()

```
void test_no_virtual()
{  
  ...
  // release_from_pointer_no_vritual_1(father);
  release_from_pointer_no_vritual_2(father);
}
```

### 执行结果

```
NoVirtualDerived address = 0x7fb983cff000
get name from father = NoVirtualDerived
No virtual base 1 pointer address = 0x7fb983cff000
get name as NoVirtualBase1 = NoVirtualBase1
No virtual base 2 pointer address = 0x7fb983dff020
get name as NoVirtualBase2 = NoVirtualBase2
Release no virtual base 2 adress = 0x7fb983dff020
LeakSanitizer: bad pointer 0x7fb983dff020
==2262==Sanitizer CHECK failed: ../../../../src/libsanitizer/sanitizer_common/sanitizer_allocator_secondary.h:122 ((IsAligned(reinterpret_cast<uptr>(p), page_size_))) != (0) (0, 0)
```

### 分析

有内存不对的错误。因为是debug，所以是报错，如果是release，则会是undefined behavior

在print_address_for_no_virutal_1()里，base1的指针和derived的指针仍是一致的，所有的get_name()也和上面一样。

唯一不同的是：我们用delete *base2来释放内存，而base2的指针和derived的并不一致，因此会导致debug版报错，因为这并不是malloc()分配的内存起点，i.e., 内存地址前面的8字节内容，本应该是heap allocation长度，但现在根本不对。当前的8字节内容，还属于base1的内存空间。

# 第二个实验，多层继承，而且virtual

## 类说明

请参考两个Base类，VirtualBase1和VirtualBase2。

以及多层继承类VirtualDerived。

他们都用到了virtual函数，而且destructor都是virtual。

我们一样用上面的方法去测试，看看结果有什么不同？

先改掉main()里面的代码
```
int main()
{
  // test_no_virtual();

  test_virtual();

  return 0;
}
```

## 先调用release_from_pointer_vritual_1()
```
void test_virtual()
{
  ...
  release_from_pointer_vritual_1(father);
  // release_from_pointer_vritual_2(father);
}
```

### 执行结果
```
VirtualDerived address = 0x7fb8721ff000
get name from father = VirtualDerived
virtual base 1 pointer address = 0x7fb8721ff000
get name as VirtualBase1 = VirtualDerived
virtual base 2 pointer address = 0x7fb8722ff028
get name as VirtualBase2 = VirtualDerived
Release virtual base 1 adress = 0x7fb8721ff000
execute ~VirtualDerived(), this = 0x7fb8721ff000
execute ~VirtualBase2(), this = 0x7fb8722ff028
execute ~VirtualBase1(), this = 0x7fb8721ff000
```

### 分析

从print_address_for_virutal_1() 和 从print_address_for_virutal_2()的地址看，base1的指针地址和dervied的指针地址一致，但base2不同。

这和上面的No Virtual是一样的。

但get_name()时，情况却完全不一样，base2->get_name()获得的是字符串是VirtualDerived。

这就是virtual的特点，函数被override了。

delete *base1，也没有什么问题，没有内存泄漏或报错，而delete *base1，其实执行的是derived的destructor，即此时，derived里的base1的destructor被替换了（override），函数地址是derived的destructor，this指针是derived。

## 再切换到release_from_pointer_vritual_2()
```
void test_virtual()
{
  ...
  // release_from_pointer_vritual_1(father);
  release_from_pointer_vritual_2(father);
}
```

### 执行结果
```
VirtualDerived address = 0x7f88c28ff000
get name from father = VirtualDerived
virtual base 1 pointer address = 0x7f88c28ff000
get name as VirtualBase1 = VirtualDerived
virtual base 2 pointer address = 0x7f88c29ff028
get name as VirtualBase2 = VirtualDerived
Release virtual base 2 adress = 0x7f88c29ff028
execute ~VirtualDerived(), this = 0x7f88c28ff000
execute ~VirtualBase2(), this = 0x7f88c29ff028
execute ~VirtualBase1(), this = 0x7f88c28ff000
```

### 分析

和virtual1基本一致，除了```Release virtual base 2 adress = 0x7f88c29ff028```

没有内存泄漏和报错。

但为什么delete *base2时，地址是不同了，i.e., *derived == 0x7f88c28ff000, *base2 == 0x7f88c29ff028，最后的解析顺序和结果完全一样呢？

因为此时derived里面的base2的解析函数也被override了，里面的函数指针地址其实已改为derived的destructor，同时调用参数，那个所谓的this指针，也不是指向base2，而是指向derived，i.e., 此时，base1, base2, derived的destructor里面的内容完全一样，this参数也一样。

这就是destructor是virtual的好处，不会产生内存泄漏或错误。



