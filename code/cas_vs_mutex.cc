#include <iostream>
#include <mutex>
#include <cassert>
#include <atomic>

namespace cas_vs_mutex {

std::mutex g_mutex;

int test_mutex(const int times) {
  std::lock_guard<std::mutex> lock(g_mutex);

  int sum = 0;
  for (int i = 0; i < times; ++i) {
    ++sum;
  }
  return sum;
}

int test_cas(const int times) {
  std::atomic<int> sum(0);
  for (int i = 0; i < times; ++i) {
    int expected = sum;
    while(sum.compare_exchange_weak(expected, expected+1, std::memory_order_relaxed));
  }
  return sum;
}

} // namespace of cas_vs_mutex


// try g++ -std=c++17 -O0 cas_vs_mutex.cc
// and g++ -std=c++17 -O2 cas_vs_mutex.cc
int main(int argc, char* argv[]) {
  using namespace cas_vs_mutex;

  constexpr int TIMES = 1 << 20;  // one million times

  auto start1 = std::chrono::steady_clock::now();
  test_mutex(TIMES);
  std::chrono::nanoseconds d1 = std::chrono::steady_clock::now() - start1;
  std::cout << "duration(ns) for test_mutex() = " << d1.count() << '\n';

  auto start2 = std::chrono::steady_clock::now();
  test_cas(TIMES);
  std::chrono::nanoseconds d2 = std::chrono::steady_clock::now() - start2;
  std:: cout << "duration(ns) for test_cas() = " << d2.count() << '\n';

  std::cout << "ratio of test_cas/test_mutex = " << d2/d1 << '\n';

  return 0;
}