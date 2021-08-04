#pragma once
#include "./memory-space.h"
#include "./memory-context.h"
#include "./blocks-classes.h"

namespace sat {

   /*****************************************************
   *
   *  PnS1 Class:
   *  - used for small block < 1024 bytes
   *  - use n page of 1 segment
   *  - pages have 64 blocks, excepted the last one which is truncated
   *
   ******************************************************/
   struct PagePnS1Class : PageClass {
      uint8_t pagingID;
      sizeid_t sizing;
      uint8_t page_per_batch;
      uint16_t page_last_size;
      PagePnS1Class(uint8_t id, uint8_t binID, uint8_t pagingID, uint8_t packing, uint8_t shift);
      virtual PageDescriptor* allocate(MemoryContext* context) override final;
   };
   struct BlockPnS1Class : BlockClass {
      sizeid_t sizing;
      uint8_t block_ratio_shift;
      uint64_t page_last_usables;
      PagePnS1Class* page_class;

      BlockPnS1Class(uint8_t id, uint8_t binID, uint8_t packing, uint8_t shift, PagePnS1Class* page_class);
      virtual address_t allocate(size_t target, MemoryContext* context) override final;
      virtual size_t getSizeMax() override final;
      virtual void print() override final;
   };

   /*****************************************************
   *
   *  PnSn Class:
   *  - used for medium block >= 1024 and < 114k bytes
   *  - use n page on n segment
   *
   ******************************************************/
   struct PagePnSnClass : PageClass {
      sizeid_t sizing;
      uint8_t page_per_batch;
      uint8_t pagingIDs[8];
      PagePnSnClass(uint8_t id, uint8_t binID, uint8_t packing, uint8_t shift, uint8_t(&& pagingIDs)[8]);
      virtual PageDescriptor* allocate(MemoryContext* context) override final;
   };
   struct BlockPnSnClass : BlockClass {
      sizeid_t sizing;
      uint8_t block_ratio_shift;
      uint64_t block_usables;
      PagePnSnClass* page_class;
      BlockPnSnClass(uint8_t id, uint8_t binID, uint8_t packing, uint8_t shift, uint8_t block_per_page_L2, PagePnSnClass* page_class);
      virtual address_t allocate(size_t target, MemoryContext* context) override final;
      virtual size_t getSizeMax() override { return sizing.size(); }
      virtual void print() override final;
   };

   /*****************************************************
   *
   *  P1Sn Class
   *  - used for medium block >= 1024 and < 229k bytes
   *  - use one page on n segment
   *
   ******************************************************/
   struct PageP1SnClass : PageClass {
      sizeid_t sizing;
      uint8_t pagingIDs[8];
      PageP1SnClass(uint8_t id, uint8_t packing, uint8_t shift, uint8_t(&& pagingIDs)[8]);
      virtual PageDescriptor* allocate(MemoryContext* context) override final;
   };
   struct BlockP1SnClass : BlockClass {
      sizeid_t sizing;
      uint8_t block_ratio_shift;
      uint64_t block_usables;
      PageClass* page_class;
      BlockP1SnClass(uint8_t id, uint8_t binID, uint8_t packing, uint8_t shift, uint8_t block_per_page_L2, PageP1SnClass* page_class);
      virtual address_t allocate(size_t target, MemoryContext* context) override final;
      virtual size_t getSizeMax() override { return sizing.size(); }
      virtual void print() override final;
   };

   /*****************************************************
   *
   *  Subunit-Span Class:
   *  - used for large block >= 64k and < 14M bytes
   *  - use one page on n subunit
   *
   ******************************************************/
   struct BlockSubunitSpanClass : BlockClass {
      uint8_t packing;
      uint16_t lengthL2;
      BlockSubunitSpanClass(uint8_t id, uint8_t packing, uint16_t lengthL2);
      virtual address_t allocate(size_t target, MemoryContext* context) override final;
      virtual size_t getSizeMax() override final;
      virtual void print() override final;
   };

   /*****************************************************
   *
   *  Unit-Span Class:
   *  - used for huge block >= 4Mb and < 4Gb bytes
   *  - use one page on n unit
   *
   ******************************************************/
   struct BlockUnitSpanClass : BlockClass {
      BlockUnitSpanClass(uint8_t id);
      virtual address_t allocate(size_t target, MemoryContext* context) override final;
      virtual size_t getSizeMax() { return 1 << 31; }
      virtual void print() override final;
   };

   void printAllBlocks();
}
