#pragma once
#include <string>
#include <vector>

struct DirectoryWatchEntry;
struct DirectoryChangeLog;

enum class FileChangeStatus {
   Added,
   Removed,
   Modified,
};

struct FileChange {
   FileChangeStatus status;
   std::wstring relativePath;
};

struct DirectoryChangeLog {

   DirectoryWatchEntry* entry;
   std::vector<FileChange> changes;

   std::string& getRootPath();

   DirectoryChangeLog(DirectoryWatchEntry* entry);
   ~DirectoryChangeLog();
};

struct Listener {
   ~Listener();
   bool Watch(std::string rootPath);
   void Unwatch();
   virtual void ReceiveChangeAsync(DirectoryChangeLog* log);
};
