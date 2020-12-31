# 引子

在知乎上，看到这样一篇文章。

[c++ make_shared 比 raw 指针更快？](https://www.zhihu.com/question/265082609/answer/289255002)

# 我的测试

## 修改

为了保持对比，我对raw指针，增加了delete语句，这样和shared_ptr就可以匹配了

```
#include <time.h>
#include <stdio.h>
#include <memory>

struct T
{
  int n;
};

int main()
{
  const int count = 20000000;
  clock_t begin = clock();
  for(int i = 0; i < count; i++) {
      T *p = new T;
      delete p;
  }
  clock_t end = clock();

  double c1 = double(end-begin)/CLOCKS_PER_SEC;
  printf("raw:%f\n", c1);

  begin = clock();
  for(int i = 0; i < count; i++)
      std::shared_ptr<T> ps(new T);
  end = clock();
  c1 = double(end-begin)/CLOCKS_PER_SEC;
  printf("shared_ptr_from_raw:%f\n", c1);

  begin = clock();
  for(int i = 0; i < count; i++)
      std::shared_ptr<T> ps = std::make_shared<T>();
  end = clock();
  c1 = double(end-begin)/CLOCKS_PER_SEC;
  printf("shared_ptr_from_make:%f\n", c1);
  return 0;
}
```

编译
```
g++ test.cc -std=c++17 -O0
g++ test.cc -std=c++17 -O2
```

## Linux

OS: Ubuntu 20.04.1 LTS

Gcc: (Ubuntu 9.3.0-17ubuntu1~20.04) 9.3.0

| mode | compile O0 | compile O2 |
| -- | -- | -- |
| raw | 0.682932 | 0.661073 |
| sptr_from_raw | 2.185521 | 1.369601 |
| sptr_from_make | 5.035151 | 0.723181 |

## Mac

OS: 10.15.7 (Catalina)

Clang: 11.0.3 (x86_64-apple-darwin19.6.0)

| mode | compile O0 | compile O2 |
| -- | -- | -- |
| raw | 1.412715 | 0.000003 |
| sptr_from_raw | 6.165197 | 3.538972 |
| sptr_from_make | 4.193568 | 1.609780 |

# 分析

Stacker Overflow，有一篇文章，说明了shard_ptr的raw和make方式的不同

[Difference in make_shared and normal shared_ptr in C++](https://stackoverflow.com/questions/20895648/difference-in-make-shared-and-normal-shared-ptr-in-c)

## O0

对于O0，raw是一次libc malloc，sptr_from_raw是两次，sptr_from_make是一次。

如果对比Linux和Mac，

Mac：可以发现malloc在Mac下的影响是比较大的，因此在sptr_from_raw的时间要大于sptr_from_make。

而Linux下，malloc效率比较高，因此用指针建造smart poitner，就要效率高些。而将所建造的对象和自己的control block在内存上连续，这些操作就要复杂和耗时多一些。

## O2

O2是考验编译器的优化水平。我们看到Clang对于raw的优化，简直是AI水平。即Clang发现循环里面的内存分配和回收，都是一个对象，干脆简化不调用malloc()函数。只是做了2百万次循环，只用到CPU的一个register即可，因此总时间是3us，每次操作的时间是 0.0015 ns。

其他优化可以参考下面一些文章。

[malloc and gcc optimization 2](https://stackoverflow.com/questions/17899497/malloc-and-gcc-optimization-2)
