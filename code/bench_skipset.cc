#include <iostream>
#include <random>
#include <algorithm>
#include <vector>
#include <string>
#include <set>

#include "skipset.h"
#include "skipset.cc"

static double sys_time() {
  return static_cast<double>(clock()) / CLOCKS_PER_SEC;
}

template <class T>
bool contains(const std::set<T>& cont, const T& val) {
  return cont.find(val) != cont.end();
}

template <class T>
bool contains(const sss::SkipSet<T>& cont, const T& val) {
  return cont.contains(val);
}

template <class Set, class T>
void benchmark(int num, const std::vector<T>& elements, 
              const std::vector<T>& search_elements) {
  const double start_insert = sys_time();
  Set element_set;
  for (int j=0; j < num; ++j)
    element_set.insert(elements[j]);
  std::cout << "-- Inserted " << num << " elements in " << (sys_time() - start_insert) << " secs\n";

  const double start_search = sys_time();
  int num_found = 0;
  for (int j=0; j < num; ++j)
  {
    if (contains(element_set, search_elements[j]))
      ++num_found;
  }
  std::cout << "-- Found " << num_found << " elements in " << (sys_time() - start_search) << " secs\n";

  const double start_erase = sys_time();
  int num_erased = 0;
  for (int j=0; j < num; ++j)
  {
    if (element_set.erase(search_elements[j]))
      ++num_erased;
  }
  std::cout << "-- Erased " << num_found << " elements in " << (sys_time() - start_erase) << " secs\n";
}

// crud : create, replace, update, delete
void bench_random_crud() {
  std::random_device rd;
  std::mt19937 g(rd());

  const int num_elements = 200000;
  std::vector<int> elements(num_elements);
  for (int j=0; j < num_elements; ++j)
    elements[j] = j;
  std::shuffle(elements.begin(), elements.end(), g);

  std::vector<int> search_elements = elements;
  std::shuffle(search_elements.begin(), search_elements.end(), g);

  using Set1 = std::set<int>;
  using Set2 = sss::SkipSet<int>;

  for (int j=0; j < 2; ++j)
  {
    std::cout << "std::set\n";
    benchmark<Set1>(num_elements, elements, search_elements);
    std::cout << '\n';

    std::cout << "SkipSet\n";
    benchmark<Set2>(num_elements, elements, search_elements);
    std::cout << '\n';
  }
}

void scan_in_random(const int set_sz, const int scope,
                    const std::vector<int>& starts) {
  std::vector<int> elements(set_sz);
  for (int i = 0; i < set_sz; ++i)
    elements[i] = i;

  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(elements.begin(), elements.end(), g);

  sss::SkipSet<int> ss;
  for (const auto element : elements) 
    ss.insert(element);

  int num_total = 0;
  auto start_time = sys_time();
  for (const auto start : starts) {
    auto it = ss.find(start);
    for (int i = 0; i < scope; ++i) {
      if (it == ss.end()) break;

      ++it;
      ++num_total;
    }
  }
  std::cout << "-- Scan in random memory layout, total " << num_total << " in " << (sys_time() - start_time) << " secs\n";
}

void scan_in_contiguous(const int set_sz, const int scope,
                       const std::vector<int>& starts) {
  sss::SkipSet<int> ss;
  for (int i = 0; i < set_sz; ++i)
    ss.insert(i);

  int num_total = 0;
  const double start_time = sys_time();
  for (const auto start : starts) {
    auto it = ss.find(start);
    for (int i = 0; i < scope; ++i) {
      if (it == ss.end()) break;

      ++it;
      ++num_total;
    }
  }
  std::cout << "-- Scan in contigous memory layout, total " << num_total << " in " << (sys_time() - start_time) << " secs\n";
}

void bench_range_scan() {
  constexpr int set_sz = 1 << 23;   // 8 Million
  constexpr int num_rand = 1000;    
  std::vector<int> scan_starts;
  scan_starts.reserve(num_rand);
  for (int i = 0; i < num_rand; ++i) {
    scan_starts.push_back(std::rand()%set_sz);
  }

  constexpr int scope = 1 << 16;    // 64 K
  scan_in_random(set_sz, scope, scan_starts);
  scan_in_contiguous(set_sz, scope, scan_starts);
}

int main()
{
  // bench_random_crud();

  bench_range_scan();

  return 0;
}