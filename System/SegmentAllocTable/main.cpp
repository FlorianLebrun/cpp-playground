#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <math.h>


// free lists contain blocks
typedef struct Block {
   uint8_t next;
} *tpBlock;


typedef struct PageDescriptor {

   uint32_t page_address;

   uint32_t block_size; // 
   uint64_t block_marks;

   uint8_t local_free = 0;        // list of deferred free blocks by this thread (migrates to `free`)
   std::atomic_int8_t xthread_free = 0;  // list of deferred free blocks freed by other threads

} *tpPageDescriptor;

const auto cPageDescriptorSize = sizeof(PageDescriptor);

typedef struct SegmentDescriptor {

   uint32_t pages_uses;
   tpPageDescriptor pages;
};

struct MultiplierID {
};

struct Multiplier {
   enum t {
      // Mutiplier id
      MULID_1 = 0,
      MULID_3 = 1,
      MULID_5 = 2,
      MULID_7 = 3,
      Count,

      // Mutiplier values
      MUL_1 = 1,
      MUL_3 = 3,
      MUL_5 = 5,
      MUL_7 = 7,
   };
};

struct MemoryRegion32 {
   static const uintptr_t cPtrShift = 32;
   static const uintptr_t cPtrMask = 0xffffffff;

   static const uintptr_t cNumUnitDividers = 6;
   static const uintptr_t cMaxSpanSizeForFragmentedUnit = Multiplier::MUL_7 << cNumUnitDividers;

   struct MemoryUnit {
      uint16_t freenext : 10;
      uint16_t index = 0;
      uint16_t length = 0;


      // Fragmentation infos
      uint8_t multiplierID : 2; // Multiplier * 1,3,5,7
      uint8_t dividerID : 3; // Divider >> 0-6

      // Memory usage
      uint8_t isUsed : 1;
      uint8_t isReserved : 1;
      uint64_t commited = 0;
   };

   struct MemoryUnitFragments {
      uint16_t dividers[];// Available units per available minimal divider
   };

   uintptr_t address;

   MemoryUnit units_table[1024];
   uint16_t units_fragments[Multiplier::Count][cNumUnitDividers];

   SegmentDescriptor segments[65536];

   MemoryRegion32() {
      units_table[0].length = 1;
      units_table[1].length = 1023;
   }

   uint32_t acquireSegments(size_t length);
   void releaseSegments(uint32_t base, size_t length);

   uint32_t acquireLargeSegments(size_t length);
   void releaseLargeSegments(uint32_t base, size_t length);

   uint32_t acquireUnitSpan(size_t length);
   uint32_t acquireUnitFragment(size_t divider, size_t multiplier);

   void releaseUnitSpan(uint32_t index, size_t length);
   void releaseUnitFragment(uint32_t index, size_t divider, size_t multiplier);

   void print();
};

void MemoryRegion32::print() {
   for (size_t i = 0; i < 1024;) {
      auto& unit = units_table[i];
      printf("@%d [%d] %s\n", i, unit.length, unit.isUsed ? "used" : "free");
      if (unit.length > 0) i += unit.length;
      else break;
   }
}

uint32_t MemoryRegion32::acquireUnitSpan(size_t length) {

   // Search best fit span
   int bestIndex = -1;
   size_t bestLength = 1024;
   for (int i = 0; i < 1024;) {
      auto& unit = units_table[i];
      if (!unit.isUsed && unit.length > length) {
         if (unit.length < bestLength) {
            bestIndex = i;
            bestLength = unit.length;
         }
      }
      i += unit.length;
   }
   if (bestIndex < 0) {
      return 0;
   }

   // Fit & update units table
   auto& unit = units_table[bestIndex];
   if (bestLength > length) {
      units_table[bestIndex + length].length = bestLength - length;
      unit.length = length;
   }
   unit.isUsed = 1;
   return bestIndex;
}

void MemoryRegion32::releaseUnitSpan(uint32_t index, size_t length) {

}

uint32_t MemoryRegion32::acquireUnitFragment(size_t divider, size_t multiplier) {
   throw;
}

void MemoryRegion32::releaseUnitFragment(uint32_t index, size_t divider, size_t multiplier) {

}

uint32_t MemoryRegion32::acquireSegments(size_t length) {
   if (length <= cMaxSpanSizeForFragmentedUnit) {
      uint32_t divider = 0;
      uint32_t multiplier = 0;
      uint32_t unitIndex = this->acquireUnitFragment(divider, multiplier);
      return unitIndex << cNumUnitDividers;
   }
   else {
      uint32_t unitIndex = this->acquireUnitSpan(length >> cNumUnitDividers);
      return unitIndex << cNumUnitDividers;
   }
}

void MemoryRegion32::releaseSegments(uint32_t base, size_t length) {

}
union tMemPtr {
   uintptr_t address;
   struct {
      uint32_t region;
      uint16_t page;
      union {
         uint16_t offset;
         struct {
            uint16_t index : 5;
            uint16_t offset : 11;
         } subpage;
      };
   };
   tMemPtr(uintptr_t address)
      : address(address) {
   }
};

struct MemoryTable {

   MemoryRegion32* regions[256] = { 0 }; // 1Tb = 256 regions of 32bits address space (4Gb)

   uintptr_t acquireSegments(size_t length) {
      return regions[0]->acquireSegments(length);
   }
   void releaseSegments(uintptr_t base, size_t length) {
      regions[base >> MemoryRegion32::cPtrShift]->releaseSegments(base & MemoryRegion32::cPtrMask, length);
   }
};

MemoryTable table;

struct MemoryProvider {


};

int main() {
   MemoryRegion32* region = new MemoryRegion32();
   uint32_t si = region->acquireUnitSpan(14);
   region->releaseUnitSpan(si, 14);
   region->print();

   return 0;
}

