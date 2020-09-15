#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <intrin.h>
#include "./chrono.h"

#include <windows.h>
#include <psapi.h>
#include <Dbghelp.h>
#pragma comment(lib,"Dbghelp.lib")

extern"C" void capture_current_context_registers(CONTEXT & context);

struct tSymbol {
  uint64_t address; // virtual address including dll base address
  uint32_t offset;  // offset related to resolved address
  uint32_t size;    // estimated size of symbol, can be zero
  uint32_t flags;   // info about the symbols, see the SYMF defines
  uint32_t nameLen;
  char* name;     // symbol name (null terminated string)

  tSymbol(char* name, uint32_t nameLen) {
    this->nameLen = nameLen;
    this->name = name;
  }
  bool resolve(uintptr_t address)
  {
    static bool isSymInitiate = false;
    if (!isSymInitiate) {
      if (!SymInitialize(GetCurrentProcess(), NULL, TRUE)) {
        printf("Issue with SymInitialize ! %s\n", ::GetLastError());
      }
      isSymInitiate = true;
    }

    IMAGEHLP_SYMBOL64* Sym = (IMAGEHLP_SYMBOL64*)alloca(sizeof(IMAGEHLP_SYMBOL64) + this->nameLen);
    uint64_t offset;
    Sym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
    Sym->MaxNameLength = this->nameLen;
    if (SymGetSymFromAddr64(GetCurrentProcess(), address, &offset, Sym)) {
      Sym->Name[this->nameLen - 1] = 0;
      this->address = Sym->Address;
      this->size = Sym->Size;
      this->offset = offset;
      this->flags = Sym->Flags;
      strcpy_s(this->name, this->nameLen, Sym->Name);
      return true;
    }
    else if (address) {
      sprintf_s(this->name, this->nameLen, "(unresolved 0x%.16X : code %d)", address, GetLastError());
      return false;
    }
    else {
      this->name[0] = 0;
      return false;
    }
  }

};

template <int cNameLen>
struct tSymbolEx : tSymbol {
  char __nameBytes[cNameLen + 1];
  tSymbolEx() : tSymbol(__nameBytes, cNameLen) { }
};

/**************************************************
** Stack tracing
**************************************************/

struct IStackMarker {
  virtual void getSymbol(char* buffer, int size) = 0;
};

struct StackMarker : IStackMarker {
  uint64_t word64[3];
  template<typename MarkerT>
  MarkerT* as() {
    _ASSERT(sizeof(typename MarkerT) <= sizeof(StackMarker));
    this->word64[0] = 0;
    this->word64[1] = 0;
    this->word64[2] = 0;
    return new(this) typename MarkerT();
  }
  intptr_t compare(StackMarker& other) {
    intptr_t c;
    if (c = other.word64[0] - this->word64[0]) return c;
    if (c = other.word64[-1] - this->word64[-1]) return c;
    if (c = other.word64[1] - this->word64[1]) return c;
    if (c = other.word64[2] - this->word64[2]) return c;
    return 0;
  }
  void copy(StackMarker& other) {
    this->word64[-1] = other.word64[-1];
    this->word64[0] = other.word64[0];
    this->word64[1] = other.word64[1];
    this->word64[2] = other.word64[2];
  }
  virtual void getSymbol(char* buffer, int size) override {
    buffer[0] = 0;
  }
};
static_assert(sizeof(StackMarker) == 4 * sizeof(uint64_t), "StackMarker size invalid");

struct StackBeacon {
  StackBeacon* parentBeacon;
  virtual void createrMarker(StackMarker& buffer) = 0;
};

template<class StackBeaconT>
struct StackBeaconHolder {
  bool isDeployed;
  StackBeaconT beacon;
  StackBeaconHolder() {
    this->isDeployed = false;
  }
  ~StackBeaconHolder() {
    this->remove();
  }
  void deploy() {
    if (!this->isDeployed) {
      sat_begin_stack_beacon(&this->beacon);
      this->isDeployed = true;
    }
  }
  void remove() {
    if (this->isDeployed) {
      sat_end_stack_beacon(&this->beacon);
      this->isDeployed = false;
    }
  }
};

struct tTimes {
  double read_register_time;
  double read_stack_time;
  double total_time;
  int64_t count;

  tTimes() {
    memset(this, 0, sizeof(*this));
  }
  void print() {
    printf("------------- Times (us) ----------\n");
    printf("read_register_time: %lg\n", read_register_time / double(count));
    printf("read_stack_time: %lg\n", read_stack_time / double(count));
    printf("-----------------------------------\n");
    printf("total_time: %lg\n", total_time / double(count));
    printf("-----------------------------------\n");
  }
} capture_times;

struct StackFrame {
  StackFrame* next;
  uintptr_t BaseAddress;
  uintptr_t InsAddress;
  uintptr_t StackAddress;

  StackFrame* enclose() {
    StackFrame* overflow = this->next;
    this->StackAddress = 0;
    this->BaseAddress = 0;
    this->InsAddress = 0;
    this->next = 0;
    return overflow;
  }
  void print() {
    tSymbolEx<512> symbol;
    int frameCount = 0;
    for (StackFrame* frame = this; frame; frame = frame->next) {
      symbol.resolve(frame->BaseAddress);
      if (!frameCount) printf("\n-------------------------------\n");
      printf("> %s\n", symbol.name);
      frameCount++;
    }
    printf(">>> %d frames <<<\n", frameCount);
  }
};

struct ThreadStackTracker {
  static const int reserve_batchSize = 512;

  StackFrame* lastStack;

  ThreadStackTracker() {
    this->_reserve = 0;
    this->lastStack = this->allocFrame();
    this->lastStack->enclose();
  }
  StackFrame* updateFrames(CONTEXT& context) {

    // Fix stack register for some terminal function
    if (context.Rip == 0 && context.Rsp != 0) {
      context.Rip = (ULONG64)(*(PULONG64)context.Rsp);
      context.Rsp += 8;
    }

    // Unroll frames
    StackFrame* stack = this->allocFrameBatch();
    StackFrame* prevStack = this->lastStack;
    StackFrame* current = stack;
    while (context.Rip) {// Check when end of stack reached

      // Reconcil stack position between current & prev
      current->StackAddress = context.Rsp;
      while (current->StackAddress < current->StackAddress) {
        prevStack = prevStack->next;
      }

      // Resolve current frame
      DWORD64 ImageBase;
      PRUNTIME_FUNCTION pFunctionEntry = ::RtlLookupFunctionEntry(context.Rip, &ImageBase, NULL);
      if (pFunctionEntry) {
        PVOID HandlerData;
        DWORD64 EstablisherFrame;
        current->InsAddress = context.Rip;
        current->BaseAddress = ImageBase + pFunctionEntry->BeginAddress;
        ::RtlVirtualUnwind(UNW_FLAG_NHANDLER,
          ImageBase,
          current->InsAddress,
          pFunctionEntry,
          &context,
          &HandlerData,
          &EstablisherFrame,
          NULL);
      }
      else {
        current->InsAddress = context.Rip;
        current->BaseAddress = context.Rip;
        context.Rip = (ULONG64)(*(PULONG64)context.Rsp);
        context.Rsp += 8;
      }

      if (current->StackAddress == prevStack->StackAddress) {
        if (current->BaseAddress == prevStack->BaseAddress && current->InsAddress == prevStack->InsAddress) {

          // Switch tail of current and prev stack
          StackFrame* tmp = current->next;
          current->next = prevStack->next;
          prevStack->next = tmp;

          // Release tail of current & prevStack
          this->_reserve = this->lastStack;
          this->lastStack = stack;
          return stack;
        }
      }

      if (current->next) current = current->next;
      else current = current->next = this->allocFrameBatch();
    }
    this->_reserve = current->enclose();
    this->lastStack = stack;
    return stack;
  }
private:
   StackFrame* _reserve;
   StackFrame* allocFrame() {
      StackFrame* frame = this->allocFrameBatch();
      this->_reserve = frame->next;
      return frame;
   }
   StackFrame* allocFrameBatch() {
      StackFrame* frames = this->_reserve;
      if (!frames) {
         frames = new StackFrame[reserve_batchSize];
         for (int i = 0; i < reserve_batchSize; i++) {
            frames[i].next = &frames[i + 1];
         }
         frames[reserve_batchSize - 1].next = this->_reserve;
      }
      this->_reserve = 0;
      return frames;
   }
};

ThreadStackTracker captureCache;

__declspec(noinline) void stackcapture() {
  Chrono c;


  // Get thread cpu context
  c.Start();
  CONTEXT context;
  //memset(&context, 0xab, sizeof(context));
  context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
  context.Rsp = (DWORD64)_AddressOfReturnAddress();
  context.Rip = 0;
  capture_times.read_register_time += c.GetDiffDouble(Chrono::US);

  // Unwinds stackframes and stack beacons
  c.Start();
  StackFrame* stack = captureCache.captureFrames(context);
  //stack->print();
  capture_times.read_stack_time += c.GetDiffDouble(Chrono::US);

  capture_times.total_time += c.GetDiffDouble(Chrono::US);
  capture_times.count++;
}

void calltest(int level) {
  if (level > 0) {
    calltest(level - 1);
  }
  else {
    stackcapture();
  }
}

void perfStackCapture() {
  for (int i = 0; i < 1000; i++) {
    calltest(25);
  }
  capture_times.print();
}

static uintptr_t result = 0;

void perfSamples() {
  Chrono c;
  int count = 1000;
  printf("------------- Samples Times (us) ----------\n");
  {
    c.Start();
    for (int i = 0; i < count; i++) {
      result = (uintptr_t)malloc(50);
    }
    double rtime = c.GetDiffDouble(Chrono::US) / double(count);
    printf("malloc_time: %lg\n", rtime);
  }
  {
    c.Start();
    for (int i = 0; i < count; i++) {
      result = result * i + 1;
    }
    double rtime = c.GetDiffDouble(Chrono::US) / double(count);
    printf("multadd_time: %lg\n", rtime);
  }
  {
     c.Start();
     for (int i = 0; i < count; i++) {
        result = result + result;
     }
     double rtime = c.GetDiffDouble(Chrono::US) / double(count);
     printf("add_time: %lg\n", rtime);
  }
}

void main() {
  perfSamples();
  perfStackCapture();
}
