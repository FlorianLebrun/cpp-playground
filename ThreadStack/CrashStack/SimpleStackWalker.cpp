#define _CRT_SECURE_NO_WARNINGS

#include "SimpleStackWalker.h"

#include <windows.h>

#pragma comment(lib, "oleaut32")

#include <cstddef>
#include <iostream>
#include <sstream>
#include <vector>

#include "cvconst.h"
#include <dbghelp.h>
#pragma comment(lib, "dbghelp")

#ifndef _M_X64
#error                                       \
    "This code only supports the X64 environment"
#endif // _M_X64


/**
 * Convert NUL terminated wide string to
 * string
 */
std::string  strFromWchar(wchar_t const* const wString)
{
   size_t const len = wcslen(wString) + 1;
   size_t const nBytes = len * sizeof(wchar_t);
   std::vector<char> chArray(nBytes);
   wcstombs(&chArray[0], wString, nBytes);
   return std::string(&chArray[0]);
}

std::string getBaseType(DWORD baseType, ULONG64 length);

struct EnumLocalCallBack {
   static BOOL CALLBACK enumSymbolsProc(PSYMBOL_INFO pSymInfo, ULONG /*SymbolSize*/, PVOID UserContext)
   {
      auto& self = *(EnumLocalCallBack*)UserContext;
      self.enumSymbols(*pSymInfo);
      return TRUE;
   }

   EnumLocalCallBack(const SimpleStackWalker& eng, std::ostream& opf, const STACKFRAME64& stackFrame, const CONTEXT& context)
      : eng(eng), opf(opf), stackFrame(stackFrame), context(context) {
   }

   void enumSymbols(const SYMBOL_INFO& symInfo) const;

private:
   const SimpleStackWalker& eng;
   std::ostream& opf;
   const STACKFRAME64& stackFrame;
   const CONTEXT& context;
};

struct RegInfo {
   RegInfo(std::string name, DWORD64 value)
      : name(std::move(name)), value(value) {}
   std::string name;
   DWORD64 value;
};

RegInfo getRegInfo(ULONG reg,
   const CONTEXT& context);

SimpleStackWalker::SimpleStackWalker(HANDLE hProcess)
   : hProcess(hProcess)
{
   DWORD dwOpts = SymGetOptions();
   dwOpts |= SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST;
   SymSetOptions(dwOpts);
   SymInitialize(hProcess, 0, true);
}

SimpleStackWalker::~SimpleStackWalker() {
   ::SymCleanup(hProcess);
}

std::string SimpleStackWalker::addressToString(DWORD64 address) const {
   std::ostringstream oss;

   // First the raw address
   oss << "0x" << (PVOID)address;

   // Then symbol, if any
   struct {
      SYMBOL_INFO symInfo;
      char name[4 * 256];
   } SymInfo = { {sizeof(SymInfo.symInfo)}, "" };

   PSYMBOL_INFO pSym = &SymInfo.symInfo;
   pSym->MaxNameLen = sizeof(SymInfo.name);

   DWORD64 uDisplacement(0);
   if (SymFromAddr(hProcess, address, &uDisplacement, pSym))
   {
      oss << " " << pSym->Name;
      if (uDisplacement != 0) {
         LONG_PTR displacement = static_cast<LONG_PTR>(uDisplacement);
         if (displacement < 0)
            oss << " - " << -displacement;
         else
            oss << " + " << displacement;
      }
   }

   // Finally any file/line number
   IMAGEHLP_LINE64 lineInfo = { sizeof(lineInfo) };
   DWORD dwDisplacement(0);
   if (SymGetLineFromAddr64(hProcess, address, &dwDisplacement, &lineInfo)) {
      oss << "   " << lineInfo.FileName << "(" << lineInfo.LineNumber << ")";
      if (dwDisplacement != 0) {
         oss << " + " << dwDisplacement << " byte" << (dwDisplacement == 1 ? "" : "s");
      }
   }

   return oss.str();
}

// Convert an inline address to a string
std::string SimpleStackWalker::inlineToString(DWORD64 address, DWORD inline_context) const {
   std::ostringstream oss;

   // First the raw address
   oss << "0x" << (PVOID)address;

   // Then symbol, if any
   struct {
      SYMBOL_INFO symInfo;
      char name[4 * 256];
   } SymInfo = { {sizeof(SymInfo.symInfo)}, "" };

   PSYMBOL_INFO pSym = &SymInfo.symInfo;
   pSym->MaxNameLen = sizeof(SymInfo.name);

   DWORD64 uDisplacement(0);
   if (SymFromInlineContext(hProcess, address, inline_context, &uDisplacement, pSym)) {
      oss << " " << pSym->Name;
      if (uDisplacement != 0) {
         LONG_PTR displacement = static_cast<LONG_PTR>(uDisplacement);
         if (displacement < 0)
            oss << " - " << -displacement;
         else
            oss << " + " << displacement;
      }
   }

   // Finally any file/line number
   IMAGEHLP_LINE64 lineInfo = { sizeof(lineInfo) };
   DWORD dwDisplacement(0);
   if (SymGetLineFromInlineContext(hProcess, address, inline_context, 0, &dwDisplacement, &lineInfo)) {
      oss << "   " << lineInfo.FileName << "(" << lineInfo.LineNumber << ")";
      if (dwDisplacement != 0) {
         oss << " + " << dwDisplacement << " byte" << (dwDisplacement == 1 ? "" : "s");
      }
   }

   return oss.str();
}

// StackTrace: try to trace the stack to the
// given output
void SimpleStackWalker::stackTrace(HANDLE hThread, std::ostream& os) {
   CONTEXT context = { 0 };
   STACKFRAME64 stackFrame = { 0 };

   context.ContextFlags = CONTEXT_FULL;
   GetThreadContext(hThread, &context);

   stackFrame.AddrPC.Offset = context.Rip;
   stackFrame.AddrPC.Mode = AddrModeFlat;

   stackFrame.AddrFrame.Offset = context.Rbp;
   stackFrame.AddrFrame.Mode = AddrModeFlat;

   stackFrame.AddrStack.Offset = context.Rsp;
   stackFrame.AddrStack.Mode = AddrModeFlat;

   os << "Frame               Code address\n";

   // Detect loops with optimised stackframes
   DWORD64 lastFrame = 0;

   while (::StackWalk64(IMAGE_FILE_MACHINE_AMD64, hProcess, hThread, &stackFrame, &context, nullptr, ::SymFunctionTableAccess64, ::SymGetModuleBase64, nullptr))
   {
      DWORD64 pc = stackFrame.AddrPC.Offset;
      DWORD64 frame = stackFrame.AddrFrame.Offset;
      if (pc == 0) {
         os << "Null address\n";
         break;
      }
      os << "0x" << (PVOID)frame << "  " << addressToString(pc) << "\n";
      if (lastFrame >= frame) {
         os << "Stack frame out of "            "sequence...\n";
         break;
      }
      lastFrame = frame;

      showVariablesAt(os, stackFrame, context);

      // Expand any inline frames
      if (const DWORD inline_count = SymAddrIncludeInlineTrace(hProcess, pc))
      {
         DWORD inline_context(0), frameIndex(0);
         if (SymQueryInlineTrace(hProcess, pc, 0, pc, pc, &inline_context, &frameIndex))
         {
            for (DWORD i = 0; i < inline_count; i++, inline_context++)
            {
               os << "-- inline frame --  " << inlineToString(pc, inline_context) << '\n';
               showInlineVariablesAt(os, stackFrame, context, inline_context);
            }
         }
      }
   }

   os.flush();
}

void SimpleStackWalker::showVariablesAt(std::ostream& os, const STACKFRAME64& stackFrame, const CONTEXT& context) const {

   IMAGEHLP_STACK_FRAME imghlp_frame = { 0 };
   imghlp_frame.InstructionOffset = stackFrame.AddrPC.Offset;
   SymSetContext(hProcess, &imghlp_frame, nullptr);

   EnumLocalCallBack callback(*this, os, stackFrame, context);
   SymEnumSymbolsEx(hProcess, 0, "*", EnumLocalCallBack::enumSymbolsProc, &callback, SYMENUM_OPTIONS_DEFAULT);
}

void SimpleStackWalker::showInlineVariablesAt(std::ostream& os, const STACKFRAME64& stackFrame, const CONTEXT& context, DWORD inline_context) const {
   if (SymSetScopeFromInlineContext(hProcess, stackFrame.AddrPC.Offset, inline_context))
   {
      EnumLocalCallBack callback(*this, os, stackFrame, context);
      SymEnumSymbolsEx(hProcess, 0, "*", EnumLocalCallBack::enumSymbolsProc, &callback, SYMENUM_OPTIONS_INLINE);
   }
}

// Helper for SymGetTypeInfo
bool SimpleStackWalker::getTypeInfo(DWORD64 modBase, ULONG typeId, IMAGEHLP_SYMBOL_TYPE_INFO getType, PVOID pInfo) const {
   return SymGetTypeInfo(hProcess, modBase, typeId, getType, pInfo);
}

// Helper for ReadProcessMemory
bool SimpleStackWalker::readMemory(LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize)   const {
   return ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, 0);
}

void SimpleStackWalker::decorateName(
   std::string& name, DWORD64 modBase,
   DWORD typeIndex) const {
   bool recurse{};
   enum SymTagEnum tag {};
   getTypeInfo(modBase, typeIndex,
      TI_GET_SYMTAG, &tag);
   switch (tag) {
   case SymTagUDT: {
      WCHAR* typeName{};
      if (getTypeInfo(modBase, typeIndex, TI_GET_SYMNAME, &typeName))
      {
         name.insert(0, " ");
         name.insert(0, strFromWchar(typeName));
         // We must free typeName
         LocalFree(typeName);
      }
      break;
   }
   case SymTagBaseType: {
      DWORD baseType{};
      ULONG64 length{};
      getTypeInfo(modBase, typeIndex, TI_GET_BASETYPE, &baseType);
      getTypeInfo(modBase, typeIndex, TI_GET_LENGTH, &length);
      name.insert(0, " ");
      name.insert(0, getBaseType(baseType, length));
      break;
   }
   case SymTagPointerType: {
      name.insert(0, "*");
      recurse = true;
      break;
   }
   case SymTagFunctionType: {
      if (name[0] == '*') {
         name.insert(0, "(");
         name += ")";
      }
      name += "()";
      recurse = true;
      break;
   }
   case SymTagArrayType: {
      if (name[0] == '*') {
         name.insert(0, "(");
         name += ")";
      }
      DWORD Count{};
      getTypeInfo(modBase, typeIndex, TI_GET_COUNT, &Count);
      name += "[";
      if (Count) {
         name += std::to_string(Count);
      }
      name += "]";
      recurse = true;
      break;
   }
   case SymTagFunction:
   case SymTagData:
      recurse = true;
      break;
   case SymTagBaseClass:
      break;
   default: {
      std::ostringstream oss;
      oss << "tag: " << tag << " ";
      name.insert(0, oss.str());
      break;
   }
   }

   if (recurse) {
      DWORD ti{};
      if (getTypeInfo(modBase, typeIndex, TI_GET_TYPEID, &ti)) {
         decorateName(name, modBase, ti);
      }
   }
}

void EnumLocalCallBack::enumSymbols(const SYMBOL_INFO& symInfo) const {
   if (!(symInfo.Flags & SYMFLAG_LOCAL)) {
      // Ignore anything not a local variable
      //return;
   }
   if (symInfo.Flags & SYMFLAG_NULL) {
      // Ignore 'NULL' objects
      //return;
   }
   std::string name(symInfo.Name, symInfo.NameLen);
   eng.decorateName(name, symInfo.ModBase, symInfo.TypeIndex);
   if ((symInfo.Flags & SYMFLAG_REGREL) || (symInfo.Flags & SYMFLAG_FRAMEREL)) {
      opf << "  " << name;

      const RegInfo reg_info = (symInfo.Flags & SYMFLAG_REGREL)
         ? getRegInfo(symInfo.Register, context)
         : RegInfo("frame", stackFrame.AddrFrame.Offset);
      if (reg_info.name.empty()) {
         opf << " [register '" << symInfo.Register << "']";
      }
      else {
         opf << " [" << reg_info.name;
         if (symInfo.Address > 0x7fffffff)
            opf << "-" << std::hex << -(int)symInfo.Address << std::dec;
         else
            opf << "+" << std::hex << (int)symInfo.Address << std::dec;
         opf << "]";

         if (symInfo.Size == sizeof(char)) {
            unsigned char data;
            eng.readMemory((PVOID)(reg_info.value + symInfo.Address), &data, sizeof(data));
            if (isprint(data))
               opf << " = '" << data << '\'';
            else
               opf << " = " << (int)data;
         }
         else if (symInfo.Size == sizeof(short)) {
            unsigned short data;
            eng.readMemory((PVOID)(reg_info.value + symInfo.Address), &data, sizeof(data));
            opf << " = " << data;
         }
         else if (symInfo.Size == sizeof(int)) {
            unsigned int data;
            eng.readMemory((PVOID)(reg_info.value + symInfo.Address), &data, sizeof(data));
            opf << " = 0x" << std::hex << data << std::dec;
         }
         else if ((symInfo.Size == 8) && (name.compare(0, 6, "double") == 0)) {
            double data;
            eng.readMemory((PVOID)(reg_info.value + symInfo.Address), &data, sizeof(data));
            opf << " = " << data;
         }
         else if ((symInfo.Size == 8) || (symInfo.Size == 0)) {
            LONGLONG data;
            eng.readMemory((PVOID)(reg_info.value + symInfo.Address), &data, sizeof(data));
            opf << " = 0x" << std::hex << data << std::dec;
         }
      }
      opf << std::endl;
   }
   else if (symInfo.Flags & SYMFLAG_REGISTER) {
      opf << "  " << name;
      const RegInfo reg_info = getRegInfo(symInfo.Register, context);
      if (reg_info.name.empty()) {
         opf << " (register '" << symInfo.Register << "\')";
      }
      else {
         opf << " (" << reg_info.name << ") = 0x" << std::hex << reg_info.value << std::dec;
      }
      opf << std::endl;
   }
   else {
      opf << "  " << name << " Flags: " << std::hex << symInfo.Flags << std::dec << std::endl;
   }
}


// Helper function: getBaseType maps PDB type
// + length to C++ name
std::string getBaseType(DWORD baseType, ULONG64 length)
{
   static struct {
      DWORD baseType;
      ULONG64 length;
      const char* name;
   } baseList[] = {
      // Table generated from dumping out
      // 'baseTypes.cpp'
      {btNoType, 0,
       "(null)"}, // Used for __$ReturnUdt
      {btVoid, 0, "void"},
      {btChar, sizeof(char), "char"},
      {btWChar, sizeof(wchar_t), "wchar_t"},
      {btInt, sizeof(signed char),
       "signed char"},
      {btInt, sizeof(short), "short"},
      {btInt, sizeof(int), "int"},
      {btInt, sizeof(__int64), "__int64"},
      {btUInt, sizeof(unsigned char),
       "unsigned char"}, // also used for
                         // 'bool' in VC6
      {btUInt, sizeof(unsigned short),
       "unsigned short"},
      {btUInt, sizeof(unsigned int),
       "unsigned int"},
      {btUInt, sizeof(unsigned __int64),
       "unsigned __int64"},
      {btFloat, sizeof(float), "float"},
      {btFloat, sizeof(double), "double"},
      {btFloat, sizeof(long double),
       "long double"},
       // btBCD
       {btBool, sizeof(bool),
        "bool"}, // VC 7.x
       {btLong, sizeof(long), "long"},
       {btULong, sizeof(unsigned long),
        "unsigned long"},
        // btCurrency
        // btDate
        // btVariant
        // btComplex
        // btBit
        // btBSTR
        {btHresult, sizeof(HRESULT), "HRESULT"},

   };

   for (int i = 0; i < sizeof(baseList) / sizeof(baseList[0]); ++i) {
      if ((baseType == baseList[i].baseType) && (length == baseList[i].length)) {
         return baseList[i].name;
      }
   }

   // Unlisted type - use the data values and
   // then fix the code (!)
   std::ostringstream oss;
   oss << "pdb type: " << baseType << "/" << (DWORD)length;
   return oss.str();
}

// Get register name and offset from the 'reg'
// value supplied
RegInfo getRegInfo(ULONG reg, const CONTEXT& context) {
#  define CASE(REG, NAME, SRC, MASK) case CV_AMD64_##REG: return RegInfo(NAME, context.SRC & MASK)

   switch (reg) {
      CASE(AL, "al", Rax, 0xff);
      CASE(BL, "bl", Rbx, 0xff);
      CASE(CL, "cl", Rcx, 0xff);
      CASE(DL, "dl", Rdx, 0xff);

      CASE(AX, "ax", Rax, 0xffff);
      CASE(BX, "bx", Rbx, 0xffff);
      CASE(CX, "cx", Rcx, 0xffff);
      CASE(DX, "dx", Rdx, 0xffff);
      CASE(SP, "sp", Rsp, 0xffff);
      CASE(BP, "bp", Rbp, 0xffff);
      CASE(SI, "si", Rsi, 0xffff);
      CASE(DI, "di", Rdi, 0xffff);

      CASE(EAX, "eax", Rax, ~0u);
      CASE(EBX, "ebx", Rbx, ~0u);
      CASE(ECX, "ecx", Rcx, ~0u);
      CASE(EDX, "edx", Rdx, ~0u);
      CASE(ESP, "esp", Rsp, ~0u);
      CASE(EBP, "ebp", Rbp, ~0u);
      CASE(ESI, "esi", Rsi, ~0u);
      CASE(EDI, "edi", Rdi, ~0u);

      CASE(RAX, "rax", Rax, ~0ull);
      CASE(RBX, "rbx", Rbx, ~0ull);
      CASE(RCX, "rcx", Rcx, ~0ull);
      CASE(RDX, "rdx", Rdx, ~0ull);
      CASE(RSP, "rsp", Rsp, ~0ull);
      CASE(RBP, "rbp", Rbp, ~0ull);
      CASE(RSI, "rsi", Rsi, ~0ull);
      CASE(RDI, "rdi", Rdi, ~0ull);

      CASE(R8, "r8", R8, ~0ull);
      CASE(R9, "r9", R9, ~0ull);
      CASE(R10, "r10", R10, ~0ull);
      CASE(R11, "r11", R11, ~0ull);
      CASE(R12, "r12", R12, ~0ull);
      CASE(R13, "r13", R13, ~0ull);
      CASE(R14, "r14", R14, ~0ull);
      CASE(R15, "r15", R15, ~0ull);

      CASE(R8B, "r8b", R8, 0xff);
      CASE(R9B, "r9b", R9, 0xff);
      CASE(R10B, "r10b", R10, 0xff);
      CASE(R11B, "r11b", R11, 0xff);
      CASE(R12B, "r12b", R12, 0xff);
      CASE(R13B, "r13b", R13, 0xff);
      CASE(R14B, "r14b", R14, 0xff);
      CASE(R15B, "r15b", R15, 0xff);

      CASE(R8W, "r8w", R8, 0xffff);
      CASE(R9W, "r8w", R8, 0xffff);
      CASE(R10W, "r10w", R10, 0xffff);
      CASE(R11W, "r10w", R10, 0xffff);
      CASE(R12W, "r10w", R10, 0xffff);
      CASE(R13W, "r10w", R10, 0xffff);
      CASE(R14W, "r10w", R10, 0xffff);
      CASE(R15W, "r10w", R10, 0xffff);

      CASE(R8D, "r8d", R8, ~0u);
      CASE(R9D, "r8d", R8, ~0u);
      CASE(R10D, "r10d", R10, ~0u);
      CASE(R11D, "r11d", R11, ~0u);
      CASE(R12D, "r12d", R12, ~0u);
      CASE(R13D, "r13d", R13, ~0u);
      CASE(R14D, "r14d", R14, ~0u);
      CASE(R15D, "r15d", R15, ~0u);
   }

#undef CASE

   return RegInfo("", 0);
}

