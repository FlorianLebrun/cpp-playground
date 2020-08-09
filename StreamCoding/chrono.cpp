
#include "chrono.h"
#include <windows.h>

Chrono::Chrono() {
  QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
}

void Chrono::Start() {
  QueryPerformanceCounter((LARGE_INTEGER*)&t0);
}

double Chrono::GetDiffDouble(PRECISION unit) {
  __int64 t1;
  QueryPerformanceCounter((LARGE_INTEGER*)&t1);
  t1-=t0;
  return (double)(t1*unit) / (double)freq;
}

float Chrono::GetDiffFloat(PRECISION unit) {
  __int64 t1;
  QueryPerformanceCounter((LARGE_INTEGER*)&t1);
  t1-=t0;
  return (float)((double)(t1*unit) / (double)freq);
}

float Chrono::GetOpsFloat(uint64_t ncycle, OPS unit) {
  return float(double(ncycle)/this->GetDiffDouble(S))/float(unit);
}

uint64_t Chrono::GetNumCycleClock() {
  __int64 t1;
  QueryPerformanceCounter((LARGE_INTEGER*)&t1);
  t1-=t0;
  return t1;
}

uint64_t Chrono::GetFreq() {
  return freq;
}

#define PERFTIME_ONE_CYCLE 0
void Chrono::PerfTest(const char* title, const std::function<void()>& cb) {
  Chrono c;
  int count = 0;
  c.Start();
#if PERFTIME_ONE_CYCLE
  cb();
  count = 1;
#else
  while (c.GetDiffFloat(Chrono::S) < 1.0f && count < 1000000) {
    for (int i = 0; i < 100; i++) {
      cb();
    }
    count += 100;
  }
#endif
  printf("%s : %.3g Mops\n", title, c.GetOpsFloat(count, Chrono::Mops));
}
