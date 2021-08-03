#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <math.h>
#include "./samples.h"


struct TextualContent {

   struct Text {
      const char* start;
      const char* end;
      Text(const char* start, const char* end) : start(start), end(end) {
      }
   };

   struct Chunk : Text {
      int position;
      int hash;
      Chunk(int position, const char* start, const char* end) : Text(start, end), position(position) {
         this->hash = 0;
      }
      std::string str() {
         return std::string(this->start, this->length());
      }
      size_t length() {
         return this->end - this->start;
      }
      bool equals(Chunk* other) {
         if (this->hash != other->hash) return false;
         if (this->length() != other->length()) return false;
         return !strncmp(this->start, other->start, this->length());
      }
   };

   std::vector<Chunk> chunks;
   Text content;

   TextualContent(const char* bytes, int length) : content(bytes, bytes + length) {
      const char* prevPtr = content.start;
      for (const char* ptr = prevPtr; ptr < content.end; ptr++) {
         if (ptr[0] == '\n') {
            this->chunks.push_back(Chunk(this->chunks.size(), prevPtr, ptr));
            prevPtr = ptr + 1;
         }
      }
   }
   void print() {
      for (auto& chunk : this->chunks) {
         printf("> %.*s\n", chunk.end - chunk.start, chunk.start);
      }
   }
};

struct DiffContent {
   typedef TextualContent::Chunk Chunk;
   struct XChunk {
      Chunk* chunkA;
      Chunk* chunkB;
      XChunk(Chunk* chunkA, Chunk* chunkB) : chunkA(chunkA), chunkB(chunkB) {
      }
   };

   TextualContent* textA;
   TextualContent* textB;
   std::vector<XChunk> chunks;
   DiffContent(TextualContent* textA, TextualContent* textB) : textA(textA), textB(textB) {
      for (auto& chunk : textA->chunks) {
         this->chunks.push_back(XChunk(&chunk, 0));
      }
      for (auto& chunk : textB->chunks) {
         this->chunks.push_back(XChunk(0, &chunk));
      }

      struct tChange {
         int32_t from;
         int32_t chunkA;
         int32_t chunkB;
         tChange(int32_t from, int32_t chunkA = -1, int32_t chunkB = -1)
            : from(from), chunkA(chunkA), chunkB(chunkB) {
         }
      };

      struct tHead {
         int32_t from = 0;
         uint32_t weight = 0;
         bool match = 1;
      };

      std::vector<tChange> changes;
      std::vector<tHead> head, stage;
      head.resize(textB->chunks.size() + 1);
      stage.resize(textB->chunks.size() + 1);
      changes.push_back(tChange(-1));

      for (size_t posA = 0; posA < textA->chunks.size(); posA++) {
         Chunk* chunkA = &textA->chunks[posA];
         tHead* chead = head.data();
         tHead* cstage = stage.data();
         printf("----------------------|%s\n", chunkA->str().c_str());
         {
            cstage[0] = chead[0];
            printf("[%d,-]\t(%d)\n", posA, cstage->weight);
            chead++, cstage++;
         }

         for (size_t posB = 0; posB < textB->chunks.size(); posB++) {
            Chunk* chunkB = &textB->chunks[posB];

            // When B match A
            if (chunkA->equals(chunkB)) {

               // Compute weight & best path
               int mode = 0;
               int weight = chead[-1].weight + 1;
               if (weight < chead[0].weight) {
                  weight = chead[0].weight;
                  mode = 1;
               }
               if (weight < cstage[-1].weight) {
                  weight = cstage[-1].weight;
                  mode = 2;
               }
               cstage[0].weight = weight;
               cstage[0].match = 1;

               // Set stage 'from'
               int from = -1;
               switch (mode) {
               case 0: // match
               {
                  if (!chead[-1].match) { // switch matching mode
                     from = changes.size();
                     changes.push_back(tChange(chead[-1].from, posA - 1, posB - 1));
                  }
                  else {
                     from = chead[-1].from;
                  }
               } break;
               case 1:
               {
                  if (chead[0].match) { // switch matching mode
                     from = changes.size();
                     changes.push_back(tChange(chead[0].from, posA - 1, posB));
                  }
                  else {
                     from = chead[0].from;
                  }
               } break;
               case 2: {
                  if (cstage[-1].match) { // switch matching mode
                     from = changes.size();
                     changes.push_back(tChange(cstage[-1].from, posA, posB - 1));
                  }
                  else {
                     from = cstage[-1].from;
                  }
               } break;
               }
               cstage[0].from = from;
            }
            // When no match
            else {
               cstage[0].match = 0;
               if (chead[0].weight > cstage[-1].weight) {
                  cstage[0].weight = chead[0].weight;
                  if (chead[0].match) {
                     cstage[0].from = changes.size();
                     changes.push_back(tChange(chead[0].from, posA - 1, posB));
                  }
                  else {
                     cstage[0].from = chead[0].from;
                  }
               }
               else {
                  cstage[0].weight = cstage[-1].weight;
                  if (cstage[-1].match) {
                     cstage[0].from = changes.size();
                     changes.push_back(tChange(cstage[-1].from, posA, posB - 1));
                  }
                  else {
                     cstage[0].from = cstage[-1].from;
                  }
               }
            }
            printf("[%d,%d]\t%d(%d)\t|%s\t%s\n", posA, posB, cstage->from, cstage->weight, chunkB->str().c_str(), cstage->match ? "=" : "+-");
            chead++, cstage++;
         }
         head.swap(stage);
      }
      tChange* cchange = &changes[head.back().from];
      std::vector<tChange*> path;
      for (;;) {
         path.push_back(cchange);
         if (cchange->from < 0) break;
         if (cchange <= &changes[cchange->from]) break;
         cchange = &changes[cchange->from];
      }
      printf("---------- result ------------\n");
      for (int i = path.size() - 1; i >= 0; i--) {
         tChange* cchange = path[i];
         printf("[%d,%d] %s\n", cchange->chunkA, cchange->chunkB, i & 1 ? "+-" : "=");
      }
   }
   void print() {
      for (auto& xchk : this->chunks) {
         char mark = ' ';
         Chunk* chunk = 0;
         if (xchk.chunkA) {
            chunk = xchk.chunkA;
            if (!xchk.chunkB) mark = '-';
         }
         else {
            chunk = xchk.chunkB;
            mark = '+';
         }
         printf("%c %.*s\n", mark, chunk->end - chunk->start, chunk->start);
      }
   }
};

void test_sample(const char** text_sample) {
   TextualContent doc1(text_sample[0], strlen(text_sample[0]));
   TextualContent doc2(text_sample[1], strlen(text_sample[1]));
   //doc1.print();

   DiffContent diff(&doc1, &doc2);
   diff.print();
}

int main() {
   //test_sample(text_sample_same);
   //test_sample(text_sample_fulldiff);
   test_sample(text_sample0);
   return 0;
}

