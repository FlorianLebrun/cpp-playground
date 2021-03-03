#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <math.h>

struct RandomVector {
   uint32_t values[16];
   RandomVector() {
      for (int i = 0; i < 16; i++) values[i] = rand() + (rand() << 16);
   }
};

struct BloomFilterSize {
   size_t m, k;
   BloomFilterSize(int capacity, double errRate) {
      double factor = log(1.0 / (pow(2.0, log(2.0))));
      this->m = ceil((double(capacity) * log(errRate)) / factor);
      this->k = ceil(log(2.0) * double(m) / capacity);
   }
   void print() {
      printf("BloomFilter size: n_bits=%d, n_hash=%d\n", m, k);
   }
};

class BloomFilter {
public:
   static const size_t BitmapCount = 2048;
   static const size_t BitmapBits = sizeof(size_t) * 8;
   static const size_t MaxFilterBits = BitmapBits * BitmapCount;
   const size_t m; // Number of bits in filter, ie. number of hash possible values
   const size_t k; // Number of hash functions per entry
private:
   size_t bitmaps[BitmapCount] = { 0 };

   void add_bit(uint32_t hash) {
      hash = hash % m;
      this->bitmaps[hash / BitmapBits] |= (size_t(1) << (hash % BitmapBits));
   }
   size_t has_bit(uint32_t hash) {
      hash = hash % m;
      return this->bitmaps[hash / BitmapBits] & (size_t(1) << (hash % BitmapBits));
   }
public:
   BloomFilter(BloomFilterSize sz) : m(sz.m), k(sz.k) {
      if (m > MaxFilterBits) throw "too big";
   }
   BloomFilter(size_t m, size_t k) : m(m), k(k) {
      if (m > MaxFilterBits) throw "too big";
   }
   void clear() {
      memset(this->bitmaps, 0, sizeof(bitmaps));
   }
   void add(uint32_t hashes[/*k*/]) {
      for (size_t i = 0; i < k; i++) {
         this->add_bit(hashes[i]);
      }
   }
   bool has(uint32_t hashes[/*k*/]) {
      for (size_t i = 0; i < k; i++) {
         if (!this->has_bit(hashes[i])) return false;
      }
      return true;
   }
   double errRate(size_t n) {
      this->clear();
      srand(clock());

      for (size_t i = 0; i < n; i++) {
         this->add(RandomVector().values);
      }

      size_t items_count = n * 1000;
      size_t items_detected = 0;
      for (size_t i = 0; i < items_count; i++) {
         if (this->has(RandomVector().values)) items_detected++;
      }

      return double(items_detected) / double(items_count);
   }
};

void main() {
   int n = 10000;

   BloomFilterSize bs(n, 0.04);
   bs.print();

   //BloomFilter filter(bs);
   BloomFilter filter(70000, 5);
   printf("BloomFilter false positive %lg %%\n", 100.0 * filter.errRate(n));
}

