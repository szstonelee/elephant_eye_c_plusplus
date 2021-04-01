# 我们看一下到底产生了多少个MyClass Objects

## MyClass

```
class MyClass
{
public:
    MyClass() 
    {
        std::cout << "MyClass default ctor\n";
    }

    MyClass(const MyClass& copy)
    {
        std::cout << "MyClass copy ctor\n";
    }

    MyClass(MyClass&& move)
    {
        std::cout << "MyClass move ctor\n";
    }
};
```

这个class 在创建时打印创建ctor (construtor) 消息。相应地，destructor方法，我们上面没有写的那个~MyClass()，被称之为dtor。

## 最简单的main()

```
int main() 
{
    MyClass a;

    return 0;
}
```

输出是：
```
MyClass default ctor
```

很简单，在main()里，生成了对象a，是一个MyClass，而且是调用的default ctor生成的。这个对象a位于执行线程main()，即主线程，的堆栈上，storage duration是auto，即main()执行完，会自动销毁这个对象a（当退出当前的stack frame时），不需要我们做什么。

## heap创建对象

```
int main() 
{
    MyClass* b = new MyClass();

    delete b;

    return 0;
}
```

输出是：

```
MyClass default ctor
```

和上面稍微不同的是，对象MyClass在heap上，storage duration是dynamic，所以需要通过删除delete堆栈上的另外一个对象b，b是一个指针对象，来达到回收内存资源。

当然，如果编译器优化，可以将b存储在register上，但MyClass依然是dynamic storage duration，必须我们人工干预删除，否则会有内存泄漏。

为了避免内存泄漏，我们可以用smart pointer，即让一个智能对象，自己作为auto storage duration，同时智能对象里面的memer data放着这个指向dynamic MyClass的指针（相当于做了一层包裹wrap），然后智能对象的destructor method里，i.e., dtor，再去delete这个dynamic对象，从而保证内存不泄漏。

这是因为：一个stack frame销毁时，C++会保证里面所有的已生成的auto storage duration objects，都会调用dtor。所以，发生了exception，也可以通过这个机制保证不会发生资源泄漏。因此处理exception时，在没有找到exception handler时，statck frame还是会依次销毁(stack unwinding)。

如果发生exception时，还没有到exception handler，再发生exception会怎样，这时，会调用std::terminate()，停掉你的程序。

所以，C++希望你能保证你所有对象的dtor不要发生exception。而一般而言，dtor只是做回收资源的事，不应该再发生exception。

## 加入函数foo

```
void foo(MyClass v)
{ 
  // do nothing
}

int main() 
{
    MyClass a;

    foo(a);

    return 0;
}
```

输出是
```
MyClass default ctor
MyClass copy ctor
```

我们知道，函数调用，其argument是pass by value，所以，两个MyClass对象，一个在main()里的a，一个在foo()上的v，分别通过default和copy构造方法ctor创建出来的。

## 函数传入引用 
```
void foo(MyClass& v)
{ 
  // do nothing
}

int main() 
{
    MyClass a;

    foo(a);

    return 0;
}
```

输出是：
```
MyClass default ctor
```

只有一次MyClass对象生成，那么函数foo()里的argument，同样是一个pass by value，不过值是reference，而referece其机理和底层仍旧是一个指针（但不允许nullptr，同时reference变量自己，不允许改变，是个const，注意：const是限制reference自己本身，而不是限制其所指向的MyClass这个对象，你可以将reference看成一个类指针的对象，其所指的才是MyClass对象），指向某个对象，这次，指向的是main() stack frame上的对象a。所以，只有一个default ctor。

## temporary对象作为argument

```
void foo(MyClass v)
{ 
    // do nothing
}

int main() 
{
    foo(MyClass());

    return 0;
}
```

如上，我们在main()里调用foo()，在foo里直接生成一个temporary对象MyClass()，结果是

```
MyClass default ctor
```

只生成了一个temporary对象MyClass，就在foo()里。

注意：temporary对象也是rvalue，即prvalue。

## 我们尝试一下引用reference作为parameter

```
void foo(MyClass& v)
{ 
    // do nothing
}

int main() 
{
    foo(MyClass());

    return 0;
}
```

结果是编译出错，错误信息如下:
```
cannot bind non-const lvalue reference of type ‘MyClass&’ to an rvalue of type ‘MyClass’
```

原因是：在foo()里，定义的是一个non-const lvalue reference， i.e. v，而在main()传入的argument，是一个tempory object(MyClass())，是一个rvalue。对于gcc而言，这是不允许的，

C++的标准语法也是不允许的。 Why? 因为rvalue是const，而```const int a = 5; int b& = a;```是非法的。

NOTE: Visual Studio通过自己的特定的extention突破了这个compiler限制。

## 修正上面的错误

```
void foo(const MyClass& v)
{ 
    // do nothing
}

int main() 
{
    foo(MyClass());

    return 0;
}
```

结果是：
```
MyClass default ctor
```

只有一个对象产生。

## reference of rvalue

```
void foo(MyClass&& v)
{ 
    // do nothing
}

int main() 
{
    foo(MyClass());

    return 0;
}
```

这个可以编译通过。

运行结果是：
```
MyClass default ctor
```
只生成一个MyClass对象

## reference to rvalue -- continue

```
void foo(MyClass&& v)
{ 
    // do nothing
}

int main() 
{
    MyClass a;
    foo(a);

    return 0;
}
```

上面这段代码编译是通不过的，错误信息如下：
```
cannot bind rvalue reference of type ‘MyClass&&’ to lvalue of type ‘MyClass’
```

rvlaue reference，是不可以和lvalue绑定的，只可以和rvalue绑定（否则名字就不应该叫rvalue reference，right?），rvalue有两种：prvalue和xvalue。上面的reference of rvalue例子是prvalue，下面我们尝试一下xvalue。

```
void foo(MyClass&& v)
{ 
    // do nothing
}

int main() 
{
    MyClass a;
    foo(std::move(a));

    return 0;
}
```

用了xvalue, i.e., std::move()，编译通过，执行结果如下：
```
MyClass default ctor
```
只有一个MyClass对象生成。

## 在复杂一点，引入STL里的container -- vecotr
```
void foo(MyClass v)
{ 
    std::vector<MyClass> mys;
    mys.push_back(v);
}

int main() 
{
    MyClass a;
    foo(a);

    return 0;
}
```

生成几个对象，分别是如何生成的？

运行结果是：
```
MyClass default ctor
MyClass copy ctor
MyClass copy ctor
```

第一个default ctor，是MyClass a生成的。

第二个copy ctor，是调用foo()时，pass by value，产生一个copy

第三个copy ctor，是mys.push_back(v)时，mys接受的是一个引用reference of v，然后根据这个引用，生成了一个copy 对象，再存入vector中。

## vector上加点小变化

```
void foo(MyClass& v)
{ 
    std::vector<MyClass> mys;
    mys.push_back(v);
}

int main() 
{
    MyClass a;
    foo(a);

    return 0;
}
```

结果是：
```
MyClass default ctor
MyClass copy ctor
```

第一个default ctor，是MyClass a；创建的

第二个MyClass copy ctor，是mys.push_back(v), 这个v是个引用，所以vector.push_back()时，必须copy一个，然后放入自己的container。

## 再用temporary object试试vector
```
void foo(MyClass v)
{ 
    std::vector<MyClass> mys;
    mys.push_back(v);
}

int main() 
{
    foo(MyClass());

    return 0;
}
```

运行结果是：
```
MyClass default ctor
MyClass copy ctor
```

首先，到foo()里面时，只生成一个MyClass对象，即那个临时的MyClass()对象，是default ctor。

然后，调用mys.push_back()时，用的是rvalue reference调用，这时，mys还必须copy ctor生成一个MyClass对象，放入自己的container里。

## 下面权当练习
```
void foo(const MyClass& v)
{ 
    std::vector<MyClass> mys;
    mys.push_back(v);
}

和

void foo(MyClass&& v)
{ 
    std::vector<MyClass> mys;
    mys.push_back(v);
}
```

运行结果都是一样的，如下：

```
MyClass default ctor
MyClass copy ctor
```
大家可以想想为什么。