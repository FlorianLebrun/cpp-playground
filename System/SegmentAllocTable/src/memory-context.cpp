#include "./memory-space.h"
#include "./memory-context.h"
#include "./blocks.h"

#include <functional>
#include <atomic>

using namespace sat;

SAT_DEBUG(static MemorySpace* g_space = 0);

MemoryContext::MemoryContext(MemorySpace* space, uint8_t id) : space(space), id(id) {
   SAT_DEBUG(g_space = space);
   for (int i = 0; i < cBlockBinCount; i++) {
      BlockClass* cls = cBlockBinTable[i];
      this->blocks_bins[i].block_size = cls->getBlockSize();
   }
}

address_t MemoryContext::BlockBin::pop() {
   while (auto page = this->pages) {

      // Compute available block in page
      uint64_t availables = page->usables ^ page->uses;
      if (availables == 0) {
         availables = page->shared_freemap.exchange(0);
         if (availables == 0) {
            this->pages = page->next;
            page->next = 0;
            page->context_id = 0;
            continue;
         }
         page->uses ^= availables;
      }

      // Find index and tag it
      SAT_ASSERT(availables != 0);
      auto index = lsb_64(availables);
      page->uses |= uint64_t(1) << index;

      // Compute block address
      uintptr_t block_index = uintptr_t(index);
      if (page->page_index) block_index += uintptr_t(page->page_index - 1) << (32 - page->block_ratio_shift);
      uintptr_t ptr = (block_index * this->block_size.packing) << this->block_size.shift;
      ptr += uintptr_t(page->segment_index) << sat::cSegmentSizeL2;

      SAT_DEBUG(BlockLocation loc(g_space, ptr));
      SAT_ASSERT(loc.descriptor == page);
      SAT_ASSERT(loc.index == index);
      return ptr;
   }
   return 0;
}

tpPageDescriptor MemoryContext::PageBin::pop(MemoryContext* context) {
   if (auto page = this->pages) {
      this->pages = page->next;
      return page;
   }
   if (auto batch = this->batches) {

      // Find index and tag it
      uint64_t availables = batch->usables ^ batch->uses;
      SAT_ASSERT(availables != 0);
      size_t index = lsb_64(availables);
      batch->uses |= uint64_t(1) << index;
      index++;


      // On batch is full
      if (batch->uses == batch->usables) {
         this->batches = batch->next;
         batch->next = this->full_batches;
         this->full_batches = batch;
      }

      // Format page
      auto page = tpPageDescriptor(&batch[index]);
      page->context_id = context->id;
      page->class_id = 0;
      page->block_ratio_shift = 0;
      page->page_index = index;
      page->segment_index = batch->segment_index;
      page->gc_marks = 0;
      page->uses = 0;
      page->usables = 0;
      page->shared_freemap = 0;
      page->next = 0;

      return page;
   }
   return 0;
}

void* MemoryContext::allocateSystemMemory(size_t length64) {
   return malloc(64 * length64);
}

void MemoryContext::releaseSystemMemory(void* base, size_t length64) {
   return free(base);
}

address_t MemoryContext::allocateBlock(size_t size) {
   size_target_t target(size);
   auto cls = sat::getBlockClass(target);
   SAT_ASSERT(size <= cls->getSizeMax());
   //printf("allocate %d for %d (lost = %lg%%)\n", cls->getSizeMax(), size, trunc(100*double(cls->getSizeMax() - size) / double(cls->getSizeMax())));
   auto addr = cls->allocate(size, this);
   SAT_DEBUG(memset(addr, 0xdd, size));
   return addr;
}

void MemoryContext::disposeBlock(address_t address) {
   if (address.regionID != 1) throw;
   sat::BlockLocation block(this->space, address);
   auto page = block.descriptor;

   uint64_t bit = uint64_t(1) << block.index;
   SAT_ASSERT(page->uses & bit);

   if (page->context_id == this->id) {
      page->uses &= ~bit;
      if (page->uses == 0) {
         auto cls = sat::cBlockClassTable[page->class_id];
         //cls->receiveEmptyPage(page, this);
         printf("empty page %.8X size:%d\n", address.ptr, cls->getSizeMax());
      }
   }
   else {
      auto prev_freemap = page->shared_freemap.fetch_or(bit);
      if (prev_freemap == 0 && page->context_id == 0) {
         _ASSERT(page->context_id == 0);
         auto cls = sat::cBlockClassTable[page->class_id];
         page->context_id = this->id;
         cls->receivePartialPage(page, this);
      }
      else {
         printf("free %d->%d\n", this->id, page->context_id);
         throw "todo";
      }
   }
}

