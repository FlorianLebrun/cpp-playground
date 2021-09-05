#pragma once
#include "./memory-space.h"
#include "./common/bitwise.h"
#include "./blocks-classes.h"

namespace sat {

   struct MemoryContext;

   struct sizeid_t {
      uint8_t packing : 3; // in { 1, 3, 5, 7 }
      uint8_t shift : 5;
      sizeid_t(uint8_t packing = 0, uint8_t shift = 0)
         : packing(packing), shift(shift) {
      }
      size_t size() {
         return size_t(packing) << shift;
      }
   };

   struct SegmentPaging {
      uint32_t offset;
      uint32_t scale;
   };

#pragma pack(push,1)
   struct MemoryDescriptor {
      uint64_t uses; // Bitmap of allocated entries
      uint64_t usables; // Bitmap of allocable entries
      uint64_t gc_marks; // Bitmap of gc marked entries
      uint64_t gc_analyzis; // Bitmap of gc analyzed entries
      uint32_t segment_index; // Absolute segment index
      uint8_t class_id;
   };
#pragma pack(pop)

   // Segment: is a memory zone of one block or paved with page
#pragma pack(push,1)
   typedef struct PageBatchDescriptor : MemoryDescriptor {
      uint8_t length;
      PageBatchDescriptor* next;

      uint8_t __reserve[18];
   } *tpPageBatchDescriptor;
   static_assert(sizeof(PageBatchDescriptor) == 64, "bad size");
#pragma pack(pop)

   // Page: is a memory zone paved with same size blocks
#pragma pack(push,1)
   typedef struct PageDescriptor : MemoryDescriptor {
      uint8_t context_id;
      uint8_t page_index; // Page index in a desriptors array, 0 when page is alone
      uint8_t block_ratio_shift; // Page index in a desriptors array, 0 when page is alone

      uint8_t __reserve[8];
      std::atomic_uint64_t shared_freemap;
      PageDescriptor* next; // Chaining slot to link page in a page queue

   } *tpPageDescriptor;
   static_assert(sizeof(PageDescriptor) == 64, "bad size");
#pragma pack(pop)

   struct PageClass {
      uint8_t id = -1;
      uint8_t binID = -1;
      PageClass(uint8_t id);
      virtual PageDescriptor* allocate(MemoryContext* context) = 0;
   };

   struct BlockClass {
      uint8_t id = -1;
      BlockClass(uint8_t id);
      virtual address_t allocate(size_t target, MemoryContext* context) = 0;
      virtual void receivePartialPage(tpPageDescriptor page, MemoryContext* context) = 0;
      virtual size_t getSizeMax() = 0;
      virtual sizeid_t getBlockSize() { throw; }
      virtual sizeid_t getPageSize() { throw; }
      virtual void print() = 0;
   };

   struct BlockLocation {
      PageDescriptor* descriptor;
      uint32_t index;

      BlockLocation(PageDescriptor* descriptor = 0, uint32_t index = 0)
         :descriptor(descriptor), index(index) {
      }
      BlockLocation(MemorySpace* space, address_t address) {
         this->set(space, address);
      }
      SAT_PROFILE void set(MemorySpace* space, address_t address) {
         auto& entry = space->regions[address.regionID]->segments_table[address.segmentID];
         auto& paging = sat::cSegmentPagings[entry.pagingID];
         auto block_ratio = (uintptr_t(address.position) + paging.offset) * paging.scale;
         this->descriptor = &tpPageDescriptor(entry.reference)[block_ratio >> 32];
         this->index = uint32_t(block_ratio) >> this->descriptor->block_ratio_shift;
      }
      operator bool() {
         return !!this->descriptor;
      }
   };

   struct MemoryStats {
      struct Bin {
         size_t used_count = 0;
         size_t cached_count = 0;
         sizeid_t base_size = 0;

      };

      size_t used_count = 0;
      size_t cached_count = 0;
      size_t used_size = 0;
      size_t cached_size = 0;

   };

   struct MemoryContext {

      struct BlockBin {
         tpPageDescriptor pages = 0;
         sizeid_t block_size;
         SAT_PROFILE address_t pop();
         void getStats(MemoryStats::Bin& stats);
      };

      struct PageBin {
         tpPageDescriptor pages = 0;
         tpPageBatchDescriptor batches = 0;
         tpPageBatchDescriptor full_batches = 0;
         tpPageDescriptor pop(MemoryContext* context);
         void getStats(MemoryStats::Bin& stats);
      };

      uint8_t id;
      MemorySpace* space;
      BlockBin blocks_bins[cBlockBinCount];
      PageBin pages_bins[cPageBinCount];

      MemoryContext(MemorySpace* space, uint8_t id);
      address_t allocateBlock(size_t size);
      void disposeBlock(address_t ptr);

      // System block allocation with 64 bytes packing
      // note: length64 is a number of contigious 64 bytes chunks
      void* allocateSystemMemory(size_t length64);
      void releaseSystemMemory(void* base, size_t length64);

      void getStats();
   };

   // Controller for secondary thread which have no heavy use of this allocator
   struct SharedMemoryContext : MemoryContext {
      std::mutex lock;
      address_t allocate(size_t size) {
         std::lock_guard<std::mutex> guard(lock);
         return this->MemoryContext::allocateBlock(size);
      }
   };
}
