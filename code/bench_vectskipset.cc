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

void cmp_insert() {
  std::srand(time(0));

  std::random_device rd;
  std::mt19937 g(rd());

  constexpr int num_elements = 2 << 20;
  std::vector<int> elements(num_elements);

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

  cmp_insert();

  return 0;
}