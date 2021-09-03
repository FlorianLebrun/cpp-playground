#include "./blocks.h"
#include "./blocks-classes.h"
#include "./common/alignment.h"

using namespace sat;

uint64_t get_usables_bits(size_t count) {
   if (count == 64) return -1;
   else return (uint64_t(1) << count) - 1;
}

void sat::printAllBlocks() {
   for (int i = 0; i < cBlockClassCount; i++) {
      cBlockClassTable[i]->print();
   }
}

PageClass::PageClass(uint8_t id) : id(id) {

}

BlockClass::BlockClass(uint8_t id) : id(id) {

}

/*****************************************************
*
*  PnS1 Class
*
******************************************************/

PagePnS1Class::PagePnS1Class(uint8_t id, uint8_t binID, uint8_t pagingID, uint8_t packing, uint8_t shift)
   : PageClass(id), sizing(packing, shift)
{
   size_t page_size = size_t(packing) << shift;
   this->binID = binID;
   this->pagingID = pagingID;
   this->page_per_batch = sat::cSegmentSize / page_size;
   if (this->page_last_size = sat::cSegmentSize % page_size) this->page_per_batch++;
   else this->page_last_size = page_size;
}

PageDescriptor* PagePnS1Class::allocate(MemoryContext* context) {
   auto& bin = context->pages_bins[this->binID];
   if (auto page = bin.pop(context)) {
      return page;
   }

   address_t area = context->space->acquireSegmentSpan(1, 0);
   auto batch = tpPageBatchDescriptor(context->allocateSystemMemory(this->page_per_batch + size_t(1)));
   memset(batch, 0, (this->page_per_batch + size_t(1)) * 64);
   batch->usables = get_usables_bits(this->page_per_batch);
   batch->uses = 0;
   batch->length = this->page_per_batch;
   batch->segment_index = area.segmentIndex;
   batch->gc_marks = 0;
   batch->class_id = this->id;
   batch->next = bin.batches;
   bin.batches = batch;

   auto& entry = context->space->regions[area.regionID]->segments_table[area.segmentID];
   entry.reference = uint64_t(&batch[1]);
   entry.pagingID = this->pagingID;

   return bin.pop(context);
}

BlockPnS1Class::BlockPnS1Class(uint8_t id, uint8_t binID, uint8_t packing, uint8_t shift, PagePnS1Class* page_class)
   : BlockClass(id), sizing(packing, shift), page_class(page_class) {
   this->binID = binID;
   this->block_ratio_shift = 26;

   size_t block_size = size_t(packing) << shift;
   size_t page_last_count = page_class->page_last_size / block_size;
   this->page_last_usables = get_usables_bits(page_last_count);
}

address_t BlockPnS1Class::allocate(size_t target, MemoryContext* context) {
   _ASSERT(target <= this->getSizeMax());
   auto& bin = context->blocks_bins[this->binID];
   if (address_t block = bin.pop()) {
      return block;
   }
   bin.pages = this->page_class->allocate(context);
   bin.pages->class_id = this->id;
   bin.pages->block_ratio_shift = this->block_ratio_shift;
   if (bin.pages->page_index != this->page_class->page_per_batch) bin.pages->usables = uint64_t(-1);
   else bin.pages->usables = this->page_last_usables;
   return bin.pop();
}

void BlockPnS1Class::receivePartialPage(tpPageDescriptor page, MemoryContext* context) {
   auto& bin = context->blocks_bins[this->binID];
   page->next = bin.pages;
   bin.pages = page;
}

void BlockPnS1Class::print() {
   int block_count = 1 << (32 - this->block_ratio_shift);
   int last_block_count = msb_64(this->page_last_usables) + 1;
   printf("block %d [pns1] page_count=%d-%d\n", this->getSizeMax(), block_count, last_block_count);
}

/*****************************************************
*
*  PnSn Class
*
******************************************************/

PagePnSnClass::PagePnSnClass(uint8_t id, uint8_t binID, uint8_t packing, uint8_t shift, uint8_t(&& pagingIDs)[8])
   : PageClass(id), sizing(packing, shift) {
   this->binID = binID;
   this->page_per_batch = 1 << (cSegmentSizeL2 - shift);
   memcpy(this->pagingIDs, &pagingIDs, 8);
}

PageDescriptor* PagePnSnClass::allocate(MemoryContext* context) {
   auto& bin = context->pages_bins[this->binID];
   if (auto page = bin.pop(context)) {
      return page;
   }

   address_t area = context->space->acquireSegmentSpan(this->sizing.packing, 0);
   auto batch = tpPageBatchDescriptor(context->allocateSystemMemory(this->page_per_batch + size_t(1)));
   memset(batch, 0, (this->page_per_batch + size_t(1)) * 64);
   batch->class_id = this->id;
   batch->usables = get_usables_bits(this->page_per_batch);
   batch->uses = 0;
   batch->length = this->page_per_batch;
   batch->segment_index = area.segmentIndex;
   batch->gc_marks = 0;
   batch->next = bin.batches;
   bin.batches = batch;

   auto* entries = &context->space->regions[area.regionID]->segments_table[area.segmentID];
   for (int i = 0; i < this->sizing.packing; i++) {
      auto& entry = entries[i];
      _ASSERT(entry.pagingID == 0 && entry.reference == 0);
      entry.reference = uint64_t(&batch[1]);
      entry.pagingID = this->pagingIDs[i];
      _ASSERT(entry.pagingID != 0);
   }

   return bin.pop(context);
}

BlockPnSnClass::BlockPnSnClass(uint8_t id, uint8_t binID, uint8_t packing, uint8_t shift, uint8_t block_per_page_L2, PagePnSnClass* page_class)
   : BlockClass(id), sizing(packing, shift), page_class(page_class) {
   this->binID = binID;
   this->block_ratio_shift = 32 - block_per_page_L2;
   this->block_usables = get_usables_bits(size_t(1) << block_per_page_L2);
}

address_t BlockPnSnClass::allocate(size_t target, MemoryContext* context) {
   _ASSERT(target <= this->getSizeMax());
   auto& bin = context->blocks_bins[this->binID];
   if (address_t block = bin.pop()) {
      return block;
   }
   bin.pages = this->page_class->allocate(context);
   bin.pages->class_id = this->id;
   bin.pages->block_ratio_shift = this->block_ratio_shift;
   bin.pages->usables = this->block_usables;
   return bin.pop();
}

void BlockPnSnClass::receivePartialPage(tpPageDescriptor page, MemoryContext* context) {
   auto& bin = context->blocks_bins[this->binID];
   page->next = bin.pages;
   bin.pages = page;
}

void BlockPnSnClass::print() {
   printf("block %d [pnsn]\n", this->getSizeMax());
}

/*****************************************************
*
*  P1Sn Class
*
******************************************************/

PageP1SnClass::PageP1SnClass(uint8_t id, uint8_t packing, uint8_t shift, uint8_t(&& pagingIDs)[8])
   : PageClass(id), sizing(packing, shift) {
   _ASSERT(shift == 16);
   memcpy(this->pagingIDs, &pagingIDs, 8);
}

PageDescriptor* PageP1SnClass::allocate(MemoryContext* context) {
   address_t area = context->space->acquireSegmentSpan(this->sizing.packing, 0);

   auto page = tpPageDescriptor(context->allocateSystemMemory(1));
   page->context_id = context->id;
   page->class_id = this->id;
   page->block_ratio_shift = 32;
   page->page_index = 0;
   page->segment_index = area.segmentIndex;
   page->gc_marks = 0;
   page->uses = 0;
   page->usables = 0;
   page->shared_freemap = 0;
   page->next = 0;

   auto* entries = &context->space->regions[area.regionID]->segments_table[area.segmentID];
   for (size_t i = 0; i < this->sizing.packing; i++) {
      auto& entry = entries[i];
      _ASSERT(entry.pagingID == 0 && entry.reference == 0);
      entry.reference = uint64_t(page);
      entry.pagingID = this->pagingIDs[i];
   }

   return page;
}

BlockP1SnClass::BlockP1SnClass(uint8_t id, uint8_t binID, uint8_t packing, uint8_t shift, uint8_t block_per_page_L2, PageP1SnClass* page_class)
   : BlockClass(id), sizing(packing, shift) {
   this->binID = binID;
   this->block_ratio_shift = 32 - block_per_page_L2;
   this->block_usables = get_usables_bits(size_t(1) << block_per_page_L2);
   this->page_class = page_class;
}

address_t BlockP1SnClass::allocate(size_t target, MemoryContext* context) {
   _ASSERT(target <= this->getSizeMax());
   auto& bin = context->blocks_bins[this->binID];
   if (address_t block = bin.pop()) {
      return block;
   }
   bin.pages = this->page_class->allocate(context);
   bin.pages->block_ratio_shift = this->block_ratio_shift;
   bin.pages->usables = this->block_usables;
   return bin.pop();
}

void BlockP1SnClass::receivePartialPage(tpPageDescriptor page, MemoryContext* context) {
   auto& bin = context->blocks_bins[this->binID];
   page->next = bin.pages;
   bin.pages = page;
}

void BlockP1SnClass::print() {
   printf("block %d [p1sn]\n", this->getSizeMax());
}

/*****************************************************
*
*  Subunit-Span Class
*
******************************************************/

BlockSubunitSpanClass::BlockSubunitSpanClass(uint8_t id, uint8_t packing, uint16_t lengthL2)
   : BlockClass(id), packing(packing), lengthL2(lengthL2) {
}

address_t BlockSubunitSpanClass::allocate(size_t target, MemoryContext* context) {
   _ASSERT(target <= this->getSizeMax());
   address_t area = context->space->acquireSegmentSpan(this->packing, this->lengthL2);

   // Create a descriptor for this block
   auto page = (PageDescriptor*)context->allocateSystemMemory(1);
   page->context_id = context->id;
   page->class_id = this->id;
   page->uses = 1;
   page->usables = 1;
   page->page_index = 0;
   page->segment_index = area.segmentIndex;

   // Mark block segments in table
   auto* entries = &context->space->regions[area.regionID]->segments_table[area.segmentID];
   auto entries_count = size_t(this->packing) << this->lengthL2;
   for (uint32_t i = 0; i < entries_count; i++) {
      auto& entry = entries[i];
      _ASSERT(entry.pagingID == 0 && entry.reference == 0);
      entry.pagingID = 0;
      entry.reference = uintptr_t(page);
   }
   return area;
}

void BlockSubunitSpanClass::receivePartialPage(tpPageDescriptor page, MemoryContext* context) {
   address_t area = 0;
   area.segmentIndex = page->segment_index;

   // Clean segments in table
   uint32_t entries_count = 0;
   auto* entries = &context->space->regions[area.regionID]->segments_table[area.segmentID];
   while (entries[entries_count].reference == uintptr_t(page)) {
      entries[entries_count].pagingID = 0;
      entries[entries_count].reference = 0;
      entries_count++;
   }

   // Release memory
   context->space->releaseSegmentSpan(page->segment_index, entries_count);
   context->releaseSystemMemory(page, 1);
}

size_t BlockSubunitSpanClass::getSizeMax() {
   return size_t(this->packing) << this->lengthL2;
}

void BlockSubunitSpanClass::print() {
   printf("block %d [sunit]\n", this->getSizeMax());
}

/*****************************************************
*
*  Unit-Span Class
*
******************************************************/

BlockUnitSpanClass::BlockUnitSpanClass(uint8_t id)
   : BlockClass(id) {
}

address_t BlockUnitSpanClass::allocate(size_t target, MemoryContext* context) {
   auto unit_count = alignX(target, sat::cUnitSize) >> sat::cUnitSizeL2;
   auto unit = context->space->acquireUnitSpan(unit_count);
   address_t area = unit->address();

   // Create a descriptor for this block
   auto page = (PageDescriptor*)context->allocateSystemMemory(1);
   page->context_id = context->id;
   page->uses = 1;
   page->usables = 1;
   page->page_index = 0;
   page->segment_index = area.segmentIndex;

   // Mark segments in table
   auto* entries = &context->space->regions[area.regionID]->segments_table[area.segmentID];
   auto entries_count = unit_count << cSegmentPerUnitL2;
   for (uint32_t i = 0; i < entries_count; i++) {
      auto& entry = entries[i];
      _ASSERT(entry.pagingID == 0 && entry.reference == 0);
      entry.pagingID = 0;
      entry.reference = uintptr_t(page);
   }
   return area;
}

void BlockUnitSpanClass::receivePartialPage(tpPageDescriptor page, MemoryContext* context) {
   address_t area = 0;
   area.segmentIndex = page->segment_index;

   // Clean segments in table
   uint32_t entries_count = 0;
   auto* entries = &context->space->regions[area.regionID]->segments_table[area.segmentID];
   while (entries[entries_count].reference == uintptr_t(page)) {
      entries[entries_count].pagingID = 0;
      entries[entries_count].reference = 0;
      entries_count++;
   }

   // Release memory
   context->space->releaseSegmentSpan(page->segment_index, entries_count);
   context->releaseSystemMemory(page, 1);
}

void BlockUnitSpanClass::print() {
   printf("block 4Gb [unit]\n", this->getSizeMax());
}
