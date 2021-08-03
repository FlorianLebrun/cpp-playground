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


struct IMemorySegmentControler {
   
};

struct MemoryRegion32 {

   struct LargeSegment {
      
   };

   LargeSegment largeSegments[1024];
   IMemorySegmentControler* entries[1];

   void acquireSegments(size_t length);
   void releaseSegments(size_t length);
};

struct MemoryTable {

   MemoryRegion32* regions[256]; // 1Tb = 256 regions of 32bits address space (4Gb)
};

MemoryTable table;

void acquireSegments(size_t length);
void releaseSegments(size_t length) {

}

int main() {
   MemoryRegion32* region = new MemoryRegion32();
   region->acquireSegments(1);
   return 0;
}

