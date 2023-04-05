#pragma once
#include <iosfwd>
#include <string>

#include <windows.h>
#include <dbghelp.h>

class SimpleStackWalker {
public:
   SimpleStackWalker(HANDLE hProcess);
   ~SimpleStackWalker();

   /** Provide a stack trace for the specified thread in the target process */
   void stackTrace(HANDLE hThread, std::ostream& os);

   /** Wrppaer for SymGetTypeInfo */
   bool getTypeInfo(DWORD64 modBase, ULONG typeId, IMAGEHLP_SYMBOL_TYPE_INFO getType, PVOID pInfo) const;

   /** Wrapper for ReadProcessMemory */
   bool readMemory(LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize) const;

   /** Add type information to a name */
   void decorateName(std::string& name, DWORD64 modBase, DWORD typeIndex) const;

private:
   SimpleStackWalker(SimpleStackWalker const&) = delete;
   SimpleStackWalker& operator=(SimpleStackWalker const&) = delete;

   std::string addressToString(DWORD64 address) const;
   std::string inlineToString(DWORD64 address, DWORD inline_context) const;

   void showVariablesAt(std::ostream& os, const STACKFRAME64& stackFrame, const CONTEXT& context) const;
   void showInlineVariablesAt(std::ostream& os, const STACKFRAME64& stackFrame, const CONTEXT& context, DWORD inline_context) const;

   HANDLE hProcess;
};
