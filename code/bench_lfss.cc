#include <cassert>
#include <random>
#include <algorithm>
#include <thread>
#include <chrono>

#include "lock_free_skip_set.h"
#include "skipset.h"
#include "skipset.cc"

std::vector<int> random_nums(const int bound) {
  std::vector<int> nums(bound);
  for (int i = 0; i < bound; ++i) {
    nums[i] = i;
  }
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(nums.begin(), nums.end(), g);

  return nums;
}

void bench_add_random_single_thread(const int bound) {
  const auto nums = random_nums(bound);

  sss::LockFreeSkipSet<int> lfss;
  const auto start_time = std::chrono::steady_clock::now();
  for (const auto num : nums) {
    lfss.add(num);
  }
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time);
  std::cout << "bench_add_random_single_thread for total " << bound/(1<<20) << " million, duration(nano) = " << duration.count() << '\n';
}

void add_random_one_thread_f(const int is, const int ie, const std::vector<int>& nums,
                             sss::LockFreeSkipSet<int>& lfss) {
  for (int i = is; i < ie; ++i) {
    lfss.add(nums[i]);
  }
}

void bench_add_random_multi_threads(const int thread_num, const int bound) {
  const auto nums = random_nums(bound);
  const int scope = bound/thread_num;

  int is = 0;
  std::vector<std::thread> threads;
  sss::LockFreeSkipSet<int> lfss;
  const auto start_time = std::chrono::steady_clock::now();
  for (int i = 0; i < thread_num; ++i) {
    threads.push_back(std::thread(add_random_one_thread_f, is, is+scope, nums, std::ref(lfss)));
    is += scope;
  }
  for (auto& th : threads) {
    th.join();
  }
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time);
  std::cout << "bench_add_random_multi_threads for total " << bound/(1<<20) << " million, thread nums = " << thread_num;
  std::cout << ", duration(nano) = " << duration.count() << '\n';
}

void bench_add_random_by_one_skipset(const int bound) {
  const auto nums = random_nums(bound);
  sss::SkipSet<int> ss;
  const auto start_time = std::chrono::steady_clock::now();
  for (const auto num : nums) {
    ss.insert(num);
  }
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time);
  std::cout << "bench_add_random_by_skipset for total " << bound/(1<<20) << " million, duration (nano) = " << duration.count() << '\n';
}

void one_skipset_for_one_thread_f(const int is, const int ie, const std::vector<int>& nums, 
                                  sss::SkipSet<int>& ss) {
  for (int i = is; i < ie; ++i) {
    ss.insert(nums[i]);
  }
}

void bench_add_random_by_multi_skipset(const int ss_num, const int bound) {
  const auto nums = random_nums(bound);
  const int scope = bound/ss_num;

  std::vector<std::thread> threads;
  std::vector<sss::SkipSet<int>> skipsets(ss_num);
  int is = 0;

  const auto start_time = std::chrono::steady_clock::now();
  for (int i = 0; i < ss_num; ++i) {
    threads.push_back(std::thread(one_skipset_for_one_thread_f, is, is+scope, nums, std::ref(skipsets[i])));
    is += scope;
  }
  for (auto& th : threads) {
    th.join();
  }
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_time);
  std::cout << "bench_add_random_by_multi_skipset, " << ss_num << " threads each with one SkipSet, duration (nano) = " << duration.count() << '\n';
}

void bench_add() {
  // bench_add_random_single_thread(8<<20);  // 8 million

  bench_add_random_multi_threads(8, 8<<20);

  // bench_add_random_by_one_skipset(8<<20);

  bench_add_random_by_multi_skipset(8, 8<<20);
}


int main() {
  bench_add();

  return 0;
}