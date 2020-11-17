# Background

Because I wrote the following two articles:

1. [Is lock-free algorithm faster than thread lock algorithm?](lock_free_vs_thread_lock.md)

2. [Skip List performance with different memory layouts](skip_list_performance_with_memory.md)

I wonder the CPU cache, i.e. memory locality, improve the performance of Skip List a lot. I design a new data structor to prove my guess.

It is called Vector Skip List. You can check the code at [code/vectskipset.h](https://github.com/szstonelee/elephant_eye_c_plusplus/blob/master/code/vectskipset.h) and [code/vectskipset.cc](https://github.com/szstonelee/elephant_eye_c_plusplus/blob/master/code/vectskipset.cc).

For simplicity, I only finished the data type of Set. You can design the data type of Map by yourself.

The idea is simple. We will use skip list tree. The differnet stuff is that under each node, it is not a single key, but a vector of keys. Each node is represented by the smallest key in the vector. The algorithm will guarentee all keys for one node will be less than the next node's represented key, i.e., the smallest key for the node is greater than all keys in the previous node. So by this way, 
1. All nodes of the linked list are sorted, it matchs the property of skip list. 
2. The keys in the vector of one node are not sorted. But the keys for one vector are contiguous in memory layout. 
3. All keys are distinct.

In the vector of keys, it is unsorted. I limit the vector capacity to a small value, e.g., 64, for matching the cache size of CPU. In this way, If we need find a key in an unsorted vector, the complexity is O(N). But if considering the effect of memory locality, the performance will be compensated. The question is, how much is the trade off？

In [bench_vectskipset.cc](https://github.com/szstonelee/elephant_eye_c_plusplus/blob/master/code/vbench_vectskipset.cc) bench_scan_cmp(), I will compare the performances for range scan between skip list and vector skip list where they are initially constructed by random insertion. If you are interested in the contigous insertion, 
please read [Skip List performance with different memory layouts](skip_list_performance_with_memory.md). In reality, if no internal optimazation for skip list periodically happens, the tree should be in random memory layout.

Because all are assumed in single threaded condition, we can use Immutable Iterator for Vector Skip List. Immutable Iterator needs the guarentee that the tree can not be modified when the iterator is being used. Check the class of VectSkipSet::ImmuIter. 

After the modification of the tree, we can new another Immutable Iterator to scan. In this way, we guarartee the integrity.

# Test Cases

## gcc O0
```
g++ skipset.cc vectskipset.cc bench_vectskipset.cc -std=c++17 -O0
```

| Skip Set (seconds) | Vector Skip Set (seconds) | Ratio of SS/VSS |
| :--: | :--: | :--: |
| 9.00089 | 2.55394 | 3.5 |


## gcc O2
```
g++ skipset.cc vectskipset.cc bench_vectskipset.cc -std=c++17 -O2
```

| Skip Set (seconds) | Vector Skip Set (seconds) | Ratio of SS/VSS |
| :--: | :--: | :--: |
| 9.09811 | 1.69832 | 5.4 |

I tried to compile with Jemalloc, the result is similiar with O2 but a little quicker.

# Recap

1. Memory layout is crucial for the performance of Skip List. When using vector with Skip List, the gap for range scan is 5.4 times.
2. Vector is good for compiler optimization. For O2 vs O0, the perfomrance difference is 30% improvement or 50% degradation.
3. You need adjust the capacity of vecctor for the best performance. For the above test case, I found the capacity of 64 is probably the best. For your size of key in your machine (because different machine has different CPU cache size), it needs to be tested.
4. When I use vector, I can lower the threshhold of the biggest height of the skip list tree. In vectskipset.h, check kMaxLevel and kCapacity. It would be better for a large tree because the height of tree is lower when extreme random number happens.
5. The Vector Skip List is the same idea of Redis's ziplist for small hash table. O(N) is same or faster than O(1) for memory layout.
6. As a bonus, when we use Vector Skip List, we save memory. The number of nodes is less. The vector is good for memory cost if you compare random keys in the heap.