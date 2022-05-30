# 如何用Smart Pointer作为参数 -- unique pointer

## 参考

本文是以下两个网上资源的理解和总结。

[Herb Sutter: GotW #91 Solution: Smart Pointer Parameters](https://herbsutter.com/2013/06/05/gotw-91-solution-smart-pointer-parameters/)

[StackOverflow: How do I pass a unique_ptr argument to a constructor or a function?](https://stackoverflow.com/questions/8114276/how-do-i-pass-a-unique-ptr-argument-to-a-constructor-or-a-function)

你可以先阅读上面这两个经典文章，也可以直接看下去，回头再参考

## unique pointer作为参数的几种形式

很显然，有如下几种形式

```
1. By value
  callee(unique_ptr<Widget> smart_w)

2. By non-const l-value reference
  callee(unique_ptr<Widget> &smart_w)

3. By const l-value reference
  callee(const unique_ptr<Widget> &smart_w)

4. By r-value reference
  callee(unique_ptr<Widget> &&smart_w)
```

注意：如果我们不用smart pointer，还有下面几种调用Widget的方式，和smart pointer相对应，对下面的调用，统称之为raw pointer
```
callee(Widget *w)
callee(Widget &w)
callee(const Widget *w)
callee(const Widget &w)
```

我们先简单复习一下raw pointer。

用*，还是&，主要区别，在于w是否可以是nullptr。

加不加const，主要是w这个raw pointer可否修改其内部数据，i.e., 只允许w调用其只读的方法 read-only methods。

下面的范例中，为了简单，如果用到raw pointer，我只表达为```Widget *w```，但你应该理解，它可能有四种变化。

## 1. By value: ```callee(unique_ptr<Widget> smart_w)```

### 函数参数By value的真正意义

caller产生一个copy，给callee，让它使用。

即从对象上看，caller有一个对象，callee也有一个对象，是两个对象，但是callee对象是从caller复制而来。

caller和callee分别管理自己对象的生命周期lifetime。callee管理起来比较简单，因为函数参数总是在其堆栈上，所以是自动管理的，i.e., 函数退出即销毁。

如果Copy by value for smart pointer，也是两个对象，即两个smart pointer对象，但smart pointer会很特别：

smart pointer里面有一个内部指针，指向一个要用的对象（一般在heap上）。即caller和callee，虽然都有一个自己的smart pointer对象，但它们**可能**通过内部指针，指向某一个共有的真正要用到的对象。

对于unique pointer，这个内部指针所指向的对象，就是实际的Widget对象，i.e., Widget directly.

对于shared pointer，这个内部指针所指向的对象，并不是实际的对象(Widget indirectly)，而是再包了一层。heap对象包了什么？一个共享计数（或者准确说：两个共享计数，但常规理解，只考虑其中的唯一strong counter）和一个真正的Widget对象指针。

### unique pointer不可以直接copy
下面这段代码是不能编译通过的
```
void caller()
{
  unique_ptr<Widget> smart_w = std::make_unique<Widget>();

  callee(smart_w);    // 编译报错，unique pointer不能直接copy
}

void callee(unique_ptr<Widget> smart_w)
{
  // use smart_w
}
```

我们必须做如下修改，才可编译通过
```
void caller()
{
  callee(std::move(smart_w));
}
```
我们必须加上std::move()，why？

### 用std::move()的意义

用std::move()，表示这个对象的资源（对于smart pointer，资源就是Widget）**理应**被转移走了，即当前这个对象（smart pointer），随后的代码，不可以再使用。

下面的代码可以编译通过，但绝对是错误的写法
```
void caller()
{
  callee(std::move(smart_w));

  smart_w->method();    // 这个很可能导致Segment Fault，是绝对错误的，因为smart_w已经无效了(Widget不见了)
}
```
换言之，std::move()，表示里面的对象smart_w，阳寿已尽，因为其内部指针所指向的Widget，转给了callee。caller的smart_w，其内部指针不再指向Widget了。

可见，std::move()是个非常明示（explicit）的资源转移信号，从代码级清晰说明了其意图，是good code。

这个，我们也称之为sink。即Widget这个资源，从caller()，下降到callee()里；或者说，caller里的smart_w，已经转移Widget的ownership到callee里的smart_w。

### 特别变种

```callee(const unique_ptr<Widget> smart_w)```是一个特别变种，本质和```unique_ptr<Widget> smart_w```差别不大，只是callee()申明smart_w不会变（但仍可以调用里面Widget的write动作）。类似```foo(int *p)```和```foo(int * const p)```的道理（注意：不是```foo(const int *p)```）。

我的个人建议：对于copy by value for unique pointer，不需要加上const，这么多不重要的修饰，代码看的脑袋都疼。

### 贤者的总结

Herb Suttter总结的很好：

>Express a “sink” function using a by-value unique_ptr parameter.

## 2. By non-const l-value reference: ```callee(unique_ptr<Widget> &smart_w)```

### l-value reference的意义

l-value reference的意义，和raw pointer指针的意义是一样的: caller和callee都指向同一对象。

那么```callee(unique_ptr<Widget> &smart_w)```，这个同一对象是什么？

当然是：```unique_ptr<Widget> smart_w```

从代码角度（不仅是针对smart pointer，而是普世意义的l-value reference），我们为什么要用reference，几个理由：

1. 避免上面的copy by value，因为copy可能是个很大的动作（cost is big），即callee()里的第二个对象（即参数）的建造constrution是很消耗资源的（CPU时间或大的内存分配）。
2. 我们应该在callee()里修改这个对象，然后callee返回后，caller()可以看到这个改过的效果。

注：当然，在callee()里，程序员有权可以不修改这个对象，如果是绝对不修改，我们不应该用non const，我们需要加上const（可以参考下面的By const l-value reference）。不管如何，这里既然没有加上const，也就是说：我们在callee()有意向要修改，至少有一行修改的代码，即使它可能出现在if等conditional语句下。

### 用在unique pointer这个对象上，又是何意义

显然，我们希望callee()里，修改唯一对象（因为没有const修饰）：即smart_w。

那么修改smart_w又是什么含义？正常来说，应该是换掉里面的Widget（因为unique ptr内容就一个指针，指向Widget）。比如：smart_w不再指向当前的Widget对象，而是另外一个Widget对象（或者清空为nullptr也可以，anyway，我们必须修改之）。

然后，callee返回后，caller可以继续使用这个smart_w，但是，里面的Widget对象，按理说，应该变了。

### 实际生产应用中，上面的逻辑几乎看不到

通过上面的分析，我们发现实践中，这样做的可能性不大。我们为什么要换掉一个unique pointer里指向的Widget呢？

所以，结论：By non-const l-value reference理论上可以用，但几乎看不到这样的实际案例。如果我是code viewer，如果有程序员这样用了l-value referecne for 
unique ptr，第一时间，我会怀疑他/她用错了，然后仔细检查callee()的逻辑代码。

### 贤者的总结

Herb Sutter的总结是：

>Use a non-const unique_ptr& parameter only to modify the unique_ptr.

我根据另外一篇文章，补上一个小点：

>It is rare. Check the code if there it is.

## 3. By const l-value reference: ```callee(const unique_ptr<Widget> &smart_w)```

### const l-value reference的意义

见上面的分析，两个理由中的2不存在了，即callee不会修改smart_w这个对象。

我们应该是避免理由1的copy cost。

### 但copy cost放在unique pointer上就不对了

我们知道，unique pointer并没有copy cost。所以理由1不充分。

既然如此，为什么不用raw pointer。即将下面使用const l-value reference的代码

```
void caller()
{
  unique_ptr<Widget> smart_w = std::make_unique<Widget>();
  callee(smart_w);
}

void callee(const unique_ptr<Widget> &smart_w)
{
  // use smart_w
}
```

改成下面的更清晰的用raw pointer的代码

```
void caller()
{
  unique_ptr<Widget> smart_w = std::make_unique<Widget>();
  callee(smart_w.get());
}

void callee(Widget *w)
{
  // use w
}
```

### 用raw pointer资源会泄漏吗？

你可能会说，unique pointer安全呀，我们用smart pointer就是为了保证资源不会泄漏。而raw pointer做不到资源安全呀。。。

这是个很大的误解!!!

**如果callee的raw pointer保证来自caller的smart pointer，它也保证资源是安全的**。

见下面的代码

```
void caller()
{
  unique_ptr<Widget> smart_w = std::make_unique<Widget>();

  callee1(smart_w.get());

  callee2(smart_w.get());

  // 可以继续用smart_w，也可以随便抛出异常

}   // guarantee: 当caller退出后，smart_w会销毁Widget资源，不管发生任何异常

void callee1(Widget *w)
{
  // use w，同时，可以随便抛出异常
}

void callee2(Widget *w)
{
  // use w，同时，可以随便抛出异常
}
```

结论：对于unique pointer，理论上，我们可以用const l-value reference，但是，用raw pointer代码更清晰，而且，没有任何资源泄漏的问题。即smart pointeer还是起到了它应该起到的作用。

你可能会说，万一callee里删除了w，不是会有问题。

是的，如果是下面的代码，会有问题
```
void callee(Widget *w)
{
  // use w

  delete w;
}
```
但是，上面的代码是坏代码（就如同std::move()后又继续用对象，code review应该不通过），我们不应该写这样的代码。即callee拿到raw pointer，它没有理由去删除这个对象，删除对象的责任，应该是caller。

### 记住贤者的话

重新引述一下Herb Sutter的话

>Don’t pass a smart pointer as a function parameter<br> 
>unless you want to use or manipulate the smart pointer itself,<br> 
>such as to share or transfer ownership.<br>
><br>
>Prefer passing objects by value, *, or &, not by smart pointer.

所以，By const l-value reference for unique pointer和上面的话相悖，因为

1. 我们即不会share它，因为它是unique pointer
2. 我们也不会transfer它（i.e., sink），因为用了const做了限定

注意：这点上，参考里面的两个文章是有抵触的，但我偏向Herb Sutter的话：

>Don’t use a const unique_ptr& as a parameter; use widget* instead.

## 4. By r-value reference: ```callee(unique_ptr<Widget> &&smart_w)```

### 右值应用的意义是什么？

对于callee，如果参数是右值应用，我们应该在callee里拿走这个对象的资源（即Widget），同时，我们应该将参数的资源，设置为空（一般是nullptr）。

当然，callee()里面可以不做这些动作（C++程序员的上帝自由），但占在caller()角度，这非常让人疑惑，callee你到底是拿，还是不拿？或者说，为了调用callee()，caller必须用std::move()，callee你就算不拿，caller也不敢继续用了。

如果，在callee()里，我们转移了资源，但没有将传入的参数的资源设置为空，那么很可能发生，同一资源被释放两次，一次在caller，一次在callee，这会导致程序非法。

### 对于unique pointer右值引用和上面的By value的对比

如果是unique pointer，我们用右值引用，需要做资源转移，同时传入参数的资源设置为空，这个代码必须明写，错写会导致非法。

但如果是上面的By value，你会发现，编译器其实帮我们做了类似的事情（即implement by compiler implicitly or automatically），即保证sink发生，资源自动转移。

这样一比较，我们的结论就来了：

>对于unique pointer，最好不要用 By r-value reference，而是用Copy by value替代。

## unique pointer作为函数参数的综合结论表

| 名称 | 函数接口形式 | 结论 |
| -- | -- | -- |
| By value | ```callee(unique_ptr<Widget> smart_w)``` | 很好Good，sink自动发生，编译器保证安全。需要注意：caller必须用std::move() |
| By non-const l-value reference | ```callee(unique_ptr<Widget> &smart_w)``` | 可以用，但实战中应该几乎不需要，建议你仔细检查callee代码 |
| By const l-value reference | ```callee(const unique_ptr<Widget> &smart_w)``` | 最好不用，推荐用raw pointer会更清晰。需要注意：不要害怕在smart pointer里用到raw pointer，这并不存在资源泄漏问题 |
| By r-value reference | ```callee(unique_ptr<Widget> &&smart_w)``` | 应该不用，而是用上面的Copy by value替代 |