Is lock-free algorithm faster than thread lock algorithm? 

Maybe not.

**Even single thread may outperform lock-free algorithm with 100-CPU-core super machine.**

In otherwords, for one specific algorithm,

**Single thread could beat multi concurrent threads which use no lock.**

or

**One CPU core could run faster than multi CPU cores in parallel of no contention.**

# Background

Basiclly, lock-free algorithms use atomic primitive and avoid any locks like mutex or spinlock.

The common method for lock-free algorithm is CAS, i.e., [compare and swap](https://en.wikipedia.org/wiki/Compare-and-swap).

CAS is based on atomic primitive.

When using atomic primitive, there is a cost. We will lose cache. Atomic data structure is not friendly for cache because it uses [memory barrier](https://en.wikipedia.org/wiki/Memory_barrier).

We need to know some basic numbers:

1. For [CPU register](https://en.wikipedia.org/wiki/Processor_register), latency is much less than 0.1 nano second.
2. For [L1 cache](https://en.wikipedia.org/wiki/CPU_cache), latency is less than 1 nano second.
3. For [main memory](https://en.wikipedia.org/wiki/Computer_memory), latency is around 100 nano seconds.
4. For atomic primitive, if read, latency is a couple of nano seconds.
5. For atomic primitive, if write, latency is around 20 nano seconds.
6. CAS uses atomic primitive for write, so latency of CAS is around 20 nano seconds.
7. For mutex, if no contention, i.e. we use [futex](https://en.wikipedia.org/wiki/Futex), latency is around 20 nano seconds. You can guess, futex is based on atomic primitive.
8. For mutex, if contention, i.e., [thread context switch](https://en.wikipedia.org/wiki/Context_switch) happens, avarage overhead is a couple of micro seconds which is huge if you compare the above numbers. And Linux scheduling a thread takes hurndres of micro seconds. 

So, if we use mutex and have no contention, for lock acquiring, it is the same in cost if comparing to one CAS. But after that, it is totally differnet. Lock-free algorithms keep using CAS for each step. But when code enter [critcal section](https://en.wikipedia.org/wiki/Critical_section), it can use L1 cache or even CPU registers.

**Critical section is cache-friendly inside the section, similiar to the whole code of single thread, which is good for performance.**

NOTE: As a whole function which includes the lock, critical section can not compete with single thread without lock, because in the boundary of the lock, i.e., lock acquiring and lock releasing, there exist memory barriers. So if your applications call lock & unlcok() with the frequency of CAS as lock-free algorithm, it is the same cost if no lock contention, i.e. no thread contex switch, happens.  

The tremendous cost of latency is for mutex with contention because blocking threads need to sleep and wake-up-again by OS scheduler. Why? it is all about cache. When thread context switch, we lose a lot of cache like L1 ~ L3 cache and [TLB](https://en.wikipedia.org/wiki/Translation_lookaside_buffer) buffer.

So we should use thread with contention for higher latency operations, like disk I/O and network IO, which are usually milli seconds.

**Trading silver for gold is good. Trading silver for copper is bad.**

# Code

You can check [cas_vs_mutex.cc](https://github.com/szstonelee/elephant_eye_c_plusplus/blob/master/code/cas_vs_mutex.cc) in code folder.

For mutex, 
```
  std::lock_guard<std::mutex> lock(g_mutex);

  int sum = 0;
  for (int i = 0; i < times; ++i) {
    ++sum;
  }
```

For CAS,
```
  std::atomic<int> sum(0);
  for (int i = 0; i < times; ++i) {
    int expected = sum;
    while(sum.compare_exchange_weak(expected, expected+1, std::memory_order_relaxed));
  }
```

# Test results

We set one million loop times and compare mutex with CAS, in two compilation options, -O0 and -O2.
```
g++ -std=c++17 -O0 cas_vs_mutex.cc
g++ -std=c++17 -O2 cas_vs_mutex.cc
```

In my MAC, the result is as follow:
| compile option | no contention mutex(ns) | CAS(ns) | ratio (CAS/mutex) | 
| :--: | -- | -- | :--: |
| O0 | 2071273 | 42319216 | 20 |
| O2 | 1994 | 17901105 | 8977 |

Single thread with mutex beats lock-free with CAS (in one thread) by nine thousand times. 

We can conclude that the lock-free code runs in parallel with one hundred threads in a 100-CPU-cores super machine will lose the performance battle. 

# Analytics

For compilation of O0, critical section in mutex uses L1 cache. So the ratio is around 20.

For compilation of O2, crititcal section in mutex uses CPU register. So the ratio is close to 9K.

In O2 compilation, the code in critical section is like the following

```
sum = 0;
store sum to register;
for one million loop:
    do register++;
store register to sum;
```

We can see the optimazation for the code if there is no atomic primitive. If memory barrier exists, compiler can not do a lot of optimzation.

# Guess and suggestion

If we compare to a lock-free data structure like skip list, maybe one thread without any lock win in performance.

That is why Redis is so fast.

If we want to use multi core:

For Redis, Antirez, the founder of Redis, suggests to use multi process of Redis.

For skip list, maybe we can use one thread for each CPU core, and split skip list to multi segments with the same number of threads. If each thread does not visit the segment of skip list in other thread, there is no data contention. We can use lock freely and achieve benefits from cache, compiler optimization and multi CPU core.

The additional benefit for no-using lock-free algorithm is transaction. We can lock the data structure, do subtraction to account A with addition to account B atomically. For lock-free algorithm, transaciton is not easy even not possible.   

Another benefit for single thread programming is the simiplicity of coding. Lock-free algorithm is difficult to be error-free. Lock algorithm is in the middle for complexity.

My suggestion about applications using lock-free algorithm is to measure. You can compare the performance in the following two scenarios:

1. lock-free
2. single thread

 Single thread programming is easy to implement, lock-free is very complex and prone to error. e.g., C++ new or C malloc is not lock-free, and C++ delete can not be called freely.

If the performance of single thread is OK, you can try to optimize it more by using multithread with segments of your data struture.

My three other articles, 

1. [skip list performance with memory layout](skip_list_performance_with_memory.md)
2. [Vector Skip List vs Skip List](vector_skip_list.md)
3. [Which Skip List is faster](which_skip_list_is_faster.md)

have the same idea.



