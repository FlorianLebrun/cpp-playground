
#include <iostream>
#include <Windows.h>
#include <dia2.h>
#include <cvconst.h>
#include "PrintSymbol.h"
#include "regs.h"

// Helper function to print the stack map table
void PrintStackMapTable(IDiaSymbol* pFunction) {
   IDiaEnumSymbols* pStackMapEnum;
   HRESULT hr = pFunction->findChildren(SymTagNull, NULL, nsNone, &pStackMapEnum);
   if (FAILED(hr)) {
      return;
   }

   IDiaSymbol* pChildSymbol = nullptr;
   ULONG celt = 0;

   wchar_t tmpBstrName[128];
   while (SUCCEEDED(pStackMapEnum->Next(1, &pChildSymbol, &celt)) && celt == 1) {
      BSTR bstrName = 0;

      std::wstring name = L"???";
      pChildSymbol->get_name(&bstrName);
      if (bstrName) {
         name = bstrName;
         SysFreeString(bstrName);
      }

      DWORD symTag;
      pChildSymbol->get_symTag(&symTag);
      if (symTag == SymTagData) {

         std::wcout << L"   > " << name << " ";
         PrintLocation(pChildSymbol);
         if (name[0] == L'$') {
            // La variable est anonyme/spilled.
            wprintf(L"Spilled variable: %s\n", name.c_str());
            //__debugbreak();
         }
         std::wcout << std::endl;
      }
      if (symTag == SymTagBlock) {
         std::wcout << L" -- Block " << name << " ";
         std::wcout << std::endl;
         PrintStackMapTable(pChildSymbol);
      }
      pChildSymbol = nullptr;
   }
}

int wmain(int argc, wchar_t* argv[]) {
   HRESULT hr = 0;
   IDiaDataSource* pSource;

   if (argc != 2) return 1;
   std::wstring filename(argv[1]);

   CoInitialize(NULL);

   hr = CoCreateInstance(__uuidof(DiaSource),
      NULL,
      CLSCTX_INPROC_SERVER,
      __uuidof(IDiaDataSource),
      (void**)&pSource);

   if (FAILED(hr)) {
      std::cerr << "Failed to create DiaSource instance." << std::endl;
      return 1;
   }

   hr = pSource->loadDataFromPdb(filename.c_str());

   if (FAILED(hr)) {
      std::cerr << "Failed to load PDB data." << std::endl;
      return 1;
   }

   IDiaSession* pSession;
   hr = pSource->openSession(&pSession);

   if (FAILED(hr)) {
      std::cerr << "Failed to open DIA session." << std::endl;
      return 1;
   }

   // Get global scope
   IDiaSymbol* pGlobal;
   hr = pSession->get_globalScope(&pGlobal);
   if (FAILED(hr)) {
      std::cerr << "Failed to get global scope." << std::endl;
      return 1;
   }

   // Enumerate functions
   IDiaEnumSymbols* pEnumFunctions;
   hr = pGlobal->findChildren(SymTagFunction, NULL, nsNone, &pEnumFunctions);
   if (FAILED(hr)) {
      std::cerr << "Failed to find function children." << std::endl;
      return 1;
   }

   IDiaSymbol* pFunction;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumFunctions->Next(1, &pFunction, &celt)) && celt == 1) {
      BSTR bstrName;
      if (SUCCEEDED(pFunction->get_name(&bstrName))) {
         std::wcout << L"Function: " << bstrName << std::endl;
         SysFreeString(bstrName);

         // Print the stack map table for the current function
         PrintStackMapTable(pFunction);
      }
      pFunction = nullptr;
   }

   return 0;
}