Is lock-free algorithm faster than thread lock algorithm? 

Maybe not.

**Even single thread without any lock may out-perfrom lock-free algorithm with 100-CPU-core super machine.**

# Background

Basiclly, lock-free algorithms use atomic primitive and avoid lock like mutex or spinlock.

The common method for lock-free algorithm is CAS, i.e., [compare and swap](https://en.wikipedia.org/wiki/Compare-and-swap).

CAS is based on atomic primitive.

When using atomic primitive, there is a cost. We will lose cache. Atomic data structure is not friendly for cache because it uses [memory barrier](https://en.wikipedia.org/wiki/Memory_barrier).

We need to know some basic numbers:

1. For [CPU register](https://en.wikipedia.org/wiki/Processor_register), latency is much less than 0.1 nano second.
2. For [L1 cache](https://en.wikipedia.org/wiki/CPU_cache), latency is less than 1 nano second.
3. For [main memory](https://en.wikipedia.org/wiki/Computer_memory), latency is around 100 nano second.
4. For atomic primitive, if read, latency is a couple of nano second.
5. For atomic primitive, if write, latency is around 20 nano second.
6. CAS uses atomic primitive for write, so latency of CAS is around 20 nano seconds.
7. For mutex, if no race, i.e. we use [futex](https://en.wikipedia.org/wiki/Futex), latency is around 20 nano second. You can guess, futex is based on atomic primitive.
8. For mutex, if race, i.e., [thread context switch](https://en.wikipedia.org/wiki/Context_switch) happens, avarage latency is a couple of micro second.

So, if we use mutex and have no race, for lock acquiring, it is the same in cost if comparing to one CAS. But after that, it is totally differnet. Lock-free algorithms keep using CAS for each step. But when code enter [critcal section](https://en.wikipedia.org/wiki/Critical_section), it can use L1 cache or even CPU register.

**Critical section is cache-friendly, similiar to single thread performance.**

NOTE: but as a whole function which includes the lock, critical section can not compete with single thread without lock, because in the boundary of the lock, i.e., lock acquiring and lock releasing, there exist memory barrier.

The tremendous cost of latency is for mutex with race because blocking threads need to sleep and wake-up-again by OS scheduler. Why? it is all about cache. When thread context switch, we lose a lot of cache like L1 ~ L3 cache and [TLB](https://en.wikipedia.org/wiki/Translation_lookaside_buffer) buffer.

So we should use thread with race for higher latency operations, like disk and network IO, which are usually milli seconds.

# Code

You can check cas_vs_mutex.cc in code folder.

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
| compile option | mutex(ns) | CAS(ns) | ratio | 
| :--: | -- | -- | -- |
| O0 | 2071273 | 42319216 | 20 |
| O2 | 1994 | 17901105 | 8977 |

# Analyze

For compilation of O0, critical section in mutex uses L1 cache. So the ratio is around 20.

For compilation of O2, crititcal section in mutex uses CPU register. So the ratio is close to 9K.

In O2 compilation, the code in critical section is like these

```
sum = 0;
store sum to register;
for one million loop:
    do register++;
store register to sum;
```

We can see the optimazation for the code if there is no atomic primitive. If memory barrier exists, compiler can not do a lot of optimzation.

# Guess

If we compare to a lock-free data structure like skip list, maybe one thread without any lock win in performance.

That is why Redis is so fast.

If we want to use multi core:

For Redis, Antirez, the founder of Redis, suggests to use multi process of Redis.

For skip list, maybe we can use tens threads for each CPU core, and split skip list to multi segments. If each thread does not visit the segment of skip list in other thread, there is no data race. We can use lock freely and achieve benefits from cache and compiler optimization.

The additional benefit for no-using lock-free algorithm is transaction. We can lock the data structure, do subtraction to account A with addition to accoutn B. For lock-free algorithm, transaciton is not easy even not possible.   



