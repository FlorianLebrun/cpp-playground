#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <set>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string.hpp>

struct SystemProgessBar {
   int vmin = 0;
   int vmax = 0;
   int value = 0;
   void Show() {

   }
   void SetRange(int vmin, int vmax) {
      this->vmin = vmin;
      this->vmax = vmax;
   }
   void SetValue(int x) {
      if (x < vmin) return this->SetValue(vmin);
      if (x > vmax) return this->SetValue(vmax);
      value = x;
      printf("%d\n", value);
   }
   void SetTitle(const char* title) {
      printf("%s\n", title);
   }
   void SetStatus(const char* msg) {
      printf("%s\n", msg);
   }
};

struct ConsoleLine {
   struct tContentStyle {
      int color;
      int position;
   };

   std::string content;
   std::vector<tContentStyle> styles;
   ConsoleLine(std::string content) : content(content) {
   }
   void print() {
      printf("line: %.*s\n", this->content.size(), this->content.c_str());
   }
};

struct ConsoleBlock {
   std::vector<ConsoleLine> lines;
   void writeLine(const char* buffer, size_t size) {
      ConsoleLine(std::string(buffer, size)).print();
   }

};

struct Console {
   std::set<ConsoleBlock*> blocks;

   void writeLine(const char* buffer, size_t size) {
      ConsoleLine(std::string(buffer, size)).print();
   }

   void write(const char* buffer, size_t size) {
      struct matcher { bool operator()(char Ch) const { return Ch == '\n'; } };
      boost::algorithm::split_iterator<const char*> parts(buffer, buffer + size, boost::token_finder(matcher()));
      while (!parts.eof()) {
         console.writeLine(parts->begin(), parts->size());
         parts++;
      }
   }

   void write(const char* buffer) {
      write(buffer, strlen(buffer));
   }
} console;


void main() {
   console.write("para A\n\npara B\npara C");
   auto& bar = *new SystemProgessBar();
   bar.SetTitle("Install");
   bar.SetRange(0, 100);
   bar.SetValue(10);
   bar.SetStatus("work on 'hello'");
   std::this_thread::sleep_for(std::chrono::seconds(1));
   bar.SetValue(15);
   bar.SetStatus("work on 'world'");
}

