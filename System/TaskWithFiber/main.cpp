#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <intrin.h>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/context/fiber.hpp>


void print(const boost::system::error_code& /*e*/)
{
   std::cout << "Hello, world!" << std::endl;
}


struct Task {
   std::string name;
   boost::context::fiber c;
   Task(std::string name)
      : name(name) {
   }
   void execute() {
      while (1) {
         std::cout << "> do: " << this->name << std::endl;
         Sleep(500);
         wait();
      }
   }
   void wait() {
      c = std::move(c).resume();
   }
};

struct Thread {
   std::thread thread;
   std::vector<Task*> tasks;
   void run() {
      this->thread = std::thread(
         [&] {
            int i = 0;
            boost::context::fiber c;
            for (;;) {
               auto task = tasks[i];

               if (!task->c) {
                  c = [=](boost::context::fiber&& c) {
                     task->c.swap(c);
                     task->execute();
                     return std::move(task->c);
                     };

               }
               c = std::move(c).resume();

               std::cout << "! switch" << std::endl;
               i = (i + 1) % tasks.size();
            }
         }
      );
      this->thread.join();
   }
};

int main() {

   Task task1("Task A");
   Task task2("Task B");
   Thread t1;
   t1.tasks.push_back(&task1);
   t1.tasks.push_back(&task2);
   t1.run();
   return 0;
}