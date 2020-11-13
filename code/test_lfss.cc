#include <cassert>
#include <random>
#include <algorithm>
#include <thread>
#include <chrono>

#include "lock_free_skip_set.h"

void simple_test() {
  std::vector<int> nums;
  for (int i = 1; i <= 25; ++i) {
    nums.push_back(i);
  }

  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(nums.begin(), nums.end(), g);

  sss::LockFreeSkipSet<int> lfss;
  for (auto num : nums)
    lfss.add(num);  
  lfss.debug_print();

  for (int i = 0; i < 11; ++i)
    lfss.remove(i);
  lfss.debug_print();

  // lfss.debug_print();
  // std::cout << "contains 10 is " << (lfss.contains(10) ? "true" : "false") << '\n';
}

void thread_insert_f(const std::vector<int>& nums, sss::LockFreeSkipSet<int>& lfss) {
  using namespace std::chrono_literals;

  for (int i = 0; i < 3; ++i) {
    for (int num : nums) {
      lfss.add(num);
    }
    // std::this_thread::sleep_for(1s);
  }
}


void test_thread_insert() {
  constexpr int total = 1 << 20;
  constexpr int thread_num = 8;

  std::random_device rd;
  std::mt19937 g(rd());

  std::vector<std::vector<int>> thread_nums(thread_num, std::vector<int>(total));
  for (int t = 0; t < thread_num; ++t) {
    for (int i = 0; i < total; ++i) {
      thread_nums[t][i] = i;
    }
    std::shuffle(thread_nums[t].begin(), thread_nums[t].end(), g);
  }

  std::vector<std::thread> threads(thread_num);
  sss::LockFreeSkipSet<int> lfss;

  for (int i = 0; i < thread_num; ++i) {
    threads[i] = std::thread(thread_insert_f, thread_nums[i], std::ref(lfss));
  }

  for (auto& th : threads) {
    th.join();
  }
  
  // check
  for (int i = 0; i < total; ++i) {
    if (!lfss.contains(i))
      std::cout << "nof found i = " << i << '\n';
  }

}

void one_action(const bool is_add, const int bound, sss::LockFreeSkipSet<int>& lfss) {
  int num = std::rand() % bound;
  if (is_add) {
    lfss.add(num);
  } else {
    lfss.remove(num);
  }
}

void test_mix_f(const std::chrono::seconds& run_time, const int bound, const bool is_add,
                sss::LockFreeSkipSet<int>& lfss) {
  bool finish = false;
  auto start_time = std::chrono::high_resolution_clock::now();

  while (!finish) {
    for (int i = 0; i < 1000; ++i) {
      one_action(is_add, bound, lfss);
    }
    auto check_time = std::chrono::high_resolution_clock::now();
    auto duration = check_time - start_time;

    if (duration > run_time) 
      finish = true;
    else
      std::this_thread::sleep_for(std::chrono::microseconds(1));
  }
}

void test_mix_add_remove() {
  sss::LockFreeSkipSet<int> lfss;

  constexpr int add_thread_num = 4;
  constexpr int rem_thread_num = 4;

  std::vector<std::thread> add_threads;
  std::vector<std::thread> rem_threads;

  std::chrono::seconds add_run_time(300);
  for (int i = 0; i < add_thread_num; ++i) {
    add_threads.push_back(std::thread(test_mix_f, add_run_time, 1<<15, true, std::ref(lfss)));
  }

  std::chrono::seconds rem_run_time(200);
  for (int i = 0; i < rem_thread_num; ++i) {
    rem_threads.push_back(std::thread(test_mix_f, rem_run_time, 1<<15, false, std::ref(lfss)));
  }

  for (auto& th : add_threads) {
    th.join();
  }
  for (auto& th : rem_threads) {
    th.join();
  }

  // check
  int count = 0;
  for (int i = 0; i < 1<<15; ++i) {
    if (lfss.contains(i)) ++count;
  }
  std::cout << "count = " << count << '\n';
}

void test_iterator() {
  sss::LockFreeSkipSet<int> lfss;
  for (int i = 0; i < 20; ++i) {
    lfss.add(i);
  }
  lfss.remove(10);

  lfss.debug_print();
  sss::LockFreeSkipSet<int>::Iterator it = lfss.locate(5);
  while (it != lfss.end()) {
    std::cout << *it << ", ";
    ++it;
  }
  std::cout << '\n';
}

int main() {
  // test_thread_insert();

  // test_mix_add_remove();

  test_iterator();

  return 0;
}