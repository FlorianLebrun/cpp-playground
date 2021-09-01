#include "./memory-space.h"
#include "./memory-context.h"
#include "./blocks.h"

#include <functional>
#include <atomic>

using namespace sat;

SAT_DEBUG(static MemorySpace* g_space = 0);

MemoryContext::MemoryContext(MemorySpace* space, uint8_t id) : space(space), id(id) {
   SAT_DEBUG(g_space = space);
}

address_t MemoryContext::BlockBin::pop() {
   if (auto page = this->pages) {

      // Find index and tag it
      uint64_t availables = page->usables ^ page->uses;
      auto index = lsb_64(availables);
      page->uses |= uint64_t(1) << index;

      // Compute block address
      uintptr_t ptr = (uintptr_t(index) * page->slabbing.packing) << page->slabbing.shift;
      ptr += uintptr_t(page->segment_index) << sat::cSegmentSizeL2;
      if (page->page_index) {
         ptr += (uintptr_t(page->page_index - 1) * page->size.packing) << page->size.shift;
      }

      // On page is full
      if (page->uses == page->usables) {
         this->pages = page->next;
         page->next = 0;
         page->context_id = 0;
      }
      printf("%d\n", page->class_id);
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
      size_t index = lsb_64(availables);
      batch->uses |= uint64_t(1) << index;
      index++;

      SAT_ASSERT(availables != 0);

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
      page->size = batch->slabbing;
      page->slabbing = sizeid_t();
      page->block_ratio_shift = 32;
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
         //printf("empty page\n");
      }
   }
   else {
      auto prev_freemap = page->shared_freemap.fetch_and(~bit);
      if (prev_freemap == 0 && page->context_id == 0) {
         _ASSERT(page->context_id == 0);
         auto cls = sat::cBlockClassTable[page->class_id];
         page->context_id = this->id;
         cls->receivePartialPage(page, this);
         //printf("unful page\n");
      }
      else {
         printf("free %d->%d\n", this->id, page->context_id);
         throw "todo";
      }
   }
}

