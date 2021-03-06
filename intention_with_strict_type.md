# C++里代码最好体现出intention，进一步，可以用type来严格约束程序可能犯的错误

## 简单一个容易犯错的例子
我们构建一个简单的日期类，然后打印一个日期：2020年10月5月的月份。

```
#include <iostream>

class MyDate {
public:
    MyDate(int d, int m, int y) 
        : day_(d), month_(m), year_(y)
    {}

    int month() {
        return month_;
    }

private:
    int day_;
    int month_;
    int year_;
};

int main() {
    MyDate d(5, 10, 2020);

    std::cout << "the month = " << d.month() << '\n';

    return 0;
}
```

上面代码可以编译，输出是：```the month = 10```

但万一不小心，我们写成了
```
    MyDate d(10, 5, 2020);
```
由于日期格式本来就很多，所以，我们可能认为没有什么问题，还是10月5日，没有想到是5月10日。

## 加强代码的表达力和目的性 intention
```
using Day = int;
using Month = int;
using Year = int;

class MyDate {
public:
    MyDate(Day d, Month m, Year y) 
        : day_(d), month_(m), year_(y)
    {
      // do nothing
    }

    Month month() const {
        return month_;
    }

private:
    Day day_;
    Month month_;
    Year year_;
};
```

这一下，至少阅读API时，我们可以更清楚。

构造MyDate时，其三个parameter，分别是Day, Month, Year，这个字面表达的很清楚。在获得month()时，返回的是Month。虽然实际数据都没有变，只不过using换了显示法，但代码的意图是更清晰了。

但我们仍避免不了caller写成
```
MyDate d(5, 10, 2020);
```

更糟糕的是，如果我们想表达2020年10月20号，上面可能误写成
```
MyDate d(10, 20, 2020);
```
这时，只有打印我们才能发现输出是：```the month = 20```

## 用结构进一步加强
```
struct Day {int val;};
struct Month {int val;};
struct Year {int val;};

class MyDate {
public:
    MyDate(Day d, Month m, Year y) 
        : day_(d), month_(m), year_(y)
    {}

    Month month() const {
        return month_;
    }

private:
    Day day_;
    Month month_;
    Year year_;
};

int main() {
    MyDate d(Day{10}, Month{20}, Year{2020});

    std::cout << "the month = " << d.month().val << '\n';

    return 0;
}
```

这时，读写代码时，我们就会发现```MyDate d(Day{10}, Month{20}, Year{2020});```看着有些不对，怎么会是Month{20}? 而且打印结果进一步说明了这个问题。

而且，你如果写成
```
MyDate d(10, 20, 2020);
```
编译是通不过的，提示错误是```no matching function for call to ‘MyDate::MyDate(int, int, int)’```，类型不对

## 进一步加强，用assert验证

当上面这个还是允许我们建造MyDate时，硬用Month{20}

下面的代码可以用assert做进一步的校验

```
#include <cassert>

struct Day {
    explicit Day(int v) {
        assert(v >= 1 && v <= 31);
    }
    int val;
};

struct Month {
    explicit Month(int v) {
        assert(v >= 1 && v <= 12);
    }
    int val;
};

struct Year {
    explicit Year(int v) {
        assert(v > 0);
    }
    int val;
};
```

这时，你如果输入是```MyDate d(Day{10}, Month{20}, Year{2020});```，会在执行时，输出assert错误
```
Month::Month(int): Assertion `v >= 1 && v <= 12' failed.
```

NOTE: 关于explici的位置
1. Day, Month, Year需要在构造函数前加入explicit了，否则```MyDate d(10, 20, 2020);```会复活。
2. 在MyDate()前加explicit，而Day, Month, Year前不加explicit，MyDate d(10, 20, 2020); 还是可以编译通过

