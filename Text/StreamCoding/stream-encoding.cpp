#include "./stream-encoding.h"
#include <codecvt>
#include <Windows.h>

using namespace streamwriter;

coding::acp::converter_t* coding::acp::converter = new coding::acp::converter_t();

void streamwriter::int64_to_ascii(uint8_t* buf, int bufsize, int64_t value, int radix) {
   _i64toa_s(value, (char*)buf, bufsize, radix);
}

void streamwriter::float64_to_ascii(uint8_t* buf, int bufsize, double value) {
   uint8_t tmp[64];
   _gcvt_s((char*)tmp, sizeof(tmp), value, std::numeric_limits<double>::digits10);
   uint8_t* dst = buf, * src = tmp;
   while ((dst++)[0] = (src++)[0]) {
      if (src[-1] == '.' && (src[0] < '0' || src[0]>'9')) (dst++)[0] = '0';
   }
}

coding::acp::converter_t::converter_t() {

   // For ACP -> unicode
   uint8_t ACP_chars[256];
   for (int i = 0; i < 256; i++)ACP_chars[i] = i;
   MultiByteToWideChar(CP_ACP, 0, LPCCH(ACP_chars), 256, LPWSTR(this->ACP2unicode_table), 256);

   // For unicode -> ACP table
   int code_max = 0;
   memset(this->unicode2ACP_table, '?', unicode2ACP_table_size);
   for (int i = 0; i < 256; i++) {
      uint32_t code = this->ACP2unicode_table[i];
      if (code < unicode2ACP_table_size)this->unicode2ACP_table[code] = i;
      if (code > code_max)code_max = code;
   }
   printf("ACP code_max = %d\n", code_max);

   // For unicode -> ACP hashmap
   for (int i = 0; i < 256; i++) {
      uint16_t code = this->ACP2unicode_table[i];
      this->unicode2ACP_chars[i].acp = i;
      this->unicode2ACP_chars[i].code = code;
      this->unicode2ACP_chars[i].hashId = ((uint8_t*)&code)[0] ^ ((uint8_t*)&code)[1];
   }
   this->unicode2ACP_chars[256].hashId = 0;
   this->unicode2ACP_chars[256].acp = 0;
   this->unicode2ACP_chars[256].code = 0;
   qsort(this->unicode2ACP_chars, 256, sizeof(tACPChar), tACPChar::compareHashId);
   memset(unicode2ACP_hash2index, 0xff, 256);
   for (int i = 255; i >= 0; i--) unicode2ACP_hash2index[unicode2ACP_chars[i].hashId] = i;
}
