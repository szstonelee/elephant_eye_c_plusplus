#include <random>
#include <algorithm>
#include <cassert>

#include "vectskipset.h"
#include "vectskipset.cc"
#include "skipset.h"
#include "skipset.cc"

void test_shuffle() {
  std::srand(time(0));

  std::random_device rd;
  std::mt19937 g(rd());

  constexpr int num_elements = 2 << 20;
  std::vector<int> elements(num_elements);
  std::vector<int> to_deletes;
  to_deletes.reserve(num_elements/2);
  std::vector<int> to_searchs;
  to_searchs.reserve(num_elements/2);
  for (int j=0; j < num_elements; ++j) {
    elements[j] = j;
    if (rand()%2 == 0) {
      to_deletes.push_back(j);
    } else {
      to_searchs.push_back(j);
    }
  }
  std::shuffle(elements.begin(), elements.end(), g);

  sss::VectSkipSet<int> vss;
  for (const auto element : elements) {
    auto res = vss.insert(element);
    assert(res);
  }

  for (const auto item : to_deletes) {
    auto res = vss.erase(item);
    assert(res);
  }

  for (const auto item : to_searchs) {
    auto res = vss.contains(item);
    assert(res);
  }

  std::cout << "vss count = " << vss.count() << ", to_searchs size = " << to_searchs.size() << '\n';
}

static double sys_time() {
  return static_cast<double>(clock()) / CLOCKS_PER_SEC;
}

void bench_k_capacity() {
  std::srand(time(0));

  std::random_device rd;
  std::mt19937 g(rd());

  constexpr int num_elements = 2 << 20;
  std::vector<int> elements(num_elements);
  std::vector<int> to_deletes;
  to_deletes.reserve(num_elements/2);
  std::vector<int> to_searchs;
  to_searchs.reserve(num_elements/2);
  for (int j=0; j < num_elements; ++j) {
    elements[j] = j;
    if (rand()%2 == 0) {
      to_deletes.push_back(j);
    } else {
      to_searchs.push_back(j);
    }
  }
  std::shuffle(elements.begin(), elements.end(), g);

  sss::VectSkipSet<int> vss;
  auto start_time = sys_time();
  for (const auto element : elements) {
    auto res = vss.insert(element);
    assert(res);
  }
  for (const auto item : to_deletes) {
    auto res = vss.erase(item);
    assert(res);
  }
  for (const auto item : to_searchs) {
    auto res = vss.contains(item);
    assert(res);
  }
  std::cout << "-- insert/erase/search " << (sys_time() - start_time) << " secs\n";
}

// if O0, vectskipset latency is 2X of skipset
// if O2, vectksipset latency is half of skipset
void cmp_insert() {
  constexpr int num_elements = 2 << 20;
  std::vector<int> elements(num_elements);
  for (int i = 0; i < num_elements; ++i) {
    elements[i] = i;
  }

  std::srand(time(0));
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(elements.begin(), elements.end(), g);

  sss::SkipSet<int> ss;
  sss::VectSkipSet<int> vss;

  auto start_vss = sys_time();
  for (const auto ele : elements) {
    vss.insert(ele);
  }
  std::cout << "-- insert for vector skip set " << (sys_time() - start_vss) << " secs\n";

  auto start_ss = sys_time();
  for (const auto ele : elements) {
    ss.insert(ele);
  }
  std::cout << "-- insert for skip set " << (sys_time() - start_ss) << " secs\n";

}

void test_immuiter() {
  constexpr int num_elements = 2 << 20;
  std::vector<int> elements(num_elements);
  for (int i = 0; i < num_elements; ++i) {
    elements[i] = i;
  }

  std::srand(time(0));
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(elements.begin(), elements.end(), g);

  sss::VectSkipSet<int> vss;
  for (const auto ele : elements) {
    vss.insert(ele);
  }

  auto it = vss.find_immutation(1500000);
  int prev = -1;
  for (int i = 0; i < 1000 && !it.end(); ++i, ++it) {
    std::cout << *it << ", ";
    if (prev != -1 && *it - prev != 1) assert(false);
    prev = *it;
  }
  std::cout << '\n';
}

// compare skip set & vector skip set for random insert then range scan
void bench_scan_cmp() {
  constexpr int set_sz = 1 << 23;   // 8 Million
  std::vector<int> elements(set_sz);
  for (int i = 0; i < set_sz; ++i) {
    elements[i] = i;
  }

  std::srand(time(0));
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(elements.begin(), elements.end(), g);

  sss::SkipSet<int> ss;
  sss::VectSkipSet<int> vss;
  for (const auto ele : elements) {
    ss.insert(ele);
    vss.insert(ele);
  }

  constexpr int num_rand = 1000;    
  std::vector<int> scan_starts;
  scan_starts.reserve(num_rand);
  for (int i = 0; i < num_rand; ++i) {
    scan_starts.push_back(std::rand()%set_sz);
  }
  constexpr int scope = 1 << 16; // 64K scope

  auto start_ss = sys_time();
  int cnt_ss = 0;
  for (const auto start : scan_starts) {
    auto it = ss.find(start);
    for (int i = 0; i < scope; ++i) {
      if (it == ss.end()) break;

      ++it;
      ++cnt_ss;
    }
  }
  std::cout << "-- Scan in random memory for skip set, total " << cnt_ss << " in " << (sys_time() - start_ss) << " secs\n";

  auto start_vss = sys_time();
  int cnt_vss = 0;
  for (const auto start : scan_starts) {
    auto it = vss.find_immutation(start);
    for (int i = 0; i < scope; ++i) {
      if (it.end()) break;

      ++it;
      ++cnt_vss;
    }
  }
  std::cout << "-- Scan in random memory for vector skip set, total " << cnt_vss << " in " << (sys_time() - start_vss) << " secs\n";
}

int main() {
  // sss::VectSkipSet<int> vss;

  // please set kCapacity to be less like 2, 3 for testing
  // std::vector<int> keys = {4, 2, 19, 7, 14, 3, 8, 5, 6, 9, 10, 11, 1, 12};
  // vss.test_insert(keys);

  // please set kCapacity to be less like 2, 3 for testing
  // std::vector<int> insert_keys = {1, 2, 3, 4, 5, 6, 7};
  // std::vector<int> delete_keys = {4, 1, 5, 6};
  // vss.test_erase(insert_keys, delete_keys);

  // test_shuffle();

  // bench_k_capacity();

  // cmp_insert();

  // test_immuiter();

  bench_scan_cmp();

  return 0;
}

