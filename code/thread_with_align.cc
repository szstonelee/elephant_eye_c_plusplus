#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>
#include <limits>

constexpr int kVectorNum = 1000000;
constexpr int kThreadNum = 2;       // NOTE: 一定是2，如果要测试alignment的话

void foo(const int id, const std::vector<int>& v, 
         std::vector<long>& sums, std::vector<int>& latencies) 
{
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < v.size(); ++i) 
    {
        if (i % kThreadNum == id) 
        {
            sums[id] += v[i];
        }
    }
    const auto end = std::chrono::steady_clock::now();
    latencies[id] = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

std::vector<int> init_values(const int num) 
{
    assert(num > 0);

    std::vector<int> values(num);
    for (int i = 0; i < num; ++i)
    {
        values[i] = i;
    }
    return values;
}

int get_max(const std::vector<int>& latencies)
{
    int max = std::numeric_limits<int>::min();

    for (const auto l : latencies)
    {
        max = std::max(max, l);
    }

    return max;
}

int get_sum(const std::vector<int>& latencies)
{
    int sum = 0;

    for (const auto l : latencies)
    {
        sum += l;
    }

    return sum;
}

struct AlignSums
{
    alignas(64) long m1;
    alignas(64) long m2;
};

void foo_with_align(const int id, const std::vector<int>& v, 
         AlignSums& sums, std::vector<int>& latencies) 
{
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < v.size(); ++i) 
    {
        if (id == 0)
        {
            sums.m1 += v[i];
        }
        else 
        {
            sums.m2 += v[i];
        }
    }
    const auto end = std::chrono::steady_clock::now();
    latencies[id] = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

int main() 
{
    auto values = init_values(kVectorNum);

    {   // 代码块 1, using parallel thread
        std::vector<long> sums(kThreadNum, 0LL);
        std::vector<int> latencies(kThreadNum, 0);
        std::vector<std::thread> tds;
        for (int i = 0; i < kThreadNum; ++i) 
        {
            tds.emplace_back(foo, i, std::cref(values), std::ref(sums), std::ref(latencies));
        }
        for (int i = 0; i < kThreadNum; ++i) 
        {
            tds[i].join();
        }
        // 我们假设机器至少双核，线程几乎同时启动，而且线程数小于核数，这样，执行时间是所有线程里最大那个
        std::cout << "block 1 in parallel cost(ms): " << get_max(latencies) << '\n';
    }

    {   // 代码块2, simulate but using single thread
        std::vector<long> sums(kThreadNum, 0LL);
        std::vector<int> latencies(kThreadNum, 0);
        for (int i = 0; i < kThreadNum; ++i) 
        {
            foo(i, values, sums, latencies);
        }
        std::cout << "block 2 in serial cost(ms): " << get_sum(latencies) << '\n';
    }

    {   // 代码块 3, using parallel thread and align
        // note: 由于vector采用动态分配，所以alignof() in vector是无效的
        AlignSums sums;
        std::vector<int> latencies(kThreadNum, 0);
        std::vector<std::thread> tds;
        for (int i = 0; i < kThreadNum; ++i) 
        {
            tds.emplace_back(foo_with_align, i, std::cref(values), std::ref(sums), std::ref(latencies));
        }
        for (int i = 0; i < kThreadNum; ++i) 
        {
            tds[i].join();
        }
        std::cout << "block 3 in parallel with alignment cost(ms): " << get_max(latencies) << '\n';
    }

    return 0;
}
