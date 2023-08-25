#include <thread>
#include <random>
#include <sstream>
#include <iostream>

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")

#include "SimpleStackWalker.h"

class Source {
   std::random_device rd;
   mutable std::mt19937 mt{ rd() };

public:
   unsigned int operator()() const {
      return mt();
   }
};

void printStack() {
   HANDLE hThread =
      OpenThread(THREAD_ALL_ACCESS, FALSE,
         GetCurrentThreadId());
   std::stringstream ss;
   std::thread thr{ [&]() {
     SimpleStackWalker eng{
         GetCurrentProcess()};
     eng.stackTrace(hThread, ss);
   } };
   thr.join();

   std::cout << ss.str() << '\n';
}

int CallLevel(int level) {
   if (level > 0) return CallLevel(level - 1);
   printStack();
   return level;
}

struct tRecTest {
   void* ptrs[64];
   static tRecTest make() {
      tRecTest e;
      e.ptrs[0] = (void*)printStack;
      e.ptrs[1] = (void*)printStack;
      e.ptrs[2] = (void*)CallLevel;
      e.ptrs[3] = (void*)CallLevel;
      return e;
   }
   static tRecTest remake(tRecTest x, int o) {
      tRecTest e;
      e.ptrs[0] = (void*)printStack;
      e.ptrs[1] = x.ptrs[3];
      e.ptrs[2] = (void*)CallLevel;
      e.ptrs[3] = (void*)CallLevel;
      return e;
   }
};

__declspec(noinline) void test() {
   tRecTest::remake(tRecTest::make(), CallLevel(100));
}

void main() {
   test();
}
