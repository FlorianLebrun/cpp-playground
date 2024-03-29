#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <thread>
#include <chrono>
#include "DirectoryWatch.h"

using namespace std::chrono_literals;

void main() {
   std::string root = "D:/git/cpp-playground/FileWatch/test";
   if (1) {
      Listener listener;
      listener.Watch(root);
      std::this_thread::sleep_for(100s);
   }
   else {
      for (int i = 0; i < 100; i++) {
         Listener listener;
         listener.Watch(root);
         for (int i = 0; i < 4; i++) {
            if (i == 2) listener.Unwatch();
            std::this_thread::sleep_for(1000ms);
            std::ofstream ofs(root + "/test.txt", std::ofstream::out | std::ofstream::app);
            ofs.write("x", 1);
         }
      }
   }
}

