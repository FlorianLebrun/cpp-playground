#include <mutex>
#include <map>
#include <vector>
#include <string>
#include <sstream>

struct PathWriter {
private:
   typedef char chr_t;
   chr_t path[2048];
   chr_t* keys[64];
   int ckey;
public:
   PathWriter() {
      this->path[0] = '\\';
      this->path[1] = '\\';
      this->path[2] = '?';
      this->path[3] = '\\';
      this->keys[0] = &this->path[3];
      this->ckey = 0;
   }
   void write(const char* cptr) {
      this->write(cptr, strlen(cptr));
   }
   void write(const char* part, int partlen) {
      chr_t* cpath = this->keys[this->ckey];
      int ck = this->ckey;
      if (partlen > 0) {
         char c0 = part[0];
         if (c0 != '/' && c0 != '\\') {
            *(cpath++) = '\\';
         }
      }
      for (int i = 0; i < partlen; i++) {
         char c0 = part[i];
         if (c0 == '/' || c0 == '\\') {
            this->keys[++ck] = cpath;
            *(cpath++) = '\\';
            continue;
         }
         else if (c0 == '.') {
            char c1 = (i + 1 < partlen) ? part[i + 1] : 0;
            char c2 = (i + 2 < partlen) ? part[i + 2] : 0;
            if (c1 == '.' && (c2 == '/' || c2 == '\\')) {
               if (ck > 0) cpath = this->keys[--ck] + 1;
               i += 2;
               continue;
            }
            else if (c1 == '/' || c1 == '\\') {
               cpath = this->keys[ck] + 1;
               i++;
               continue;
            }
         }
         *(cpath++) = c0;
      }
      this->keys[++ck] = cpath;
      this->ckey = ck;
   }
   chr_t* c_str() {
      this->keys[this->ckey][0] = 0;
      return this->path;
   }
   int size() {
      return this->keys[this->ckey] - this->path;
   }
};

#include <Windows.h>
void main() {
   PathWriter w;
   w.write("d:/aaaa\\bbbb/cc");
   w.write("../../xxxx");
   w.write("./y");
   printf("%s\n", w.c_str());


#define BUFSIZE 4096
   TCHAR  buffer[BUFSIZE] = TEXT("\\\\?\\");
   TCHAR  buf[BUFSIZE] = TEXT("");
   TCHAR** lppPart = { NULL };
   GetFullPathNameA("d:/aaaa\\bbbb/cc/../../xxxx/./y", BUFSIZE - 4, &buffer[4], lppPart);
   printf("%s\n", buffer);
}
