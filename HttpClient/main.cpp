#include <string>
#include <windows.h>
#include <wininet.h>

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
   provider.sendRequest("https", "www.google.com", "GET", "/");

}

