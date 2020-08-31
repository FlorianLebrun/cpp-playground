#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <windows.h>
#include <wininet.h>

/*

*/
struct tUri {
   std::string scheme;
   std::string authority;
   std::string path;
};
struct Range {
   const char* begin;
   const char* end;
   Range(const char* _begin, const char* _end)
      :begin(_begin), end(_end)
   {
   }
};
struct tPatternStream {
   std::ostringstream result;
   std::map<std::string, std::string> parameters;
   std::map<std::string, std::string> globals;

   bool writeExpression(std::string value, std::string format) {
      auto it = this->parameters.find(value);
      if (it == this->parameters.end()) return false;
      result << it->second;
      return true;
   }
   tPatternStream& writePattern(const char* ptr, const char* end) {
      while (ptr < end) {
         char c = ptr[0];
         if (c == '{' && ++ptr < end && ptr[0] != '{') {
            const char* start = ptr;
            while (ptr < end) {
               if (ptr[0] == '}') {
                  if (!this->writeExpression(std::string(start, ptr - start), "")) {
                     ptr = start;
                  }
                  break;
               }
               ptr++;
            }
         }
         else result << c;
         ptr++;
      }
      return *this;
   }
   tPatternStream& writePattern(std::string& pattern) {
      return this->writePattern(pattern.c_str(), pattern.c_str() + pattern.size());
   }
   std::string str() {
      return result.str();
   }
};

struct tParsedUri {
   std::string parse(std::string pattern) {
      std::ostringstream result;
      int length = pattern.size();
      const char* ptr = pattern.c_str();
      for (int i = 0; i < length; i++) {
         if (ptr[i] == '{') {
            if (++i < length) {
               if (ptr[i] != '{') {
                  int startIndex = i;
                  for (int j = i; j < length; j++) {
                     if (ptr[i] == '}') {
                     }
                  }
               }
               else result << ptr[i];
            }
         }
         else result << ptr[i];
      }
      return result.str();
   }

};

struct tParsedAuthority {
   std::string domain;
   DWORD port;
   tParsedAuthority(std::string& authority) {
      int portIndex = authority.find(':');
      if (portIndex >= 0) {
         this->domain = authority.substr(0, portIndex);
         this->port = atoi(&authority.c_str()[portIndex]);
      }
      else {
         this->domain = authority;
         this->port = 0;
      }
   }
   std::string toString() {
      char tmp[16];
      if (this->port) return this->domain + ":" + std::string(itoa(this->port, tmp, 10));
      else return domain;
   }
};

struct InternetProvider {
   HINTERNET hSession;

   InternetProvider() {
      this->hSession = InternetOpenA("WebHosting", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
   }
   void sendRequest(std::string scheme, std::string authority, std::string method, std::string path, std::string data = "") {
      tParsedAuthority authority_s(authority);

      // Prepare connexion options
      DWORD requestFlags = 0, connectFlags = 0;
      if (scheme == "https") {
         if (!authority_s.port) authority_s.port = INTERNET_DEFAULT_HTTPS_PORT;
         connectFlags = INTERNET_SERVICE_HTTP;
         requestFlags = INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_SECURE;
      }
      else if (scheme == "http") {
         if (!authority_s.port) authority_s.port = INTERNET_DEFAULT_HTTP_PORT;
         connectFlags = INTERNET_SERVICE_HTTP;
         requestFlags = INTERNET_FLAG_KEEP_CONNECTION;
      }

      // Prepare referer header;
      std::string referer = scheme + "://" + authority_s.toString() + path;
      printf("[%s] %s\n", method.c_str(), referer.c_str());

      HINTERNET hConnect = InternetConnectA(
         this->hSession,
         authority_s.domain.c_str(),
         authority_s.port,
         "",
         "",
         connectFlags,
         0,
         0);

      HINTERNET hRequest = HttpOpenRequestA(
         hConnect,
         method.c_str(), // METHOD
         path.c_str(),   // URI
         referer.c_str(),
         NULL,
         NULL,
         requestFlags,
         0);

      if (!HttpSendRequestA(hRequest, NULL, 0, (void*)data.c_str(), data.size())) {
         printf("HttpSendRequest error : (%lu)\n", GetLastError());

         InternetErrorDlg(
            GetDesktopWindow(),
            hRequest,
            ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED,
            FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
            FLAGS_ERROR_UI_FLAGS_GENERATE_DATA |
            FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
            NULL);
      }
      DWORD dwFileSize;
      //dwFileSize = (DWORD)atol(bufQuery);
      dwFileSize = BUFSIZ;

      char* buffer = new char[dwFileSize + 1];

      while (true) {
         DWORD dwBytesRead;
         BOOL bRead;

         bRead = InternetReadFile(
            hRequest,
            buffer,
            dwFileSize + 1,
            &dwBytesRead);

         if (dwBytesRead == 0) break;

         if (!bRead) {
            printf("InternetReadFile error : <%lu>\n", GetLastError());
         }
         else {
            buffer[dwBytesRead] = 0;
            printf("Retrieved %lu data bytes: %s\n", dwBytesRead, buffer);
         }
      }

      InternetCloseHandle(hRequest);
      InternetCloseHandle(hConnect);
   }
};

void main() {
   InternetProvider provider;
   //provider.sendRequest("https", "www.google.com", "GET", "/");

   tPatternStream pattern;
   pattern.parameters["x"] = "hello";
   pattern.parameters["y"] = "world";
   pattern.parameters["ws-url"] = "https://www.website.com";
   pattern.parameters["userID"] = "johnd";

   // std::cout << "result: " << pattern.append(std::string("x={x} y={abc}\n")).str();
   std::cout << "result: " << pattern.writePattern(std::string("{ws-url}/do/somthing/{userID}")).str();
}

