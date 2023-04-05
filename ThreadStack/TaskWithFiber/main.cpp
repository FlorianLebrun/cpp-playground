#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <intrin.h>
#include <iostream>
#include <vector>
#include <thread>
#include <boost/context/fiber.hpp>
#include <Windows.h>


struct Task;
struct Thread;
typedef boost::context::fiber Fiber;

__declspec(thread) Thread* this_thread = 0;

struct Thread {
   std::thread thread;
   std::vector<Task*> tasks;
   Fiber main;

   Task* task = 0;
   Fiber* anchor = 0;

   void run();
   void yield();
   static Fiber task_proc(Fiber&&);
};

struct Task {
   std::string name;
   Fiber fiber;
   int count = 0;
   Task(std::string name)
      : name(name) {
   }
   void execute() {
      while (1) {
         std::cout << "> do: " << this->name << " " << this->count << std::endl;
         Sleep(500);
         this_thread->yield();
         this->count++;
      }
   }
};

void Thread::run() {
   this->thread = std::thread(
      [&] {
         int i = 0;
         boost::context::fiber c;
         this_thread = this;
         this->anchor = &this->main;
         for (;;) {
            _ASSERT(!this->task);
            this->task = tasks[i];

            if (!this->task->fiber) {
               std::cout << "!!! start " << this->task->name << std::endl;
               this->task->fiber = Fiber(Thread::task_proc);
            }
            else {
               std::cout << "!!! resume " << this->task->name << std::endl;
            }
            *this->anchor = std::move(this->task->fiber).resume();
            this->anchor = &this->main;

            std::cout << "! switch" << std::endl << std::endl;
            i = rand() % tasks.size();
         }
      }
   );
   this->thread.join();
}

Fiber Thread::task_proc(Fiber&& previous) {
   this_thread->anchor->swap(previous);
   this_thread->anchor = &this_thread->task->fiber;
   _ASSERT(!previous);

   this_thread->anchor = &this_thread->task->fiber;
   this_thread->task->execute();

   this_thread->task = 0;
   return std::move(this_thread->main);

}

void Thread::yield() {
   this->anchor = &this->task->fiber;
   this->task = 0;

   *this_thread->anchor = std::move(this->main).resume();
}

int main() {

   Task task1("Task A");
   Task task2("Task B");
   Thread t1;
   t1.tasks.push_back(&task1);
   t1.tasks.push_back(&task2);
   t1.run();
   return 0;
}