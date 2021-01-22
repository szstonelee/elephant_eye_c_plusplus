#include <iostream>

class NoVirtualBase1
{
public:
  char* get_name() 
  {
    return name_;
  }

private:
  char name_[32] = "NoVirtualBase1";
  char buf_[1<<20];  
};

class VirtualBase1
{
public:
  virtual ~VirtualBase1() 
  {
    std::cout << "execute ~VirtualBase1(), this = " << this << '\n';
  }

  virtual char* get_name()
  {
    return name_;
  }

private:
  char name_[32] = "VirtualBase1";
  char buf_[1<<20];  
};

class NoVirtualBase2
{
public:
  char* get_name() 
  {
    return name_;
  }

private:
  char name_[32] = "NoVirtualBase2";
  char buf_[1<<20];
};

class VirtualBase2
{
public:
  virtual ~VirtualBase2()
  {
    std::cout << "execute ~VirtualBase2(), this = " << this << '\n';
  }

  virtual char* get_name() 
  {
    return name_;
  }

private:
  char name_[32] = "VirtualBase2";
  char buf_[1<<20];
};

class NoVirtualDerived : public NoVirtualBase1, public NoVirtualBase2
{
public:
  char* get_name() 
  {
    return name_;  
  }

private:
  char name_[32] = "NoVirtualDerived";
  char buf_[1<<20];
};

class VirtualDerived : public VirtualBase1, public VirtualBase2
{
public:
  virtual ~VirtualDerived()
  {
    std::cout << "execute ~VirtualDerived(), this = " << this << '\n';
  }

  virtual char* get_name() 
  {
    return name_;  
  }

private:
  char name_[32] = "VirtualDerived";
  char buf_[1<<20];
};


void print_address_for_no_virutal_1(NoVirtualBase1* p)
{
  std::cout << "No virtual base 1 pointer address = " << p << '\n';
  std::cout << "get name as NoVirtualBase1 = " << p->get_name() << '\n';
};

void print_address_for_virutal_1(VirtualBase1* p)
{
  std::cout << "virtual base 1 pointer address = " << p << '\n';
  std::cout << "get name as VirtualBase1 = " << p->get_name() << '\n';
};

void release_from_pointer_no_virtual_1(NoVirtualBase1* p)
{
  std::cout << "Release no virtual base 1 adress = " << p << '\n';
  delete p;
}

void release_from_pointer_virtual_1(VirtualBase1* p)
{
  std::cout << "Release virtual base 1 adress = " << p << '\n';
  delete p;
}

void print_address_for_no_virtual_2(NoVirtualBase2* p)
{
  std::cout << "No virtual base 2 pointer address = " << p << '\n';
  std::cout << "get name as NoVirtualBase2 = " << p->get_name() << '\n';
}

void print_address_for_virtual_2(VirtualBase2* p)
{
  std::cout << "virtual base 2 pointer address = " << p << '\n';
  std::cout << "get name as VirtualBase2 = " << p->get_name() << '\n';
}

void release_from_pointer_no_virtual_2(NoVirtualBase2* p)
{
  std::cout << "Release no virtual base 2 adress = " << p << '\n';
  delete p;
}

void release_from_pointer_virtual_2(VirtualBase2* p)
{
  std::cout << "Release virtual base 2 adress = " << p << '\n';
  delete p;
}

void test_no_virtual()
{
  auto* father = new NoVirtualDerived();

  std::cout << "NoVirtualDerived address = " << father << '\n';
  std::cout << "get name from father = " << father->get_name() << '\n';

  print_address_for_no_virutal_1(father);
  print_address_for_no_virtual_2(father);

  // switch one of the release with g++ -fsanitize=leak to check memory leakage
  release_from_pointer_no_virtual_1(father);
  // release_from_pointer_no_virtual_2(father);
}

void test_virtual()
{
  auto* father = new VirtualDerived();

  std::cout << "VirtualDerived address = " << father << '\n';
  std::cout << "get name from father = " << father->get_name() << '\n';

  print_address_for_virutal_1(father);
  print_address_for_virtual_2(father);

  // switch one of the release with g++ -fsanitize=leak to check memory leakage
  release_from_pointer_virtual_1(father);
  // release_from_pointer_virtual_2(father);
}

int main()
{
  test_no_virtual();

  // test_virtual();

  return 0;
}