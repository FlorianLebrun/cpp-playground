#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <windows.h>

struct Listener {
   void ReceiveChange(DWORD Action, std::wstring path) {
      printf("> file: %.*S\n", path.length(), path.c_str());
   }
};

struct WatchPool {

   struct tWatchEntry {
      OVERLAPPED overlapped = { 0 };
      HANDLE hFile = 0;
      std::string path;
      DWORD filter = 0;
      Listener* listener = 0;
      BYTE dataBuf[1024];
      DWORD dataSize;
      ~tWatchEntry() {
         if (this->hFile) {
            CancelIo(this->hFile);
            CloseHandle(this->hFile);
         }
      }
      bool listen() {
         return ReadDirectoryChangesW(this->hFile, this->dataBuf, sizeof(this->dataBuf), true, this->filter, &this->dataSize, &this->overlapped, NULL);
      }
      void dispatch() {
         auto info = (FILE_NOTIFY_INFORMATION*)this->dataBuf;
         LPBYTE dataEnd = this->dataBuf + sizeof(this->dataBuf);
         while (LPBYTE(info) < dataEnd && LPBYTE(&info->FileName[info->FileNameLength / sizeof(wchar_t)]) < dataEnd) {
            this->listener->ReceiveChange(info->Action, std::wstring(info->FileName, info->FileNameLength / sizeof(wchar_t)));

            if (info->NextEntryOffset == 0UL) break;
            info = (PFILE_NOTIFY_INFORMATION)(LPBYTE(info) + info->NextEntryOffset);
         }
      }
   };

   std::vector<tWatchEntry*> entries;
   std::thread* thread = 0;
   std::mutex lock;
   HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);


   bool addWatch(Listener* listener, std::string path) {
      tWatchEntry* entry = new tWatchEntry();
      entry->path = path;
      entry->filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;

      entry->hFile = CreateFileA(
         path.c_str(),       // Chemin a surveiller
         FILE_LIST_DIRECTORY,        // C'est un chemin qu'il faut ouvrir
         FILE_SHARE_READ |        // Mode de partage, l'exemple donné dans le SDK est
         FILE_SHARE_DELETE |        // faux, du moins avec Xp car il manque le partage
         FILE_SHARE_WRITE,           // en écriture
         0,                        // sécurité par défaut
         OPEN_EXISTING,              // Le chemin doit exister
         FILE_FLAG_BACKUP_SEMANTICS  // Attribut de surcharge des droits liés au dossier
         | FILE_FLAG_OVERLAPPED,  // Attribut de fonction effectuée après coup
         0);                         // Pas d'attributs à copier

      if (CreateIoCompletionPort(entry->hFile, this->hIOCP, (ULONG_PTR)entry->hFile, 0) == NULL) {
         delete entry;
         return false;
      }

      if (!entry->listen()) {
         delete entry;
         return false;
      }

      std::lock_guard<std::mutex> guard(lock);
      entries.push_back(entry);
      if (!thread) thread = new std::thread([this]() {this->loop(); });
      printf("Watch changes at '%s'...\n", path.c_str());
      return true;
   }
   void unwatch(Listener* listener) {
      //FindCloseChangeNotification(handle);
   }
   void loop() {
      ULONG_PTR completionKey = NULL;
      LPOVERLAPPED overlapped = NULL;
      DWORD bytesTransferred = 0;
      while (GetQueuedCompletionStatus(this->hIOCP, &bytesTransferred, &completionKey, &overlapped, INFINITE)) {
         printf("Change detected ...\n");
         auto entry = (tWatchEntry*)overlapped;
         entry->dispatch();
         entry->listen();
      }
   }

} watchPool;

void main() {
   Listener listener;
   std::string root = "D:/git/cpp-playground/FileWatch/test";
   watchPool.addWatch(&listener, root);
   while (1) {
      Sleep(1000);
      std::ofstream ofs(root + "/test.txt", std::ofstream::out | std::ofstream::app);
      ofs.write("x", 1);
   }
   system("pause");
   printf("\n");
}

