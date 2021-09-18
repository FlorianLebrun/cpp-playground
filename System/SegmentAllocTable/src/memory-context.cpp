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
      this->blocks_bins[i].slab_size = cls->getBlockSize();
   }
   for (int i = 0; i < cPageBinCount; i++) {
      PageClass* cls = cPageBinTable[i];
      this->pages_bins[i].slab_size = cls->getPageSize();
   }
}

void MemoryContext::BlockBin::getStats(MemoryStats::Bin& stats) {
   for (auto page = this->pages; page; page = page->next) {
      auto uses = page->uses ^ page->shared_freemap;
      stats.slab_cached_count += Bitmap64(page->usables ^ uses).count();
      if (uses == page->usables) stats.pages_full_count++;
      if (uses == 0) stats.pages_empty_count++;
      stats.pages_count++;
   }
}

void MemoryContext::BlockBin::scavenge(MemoryContext* context) {
   auto pprev = &this->pages;
   while (auto page = *pprev) {
      page->uses ^= page->shared_freemap.exchange(0);
      if (page->uses == 0) {
         *pprev = page->next;
         auto cls = cBlockClassTable[page->class_id];
         cls->getPageClass()->release(page, context);
      }
      else {
         pprev = &(*pprev)->next;
      }
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
      uintptr_t ptr = (block_index * this->slab_size.packing) << this->slab_size.shift;
      ptr += uintptr_t(page->segment_index) << sat::cSegmentSizeL2;

      SAT_DEBUG(BlockLocation loc(g_space, ptr));
      SAT_ASSERT(loc.descriptor == page);
      SAT_ASSERT(loc.index == index);
      return ptr;
   }
   return 0;
}

void MemoryContext::PageBin::getStats(MemoryStats::Bin& stats) {
   for (auto batch = this->batches; batch; batch = batch->next) {
      auto uses = batch->uses;
      stats.slab_cached_count += Bitmap64(batch->usables ^ uses).count();
      if (uses == batch->usables) stats.pages_full_count++;
      if (uses == 0) stats.pages_empty_count++;
      stats.pages_count++;
   }
}

tpPageDescriptor MemoryContext::PageBin::pop(MemoryContext* context) {
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

void MemoryContext::scavenge() {
   for (int i = 0; i < cBlockBinCount; i++) {
      auto& bin = this->blocks_bins[i];
      bin.scavenge(this);
   }
}

void MemoryContext::getStats() {
   int blockCacheSize = 0;
   for (int i = 0; i < cBlockBinCount; i++) {
      MemoryStats::Bin stats;
      auto cls = cBlockBinTable[i];
      auto& bin = this->blocks_bins[i];
      bin.getStats(stats);
      auto cache_size = stats.slab_cached_count * bin.slab_size.size();
      printf("block '%d': count=%d, empty_page=%d/%d\n", bin.slab_size.size(), stats.slab_cached_count, stats.pages_empty_count, stats.pages_count);
      blockCacheSize += cache_size;
   }

   int pageCacheSize = 0;
   int pageEmptySpanCount = 0;
   for (int i = 0; i < cPageBinCount; i++) {
      MemoryStats::Bin stats;
      auto cls = cPageBinTable[i];
      auto& bin = this->pages_bins[i];
      bin.getStats(stats);
      auto cache_size = stats.slab_cached_count * bin.slab_size.size();
      printf("page '%d': count=%d\n", i, stats.slab_cached_count);
      pageCacheSize += cache_size;
   }

   printf("> block cache=%d\n", blockCacheSize);
   printf("> page cache=%d\n", pageCacheSize);
}