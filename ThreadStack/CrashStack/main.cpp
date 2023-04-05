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
  mutable std::mt19937 mt{rd()};

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
  std::thread thr{[&]() {
    SimpleStackWalker eng{
        GetCurrentProcess()};
    eng.stackTrace(hThread, ss);
  }};
  thr.join();

  std::cout << ss.str() << '\n';
}

void CallLevel(int level) {
   if (level > 0) return CallLevel(level - 1);
   printStack();
}

void main() {
   CallLevel(100);
}
