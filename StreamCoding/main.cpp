#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <codecvt>
#include "./chrono.h"
#include <Windows.h>
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
      virtual IStringOutStream* Reset() override {
         this->total_length = 0;
         this->length = 0;
         return this;
      }
      virtual int TotalLength() override {
         return this->total_length;
      }
      virtual void Append(int requiredSize) override {
         //fwrite(this->buffer, 1, this->length, stdout);
         this->total_length += this->length;
         this->length = 0;
      }
      virtual void Complete() override {
         //fwrite(this->buffer, 1, this->length, stdout);
         this->total_length += this->length;
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

coding::acp::converter_t* coding::acp::converter = new coding::acp::converter_t();

void reportTime(double time_ms, int size) {
   printf("  %d bytes - %lg ms\n", size, time_ms);
}

void write_polymorph(writer& output, IEncoding* outputEncoding, reader input, IEncoding* inputEncoding) {
   uint32_t fragment[64];
   uint32_t* fragment_end = &fragment[sizeof(fragment) / sizeof(fragment[0])];
   do {
      fragment_end = inputEncoding->decodeFragment(fragment, fragment_end, input);
      outputEncoding->encodeFragment(output, fragment, fragment_end);
   } while (fragment_end == &fragment[sizeof(fragment) / sizeof(fragment[0])]);
}

template <class Tin, class Tout>
void test_static(std::vector<char>& chars) {
   printf("\n> static - %s -> %s\n", Tin::name(), Tout::name());
   stream_utf8 out(stream->Reset());
   Chrono chrono;
   out.put<Tin, Tout>(&chars[0], chars.size());
   out.stream.complete();
   reportTime(chrono.GetDiffDouble(Chrono::MS), stream->TotalLength());
}

void test_dynamic_by_fragment(std::vector<char>& chars, IEncoding* inputEncoding, IEncoding* outputEncoding) {
   printf("\n> dynamic.charBatch - %s -> %s\n", inputEncoding->getName(), outputEncoding->getName());
   writer out(stream->Reset());
   Chrono chrono;
   write_polymorph(out, outputEncoding, reader(&chars[0], chars.size()), inputEncoding);
   out.complete();
   reportTime(chrono.GetDiffDouble(Chrono::MS), stream->TotalLength());
}

void test_dynamic_char_by_char(std::vector<char>& chars, IEncoding* inputEncoding, IEncoding* outputEncoding) {
   printf("\n> dynamic.charByChar - %s -> %s\n", inputEncoding->getName(), outputEncoding->getName());
   writer out(stream->Reset());
   reader in(&chars[0], chars.size());
   Chrono chrono;
   while (!in.empty()) {
      outputEncoding->writeCode(out, inputEncoding->readCode(in));
   }
   out.complete();
   reportTime(chrono.GetDiffDouble(Chrono::MS), stream->TotalLength());
}

void test_std_codecvt_utf8(std::vector<char>& chars) {
   printf("\n> std::codecvt_utf8\n");
   std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
   std::wstring in((wchar_t*)&chars[0], chars.size() / 2);
   Chrono chrono;
   std::string fullpath = utf8_conv.to_bytes(in);
   reportTime(chrono.GetDiffDouble(Chrono::MS), fullpath.size());
}

void test_win32_MultiByteToWideChar(std::vector<char>& chars, int codepage, const char* codepage_name) {
   printf("\n> win32 MultiByte To WChar - %s\n", codepage_name);
   std::wstring out(chars.size(), 0);
   Chrono chrono;
   int out_size;
   //out_size = MultiByteToWideChar(codepage, 0, chars.data(), chars.size(), 0, 0);
   out_size = MultiByteToWideChar(codepage, 0, chars.data(), chars.size(), &out[0], out.size());
   reportTime(chrono.GetDiffDouble(Chrono::MS), out_size);
}

void test_win32_WideCharToMultiByte(std::vector<char>& chars, int codepage, const char* codepage_name) {
   printf("\n> win32 WChar To MultiByte - %s\n", codepage_name);
   std::string out(chars.size()*2, 0);
   Chrono chrono;
   int out_size;
   //out_size = WideCharToMultiByte(codepage, 0, (wchar_t*)chars.data(), chars.size() / 2, 0, 0, 0, 0);
   out_size = WideCharToMultiByte(codepage, 0, (wchar_t*)chars.data(), chars.size() / 2, &out[0], out.size(), 0, 0);
   reportTime(chrono.GetDiffDouble(Chrono::MS) * 2, out_size * 2);
}

void test_fast_copy(std::vector<char>& chars) {
   printf("\n> fast copy\n");
   int length = chars.size();
   char* out = new char[length];
   Chrono chrono;
   //memcpy(out, &chars[0], chars.size());
   for (int i = 0; i < length; i++) out[i] = chars[i];
   reportTime(chrono.GetDiffDouble(Chrono::MS), length);
}

IEncoding* inputEncoding = new coding::acp();
IEncoding* outputEncoding = new coding::utf8();

void main() {
   std::vector<char> chars;
   for (int i = 0; i < 10000000; i++) chars.push_back(rand());

   printf("\n\n-------------- copy --------------\n");
   test_static<coding::ascii, coding::ascii>(chars);
   test_static<coding::utf8, coding::utf8>(chars);
   test_static<coding::acp, coding::acp>(chars);

   printf("\n\n-------------- static convert --------------\n");
   test_static<coding::acp, coding::utf8>(chars);
   test_static<coding::utf8, coding::acp>(chars);

   printf("\n\n-------------- dynamic convert --------------\n");
   test_dynamic_by_fragment(chars, inputEncoding, outputEncoding);
   test_dynamic_char_by_char(chars, inputEncoding, outputEncoding);
   test_dynamic_by_fragment(chars, outputEncoding, inputEncoding);
   test_dynamic_char_by_char(chars, outputEncoding, inputEncoding);

   printf("\n\n-------------- win32 convert --------------\n");
   test_win32_MultiByteToWideChar(chars, CP_UTF8, "UTF8");
   test_win32_WideCharToMultiByte(chars, CP_UTF8, "UTF8");
   test_win32_MultiByteToWideChar(chars, CP_ACP, "ACP");
   test_win32_WideCharToMultiByte(chars, CP_ACP, "ACP");

   printf("\n\n-------------- other --------------\n");
   test_std_codecvt_utf8(chars);
   test_fast_copy(chars);
}
