#include <chrono>
#include <iostream>
#include <cassert>

#include "askipset.h"

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

int main() {
  constexpr int total = 8<<20;

  sss::ASkipSet<int> ass;

  const auto nums = random_nums(total);

  for (const auto num : nums) {
    bool success = ass.insert(num);
    assert(success);
  }

  // random scan
  const auto start_time1 = std::chrono::steady_clock::now();
  auto it1 = ass.find(0);
  while (it1 != ass.end()) {
    const int key = *it1;
    ++it1;
  }
  const auto end_time1 = std::chrono::steady_clock::now();
  const auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end_time1-start_time1);
  std::cout << "scan before adjust, duration(us) = " << duration1.count() << '\n';

  // adjust
  ass.set_threashold(1, 0);
  const auto start_time2 = std::chrono::steady_clock::now();
  ass.erase(total-1);   // will trigger all construction
  const auto end_time2 = std::chrono::steady_clock::now();
  const auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end_time2-start_time2);
  std::cout << "adjust duration(us) = " << duration2.count() << '\n';

  // scan after adjust
  const auto start_time3 = std::chrono::steady_clock::now();
  auto it2 = ass.find(0);
  while (it2 != ass.end()) {
    const int key = *it2;
    ++it2;
  }
  const auto end_time3 = std::chrono::steady_clock::now();
  const auto duration3 = std::chrono::duration_cast<std::chrono::microseconds>(end_time3-start_time3);
  std::cout << "scan after adjust, duration(us) = " << duration3.count() << '\n';

  return 0;
}