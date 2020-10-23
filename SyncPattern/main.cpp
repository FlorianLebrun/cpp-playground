#include <mutex>
#include <Windows.h>
#include "spinlock.h"
#include "chrono.h"

struct critical_section {
   CRITICAL_SECTION CriticalSection;
   critical_section() {
      if (!InitializeCriticalSectionAndSpinCount(&CriticalSection, 0x00000400)) throw;
   }
   ~critical_section() {
      DeleteCriticalSection(&CriticalSection);
   }
   void lock() {
      EnterCriticalSection(&CriticalSection);
   }
   void unlock() {
      LeaveCriticalSection(&CriticalSection);
   }
};

class Task {
public:
   virtual void execute() = 0;
};

class ReadThreadIdTask : public Task {
public:
   int tid;
   virtual void execute() {
      tid = GetCurrentThreadId();
   }
};

static int x = 0;

class MutexTask : public Task {
public:
   std::mutex guard;
   virtual void execute() {
      guard.lock();
      x++;
      guard.unlock();
   }
};

class CriticalSectionTask : public Task {
public:
   critical_section guard;
   virtual void execute() {
      guard.lock();
      x++;
      guard.unlock();
   }
};

Task* Mutex_task = new MutexTask();
Task* ThreadId_task = new ReadThreadIdTask();
Task* CriticalSection_task = new CriticalSectionTask();

__declspec(noinline) void measureTask(const char* title, Task* task) {
   Chrono c;
   int count = 1000000;
   c.Start();
   for (int i = 0; i < count; i++) {
      task->execute();
   }
   printf("%s = %lg ns\n", title, c.GetDiffDouble(Chrono::NS) / double(count));
}

void main() {
   printf("sizeof(CRITICAL_SECTION) = %d\n", sizeof(CRITICAL_SECTION));
   measureTask("ThreadId", ThreadId_task);
   measureTask("CriticalSection", CriticalSection_task);
   measureTask("Mutex", Mutex_task);
}
