#include <mutex>
#include <map>
#include <vector>
#include <string>
#include <sstream>

struct Uri {
   typedef std::map<std::string, std::string> tParams;

   struct tChunk {
      const char* begin;
      const char* end;
      tChunk(std::string& str) : begin(str.begin()._Ptr), end(str.end()._Ptr) {}
      tChunk(const char* _begin, const char* _end) : begin(_begin), end(_end) {}
      operator const char& () { return *this->begin; }
      void operator ++ () { this->begin++; }
   };

   std::vector<tChunk> stack;
   tParams parameters;

   std::string scheme;
   std::string identifier;

   bool parseChunk(tChunk& chunk) {
      std::ostringstream s_scheme;
      while (chunk.begin < chunk.end) {
         auto c = chunk;
         if ((c >= 'a' && c <= 'z') || c == '+' || c == '-' || c == '.') {
            s_scheme << c;
         }
         else if (c >= 'A' && c <= 'Z') {
            s_scheme << (c - 'a');
         }
         else if (c == ':') {
            this->scheme = s_scheme.str();
            this->identifier = std::string(chunk.begin + 1, chunk.end);
            return true;
         }
         else break;
         chunk++;
      }
      return false;
   }
   bool parse(std::string value) {
      this->stack.push_back(tChunk(value));
   }
};

void main() {
   Uri uri;
   uri.parameters["config-dir"] = "/config";
   uri.parameters["param0"] = "user name";
}
