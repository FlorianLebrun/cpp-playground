#pragma once
#include <mutex>
#include <stdint.h>
#include "./bitmap64.h"

#if _DEBUG
#define USE_SAT_DEBUG 1
#else
#define USE_SAT_DEBUG 0
#endif

#if USE_SAT_DEBUG
#define SAT_DEBUG(x) x
#define SAT_ASSERT(x) _ASSERT(x)
#define SAT_INLINE _ASSERT(x)
#define SAT_PROFILE __declspec(noinline)
#else
#define SAT_DEBUG(x)
#define SAT_ASSERT(x)
#define SAT_PROFILE
#endif

namespace sat {

   struct MemorySpace;
   struct MemoryRegion32;
   struct MemoryUnit;

#pragma pack(push,1)
   union address_t {
      uint64_t ptr;
      struct {
         uint16_t position;
         uint16_t segmentID;
         uint32_t regionID;
      };
      struct {
         uint16_t position;
         uint32_t segmentIndex;
      };
      struct Unit {
         uint16_t position;
         uint16_t segmentID : 6;
         uint16_t unitID : 10;
         uint32_t regionID;
      } unit;
      address_t() : ptr(0) {}
      address_t(uint64_t ptr) : ptr(ptr) {}
      operator uint64_t() { return ptr; }
      operator void* () { return (void*)ptr; }
   };
   static_assert(sizeof(address_t) == 8, "bad size");
#pragma pack(pop)

   const size_t cSegmentSizeL2 = 16;
   const size_t cSegmentSize = size_t(1) << cSegmentSizeL2;
   const uintptr_t cSegmentMask = cSegmentSize - 1;

   const size_t cUnitSizeL2 = 22;
   const size_t cUnitSize = size_t(1) << cUnitSizeL2;
   const uintptr_t cUnitMask = cUnitSize - 1;

   const size_t cRegionSizeL2 = 32;
   const size_t cRegionSize = size_t(1) << cRegionSizeL2;
   const uintptr_t cRegionMask = cRegionSize - 1;

   const size_t cSegmentPerUnitL2 = 6;
   const size_t cSegmentPerUnit = size_t(1) << cSegmentPerUnitL2;

   const size_t cPackingCount = 4;

   struct SegmentEntry {
      uint64_t pagingID : 8;
      uint64_t reference : 56;
   };
   static_assert(sizeof(SegmentEntry) == 8, "bad size");

   struct size_target_t {
      uint16_t packing;
      uint16_t shift;
      size_target_t(size_t size) {
         size_t exp, base;
         if (size > 8) {
            exp = size_t(msb_32(size)) - 2;
            base = size >> exp;
         }
         else {
            exp = 0;
            if (!(base = size)) return;
         }
         if (size != (base << exp)) base++;
         while ((base & 1) == 0) { exp++; base >>= 1; }
         this->packing = base;
         this->shift = exp;
         _ASSERT(size > this->lower() && size <= this->value());
      }
      size_t value() {
         return size_t(this->packing) << this->shift;
      }
      size_t lower() {
         if (this->shift > 3) {
            if (this->packing > 1) return (this->packing - size_t(1)) << this->shift;
            else return size_t(7) << (this->shift - 3);
         }
         else return 0;
      }
   };

   struct MemoryUnit {
      MemoryUnit* linknext;

      int16_t position;
      int16_t width = 0; // span first entry is length-1, last is -width of first entry, otherwise 0

      uint8_t regionID;
      union {
         struct {

            // Memory usage
            uint8_t isUsed : 1;
            uint8_t isReserved : 1;
            uint8_t isForbidden : 1;
            uint8_t is_misclassed : 1;

            // Other infos
            uint8_t isFragmented : 1;
            uint8_t isFrontier : 1;
         };
         uint8_t options = 0;
      };

      int8_t fragments_spare_metric = 0;
      BinaryHierarchyBitmap64 fragments_bitmap;

      uint64_t commited = 0;

      uintptr_t address();

      void commitMemorySpan();

      void commitMemory();
      bool reserveMemory();
      void decommitMemory();
      void releaseMemory();

      void setWidth(uint16_t width) {
         this[0].width = width;
         this[width].width = -width;
      }
   };
   static_assert(sizeof(MemoryUnit) <= 64, "bad size");

   struct MemoryRegion32 {
      uint8_t regionID = 0;
      uintptr_t address = 0;

      MemoryUnit units_table[1024];
      SegmentEntry segments_table[65536];

      MemoryRegion32(uint8_t regionID, bool restrictSmallPtrZone = true);
      ~MemoryRegion32();

      // Unit aligned allocation
      MemoryUnit* acquireUnitSpan(size_t length);
      void releaseUnitSpan(MemoryUnit* unit, size_t length);
      bool analyzeUnitFreeSpan(size_t index);

      // Helpers
      void print();
      void check();
   };

   struct MemorySubunitManifold {

      static const auto cBinPerManifold = cSegmentPerUnitL2 + 1;
      static const auto cBinEmptySizeId = cSegmentPerUnitL2;

      struct SubunitBin {
         int32_t unseenUnitCount = 0;
         MemoryUnit* units = 0;
      };

      SubunitBin fullpages;
      SubunitBin bins[cBinPerManifold];

      uint8_t packing;
      size_t garbage_count = 0;
      std::mutex lock;

      address_t acquireSubunitSpan(MemorySpace* space, size_t lengthL2);
      void releaseSubunitSpan(MemorySpace* space, address_t adress, size_t lengthL2);
      void scavengeCaches(MemorySpace* space);

   };

   struct MemorySpace {

      MemoryRegion32* regions[256] = { 0 }; // 1Tb = 256 regions of 32bits address space (4Gb)
      MemorySubunitManifold subunits_manifolds[cPackingCount];

      MemorySpace();
      ~MemorySpace();

      // Region management
      MemoryRegion32* createRegion(size_t regionID);

      // Unit aligned allocation
      MemoryUnit* acquireUnitSpan(size_t length);
      MemoryUnit* tryAcquireUnitSpan(size_t length);

      // Segment aligned allocation
      address_t acquireSegmentSpan(size_t length);
      address_t acquireSegmentSpan(size_t packing, size_t shift);
      void releaseSegmentSpan(address_t base, size_t length);

      // Garbaging
      void scavengeCaches();

      // Helpers
      void print();
      void check();
   };
}
