#include <stdint.h>
#include <string.h>

struct IStringOutStream {
   virtual char* Ptr() = 0;
   virtual int Length() = 0;
   virtual int Allocated() = 0;
   virtual void Resize(int size) = 0;
   virtual void Append(int expectedSize) = 0;
   virtual void Complete() = 0;
};

namespace streamwriter {

   class reader {
   public:
      const uint8_t* ptr;
      const uint8_t* ptrEnd;
      reader(const void* ptr, int size)
         :ptr((const uint8_t*)ptr), ptrEnd(((const uint8_t*)ptr) + size)
      {
      }
      reader(const void* begin, const void* end)
         :ptr((const uint8_t*)begin), ptrEnd((const uint8_t*)end + 1)
      {
      }
      uint8_t pop() {
         if (this->ptr >= this->ptrEnd) return 0;
         else return (this->ptr++)[0];
      }
   };

   class writer {
      IStringOutStream* stream;
   public:
      uint8_t* ptr;
      uint8_t* ptrStart;
      uint8_t* ptrEnd;

      writer(IStringOutStream* stream) {
         this->stream = stream;
         if (!this->stream->Ptr()) this->stream->Append(512);
         this->_link_buffer();
      }
      void flush() {
         this->stream->Resize(this->ptr - this->ptrStart);
         this->stream->Append(512);
      }
      void expand() {
         this->flush();
         this->_link_buffer();
      }
      void push(uint8_t byte) {
         if (this->ptr >= this->ptrEnd) this->expand();
         (this->ptr++)[0] = byte;
      }
      void write_bytes(const void* buffer, int size) {
         const char* bytes = (char*)buffer;
         while (this->ptr + size > this->ptrEnd) {
            int count = this->ptrEnd - this->ptr;
            size -= count;
            bytes += count;
            memcpy(this->ptr, bytes, count);
            this->ptr = this->ptrEnd;
            this->expand();
         }
         memcpy(this->ptr, bytes, size);
      }
      template<class output_coding, typename value_t>
      void write_values(const value_t* values, int count) {

         // Safe reading (ie. with no overflow risk)
         const value_t* valuesEnd = &values[count];
         while (values < valuesEnd) {

            // Safe writing (ie. with no overflow risk)
            uint8_t* outputSafeLimit = this->ptrEnd - output_coding::code_size_max;
            while (this->ptr <= outputSafeLimit) {
               if (values >= valuesEnd) return;
               output_coding::write_code(this->ptr, (values++)[0]);
            }

            // Unsafe writing (ie. with overflow risk)
            uint8_t* ptr_current = this->ptr;
            do {
               if (values >= valuesEnd) return;
               output_coding::write_code(*this, (values++)[0]);
            } while (this->ptr == ptr_current);
         }
      }
      template<class output_coding, class input_coding>
      void write_string(reader input) {

         // Safe reading (ie. with no overflow risk)
         const uint8_t* inputSafeLimit = input.ptrEnd - input_coding::code_size_max;
         while (input.ptr < inputSafeLimit) {

            // Safe writing (ie. with no overflow risk)
            uint8_t* outputSafeLimit = this->ptrEnd - output_coding::code_size_max;
            while (this->ptr <= outputSafeLimit) {
               if (input.ptr >= inputSafeLimit) goto write_inputInUnsafeLimit;
               uint32_t code = input_coding::read_code(input.ptr);
               output_coding::write_code(this->ptr, code);
            }

            // Unsafe writing (ie. with overflow risk)
            uint8_t* outputPtrCurrent = this->ptr;
            do {
               if (input.ptr >= inputSafeLimit) goto write_inputInUnsafeLimit;
               uint32_t code = input_coding::read_code(input.ptr);
               output_coding::write_code(*this, code);
            } while (this->ptr == outputPtrCurrent);
         }

         // Unsafe reading (ie. with overflow risk)
      write_inputInUnsafeLimit:
         while (input.ptr < input.ptrEnd) {

            // Safe writing (ie. with no overflow risk)
            uint8_t* outputSafeLimit = this->ptrEnd - output_coding::code_size_max;
            while (this->ptr <= outputSafeLimit) {
               if (input.ptr >= input.ptrEnd) return;
               uint32_t code = input_coding::read_code(input);
               output_coding::write_code(this->ptr, code);
            }

            // Unsafe writing (ie. with overflow risk)
            uint8_t* outputPtrCurrent = this->ptr;
            do {
               if (input.ptr >= input.ptrEnd) return;
               uint32_t code = input_coding::read_code(input);
               output_coding::write_code(*this, code);
            } while (this->ptr == outputPtrCurrent);
         }
      }
      template<class output_coding, class input_coding>
      void write(reader input) {
         if (input_coding::use_integral_coding) {
            this->write_values<input_coding::character_t>((const input_coding::character_t*)input.ptr, input.count());
         }
         else {
            this->write_string<output_coding, input_coding>(input);
         }
      }
      template<class output_coding, class input_coding>
      void write(const void* bytes, int count) {
         if (input_coding::use_integral_coding) {
            this->write_values<output_coding, input_coding::character_t>((const input_coding::character_t*)bytes, count);
         }
         else {
            this->write_string<output_coding, input_coding>(reader(bytes, count));
         }
      }
   private:
      void _link_buffer() {
         this->ptrStart = (uint8_t*)this->stream->Ptr();
         this->ptr = this->ptrStart + stream->Length();
         this->ptrEnd = this->ptrStart + stream->Allocated();
      }
   };
}

namespace streamwriter {
   namespace coding {

      class ascii {
      public:
         typedef uint8_t character_t;
         static const bool use_integral_coding = true;
         static const int code_size_max = sizeof(character_t);

         static __forceinline uint32_t read_code(reader& stream) {
            return stream.pop();
         }
         static __forceinline uint32_t read_code(const uint8_t*& ptr) {
            return (ptr++)[0];
         }
         static __forceinline void write_code(writer& stream, uint32_t code) {
            stream.push(code);
         }
         static __forceinline void write_code(uint8_t*& ptr, uint32_t code) {
            (ptr++)[0] = code;
         }
      };

      class utf8 {
      public:
         typedef void character_t;
         static const bool use_integral_coding = false;
         static const int code_size_max = 4;

         static __forceinline uint32_t read_code(reader& stream) {
            return stream.pop();
         }
         static __forceinline uint32_t read_code(const uint8_t*& ptr) {
            return (ptr++)[0];
         }
         static __forceinline void write_code(writer& stream, uint32_t code) {
            if (code < 128) { // 1 byte
               stream.push(code);
            }
            else if (code < 2048) { // 2 bytes
               stream.push((code & 0x40) ? 0xC3 : 0xC2);
               stream.push((code & 0x3f) | 0x80);
            }
         }
         static __forceinline void write_code(uint8_t*& ptr, uint32_t code) {
            if (code < 128) { // 1 byte
               (ptr++)[0] = code;
            }
            else if (code < 2048) { // 2 bytes
               (ptr++)[0] = (code & 0x40) ? 0xC3 : 0xC2;
               (ptr++)[0] = (code & 0x3f) | 0x80;
            }
         }
      };

      class utf8_escaped {
      public:
         typedef void character_t;
         static const bool use_integral_coding = false;
         static const int code_size_max = 6;

         static __forceinline uint32_t read_code(reader& stream) {
            return stream.pop();
         }
         static __forceinline uint32_t read_code(const uint8_t*& ptr) {
            return (ptr++)[0];
         }
         static __forceinline void write_code(writer& stream, uint32_t code) {
            if (code < 128) { // 1 byte
               if (code < 32) {
                  stream.push('\\');
                  switch (code) {
                  case '\b':stream.push('b'); break;// \b  Backspace (ascii code 08)
                  case '\f':stream.push('f'); break;// \f  Form feed (ascii code 0C)
                  case '\n':stream.push('n'); break;// \n  New line
                  case '\r':stream.push('r'); break;// \r  Carriage return
                  case '\t':stream.push('t'); break;// \t  Tab
                  default:
                     stream.push('u');
                     stream.push('0');
                     stream.push('0');
                     stream.push('0' + (code >> 4));
                     stream.push('0' + (code & 0xf));
                  }
               }
               else if (code == '\"') {// \"  Double quote
                  stream.push('\\');
                  stream.push('\"');
               }
               else if (code == '\\') {// \\  Backslash character
                  stream.push('\\');
                  stream.push('\\');
               }
               else {
                  stream.push(code);
               }
            }
            else if (code < 2048) { // 2 bytes
               stream.push((code & 0x40) ? 0xC3 : 0xC2);
               stream.push((code & 0x3f) | 0x80);
            }
         }
         static __forceinline void write_code(uint8_t*& ptr, uint32_t code) {
            if (code < 128) { // 1 byte
               if (code < 32) {
                  (ptr++)[0] = '\\';
                  switch (code) {
                  case '\b':(ptr++)[0] = 'b'; break;// \b  Backspace (ascii code 08)
                  case '\f':(ptr++)[0] = 'f'; break;// \f  Form feed (ascii code 0C)
                  case '\n':(ptr++)[0] = 'n'; break;// \n  New line
                  case '\r':(ptr++)[0] = 'r'; break;// \r  Carriage return
                  case '\t':(ptr++)[0] = 't'; break;// \t  Tab
                  default:
                     ptr[0] = 'u';
                     ptr[1] = '0';
                     ptr[2] = '0';
                     ptr[3] = '0' + (code >> 4);
                     ptr[4] = '0' + (code & 0xf);
                     ptr += 5;
                  }
               }
               else if (code == '\"') {// \"  Double quote
                  (ptr++)[0] = '\\';
                  (ptr++)[0] = '\"';
               }
               else if (code == '\\') {// \\  Backslash character
                  (ptr++)[0] = '\\';
                  (ptr++)[0] = '\\';
               }
               else {
                  (ptr++)[0] = code;
               }
            }
            else if (code < 2048) { // 2 bytes
               (ptr++)[0] = (code & 0x40) ? 0xC3 : 0xC2;
               (ptr++)[0] = (code & 0x3f) | 0x80;
            }
         }
      };

   }
}

namespace streamwriter {

   struct stream_utf8 {
      typedef coding::utf8 default_coding;

      writer stream;

      stream_utf8(IStringOutStream* output)
         : stream(output)
      {
      }
      stream_utf8& put_bytes(const void* bytes, int count) {
         stream.write_bytes(bytes, count);
         return *this;
      }
      template<class output_coding = default_coding>
      stream_utf8& put(int codepoint) {
         output_coding::write_code(stream, (uint32_t)value);
         return *this;
      }
      template<class input_coding, class output_coding = default_coding>
      stream_utf8& put(const void* bytes, int count) {
         stream.write<output_coding, input_coding>(bytes, count);
         return *this;
      }
      /*template<class output_coding = default_coding>
      stream_utf8& put(aString value) {
         tpChar bytes = value->Ptr();
         int count = value->Length();
         switch (value->Encoding()) {
         case StringEncoding::ASCII:
            stream.write<output_coding, coding::ascii>(bytes, count);
            return *this;
         case StringEncoding::Utf8:
            stream.write<output_coding, coding::utf8>(bytes, count);
            return *this;
         default:
            stream.write_bytes(bytes, count);
            return *this;
         }
      }*/
      stream_utf8& puti(int64_t value, int radix = 10) {
         uint8_t buf[64];
         _i64toa_s(value, (char*)buf, sizeof(buf), radix);
         stream.write_values<default_coding, uint8_t>(buf, strlen((char*)buf));
         return *this;
      }
      stream_utf8& putf(double value) {
         uint8_t tmp[64], buf[64];
         _gcvt_s((char*)tmp, sizeof(tmp), value, std::numeric_limits<double>::digits10);
         uint8_t* dst = buf, * src = tmp;
         while ((dst++)[0] = (src++)[0]) {
            if (src[-1] == '.' && (src[0] < '0' || src[0]>'9')) (dst++)[0] = '0';
         }
         stream.write_values<default_coding, uint8_t>(buf, dst - buf);
         return *this;
      }
   };

}