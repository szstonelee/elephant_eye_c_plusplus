
哪个Skip List快点：我这里用中文写，因为快点

# 背景资料

## Skip List 种类 

有如下几个Skip List

1. 常规Skip List，Class SkipSet，参考 skipset (header & cc file)
2. 向量Skip List，Class VectSkipSet，参考 vectskipset (header & cc file)
3. 无锁Skip List，Class LockFreeSkipSet，参考 lock_free_skip_set.h

## 区分

| 区分  | Skip Set | Vector Skip Set | Lock Free Skip Set |
| :--: | :--: | :--: | :--: |
| 直接支持多线程 | 不 | 不 | 支持 | 
| 内存布局 | 多级Linked List | 局部使用Vector | 多级Atomic Linked List |

# 性能分析一：insert建立树

我们随机输入8百万个随机分布的数，建立skip list树，比较所花的时间。

## Lock Free Skip Set

### 单线程

参考 bench_lfss.cc，里面的bench_ad()里的bench_add_random_single_thread()

```
g++ -std=c++17 -O2 bench_lfss.cc
```

结果是：20382689 us

### 多线程

参考 bench_add_random_multi_threads()。

比如：8个线程，这里将8百万个数，每个线程并发处理1百万个数的插入，而且每个线程处理的数没有交集，是最完美的线程并发场景了。

| 线程数 | 时间 (us) |
| :--: | :--: |
| 1 | 21289500 |
| 2 | 9976874 |
| 4 | 5038844 |
| 8 | 3389950 |
| 16 | 3379878 |

NOTE: 
1. 这里线程数1，表示再启动一个线程去做，和上面的单线程稍微不同，但结果应该差不多
2. 由于我的Mac是4 Cores并可以超线程一倍，理论上是相当于8个virtual cores，

结论：
1. 从上表也可以看出到，当线程数倍数增加时，性能也是倍数增加。符合Lock Free Skip List的预期，也是Lock Free Skip List的优势。
2. 线程数到了机器实际核数时，基本上到了性能上限。如果更多线程数，实际上并没有什么提高。

## Skip Set

### 单个线程单个树

参考：bench_add_random_by_one_skipset()

我们也是8百万个数，用Skip Set建立一个树

结果：16745350 us

我们可以看到，如果和Lock Free Skip Set对比的话：

1. Skip Set要好于单线程下的LockFreeSkipSet，因为它没有用到atomic，但基本一个数量级，因为主要时间还是花在main memory的随机访问上了
2. 如果和LockFreeSkipSet多线程对比，LockFreeSkipSet的性能将随着cpu核数的增加而倍数增加，这也是LockFreeSkipSet的优势

### 多个线程多个子树

由于SkipSet不支持多线程，但我们想使用多核（因此多线程），我们可以将一个8百万的大树，分裂称8个小树，每个小树是1百万个key，并且每个树之间不冲突。

可以参考 bench_add_random_by_multi_skipset()

结果：2448832 us

结论：
1. 也是和机器的核数成线性关系，因为也是并发不冲突
2. 因为没有使用atomic，因此和LockFreeSkipSet最好的情况比，有30%的提高
3. 但麻烦是多个小树，要保证各个小树都同样大小，并且没有重叠，是挑战，上层的代码要复杂很多

## Vector Skip Set

参考 bench_add_random_by_vector_skip_set()。为了简单，我们只考虑一个线程，一个大树。但实际Vector Skip Set同样可以用多个子树组成，然后让每个线程只负责一个子树，这样就类似SkipSet一样可以享受多核的好处，麻烦就是代码复杂了。

结果：12181049 us

结论：比SkipSet还要快，因为Vector的原因

# 性能分析二：range scan

## 测试用例

我们还是用8百万个随机数组成的一颗大树，测试各个不同类型的Skip Set的Range Scan性能。

方法是：产生1千多个启动随机数，对于每个启动随机数，开始一次Range Scan，Range的范围是64K（6万多个连续值的范围）。

在扫描过程中，无insert, remove等修改操作影响树。

请参考：bench_scan_cmp()

## Skip Set

参考：bench_scan_skipset()

结果：9386557 us

## Lock Free Skip Set

我们用多线程，并发测试其性能，并发的方法是：每个线程都分到同样比例的启动随机数，而且无重叠。因此是最完美的并发环境。

参考：bench_scan_lockfreeskipset()

| 线程数 | 时间（us）|
| :--: | :--: |
| 1 | 9957729 |
| 2 | 4954618 |
| 4 | 2598678 |
| 8 | 1757191 |
| 16 | 1729390 |

结论：
1. Lock Free Skip Set又展现了和硬件核数相关的性能提升，核数增加一倍，性能也接近倍数
2. 超过硬件核数再增加线程数，意义不大
3. 单线程下，和SkipSet对比，SkipSet稍微好些，这是因为atomic的开销，不过基本一个量级，因为大部分的时间还是花在每个key在Main Memory上的单个读取

## Vector Skip Set

Vector Skip Set，对于单个树，像Skip Set一样，不支持多线程并发读取（除非没有修改，但现实中这几乎不可能）

所以，我们只测试单线程的性能，参考：bench_scan_vectskipset()

结果：1775336 us

结论：
1. 哇塞，单线程下的VectSkipSet居然和4硬核8虚核的多线程性能相当。可见Vector的伟大功效
2. 不过当机器核数增大下，LockFreeSkipSet的更多线程将会击败单线程下的VectSkipSet
3. 但是：VectSkipSet可以反败为胜，因为有以下的优化武器可以用

## 更多的优化考虑
1. 重新组织整个树的内存分布，使每个key按照order，在内存上尽量连续，这个可以带来10倍（相比vector而言），或者50倍（相比skipset而言）的性能提升。这个方法：对于几个类型的Skip List都可以采用，但VectorSkipSet和SkipSet会更好做，因为它们在单线程下执行，编码容易很多。

2. 如果想让Range Scan针对VectorSkipSet和常规SkipSet也能使用多线程（多核），可以让整个大数分裂称多个小树，每个小树基本高度一致，同时互不重叠。这会让VectorSkipSet和SkipSet享受多核的好处，但是会带来编码的复杂。

## 其他因素的影响

1. 我们做Range Scan测试时，是没有其他并发修改的影响，比如insert, remove动作。如果出现上面的动作会如何，肯定会带来性能的降低。如果是多线程的LockFreeSkipSet，其性能影响降低会更大，如果相比单线程下的SkipSet或VectSkipSet。这是我的个人推测，有兴趣的人可以写程序测试一下。

2. 如果出现了负载过大，导致线程竞争下出现重读更多（你需要进入代码仔细研读才能想象这个场景），这会让LockFreeSkipSet性能下降。但这对于单线程的SkipSet或VectSkipSet，则没有任何影响。


