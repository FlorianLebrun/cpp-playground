#include <exception>
#include <stdio.h>
#include <windows.h>


void func3() {
   (*(int*)0) = 0;
}

void func2() {
   try {
      func3();
   }
   catch (std::exception e) {
      printf("> catched by func2: %s\n", e.what());
   }
}

void func1() {
   try {
      func2();
   }
   catch (std::exception e) {
      printf("> catched by func1: %s\n", e.what());
   }
}

int HandleLowLevelToCXXException(void* ptr) {
   throw std::exception("crash");
}

void main() {
   __try {
      func1();
   }
   __except (HandleLowLevelToCXXException(_exception_info())) {

   }
}
