#include <vector>
#include <string>
#include <iostream>
#include "./chrono.h"
#include "./streamwriter.h"

IStringOutStream* NewConsoleOutputStream() {
   class COutStream : public IStringOutStream {
   public:
      int length;
      char buffer[512];
      COutStream() {
         this->length = 0;
      }
      virtual void Append(int requiredSize) override {
         //fwrite(this->buffer, 1, this->length, stdout);
         this->length = 0;
      }
      virtual void Complete() override {
         //fwrite(this->buffer, 1, this->length, stdout);
         this->length = 0;
      }
      virtual char* Ptr() override {
         return this->buffer;
      }
      virtual int Length() override {
         return this->length;
      }
      virtual int Allocated() override {
         return sizeof(this->buffer);
      }
      virtual void Resize(int newSize) override {
         this->length = (sizeof(this->buffer) < newSize) ? sizeof(this->buffer) : newSize;
      }
   };
   return new COutStream();
}

extern"C" IStringOutStream * stream = NewConsoleOutputStream();

using namespace streamwriter;

void main() {
   Chrono chrono;
   std::vector<char> chars;
   for (int i = 0; i < 10000000; i++) chars.push_back(rand());

   streamwriter::stream_utf8 out(stream);
   chrono.Start();
   out.put<coding::ascii>(&chars[0], chars.size());
   printf("\n> time: %lg ms\n", chrono.GetDiffDouble(Chrono::MS));
}
