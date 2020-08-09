#include <vector>
#include <string>
#include <iostream>
#include "./chrono.h"
#include "./streamwriter.h"

using namespace streamwriter;

IStringOutStream* NewConsoleOutputStream() {
   class COutStream : public IStringOutStream {
   public:
      int length;
      int total_length;
      char buffer[512];

      COutStream() {
         this->total_length = 0;
         this->length = 0;
      }
      virtual void Append(int requiredSize) override {
         //fwrite(this->buffer, 1, this->length, stdout);
         this->total_length += this->length;
         this->length = 0;
      }
      virtual void Complete() override {
         //fwrite(this->buffer, 1, this->length, stdout);
         this->total_length += this->length;
         printf("> total length = %d bytes\n", this->total_length);
         this->total_length = 0;
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
extern"C" IEncoding * inputEncoding = new coding::ascii();
extern"C" IEncoding * outputEncoding = new coding::utf8();

void write_polymorph(writer& output, IEncoding* outputEncoding, reader input, IEncoding* inputEncoding) {
   uint32_t fragment[64];
   uint32_t* fragment_end = &fragment[sizeof(fragment) / sizeof(fragment[0])];
   do {
      fragment_end = inputEncoding->decodeFragment(fragment, fragment_end, input);
      outputEncoding->encodeFragment(output, fragment, fragment_end);
   } while (fragment_end == &fragment[sizeof(fragment) / sizeof(fragment[0])]);
}

void test1(std::vector<char>& chars) {
   streamwriter::stream_utf8 out(stream);
   Chrono chrono;
   out.put<coding::ascii>(&chars[0], chars.size());
   printf("\n> test1: %lg ms\n", chrono.GetDiffDouble(Chrono::MS));
   out.stream.complete();
}

void test2(std::vector<char>& chars) {
   streamwriter::stream_utf8 out(stream);
   Chrono chrono;
   out.put<coding::utf8>(&chars[0], chars.size());
   printf("\n> test2: %lg ms\n", chrono.GetDiffDouble(Chrono::MS));
   out.stream.complete();
}

void test3(std::vector<char>& chars) {
   streamwriter::writer out(stream);
   Chrono chrono;
   write_polymorph(out, outputEncoding, reader(&chars[0], chars.size()), inputEncoding);
   printf("\n> test3: %lg ms\n", chrono.GetDiffDouble(Chrono::MS));
   out.complete();
}

void test_fastest(std::vector<char>& chars) {
   char* out = new char[chars.size()];
   Chrono chrono;
   //memcpy(out, &chars[0], chars.size());
   for (int i = 0; i < chars.size(); i++) out[i] = chars[i];
   printf("\n> test_fastest: %lg ms\n", chrono.GetDiffDouble(Chrono::MS));
}

void main() {
   std::vector<char> chars;
   for (int i = 0; i < 10000000; i++) chars.push_back(rand());

   test1(chars);
   test2(chars);
   test3(chars);

   test_fastest(chars);

   test1(chars);
   test2(chars);
   test3(chars);

   test_fastest(chars);
}
