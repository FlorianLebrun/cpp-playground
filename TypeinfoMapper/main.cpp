#include <typeinfo>
#include <stdio.h>

struct A {
   static void* operator new(size_t s) {
      return malloc(s);
   }
   virtual int k() { return 0; }
};

struct B : A {
   virtual int k() override { return 1; }
};

struct C : B {
   virtual int k() override { return 2; }
};

template<class T>
void printid() {
   printf("printid %lld\n", typeid(T).hash_code());
}

void printx(A* x) {
   printf("printx %lld\n", typeid(*x).hash_code());
}

extern A* a = new A();
extern A* b = new B();
extern A* c = new C();

template<class T>
T* create() {
   T* x = new(T);
   printf("create %d\n", x->k());
   return x;
}

void main() {
   printid<A>();
   printid<B>();
   printid<C>();

   printx(a);
   printx(b);
   printx(c);
   new A();
   create<B>();
}
