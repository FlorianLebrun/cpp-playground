
#ifdef E_WAM
#include "../Utils.h"
#include "../../INCLUDES_BEGIN.h"
#endif

#include <string>
#include <vector>
#include <iostream>
#include <intrin.h>
#include <windows.h>
#include "./PersistentState.h"

#ifdef E_WAM
#include "../../INCLUDES_END.h"
#endif

namespace wFS {

   struct PersistentHeap;

   struct ObjectPreambule {
      uint32_t segmentIndex;
      uint32_t size;
      static ObjectPreambule* fromPtr(void* ptr) {
         return (ObjectPreambule*)(BytesPointer(ptr) - sizeof(ObjectPreambule));
      }
      static Persistent* toPtr(ObjectPreambule* object) {
         return (Persistent*)(BytesPointer(object) + sizeof(ObjectPreambule));
      }
   };

   struct PoolDescriptor {
      static const uint32_t c_objectSizeMaxL2 = 23;
      static const uint32_t c_objectSizeMax = 1 << c_objectSizeMaxL2;
      static const uint32_t c_objectSizeMinL2 = 5;
      static const uint32_t c_objectSizeMin = 1 << c_objectSizeMinL2;
      static const uint32_t c_objectSizeClassCount = c_objectSizeMaxL2 - c_objectSizeMinL2;

      ObjectPreambule* AllocObject(size_t size, PersistentHeap* heap);
      void FreeObject(ObjectPreambule* ptr, PersistentHeap* heap);
   private:
      struct tFreeObject : public Persistent {
         Ref<tFreeObject> next;
      };
      Ref<tFreeObject> freeObjects[c_objectSizeClassCount];
      uint16_t GetIndexFromSize(size_t size);
      size_t GetSizeFromIndex(uint16_t sizeIndex);
      ObjectPreambule* AllocInSegmentObject(uint16_t sizeIndex, PersistentHeap* heap);
      void FreeInSegmentObject(ObjectPreambule* ptr, uint16_t sizeIndex, PersistentHeap* heap);
   };

   struct SegmentDescriptor {
      uint32_t size;
      SegmentDescriptor() {
         this->size = 0;
      }
   };

   union HeapSignature {
      uint64_t _bits;
      struct {
         unsigned version : 16; // algorithm version
         unsigned alignement : 8; // structure alignement
         unsigned addressmode : 6; // address mode: 4: 32bits, 8: 64bits
         unsigned endian : 1; // 0: little endian, 1: big endian
      };
      HeapSignature() {
         struct tAlignTest { uint8_t x; uintptr_t y; };
         this->_bits = 0;
         this->version = 1;
         this->alignement = sizeof(tAlignTest) - sizeof(uintptr_t);
         this->addressmode = sizeof(void*);
         this->endian = 0;
      }
   };

   struct HeapDescriptor {
      HeapSignature signature;
      PoolDescriptor pool;
      Ref<Persistent> root;
      uint32_t segmentsCount;
      SegmentDescriptor segmentsTable[2048];
      HeapDescriptor() {
         this->segmentsCount = 1;
         memset(this->segmentsTable, 0, sizeof(this->segmentsTable));
      }
      bool checkValidity(size_t fileSize) {
         if (fileSize < sizeof(HeapDescriptor)) return false;
         if (this->signature._bits != HeapSignature()._bits) return false;
         return true;
      }
   };

   class SegmentMemory {
   public:
      std::string location;
      uint32_t segmentIndex;
      SegmentMemory(uint32_t segmentIndex) {
         this->segmentIndex = segmentIndex;
         this->hFile = 0;
         this->hFileMap = 0;
         this->ViewPtr = 0;
         this->ViewSize = 0;
      }
      SegmentMemory(std::string location, uint32_t segmentIndex) : SegmentMemory(segmentIndex) {
         this->location = location;
      }
      ~SegmentMemory() {
         this->Close();
      }
      void Create(size_t size) {
         if (!this->hFile) {
            this->hFile = CreateFileA(this->GetFilename().c_str(), GENERIC_READ | GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
         }
         if (!this->ViewPtr) {
            this->ViewSize = size;
            this->hFileMap = CreateFileMappingA(this->hFile, NULL, PAGE_READWRITE, 0, size, NULL);
            this->ViewPtr = MapViewOfFile(this->hFileMap, FILE_MAP_WRITE, 0, 0, size);
         }
      }
      bool Open() {
         if (!this->hFile) {
            auto hFile = CreateFileA(this->GetFilename().c_str(), GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE) return false;
            else this->hFile = hFile;
         }
         if (!this->ViewPtr) {
            this->ViewSize = GetFileSize(this->hFile, 0);
            this->hFileMap = CreateFileMappingA(this->hFile, NULL, PAGE_READWRITE, 0, this->ViewSize, NULL);
            this->ViewPtr = MapViewOfFile(this->hFileMap, FILE_MAP_WRITE, 0, 0, this->ViewSize);
         }
         return true;
      }
      void Remove() {
         this->Close();
         DeleteFileA(this->GetFilename().c_str());
      }
      void Resize(size_t size) {
         if (this->hFile) {
            if (this->ViewPtr) UnmapViewOfFile(this->ViewPtr);
            if (this->hFileMap) CloseHandle(this->hFileMap);
            this->ViewSize = size;
            SetFilePointer(this->hFile, this->ViewSize, 0, FILE_BEGIN);
            SetEndOfFile(this->hFile);
            this->hFileMap = CreateFileMappingA(this->hFile, NULL, PAGE_READWRITE, 0, this->ViewSize, NULL);
            this->ViewPtr = MapViewOfFile(this->hFileMap, FILE_MAP_WRITE, 0, 0, this->ViewSize);
         }
         else throw "shall be open before";
      }
      void Close() {
         if (this->ViewPtr) UnmapViewOfFile(this->ViewPtr);
         this->ViewPtr = 0;
         this->ViewSize = 0;
         if (this->hFileMap) CloseHandle(this->hFileMap);
         this->hFileMap = 0;
         if (this->hFile) CloseHandle(this->hFile);
         this->hFile = 0;
      }
      std::string GetFilename() {
         char tmp[16];
         if (!this->segmentIndex) return this->location + "/heap.mem";
         else return this->location + "/heap." + itoa(segmentIndex, tmp, 10) + ".mem";
      }
      uintptr_t GetBaseAddress() {
         if (!this->ViewPtr) this->Open();
         return (uintptr_t)this->ViewPtr;
      }
      size_t GetSize() {
         if (!this->ViewPtr) this->Open();
         return this->ViewSize;
      }
   protected:
      HANDLE hFile;
      HANDLE hFileMap;
      LPVOID ViewPtr;
      size_t ViewSize;
   };

   class HeapMemory : public SegmentMemory {
   public:
      HeapMemory() : SegmentMemory(0) {
         // MUST add location manually
      }
      HeapMemory(std::string filename)
         : SegmentMemory(filename, 0) {
      }
      HeapDescriptor* operator ->() {
         return (HeapDescriptor*)this->ViewPtr;
      }
      HeapDescriptor& operator *() {
         return *(HeapDescriptor*)this->ViewPtr;
      }
   };

   class PersistentHeap {
   public:
      HeapMemory heapMemory;
      std::vector<SegmentMemory*> segmentMemories;
      std::string location;

      PersistentHeap(const char* location);
      ~PersistentHeap();
      void Reset();

      void* AllocMemory(size_t size);
      void FreeMemory(void* ptr);

      SegmentMemory* AllocSegment(size_t size);
      void FreeSegment(uint32_t segmentIndex);
      SegmentMemory* MapSegment(uint32_t segmentIndex);
   };

   static PersistentHeap* persistent_heap = nullptr;

   Ref<Persistent> wFS::GetRootObject() {
      return persistent_heap->heapMemory->root;
   }

   void wFS::SetRootObject(Ref<Persistent> root) {
      persistent_heap->heapMemory->root = root;
   }

   void wFS::OpenHeap(const char* location) {
      if (!persistent_heap) {
         persistent_heap = new PersistentHeap(location);
      }
   }

   void wFS::ResetHeap() {
      if (persistent_heap) {
         persistent_heap->Reset();
      }
   }

   void wFS::CloseHeap() {
      if (persistent_heap) {
         delete persistent_heap;
         persistent_heap = nullptr;
      }
   }


   PersistentHeap::PersistentHeap(const char* location) : location(location) {
      this->heapMemory.location = this->location;

      // Open existing heap
      bool valid = false;
      if (this->heapMemory.Open()) {
         valid = this->heapMemory->checkValidity(this->heapMemory.GetSize());
         if (!valid) this->heapMemory.Close();
      }

      // Create new heap when no valid existing heap
      if (!valid) {
         this->heapMemory.Create(sizeof(HeapDescriptor));
         (*this->heapMemory).HeapDescriptor::HeapDescriptor();
      }

      // Initiate segment table
      this->segmentMemories.resize(this->heapMemory->segmentsCount);
   }

   PersistentHeap::~PersistentHeap() {
      for (auto segment : this->segmentMemories) {
         if (segment) delete segment;
      }
      this->segmentMemories.clear();
      this->heapMemory.Close();
   }

   void PersistentHeap::Reset() {

      // Clean current memory
      for (auto segment : this->segmentMemories) {
         if (segment) delete segment;
      }
      this->segmentMemories.clear();

      // Recreate heap
      this->heapMemory.Close();
      this->heapMemory.Create(sizeof(HeapDescriptor));
      (*this->heapMemory).HeapDescriptor::HeapDescriptor();

      // Initiate segment table
      this->segmentMemories.resize(this->heapMemory->segmentsCount);
   }

   void* PersistentHeap::AllocMemory(size_t size) {
      //printf("> Alloc %d\n", (int)size);
      auto desc = *this->heapMemory;
      return ObjectPreambule::toPtr(this->heapMemory->pool.AllocObject(size + sizeof(ObjectPreambule), this));
   }

   void PersistentHeap::FreeMemory(void* ptr) {
      //printf("> Free %d\n", ObjectPreambule::from(ptr)->size);
      return this->heapMemory->pool.FreeObject(ObjectPreambule::fromPtr(ptr), this);
   }

   SegmentMemory* PersistentHeap::AllocSegment(size_t size) {
      uint32_t segmentIndex = this->heapMemory->segmentsCount++;

      SegmentDescriptor& segmentDesc = this->heapMemory->segmentsTable[segmentIndex];
      segmentDesc.size = size;

      SegmentMemory* segment = new SegmentMemory(this->location, segmentIndex);
      segment->Create(size);
      this->segmentMemories.push_back(segment);

      _ASSERT(this->segmentMemories.size() == this->heapMemory->segmentsCount);
      return segment;

   }

   void PersistentHeap::FreeSegment(uint32_t segmentIndex) {
      if (auto segment = this->MapSegment(segmentIndex)) {
         SegmentDescriptor& segmentDesc = this->heapMemory->segmentsTable[segmentIndex];
         segmentDesc.size = 0;
         segment->Remove();
         delete segment;
         this->segmentMemories[segmentIndex] = 0;
      }
   }

   SegmentMemory* PersistentHeap::MapSegment(uint32_t segmentIndex) {
      auto segment = this->segmentMemories[segmentIndex];
      if (!segment) {
         segment = new SegmentMemory(this->location, segmentIndex);
         this->segmentMemories[segmentIndex] = segment;
      }
      return segment;
   }

   Persistent* BaseRef::get() const {
      if (this->_bits) {
         return (Persistent*)(persistent_heap->MapSegment(this->segment)->GetBaseAddress() + offset);
      }
      else return 0;
   }

   void BaseRef::set(Persistent* ptr) {
      if (ptr) {
         auto object = ObjectPreambule::fromPtr(ptr);
         this->segment = object->segmentIndex;
         this->offset = uintptr_t(ptr) - persistent_heap->MapSegment(this->segment)->GetBaseAddress();
      }
      else this->_bits = 0;
   }

   void BaseRef::replace(Persistent* ptr) {
      Persistent* prevPtr = this->get();
      if (ptr != prevPtr) {
         if (ptr) {
            auto object = ObjectPreambule::fromPtr(ptr);
            this->segment = object->segmentIndex;
            this->offset = uintptr_t(ptr) - persistent_heap->MapSegment(this->segment)->GetBaseAddress();
         }
         if (prevPtr) {
            delete prevPtr;
         }
      }
   }

   void* Persistent::operator new(size_t size) {
      return persistent_heap->AllocMemory(size);
   }

   void Persistent::operator delete(void* ptr) {
      if (ptr) persistent_heap->FreeMemory(ptr);
   }
   void* Persistent::resize(Persistent* ptr, size_t newsize) {
      if (ptr) {
         auto object = ObjectPreambule::fromPtr(ptr);
         newsize += sizeof(ObjectPreambule);
         if (newsize > object->size) {
            auto newPtr = persistent_heap->AllocMemory(newsize);
            memcpy(newPtr, ptr, object->size);
            persistent_heap->FreeMemory(ptr);
            return newPtr;
         }
         return ptr;
      }
      else {
         return persistent_heap->AllocMemory(newsize);
      }
   }

   void* String::operator new(size_t size, size_t count) {
      String* ptr = (String*)persistent_heap->AllocMemory(size + count);
      ptr->length = count;
      return ptr;
   }

   String* String::New(const char* chars, int32_t count) {
      if (count < 0) count = strlen(chars);
      String* str = new (count + sizeof(String)) String();
      memcpy(str->chars, chars, count);
      str->length = count;
      return str;
   }

   uint16_t PoolDescriptor::GetIndexFromSize(size_t size) {
      if (size >= c_objectSizeMin) {
#ifdef __x86_64__
         unsigned long sizeL2;
         _BitScanReverse64(&sizeL2, (size << 1) - 1);
         return sizeL2 - c_objectSizeMinL2;
#else
         unsigned long sizeL2;
         _BitScanReverse(&sizeL2, (size << 1) - 1);
         return sizeL2 - c_objectSizeMinL2;
#endif
      }
      return 0;
   }

   size_t PoolDescriptor::GetSizeFromIndex(uint16_t sizeIndex) {
      uint16_t sizeL2 = sizeIndex + c_objectSizeMinL2;
      return size_t(1) << sizeL2;
   }

   ObjectPreambule* PoolDescriptor::AllocObject(size_t size, PersistentHeap* heap) {
      uint16_t sizeIndex = this->GetIndexFromSize(size);
      if (sizeIndex < c_objectSizeClassCount) {
         return this->AllocInSegmentObject(sizeIndex, heap);
      }
      else {
         SegmentMemory* segment = heap->AllocSegment(size);
         ObjectPreambule* ref = (ObjectPreambule*)segment->GetBaseAddress();
         ref->segmentIndex = segment->segmentIndex;
         ref->size = segment->GetSize();
         return ref;
      }
      return 0;
   }

   void PoolDescriptor::FreeObject(ObjectPreambule* ref, PersistentHeap* heap) {
      uint16_t sizeIndex = this->GetIndexFromSize(ref->size);
      if (sizeIndex < c_objectSizeClassCount) {
         this->FreeInSegmentObject(ref, sizeIndex, heap);
      }
      else {
         heap->FreeSegment(ref->segmentIndex);
      }
   }

   ObjectPreambule* PoolDescriptor::AllocInSegmentObject(uint16_t sizeIndex, PersistentHeap* heap) {

      // Create segment root object
      if (sizeIndex >= c_objectSizeClassCount) {
         _ASSERT(this->GetSizeFromIndex(sizeIndex) == c_objectSizeMax);
         SegmentMemory* segment = heap->AllocSegment(c_objectSizeMax);
         ObjectPreambule* object = (ObjectPreambule*)segment->GetBaseAddress();
         object->segmentIndex = segment->segmentIndex;
         object->size = c_objectSizeMax;
         return object;
      }

      // Search in free list
      else if (tFreeObject* ref = this->freeObjects[sizeIndex]) {
         this->freeObjects[sizeIndex] = ref->next;
         return ObjectPreambule::fromPtr(ref);
      }

      // Split upper object
      else if (ObjectPreambule* object = this->AllocInSegmentObject(sizeIndex + 1, heap)) {

         // Resize the upper object
         object->size >>= 1;

         // Free the buddy part
         auto buddy = (ObjectPreambule*)(uintptr_t(object) + object->size);
         buddy->segmentIndex = object->segmentIndex;
         buddy->size = object->size;
         this->FreeInSegmentObject(buddy, sizeIndex, heap);

         return object;
      }

      // Error: Cannot allocate object
      else {
         return 0;
      }
   }

   void PoolDescriptor::FreeInSegmentObject(ObjectPreambule* object, uint16_t sizeIndex, PersistentHeap* heap) {

      // Remove segment root object
      if (sizeIndex >= c_objectSizeClassCount) {
         heap->FreeSegment(object->segmentIndex);
      }
      // Push in free list
      else {
         tFreeObject* ref = (tFreeObject*)ObjectPreambule::toPtr(object);
         ref->next = this->freeObjects[sizeIndex];
         this->freeObjects[sizeIndex] = ref;
      }
   }
}

