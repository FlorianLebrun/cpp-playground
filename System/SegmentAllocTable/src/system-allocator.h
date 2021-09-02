#pragma once
#include "./memory-space.h"
#include "./memory-context.h"
#include "./common/alignment.h"

namespace sat {
   namespace Slabbing {

      struct GlobalSlabbingAllocator;
      struct SlabbingSegment;
      struct SlabSpan;

      const size_t cSlabSizeL2 = 6;
      const size_t cSlabSize = uintptr_t(1) << cSlabSizeL2;
      const size_t cSlabsPerController = 17;
      const size_t cSlabsPerSegment = sat::cSegmentSize / 64;
      const size_t cSlabsPerSpanMax = cSlabsPerSegment - cSlabsPerController;

      typedef union Slab64 {
         Slab64* next;
         char bytes[cSlabSize];
      } *tpSlab64;
      static_assert(sizeof(Slab64) == cSlabSize, "Bad size");

      typedef struct SlabSpan {
         SlabSpan* next;
         SlabSpan** pprevNext;
         SlabbingSegment* segment;
         uint32_t index;
         uint32_t length;

         SlabSpan* pull() {
            if ((*this->pprevNext) = this->next) {
               this->next->pprevNext = this->pprevNext;
            }
#if _DEBUG
            this->pprevNext = 0;
            this->next = 0;
#endif
            return this;
         }
      } *tpSlabSpan;

      static_assert(sizeof(SlabSpan) <= cSlabSize, "Bad size");

      typedef struct SlabbingSegment : sat::MemoryDescriptor {
         uint16_t used = 0;
         uint8_t __reserve[32];
         uint8_t slabmap[1024] = { 0 };

         uint16_t getSizeAt(size_t index) {
            uint16_t size = this->slabmap[index];
            if (size != 0xff) return size;
            else return *(uint16_t*)&this->slabmap[index + 1];
         }
         void setSizeAt(size_t index, size_t size) {
            if (size < 0xff) {
               this->slabmap[index] = size;
            }
            else {
               this->slabmap[index] = 0xff;
               *(uint16_t*)&this->slabmap[index + 1] = size;
            }
         }
      } *tpSlabbingSegment;

      static_assert(sizeof(SlabbingSegment) == cSlabSize * cSlabsPerController, "Bad size");

      struct SlabSpanList {
         SlabSpan* first = 0;
         void push(SlabSpan* span) {
            if (span->next = this->first) {
               this->first->pprevNext = &span->next;
            }
            span->pprevNext = &this->first;
            this->first = span;
         }
         SlabSpan* pull() {
            return this->first ? this->first->pull() : 0;
         }
      };

      struct SlabSpanCollection {

         SlabSpanList sizings[64] = { 0 };
         uint64_t presences = 0;
         uint16_t offset = 0;

         SlabSpan* acquire(size_t length) {
            auto fittedSizeIdx = length - this->offset;
            auto sizingMask = uint64_t(-1) << fittedSizeIdx;
            while (auto sizingBitmap = this->presences & sizingMask) {
               auto sizeIdx = lsb_64(sizingBitmap);
               auto& sizing = this->sizings[sizeIdx];
               if (auto span = sizing.pull()) {
                  if (!sizing.first) this->presences &= ~(uint64_t(1) << sizeIdx);
                  return span;
               }
               else if (!sizing.first) { // clean invalid presence bit
                  this->presences &= ~(uint64_t(1) << sizeIdx);
               }
            }
            return 0;
         }
         void release(SlabSpan* span) {
            auto sizeIdx = span->length - this->offset;
            _ASSERT(sizeIdx >= 0 && sizeIdx < 64);
            this->sizings[sizeIdx].push(span);
            this->presences |= uint64_t(1) << sizeIdx;
         }
      };

      class GlobalSlabbingAllocator {
      public:
         std::mutex lock;
         MemorySpace* space;

         GlobalSlabbingAllocator() {
            for (int i = 0; i < cNumCollections; i++) {
               this->collections[i].offset = i * 64;
            }
         }
         size_t getMaxAllocatedSize() {
            return cSlabSize * cSlabsPerSpanMax;
         }
         size_t getMinAllocatedSize() {
            return cSlabSize;
         }
         size_t getAllocatedSize(size_t size) {
            return alignX(size, cSlabSize);
         }
         void* allocate(size_t size) {
            intptr_t length = alignX(size, cSlabSize) >> cSlabSizeL2;

            std::lock_guard<std::mutex> holder(this->lock);
            auto span = this->acquire(length);
            span->segment->setSizeAt(span->index, length);
            if (span->length > length) {
               SlabSpan* freespan = tpSlabSpan(&tpSlab64(span)[length]);
               freespan->segment = span->segment;
               freespan->index = span->index + length;
               freespan->length = span->length - length;
               this->release(freespan);
            }
            return span;
         }
         void free(address_t addr) {
            SlabbingSegment* segment = tpSlabbingSegment(uintptr_t(addr.segmentID) << sat::cSegmentSizeL2);
            size_t index = addr.position >> cSlabSizeL2;
            size_t length = segment->getSizeAt(index);
            _ASSERT(length > 0); // Not free

            auto span = tpSlabSpan(addr.ptr);
            span->segment = segment;
            span->index = index;
            span->length = length;

            std::lock_guard<std::mutex> holder(this->lock);
            return this->release(span);
         }

      private:
         static const auto cNumCollections = 16;
         SlabSpanCollection collections[cNumCollections];

         SlabSpan* acquire(intptr_t length) {
            tpSlabSpan span;
            if (length > cSlabsPerSpanMax) return 0;

            auto collectionIdx = length >> 6;
            if (collectionIdx < cNumCollections) {
               for (int i = collectionIdx; i < cNumCollections; i++) {
                  span = this->collections[i].acquire(length);
                  if (span) return span;
               }
            }

            auto segmentIndex = space->acquireSegmentSpan(1);
            auto segment = tpSlabbingSegment(segmentIndex << sat::cSegmentSizeL2);
            segment->SlabbingSegment::SlabbingSegment();
            span = tpSlabSpan(&tpSlab64(segment)[cSlabsPerController]);
            span->segment = segment;
            span->index = cSlabsPerController;
            span->length = cSlabsPerSpanMax;
            return span;
         }
         void release(SlabSpan* span) {
            auto collectionIdx = span->length >> 6;
            if (collectionIdx < cNumCollections) {
               this->collections[collectionIdx].release(span);
            }
            else throw "bad span length";
         }
      };
   }
}