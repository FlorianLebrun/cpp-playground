#include "DirectoryWatch.h"
#include <iostream>
#include <mutex>
#include <windows.h>

#define TRACE_WatchPool(x) //x

struct DirectoryWatchEntry {

   // Handling data
   OVERLAPPED overlapped = { 0 };
   DirectoryWatchEntry* back = 0, * next = 0;
   std::atomic_int32_t nref = 2;
   std::mutex lock;

   // Watch descriptor
   HANDLE hFile = 0;
   DWORD filter = 0;
   std::string path;
   Listener* listener = 0;

   // Log of changes
   BYTE dataBuf[1024];
   DWORD dataSize;

   DirectoryWatchEntry(Listener* listener, std::string& path)
      : listener(listener), path(path) {
   }
   ~DirectoryWatchEntry() {
      TRACE_WatchPool(std::cout << ">>>> Delete Watch <<<<" << std::endl);
      _ASSERT(this->listener == 0 && this->nref == 0);
      this->closeFile();
   }

   DirectoryWatchEntry* retain() {
      this->nref++;
      return this;
   }
   void release() {
      if (--this->nref == 0) delete this;
   }

   bool openFile(HANDLE hFile, DWORD filter) {
      _ASSERT(hFile);
      this->hFile = hFile;
      this->filter = filter;
      return this->listenFile();
   }
   bool listenFile() {
      return ReadDirectoryChangesW(this->hFile, this->dataBuf, sizeof(this->dataBuf), true, this->filter, &this->dataSize, &this->overlapped, NULL);
   }
   void closeFile() {
      if (this->hFile) {
         TRACE_WatchPool(std::cout << ">> Close file" << std::endl);
         CloseHandle(this->hFile);
         this->hFile = 0;
      }
   }

   void cancel() {
      TRACE_WatchPool(std::cout << ">> Cancel Watch" << std::endl);
      if (this->hFile) CancelIo(this->hFile);
      std::lock_guard<std::mutex> guard(this->lock);
      this->listener = NULL;
      this->release();
   }
   void execute() {
      TRACE_WatchPool(std::cout << ">> Execute Watch" << std::endl);

      // Create change log
      auto log = new DirectoryChangeLog(this);
      auto info = PFILE_NOTIFY_INFORMATION(this->dataBuf);
      LPBYTE dataEnd = this->dataBuf + sizeof(this->dataBuf);
      log->entry = this;
      while (LPBYTE(info) < dataEnd && LPBYTE(&info->FileName[info->FileNameLength / sizeof(wchar_t)]) < dataEnd) {
         FileChange change;
         switch (info->Action) {
         case FILE_ACTION_ADDED:
         case FILE_ACTION_RENAMED_NEW_NAME:
            change.status = FileChangeStatus::Added;
            break;
         case FILE_ACTION_REMOVED:
         case FILE_ACTION_RENAMED_OLD_NAME:
            change.status = FileChangeStatus::Removed;
            break;
         case FILE_ACTION_MODIFIED:
            change.status = FileChangeStatus::Modified;
            break;
         }
         change.relativePath = std::wstring(info->FileName, info->FileNameLength / sizeof(wchar_t));
         log->changes.push_back(change);
         if (info->NextEntryOffset == 0UL) break;
         info = PFILE_NOTIFY_INFORMATION(LPBYTE(info) + info->NextEntryOffset);
      }

      // Dispatch change log
      std::unique_lock<std::mutex> guard(this->lock);
      if (this->listener) {
         this->listener->ReceiveChangeAsync(log);
         if (this->listenFile()) return;
      }

      // When listening is canceled
      guard.unlock();
      this->closeFile();
      this->release();
      delete log;
   }
};

struct WatchPool {

   std::mutex lock;
   DirectoryWatchEntry* entries = 0;
   std::thread* thread = 0;
   HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

   HANDLE CreateListenableFileHandle(std::string& path) {
      HANDLE hFile = CreateFileA(
         path.c_str(),
         FILE_LIST_DIRECTORY,
         FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, 0,
         OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, 0
      );
      if (hFile) {
         if (!CreateIoCompletionPort(hFile, this->hIOCP, (ULONG_PTR)hFile, 0)) {
            CloseHandle(hFile); // Cannot link to completion port
            hFile = NULL;
         }
      }
      return hFile;
   }
   bool watch(Listener* listener, std::string path) {
      HANDLE hFile = this->CreateListenableFileHandle(path);
      if (!hFile) return false;

      auto entry = new DirectoryWatchEntry(listener, path);
      DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;
      if (entry->openFile(hFile, filter)) {
         std::lock_guard<std::mutex> guard(lock);
         entry->next = this->entries;
         if (this->entries) this->entries->back = entry;
         this->entries = entry;
         if (!thread) thread = new std::thread([this]() {this->loop(); });
         return true;
      }
      else {
         delete entry;
         return false;
      }
   }
   void unwatch(Listener* listener) {
      std::lock_guard<std::mutex> guard(lock);
      for (auto entry = this->entries; entry; ) {
         auto current = entry;
         entry = entry->next;
         if (current->listener == listener) {
            if (current->next) current->next->back = current->back;
            if (current->back) current->back->next = current->next;
            else this->entries = current->next;
            current->cancel();
         }
      }
   }
   void loop() {
      ULONG_PTR completionKey = NULL;
      LPOVERLAPPED overlapped = NULL;
      DWORD bytesTransferred = 0;
      while (GetQueuedCompletionStatus(this->hIOCP, &bytesTransferred, &completionKey, &overlapped, INFINITE)) {
         auto entry = (DirectoryWatchEntry*)overlapped;
         entry->execute();
      }
   }

};

static WatchPool g_watchPool;

DirectoryChangeLog::DirectoryChangeLog(DirectoryWatchEntry* entry)
   : entry(entry->retain()) {
}

DirectoryChangeLog::~DirectoryChangeLog() {
   this->entry->release();
}

std::string& DirectoryChangeLog::getRootPath() {
   return this->entry->path;
}


Listener::~Listener() {
   this->Unwatch();
}

bool Listener::Watch(std::string rootPath) {
   return g_watchPool.watch(this, rootPath);
}

void Listener::Unwatch() {
   g_watchPool.unwatch(this);
}

void Listener::ReceiveChangeAsync(DirectoryChangeLog* log) {
   auto getStatusName = [](FileChangeStatus status) {
      switch (status)
      {
      case FileChangeStatus::Added:
         return "Added";
      case FileChangeStatus::Removed:
         return "Removed";
      case FileChangeStatus::Modified:
         return "Modified";
      default:
         return "?";
      }
   };
   for (auto& change : log->changes) {
      printf("[%s] %.*S\n", getStatusName(change.status), change.relativePath.length(), change.relativePath.c_str());
   }
   delete log;
}
