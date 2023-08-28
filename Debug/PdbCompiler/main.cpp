// Dia2Dump.cpp : Defines the entry point for the console application.
//
// This is a part of the Debug Interface Access SDK
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// This source code is only intended as a supplement to the
// Debug Interface Access SDK and related electronic documentation
// provided with the library.
// See these sources for detailed information regarding the
// Debug Interface Access SDK API.
//

#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>
#include "dia2.h"
#include "Dia2Dump.h"
#include "PrintSymbol.h"
#include <wchar.h>

#include "Callback.h"

void DumpOut(const char* name);

class DiaString {
   BSTR pStr;
public:
   DiaString(BSTR pStr = 0)
      : pStr(pStr) {
   }
   ~DiaString() {
      if (this->pStr) {
         SysFreeString(this->pStr);
         this->pStr = 0;
      }
   }
   bool operator == (const wchar_t* str) const {
      return !wcscmp(this->pStr ? this->pStr : L"", str);
   }
   const wchar_t* str() const {
      return this->pStr;
   }
};

class DiaSymbol {
   IDiaSymbol* pSymbol;
public:
   DiaSymbol(IDiaSymbol* pSymbol = 0)
      : pSymbol(pSymbol) {
   }
   DiaSymbol(const DiaSymbol& pOther) : pSymbol(*pOther) {
      pSymbol->AddRef();
   }
   ~DiaSymbol() {
      if (this->pSymbol) {
         this->pSymbol->Release();
         this->pSymbol = 0;
      }
   }
   enum SymTagEnum GetTag() const {
      DWORD dwSymTag;
      if (pSymbol->get_symTag(&dwSymTag) != S_OK) return SymTagNull;
      return (enum SymTagEnum)dwSymTag;
   }
   DiaString GetName() const {
      BSTR bstrName = 0;
      if (pSymbol->get_name(&bstrName) != S_OK) return 0;
      return bstrName;
   }
   DiaString GetUndecoratedName() const {
      BSTR bstrName = 0;
      if (pSymbol->get_undecoratedName(&bstrName) != S_OK) return 0;
      return bstrName;
   }
   DiaSymbol GetType() {
      IDiaSymbol* pType = 0;
      if (pSymbol->get_type(&pType) != S_OK) return DiaSymbol();
      return DiaSymbol(pType);
   }
   enum DataKind GetDataKind() {
      DWORD dwDataKind;
      if (pSymbol->get_dataKind(&dwDataKind) != S_OK) return DataIsUnknown;
      return (enum DataKind)dwDataKind;
   }
   IDiaSymbol* operator ->() const {
      return this->pSymbol;
   }
   IDiaSymbol* operator *() const {
      return this->pSymbol;
   }
   void operator = (IDiaSymbol* pNew) {
      if (this->pSymbol) this->pSymbol->Release();
      this->pSymbol = pNew;
   }
   operator bool() {
      return this->pSymbol != 0;
   }
   static DiaSymbol wrap(IDiaSymbol* pSymbol) {
      pSymbol->AddRef();
      return DiaSymbol(pSymbol);
   }
};

class DiaEnumerate {
   DiaSymbol& symbol;
   struct ending {};
public:
   struct iterator {
      IDiaEnumSymbols* pEnumChildren = 0;
      DiaSymbol current;
      ULONG celt = 0;
      iterator(DiaSymbol& symbol, enum SymTagEnum filter) {
         symbol->findChildren(filter, NULL, nsNone, &this->pEnumChildren);
         this->operator ++();
      }
      ~iterator() {
         this->pEnumChildren->Release();
      }
      DiaSymbol& operator ->() {
         return this->current;
      }
      DiaSymbol& operator *() {
         return this->current;
      }
      bool operator != (ending& value) {
         return this->current;
      }
      void operator ++() {
         IDiaSymbol* pChild;
         bool success = SUCCEEDED(pEnumChildren->Next(1, &pChild, &this->celt)) && (this->celt == 1);
         if (success) this->current = pChild;
         else this->current = 0;
      }
   };
   enum SymTagEnum filter = SymTagNull;
   DiaEnumerate(DiaSymbol& symbol, enum SymTagEnum filter = SymTagNull)
      : symbol(symbol), filter(filter) {
   }
   iterator begin() {
      return iterator(this->symbol, this->filter);
   }
   ending end() {
      return ending();
   }
};

bool IsObjectClass(DiaSymbol& symbol) {
   for (auto& child : DiaEnumerate(symbol, SymTagBaseClass)) {
      if (IsObjectClass(child)) {
         return true;
      }
   }
   if (symbol.GetName() == L"_aObject") {
      return true;
   }
   return false;
}

DiaSymbol GetObjectClassParent(DiaSymbol& symbol) {
   for (auto& child : DiaEnumerate(symbol, SymTagBaseClass)) {
      if (IsObjectClass(child)) {
         return child;
      }
   }
   return DiaSymbol();
}

void PrintClass(DiaSymbol& symbol) {
   auto parent = GetObjectClassParent(symbol);
   auto parentName = parent ? parent.GetName().str() : L"_root";
   wprintf(L"class %s (%s)\n", symbol.GetName().str(), parentName);

   for (auto& field : DiaEnumerate(symbol, SymTagData)) {
      auto dwDataKind = field.GetDataKind();
      if (dwDataKind == DataIsMember) {
         wprintf(L"   ");
         PrintName(*field);
         if (auto ftype = field.GetType()) {
            wprintf(L": ");
            PrintType(*ftype);
         }

         //wprintf(L" [%s] ", rgDataKind[dwDataKind]);
         PrintLocation(*field);

         putwchar(L'\n');
      }
   }
   wprintf(L"endClass\n");
}

void CompileStructMap(DiaSymbol& global) {
   wprintf(L"\n\n*** CompileStructMap\n");

   for (auto& symbol : DiaEnumerate(global, SymTagUDT)) {
      if (IsObjectClass(symbol)) {
         PrintClass(symbol);
         putwchar(L'\n');
      }
   }
   putwchar(L'\n');
}

////////////////////////////////////////////////////////////
//
int wmain(int argc, wchar_t* argv[])
{
   IDiaDataSource* g_pDiaDataSource;
   auto g_szFilename = argv[argc - 1];

   if (!LoadDataFromPdb(g_szFilename, &g_pDiaDataSource, &g_pDiaSession, &g_pGlobalSymbol)) {
      return -1;
   }
   {
      auto global = DiaSymbol::wrap(g_pGlobalSymbol);

      DumpOut("CompileStructMap");
      CompileStructMap(global);
   }
   Cleanup();

   return 0;
}
