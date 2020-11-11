#include "atomic_flag_reference.h"
#include <iostream>

template<class T>
struct Node {
  T val;
  sss::FlagReference<Node> next;

  Node(T v) : val(v), next(nullptr, false) {}
};

int main() {
  Node<int>* root = new Node(100);

  Node<int>* another = new Node(555);

  sss::FlagReference<Node<int>> fr(root, true);

  std::cout << "before set_ref value  = " << fr.get_ref()->val << '\n';

  fr.set_ref(another);

  std::cout << "after set_ref value = " << fr.get_ref()->val << '\n';
  std::cout << "after set_ref flag = " << fr.get_flag() << '\n';

  delete root;
  delete another;

  return 0;
}