# thread并发，内存memory alignment和CPU cache line冲突

## 引子

在知乎上上看到一个帖子，[并发吹剑录（三）：『伪共享』凌乱记](https://zhuanlan.zhihu.com/p/359679079)

总体观点我是认同的，就是：thread并发时，你让thread访问不同的内存地址，希望提升性能。但这分开的内存，可能在一个cache line上，导致在CPU总线上有冲突（必须有写操作），从而降低性能。

但是我觉得文中的时间测量有点问题，就是时间包含了线程启动和销毁，这个并不可忽视。

所以，我源自作者的代码，但做了一点修正，就是在latency计算上，不考虑thread的影响，而且假设thread是几乎同时启动的而且机器至少2核以上（我的机器保证如此），具体可参考[代码](code/thread_with_align.cc)

注意：编译时不要打开O2，因为编译器优化可能让效果不是代码所反映的，比如：我们希望看到两个线程在block1代码中，冲突访问sums，因为cache line是64字节，所以，sums看上去是两个线程分别访问不同的单位，但由于CPU的架构，这些sum值其实在一个cache line上，导致在CPU总线上发生冲突，这是我们要测试的场景。但如果打开O2，在foo()中，编译器很可能将```sums[id]```定义成一个register，然后在register里做计算，最后才存回```sums[id]```，这就失去了这个程序测试的意义。

```
g++ -stc=c++17 -pthread thread_with_align.cc
```

我们分析三种模式的情况：

1. 两个thread，但是2个thread写入的sums，是没有alignment的，即会发生CPU总线冲突
2. 单个线程（main线程）顺序执行，下面称之为serial
3. 两个thread，但block3中的两个thread写入的AlignSums，是通过cache line（64字节）分开的，没有CPU总线冲突

## 测试结果

测试结果也很有趣，我将kVectorNum设置为不同值，1'000， 10'000， 100'000, 1'000'000，而且做多次测试，观察效果

### kVectorNum = 1'000

大部分情况下，都是thread小于serial，至于thread，有没有alignment，差别不大，一般情况下，都是alignment稍好一点，但有一次alignment时，cost超过serial

分析：由于循环量很少，同时values占内存不大，所以，这时，thread比较有效，但thread中，是否是alignment to cache line，影响不起决定性作用。

### kVectorNum = 10'000

大部分情况下，都是thread with align最小。thread次之，一般都比serial好，但有一次thread超过了serial

分析：随着循环量加大，同时values占内存加大，thread with align开始占优。而thread还是略好于serial的原因是：并发的效果还是大于CPU总线冲突的效果。

### kVectorNum = 100'000

thread with align最小，thread的cost开始超过serial了

分析：这时明显CPU总线冲突的影响更大，超过了并发的好处。原因是：循环数以及vlaues的加大(value需要占用更多的CPU cache，cache1->cache2->cache3)

### kVectorNum = 1'000'000

结果和kVectorNum = 100'000类似。分析也一样

有兴趣的人可以测试，由于每个人的机器不同，cache的级数和大小也不同，以及其他方面的影响（比如：循环数少时，内存和cache的临时作用，以及OS中其他process的影响，会更明显），我相信数据结果和我上面的可能有不同，但总趋势应该是一致的。