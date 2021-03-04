# Background
[In my another article](lock_free_vs_thread_lock.md), which compares the lock free algorithm with the one of single threaded, it hints that memory locality, or accurately the cache factor, maybe determine the performance.

In this article, I will try to deep into the performance of skip list in differenct kind memory layouts, i.e., random or contiguous.

First, you should read an amasing post, [Skip lists, are they really performing as good as Pugh paper claim?](https://stackoverflow.com/questions/31580869/skip-lists-are-they-really-performing-as-good-as-pugh-paper-claim). 

From the post, we know:

1. Changing the memory layout of the next pointer of pointer, which first is a pointer to a remote array and then a local array of pointers, will improve the performance of skip list by 30%.
2. Optimizing the memory allocation, which uses the fixed allocator, will improve by 30% comparing to optimization 1.
3. Using sorted input, which makes the skip list tree in contiguous memory layout, will improve by 4 times comparing to optimization 2.

NOTE: if you want to know more about variant struct, [please read this article.](variant_struct.md)

# Range scan
Now I will compare the performance for range scan of skip list because the range scan feature is the reason we choose skip list over hash map.

For simplicity, 
1. I borrow a lot of code from the post
2. I use skip set. It is the same thing when you change the type of set to map for skip list.
3. I do not use the fixed allocator but only the local array. 

You can check the whole code in following links

[code/skipset.h](https://github.com/szstonelee/elephant_eye_c_plusplus/blob/master/code/skipset.h)

[code/skipset.cc](https://github.com/szstonelee/elephant_eye_c_plusplus/blob/master/code/skipset.cc)

[code/bench_skipset.cc](https://github.com/szstonelee/elephant_eye_c_plusplus/blob/master/code/bench_skipset.cc)

I will construct two skip set trees, one with random input, the other with sorted input. The numbers of tree node are same, i.e., eight million for each.

For random input, check the code in scan_in_random(). For sorted input, check the code in scan_in_contiguous().

For the range scan, there are 1000 scans with random start node. Each scan range is 64K. Check bench_range_scan().

After the sorted input, the nodes in skip set will be construted one by one. Because the memory allocator, which supports the malloc() in create_node(), usually allocates the memory contigusously or one by one. I will show you two memory allocators. One is the standard libc, the other is Jemalloc.

I will try the following test cases in my MacOS.

1. gcc -O0
2. gcc -O2
3. gcc -O2 with Jemalloc

# Test cases

## gcc O0
```
g++ skipset.cc bench_skipset.cc -std=c++17 -O0
```

| random time (ms) | contiguous time (ms) | random/contiguous |
| :---: | :---: | :---: |
| 9028.68 | 677.074 | 13.3 |

## gcc O2
```
g++ skipset.cc bench_skipset.cc -std=c++17 -O2
```

| random time (ms) | contiguous time (ms) | random/contiguous |
| :---: | :---: | :---: |
| 8610.69 | 189.151 | 45.5 |

## gcc O2 with Jemalloc
You should install Jemalloc first. In Mac, it is 
```
brew install jemalloc
```

Check [Jemalloc help](https://github.com/jemalloc/jemalloc/wiki/Getting-Started)
```
g++ skipset.cc bench_skipset.cc -std=c++17 -O2 -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs`
```
It will compile the code by using dynamic memory library of Jemalloc. You can check it for sure
```
otool -L a.out
```
And see the following results or similiar
```
/usr/local/opt/jemalloc/lib/libjemalloc.2.dylib (compatibility version 0.0.0, current version 0.0.0)
```

NOTE: in Linux, please use ldd tool to check.

The result for Jemalloc is 

| random time (ms) | contiguous time (ms) | random/contiguous |
| :---: | :---: | :---: |
| 8787.97 | 162.122 | 54.2 |

# Recap

Memory layout is crucial for the performance of skip list in range scan.

You can try other type like making key as string. When the key size grows and/or the Node is composed of the pair fields of key/value, the memory size of one node will grow and the effect of CPU cache will degrade. But from the above test cases, I am confident that the gap of the performances would be huge.

So one way to optimize the performance of skip list is to periodically reconstrut it totally again or partially if the latency of whole reconstruting is big.

A new same baby is a better man, right?

# 新补充，Lock Free Skip List的数据报告

因为后来又编写了Lock Free Skip Set（参考：[Which Skip List is faster](which_skip_list_is_faster.md)），我突然有兴趣看一下用atomic的Skip List，在不同的建树内存结构下，是什么样的结构。

测试用例和上面的一样，请查看bench_lfss.cc里的bench_range_scan_in_random_and_contiguous()

结果如下：

## gcc

| random time (ms) | contiguous time (ms) | random/contiguous |
| :---: | :---: | :---: |
| 12316 | 2888 | 4.3 |

## gcc O2

| random time (ms) | contiguous time (ms) | random/contiguous |
| :---: | :---: | :---: |
| 10134 | 436 | 23.2 |

## gcc O2 with Jemalloc

| random time (ms) | contiguous time (ms) | random/contiguous |
| :---: | :---: | :---: |
| 15435 | 417 | 37.0 |

## 结论

1. 单看Lock Free Skip List，内存连续相比内存散列还是要好很多，可以到40倍。这是因为即使用了atomic，但和main memory比起来，main memory上所花的时间更多。
2. 如果将Lock Free Skip List和Skip List做横向对比，可以发现，Skip Set所花的时间更好(有近3倍的差别)，Skip Set的倍数也稍好。这就是atomic和非atomic的区别了。