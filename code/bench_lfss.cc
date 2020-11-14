#include <cassert>
#include <random>
#include <algorithm>
#include <thread>
#include <chrono>

#include "lock_free_skip_set.h"
#include "skipset.h"
#include "skipset.cc"
#include "vectskipset.h"
#include "vectskipset.cc"

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
  const auto end_time = std::chrono::steady_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
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
    threads.push_back(std::thread(add_random_one_thread_f, is, is+scope, std::ref(nums), std::ref(lfss)));
    is += scope;
  }
  for (auto& th : threads) {
    th.join();
  }
  const auto end_time = std::chrono::steady_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
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
  const auto end_time = std::chrono::steady_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
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
    threads.push_back(std::thread(one_skipset_for_one_thread_f, is, is+scope, std::ref(nums), std::ref(skipsets[i])));
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

std::vector<int> scan_starts(const int bound, const int start_num) {
  std::vector<int> starts;
  starts.reserve(start_num);
  for (int i = 0; i < start_num; ++i) {
    starts.push_back(std::rand() % bound);
  }
  return starts;
}

void bench_scan_skipset(const std::vector<int>& nums,
                        const std::vector<int>& starts, const int scan_scope) {
  sss::SkipSet<int> ss;
  for (const auto num : nums) {
    ss.insert(num);
  }

  int total = 0;
  const auto start_time = std::chrono::steady_clock::now();
  for (const auto start : starts) {
    auto it = ss.find(start);
    for (int i = 0; i < scan_scope; ++i) {
      if (it == ss.end()) break;

      int key = *it;
      ++total;

      ++it;      
    }
  }
  const auto end_time = std::chrono::steady_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time-start_time);
  std::cout << "bench_scan_skipset (us) = " << duration.count() << ", for total = " << total << '\n';
}

void bench_scan_vectskipset(const std::vector<int>& nums,
                        const std::vector<int>& starts, const int scan_scope) {
  sss::VectSkipSet<int> vss;
  for (const auto num : nums) {
    vss.insert(num);
  }

  int total = 0;
  const auto start_time = std::chrono::steady_clock::now();
  for (const auto start : starts) {
    auto it = vss.find_immutation(start);
    for (int i = 0; i < scan_scope; ++i) {
      if (it.end()) break;

      int key = *it;
      ++total;

      ++it;      
    }
  }
  const auto end_time = std::chrono::steady_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time-start_time);
  std::cout << "bench_scan_vectskipset (us) = " << duration.count() << ", for total = " << total << '\n';
}

void thread_lfss_scan(sss::LockFreeSkipSet<int>& lfss, int& thread_total,
                      const std::vector<int>& starts, const int scan_scope,
                      const int is, const int ie) {
  int total = 0;
  for (int i = is; i < ie; ++i) {
    const auto start = starts[i];
    auto it = lfss.locate(start);
    for (int j = 0; j < scan_scope; ++j) {
      if (it == lfss.end()) break;
      
      int key = *it;
      ++total;
      ++it;
    }
  }
  thread_total = total;
}

void bench_scan_lockfreeskipset(const std::vector<int>& nums,
                                const std::vector<int>& starts, const int scan_scope,
                                const int thread_num) {
  sss::LockFreeSkipSet<int> lfss;
  for (const auto num : nums) {
    lfss.add(num);
  }

  int index = 0;
  const int thread_scan_scope = starts.size() / thread_num;
  std::vector<std::thread> threads;
  std::vector<int> thread_totals(thread_num, 0);

  const auto start_time = std::chrono::steady_clock::now();
  for (int i = 0; i < thread_num; ++i) {
    threads.push_back(std::thread(std::ref(thread_lfss_scan), 
                                  std::ref(lfss), std::ref(thread_totals[i]), 
                                  std::ref(starts), scan_scope, index, index+thread_scan_scope));
    index += thread_scan_scope;
  }
  for (auto& th : threads) {
    th.join();
  }
  int total = 0;
  for (const auto thread_total : thread_totals) {
    total += thread_total;
  }
  const auto end_time = std::chrono::steady_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time-start_time);
  std::cout << "bench_scan_lockfreeskipset (us) = " << duration.count() << ", total = " << total << '\n';
}

void bench_scan_cmp() {
  constexpr int bound = 8<<20;
  constexpr int start_num = 1<<10;    // 1024
  constexpr int scan_scope = 1 << 16;   // 64K

  const auto nums = random_nums(bound);
  const auto starts = scan_starts(bound, start_num);

  bench_scan_skipset(nums, starts, scan_scope);
  bench_scan_vectskipset(nums, starts, scan_scope);
  bench_scan_lockfreeskipset(nums, starts, scan_scope, 8);
}

int main() {
  // bench_add();

  bench_scan_cmp();

  return 0;
}