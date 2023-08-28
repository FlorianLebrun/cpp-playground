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
#include "dia2.h"
#include "Dia2Dump.h"
#include "PrintSymbol.h"

#include "Callback.h"

#pragma warning (disable : 4100)

IDiaSession* g_pDiaSession;
IDiaSymbol* g_pGlobalSymbol;
DWORD g_dwMachineType = CV_CFL_80386;

void DumpOut(const char* name) {
   TCHAR currentDir[MAX_PATH];
   if (!GetCurrentDirectory(MAX_PATH, currentDir)) throw;

   char path[MAX_PATH];
   sprintf(path, "%s\\%s.txt", currentDir, name);
   if (!freopen(path, "w", stdout)) throw;
}

////////////////////////////////////////////////////////////
// Create an IDiaData source and open a PDB file
//
bool LoadDataFromPdb(
   const wchar_t* szFilename,
   IDiaDataSource** ppSource,
   IDiaSession** ppSession,
   IDiaSymbol** ppGlobal)
{
   wchar_t wszExt[MAX_PATH];
   const wchar_t* wszSearchPath = L"SRV**\\\\symbols\\symbols"; // Alternate path to search for debug data
   DWORD dwMachType = 0;

   HRESULT hr = CoInitialize(NULL);

   // Obtain access to the provider

   hr = CoCreateInstance(__uuidof(DiaSource),
      NULL,
      CLSCTX_INPROC_SERVER,
      __uuidof(IDiaDataSource),
      (void**)ppSource);

   if (FAILED(hr)) {
      wprintf(L"CoCreateInstance failed - HRESULT = %08X\n", hr);

      return false;
   }

   _wsplitpath_s(szFilename, NULL, 0, NULL, 0, NULL, 0, wszExt, MAX_PATH);

   if (!_wcsicmp(wszExt, L".pdb")) {
      // Open and prepare a program database (.pdb) file as a debug data source

      hr = (*ppSource)->loadDataFromPdb(szFilename);

      if (FAILED(hr)) {
         wprintf(L"loadDataFromPdb failed - HRESULT = %08X\n", hr);

         return false;
      }
   }

   else {
      CCallback callback; // Receives callbacks from the DIA symbol locating procedure,
                          // thus enabling a user interface to report on the progress of
                          // the location attempt. The client application may optionally
                          // provide a reference to its own implementation of this
                          // virtual base class to the IDiaDataSource::loadDataForExe method.
      callback.AddRef();

      // Open and prepare the debug data associated with the executable

      hr = (*ppSource)->loadDataForExe(szFilename, wszSearchPath, &callback);

      if (FAILED(hr)) {
         wprintf(L"loadDataForExe failed - HRESULT = %08X\n", hr);

         return false;
      }
   }

   // Open a session for querying symbols

   hr = (*ppSource)->openSession(ppSession);

   if (FAILED(hr)) {
      wprintf(L"openSession failed - HRESULT = %08X\n", hr);

      return false;
   }

   // Retrieve a reference to the global scope

   hr = (*ppSession)->get_globalScope(ppGlobal);

   if (hr != S_OK) {
      wprintf(L"get_globalScope failed\n");

      return false;
   }

   // Set Machine type for getting correct register names

   if ((*ppGlobal)->get_machineType(&dwMachType) == S_OK) {
      switch (dwMachType) {
      case IMAGE_FILE_MACHINE_I386: g_dwMachineType = CV_CFL_80386; break;
      case IMAGE_FILE_MACHINE_IA64: g_dwMachineType = CV_CFL_IA64; break;
      case IMAGE_FILE_MACHINE_AMD64: g_dwMachineType = CV_CFL_AMD64; break;
      }
   }

   return true;
}

////////////////////////////////////////////////////////////
// Release DIA objects and CoUninitialize
//
void Cleanup()
{
   if (g_pGlobalSymbol) {
      g_pGlobalSymbol->Release();
      g_pGlobalSymbol = NULL;
   }

   if (g_pDiaSession) {
      g_pDiaSession->Release();
      g_pDiaSession = NULL;
   }

   CoUninitialize();
}

////////////////////////////////////////////////////////////
// Dump all the data stored in a PDB
//
void DumpAllPdbInfo(IDiaSession* pSession, IDiaSymbol* pGlobal)
{
   DumpOut("DumpAllMods");
   DumpAllMods(pGlobal);
   DumpOut("DumpAllPublics");
   DumpAllPublics(pGlobal);
   DumpOut("DumpAllSymbols");
   DumpAllSymbols(pGlobal);
   DumpOut("DumpAllGlobals");
   DumpAllGlobals(pGlobal);
   DumpOut("DumpAllTypes");
   DumpAllTypes(pGlobal);
   DumpOut("DumpAllFiles");
   DumpAllFiles(pSession, pGlobal);
   DumpOut("DumpAllLines");
   DumpAllLines(pSession, pGlobal);
   DumpOut("DumpAllSecContribs");
   DumpAllSecContribs(pSession);
   DumpOut("DumpAllDebugStreams");
   DumpAllDebugStreams(pSession);
   DumpOut("DumpAllInjectedSources");
   DumpAllInjectedSources(pSession);
   DumpOut("DumpAllFPO");
   DumpAllFPO(pSession);
   DumpOut("DumpAllOEMs");
   DumpAllOEMs(pGlobal);
   DumpOut("DumpLog");
}

////////////////////////////////////////////////////////////
// Dump all the modules information
//
bool DumpAllMods(IDiaSymbol* pGlobal)
{
   wprintf(L"\n\n*** MODULES\n\n");

   // Retrieve all the compiland symbols

   IDiaEnumSymbols* pEnumSymbols;

   if (FAILED(pGlobal->findChildren(SymTagCompiland, NULL, nsNone, &pEnumSymbols))) {
      return false;
   }

   IDiaSymbol* pCompiland;
   ULONG celt = 0;
   ULONG iMod = 1;

   while (SUCCEEDED(pEnumSymbols->Next(1, &pCompiland, &celt)) && (celt == 1)) {
      BSTR bstrName;

      if (pCompiland->get_name(&bstrName) != S_OK) {
         wprintf(L"ERROR - Failed to get the compiland's name\n");

         pCompiland->Release();
         pEnumSymbols->Release();

         return false;
      }

      wprintf(L"%04X %s\n", iMod++, bstrName);

      // Deallocate the string allocated previously by the call to get_name

      SysFreeString(bstrName);

      pCompiland->Release();
   }

   pEnumSymbols->Release();

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump all the public symbols - SymTagPublicSymbol
//
bool DumpAllPublics(IDiaSymbol* pGlobal)
{
   wprintf(L"\n\n*** PUBLICS\n\n");

   // Retrieve all the public symbols

   IDiaEnumSymbols* pEnumSymbols;

   if (FAILED(pGlobal->findChildren(SymTagPublicSymbol, NULL, nsNone, &pEnumSymbols))) {
      return false;
   }

   IDiaSymbol* pSymbol;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
      PrintPublicSymbol(pSymbol);

      pSymbol->Release();
   }

   pEnumSymbols->Release();

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump all the symbol information stored in the compilands
//
bool DumpAllSymbols(IDiaSymbol* pGlobal)
{
   wprintf(L"\n\n*** SYMBOLS\n\n\n");

   // Retrieve the compilands first

   IDiaEnumSymbols* pEnumSymbols;

   if (FAILED(pGlobal->findChildren(SymTagCompiland, NULL, nsNone, &pEnumSymbols))) {
      return false;
   }

   IDiaSymbol* pCompiland;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSymbols->Next(1, &pCompiland, &celt)) && (celt == 1)) {
      wprintf(L"\n** Module: ");

      // Retrieve the name of the module

      BSTR bstrName;

      if (pCompiland->get_name(&bstrName) != S_OK) {
         wprintf(L"(???)\n\n");
      }

      else {
         wprintf(L"%s\n\n", bstrName);

         SysFreeString(bstrName);
      }

      // Find all the symbols defined in this compiland and print their info

      IDiaEnumSymbols* pEnumChildren;

      if (SUCCEEDED(pCompiland->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren))) {
         IDiaSymbol* pSymbol;
         ULONG celtChildren = 0;

         while (SUCCEEDED(pEnumChildren->Next(1, &pSymbol, &celtChildren)) && (celtChildren == 1)) {
            PrintSymbol(pSymbol, 0);
            pSymbol->Release();
         }

         pEnumChildren->Release();
      }

      pCompiland->Release();
   }

   pEnumSymbols->Release();

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump all the global symbols - SymTagFunction,
//  SymTagThunk and SymTagData
//
bool DumpAllGlobals(IDiaSymbol* pGlobal)
{
   IDiaEnumSymbols* pEnumSymbols;
   IDiaSymbol* pSymbol;
   enum SymTagEnum dwSymTags[] = { SymTagFunction, SymTagThunk, SymTagData };
   ULONG celt = 0;

   wprintf(L"\n\n*** GLOBALS\n\n");

   for (size_t i = 0; i < _countof(dwSymTags); i++, pEnumSymbols = NULL) {
      if (SUCCEEDED(pGlobal->findChildren(dwSymTags[i], NULL, nsNone, &pEnumSymbols))) {
         while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
            PrintGlobalSymbol(pSymbol);

            pSymbol->Release();
         }

         pEnumSymbols->Release();
      }

      else {
         wprintf(L"ERROR - DumpAllGlobals() returned no symbols\n");

         return false;
      }
   }

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump all the types information
//  (type symbols can be UDTs, enums or typedefs)
//
bool DumpAllTypes(IDiaSymbol* pGlobal)
{
   wprintf(L"\n\n*** TYPES\n");

   bool f1 = DumpAllUDTs(pGlobal);
   bool f2 = DumpAllEnums(pGlobal);
   bool f3 = DumpAllTypedefs(pGlobal);

   return f1 && f2 && f3;
}

////////////////////////////////////////////////////////////
// Dump all the user defined types (UDT)
//
bool DumpAllUDTs(IDiaSymbol* pGlobal)
{
   wprintf(L"\n\n** User Defined Types\n\n");

   IDiaEnumSymbols* pEnumSymbols;

   if (FAILED(pGlobal->findChildren(SymTagUDT, NULL, nsNone, &pEnumSymbols))) {
      wprintf(L"ERROR - DumpAllUDTs() returned no symbols\n");

      return false;
   }

   IDiaSymbol* pSymbol;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
      PrintTypeInDetail(pSymbol, 0);

      pSymbol->Release();
   }

   pEnumSymbols->Release();

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump all the enum types from the pdb
//
bool DumpAllEnums(IDiaSymbol* pGlobal)
{
   wprintf(L"\n\n** ENUMS\n\n");

   IDiaEnumSymbols* pEnumSymbols;

   if (FAILED(pGlobal->findChildren(SymTagEnum, NULL, nsNone, &pEnumSymbols))) {
      wprintf(L"ERROR - DumpAllEnums() returned no symbols\n");

      return false;
   }

   IDiaSymbol* pSymbol;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
      PrintTypeInDetail(pSymbol, 0);

      pSymbol->Release();
   }

   pEnumSymbols->Release();

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump all the typedef types from the pdb
//
bool DumpAllTypedefs(IDiaSymbol* pGlobal)
{
   wprintf(L"\n\n** TYPEDEFS\n\n");

   IDiaEnumSymbols* pEnumSymbols;

   if (FAILED(pGlobal->findChildren(SymTagTypedef, NULL, nsNone, &pEnumSymbols))) {
      wprintf(L"ERROR - DumpAllTypedefs() returned no symbols\n");

      return false;
   }

   IDiaSymbol* pSymbol;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
      PrintTypeInDetail(pSymbol, 0);

      pSymbol->Release();
   }

   pEnumSymbols->Release();

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump OEM specific types
//
bool DumpAllOEMs(IDiaSymbol* pGlobal)
{
   wprintf(L"\n\n*** OEM Specific types\n\n");

   IDiaEnumSymbols* pEnumSymbols;

   if (FAILED(pGlobal->findChildren(SymTagCustomType, NULL, nsNone, &pEnumSymbols))) {
      wprintf(L"ERROR - DumpAllOEMs() returned no symbols\n");

      return false;
   }

   IDiaSymbol* pSymbol;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
      PrintTypeInDetail(pSymbol, 0);

      pSymbol->Release();
   }

   pEnumSymbols->Release();

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// For each compiland in the PDB dump all the source files
//
bool DumpAllFiles(IDiaSession* pSession, IDiaSymbol* pGlobal)
{
   wprintf(L"\n\n*** FILES\n\n");

   // In order to find the source files, we have to look at the image's compilands/modules

   IDiaEnumSymbols* pEnumSymbols;

   if (FAILED(pGlobal->findChildren(SymTagCompiland, NULL, nsNone, &pEnumSymbols))) {
      return false;
   }

   IDiaSymbol* pCompiland;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSymbols->Next(1, &pCompiland, &celt)) && (celt == 1)) {
      BSTR bstrName;

      if (pCompiland->get_name(&bstrName) == S_OK) {
         wprintf(L"\nCompiland = %s\n\n", bstrName);

         SysFreeString(bstrName);
      }

      // Every compiland could contain multiple references to the source files which were used to build it
      // Retrieve all source files by compiland by passing NULL for the name of the source file

      IDiaEnumSourceFiles* pEnumSourceFiles;

      if (SUCCEEDED(pSession->findFile(pCompiland, NULL, nsNone, &pEnumSourceFiles))) {
         IDiaSourceFile* pSourceFile;

         while (SUCCEEDED(pEnumSourceFiles->Next(1, &pSourceFile, &celt)) && (celt == 1)) {
            PrintSourceFile(pSourceFile);
            putwchar(L'\n');

            pSourceFile->Release();
         }

         pEnumSourceFiles->Release();
      }

      putwchar(L'\n');

      pCompiland->Release();
   }

   pEnumSymbols->Release();

   return true;
}

////////////////////////////////////////////////////////////
// Dump all the line numbering information contained in the PDB
//
bool DumpAllLines(IDiaSession* pSession, IDiaSymbol* pGlobal)
{
   wprintf(L"\n\n*** LINES\n\n");

   IDiaEnumSectionContribs* pEnumSecContribs;

   if (FAILED(GetTable(pSession, __uuidof(IDiaEnumSectionContribs), (void**)&pEnumSecContribs))) {
      return false;
   }

   IDiaSectionContrib* pSecContrib;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSecContribs->Next(1, &pSecContrib, &celt)) && (celt == 1)) {

      DWORD dwRVA;
      DWORD dwLength;

      if (SUCCEEDED(pSecContrib->get_relativeVirtualAddress(&dwRVA)) &&
         SUCCEEDED(pSecContrib->get_length(&dwLength))) {

         DumpAllLines(pSession, dwRVA, dwLength);
      }

      pSecContrib->Release();
   }

   pEnumSecContribs->Release();

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump all the line numbering information for a given RVA
// and a given range
//
bool DumpAllLines(IDiaSession* pSession, DWORD dwRVA, DWORD dwRange)
{
   // Retrieve and print the lines that corresponds to a specified RVA

   IDiaEnumLineNumbers* pLines;

   if (FAILED(pSession->findLinesByRVA(dwRVA, dwRange, &pLines))) {
      return false;
   }

   PrintLines(pLines);

   pLines->Release();

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump all the section contributions from the PDB
//
//  Section contributions are stored in a table which will
//  be retrieved via IDiaSession->getEnumTables through
//  QueryInterface()using the REFIID of the IDiaEnumSectionContribs
//
bool DumpAllSecContribs(IDiaSession* pSession)
{
   wprintf(L"\n\n*** SECTION CONTRIBUTION\n\n");

   IDiaEnumSectionContribs* pEnumSecContribs;

   if (FAILED(GetTable(pSession, __uuidof(IDiaEnumSectionContribs), (void**)&pEnumSecContribs))) {
      return false;
   }

   wprintf(L"    RVA        Address       Size    Module\n");

   IDiaSectionContrib* pSecContrib;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSecContribs->Next(1, &pSecContrib, &celt)) && (celt == 1)) {
      PrintSecContribs(pSecContrib);

      pSecContrib->Release();
   }

   pEnumSecContribs->Release();

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump all debug data streams contained in the PDB
//
bool DumpAllDebugStreams(IDiaSession* pSession)
{
   IDiaEnumDebugStreams* pEnumStreams;

   wprintf(L"\n\n*** DEBUG STREAMS\n\n");

   // Retrieve an enumerated sequence of debug data streams

   if (FAILED(pSession->getEnumDebugStreams(&pEnumStreams))) {
      return false;
   }

   IDiaEnumDebugStreamData* pStream;
   ULONG celt = 0;

   for (; SUCCEEDED(pEnumStreams->Next(1, &pStream, &celt)) && (celt == 1); pStream = NULL) {
      PrintStreamData(pStream);

      pStream->Release();
   }

   pEnumStreams->Release();

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump all the injected source from the PDB
//
//  Injected sources data is stored in a table which will
//  be retrieved via IDiaSession->getEnumTables through
//  QueryInterface()using the REFIID of the IDiaEnumSectionContribs
//
bool DumpAllInjectedSources(IDiaSession* pSession)
{
   wprintf(L"\n\n*** INJECTED SOURCES TABLE\n\n");

   IDiaEnumInjectedSources* pEnumInjSources = NULL;

   if (FAILED(GetTable(pSession, __uuidof(IDiaEnumInjectedSources), (void**)&pEnumInjSources))) {
      return false;
   }

   IDiaInjectedSource* pInjSource;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumInjSources->Next(1, &pInjSource, &celt)) && (celt == 1)) {
      PrintGeneric(pInjSource);

      pInjSource->Release();
   }

   pEnumInjSources->Release();

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump info corresponing to a given injected source filename
//
bool DumpInjectedSource(IDiaSession* pSession, const wchar_t* szName)
{
   // Retrieve a source that has been placed into the symbol store by attribute providers or
   //  other components of the compilation process

   IDiaEnumInjectedSources* pEnumInjSources;

   if (FAILED(pSession->findInjectedSource(szName, &pEnumInjSources))) {
      wprintf(L"ERROR - DumpInjectedSources() could not find %s\n", szName);

      return false;
   }

   IDiaInjectedSource* pInjSource;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumInjSources->Next(1, &pInjSource, &celt)) && (celt == 1)) {
      PrintGeneric(pInjSource);

      pInjSource->Release();
   }

   pEnumInjSources->Release();

   return true;
}

////////////////////////////////////////////////////////////
// Dump all the source file information stored in the PDB
// We have to go through every compiland in order to retrieve
//   all the information otherwise checksums for instance
//   will not be available
// Compilands can have multiple source files with the same
//   name but different content which produces diffrent
//   checksums
//
bool DumpAllSourceFiles(IDiaSession* pSession, IDiaSymbol* pGlobal)
{
   wprintf(L"\n\n*** SOURCE FILES\n\n");

   // To get the complete source file info we must go through the compiland first
   // by passing NULL instead all the source file names only will be retrieved

   IDiaEnumSymbols* pEnumSymbols;

   if (FAILED(pGlobal->findChildren(SymTagCompiland, NULL, nsNone, &pEnumSymbols))) {
      return false;
   }

   IDiaSymbol* pCompiland;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSymbols->Next(1, &pCompiland, &celt)) && (celt == 1)) {
      BSTR bstrName;

      if (pCompiland->get_name(&bstrName) == S_OK) {
         wprintf(L"\nCompiland = %s\n\n", bstrName);

         SysFreeString(bstrName);
      }

      // Every compiland could contain multiple references to the source files which were used to build it
      // Retrieve all source files by compiland by passing NULL for the name of the source file

      IDiaEnumSourceFiles* pEnumSourceFiles;

      if (SUCCEEDED(pSession->findFile(pCompiland, NULL, nsNone, &pEnumSourceFiles))) {
         IDiaSourceFile* pSourceFile;

         while (SUCCEEDED(pEnumSourceFiles->Next(1, &pSourceFile, &celt)) && (celt == 1)) {
            PrintSourceFile(pSourceFile);
            putwchar(L'\n');

            pSourceFile->Release();
         }

         pEnumSourceFiles->Release();
      }

      putwchar(L'\n');

      pCompiland->Release();
   }

   pEnumSymbols->Release();

   return true;
}

////////////////////////////////////////////////////////////
// Dump all the FPO info
//
//  FPO data stored in a table which will be retrieved via
//    IDiaSession->getEnumTables through QueryInterface()
//    using the REFIID of the IDiaEnumFrameData
//
bool DumpAllFPO(IDiaSession* pSession)
{
   IDiaEnumFrameData* pEnumFrameData;

   wprintf(L"\n\n*** FPO\n\n");

   if (FAILED(GetTable(pSession, __uuidof(IDiaEnumFrameData), (void**)&pEnumFrameData))) {
      return false;
   }

   IDiaFrameData* pFrameData;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumFrameData->Next(1, &pFrameData, &celt)) && (celt == 1)) {
      PrintFrameData(pFrameData);

      pFrameData->Release();
   }

   pEnumFrameData->Release();

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump FPO info for a function at the specified RVA
//
bool DumpFPO(IDiaSession* pSession, DWORD dwRVA)
{
   IDiaEnumFrameData* pEnumFrameData;

   // Retrieve first the table holding all the FPO info

   if ((dwRVA != 0) && SUCCEEDED(GetTable(pSession, __uuidof(IDiaEnumFrameData), (void**)&pEnumFrameData))) {
      IDiaFrameData* pFrameData;

      // Retrieve the frame data corresponding to the given RVA

      if (SUCCEEDED(pEnumFrameData->frameByRVA(dwRVA, &pFrameData))) {
         PrintGeneric(pFrameData);

         pFrameData->Release();
      }

      else {
         // Some function might not have FPO data available (see ASM funcs like strcpy)

         wprintf(L"ERROR - DumpFPO() frameByRVA invalid RVA: 0x%08X\n", dwRVA);

         pEnumFrameData->Release();

         return false;
      }

      pEnumFrameData->Release();
   }

   else {
      wprintf(L"ERROR - DumpFPO() GetTable\n");

      return false;
   }

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump FPO info for a specified function symbol using its
//  name (a regular expression string is used for the search)
//
bool DumpFPO(IDiaSession* pSession, IDiaSymbol* pGlobal, const wchar_t* szSymbolName)
{
   IDiaEnumSymbols* pEnumSymbols;
   IDiaSymbol* pSymbol;
   ULONG celt = 0;
   DWORD dwRVA;

   // Find first all the function symbols that their names matches the search criteria

   if (FAILED(pGlobal->findChildren(SymTagFunction, szSymbolName, nsRegularExpression, &pEnumSymbols))) {
      wprintf(L"ERROR - DumpFPO() findChildren could not find symol %s\n", szSymbolName);

      return false;
   }

   while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
      if (pSymbol->get_relativeVirtualAddress(&dwRVA) == S_OK) {
         PrintPublicSymbol(pSymbol);

         DumpFPO(pSession, dwRVA);
      }

      pSymbol->Release();
   }

   pEnumSymbols->Release();

   putwchar(L'\n');

   return true;
}

////////////////////////////////////////////////////////////
// Dump a specified compiland and all the symbols defined in it
//
bool DumpCompiland(IDiaSymbol* pGlobal, const wchar_t* szCompName)
{
   IDiaEnumSymbols* pEnumSymbols;

   if (FAILED(pGlobal->findChildren(SymTagCompiland, szCompName, nsCaseInsensitive, &pEnumSymbols))) {
      return false;
   }

   IDiaSymbol* pCompiland;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSymbols->Next(1, &pCompiland, &celt)) && (celt == 1)) {
      wprintf(L"\n** Module: ");

      // Retrieve the name of the module

      BSTR bstrName;

      if (pCompiland->get_name(&bstrName) != S_OK) {
         wprintf(L"(???)\n\n");
      }

      else {
         wprintf(L"%s\n\n", bstrName);

         SysFreeString(bstrName);
      }

      IDiaEnumSymbols* pEnumChildren;

      if (SUCCEEDED(pCompiland->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren))) {
         IDiaSymbol* pSymbol;
         ULONG celt_ = 0;

         while (SUCCEEDED(pEnumChildren->Next(1, &pSymbol, &celt_)) && (celt_ == 1)) {
            PrintSymbol(pSymbol, 0);

            pSymbol->Release();
         }

         pEnumChildren->Release();
      }

      pCompiland->Release();
   }

   pEnumSymbols->Release();

   return true;
}

////////////////////////////////////////////////////////////
// Dump the line numbering information for a specified RVA
//
bool DumpLines(IDiaSession* pSession, DWORD dwRVA)
{
   IDiaEnumLineNumbers* pLines;

   if (FAILED(pSession->findLinesByRVA(dwRVA, MAX_RVA_LINES_BYTES_RANGE, &pLines))) {
      return false;
   }

   PrintLines(pLines);

   pLines->Release();

   return true;
}

////////////////////////////////////////////////////////////
// Dump the all line numbering information for a specified
//  function symbol name (as a regular expression string)
//
bool DumpLines(IDiaSession* pSession, IDiaSymbol* pGlobal, const wchar_t* szFuncName)
{
   IDiaEnumSymbols* pEnumSymbols;

   if (FAILED(pGlobal->findChildren(SymTagFunction, szFuncName, nsRegularExpression, &pEnumSymbols))) {
      return false;
   }

   IDiaSymbol* pFunction;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSymbols->Next(1, &pFunction, &celt)) && (celt == 1)) {
      PrintLines(pSession, pFunction);

      pFunction->Release();
   }

   pEnumSymbols->Release();

   return true;
}

////////////////////////////////////////////////////////////
// Dump the symbol information corresponding to a specified RVA
//
bool DumpSymbolWithRVA(IDiaSession* pSession, DWORD dwRVA, const wchar_t* szChildname)
{
   IDiaSymbol* pSymbol;
   LONG lDisplacement;

   if (FAILED(pSession->findSymbolByRVAEx(dwRVA, SymTagNull, &pSymbol, &lDisplacement))) {
      return false;
   }

   wprintf(L"Displacement = 0x%X\n", lDisplacement);

   PrintGeneric(pSymbol);

   DumpSymbolWithChildren(pSymbol, szChildname);

   while (pSymbol != NULL) {
      IDiaSymbol* pParent;

      if ((pSymbol->get_lexicalParent(&pParent) == S_OK) && pParent) {
         wprintf(L"\nParent\n");

         PrintSymbol(pParent, 0);

         pSymbol->Release();

         pSymbol = pParent;
      }

      else {
         pSymbol->Release();
         break;
      }
   }

   return true;
}

////////////////////////////////////////////////////////////
// Dump the symbols information where their names matches a
//  specified regular expression string
//
bool DumpSymbolsWithRegEx(IDiaSymbol* pGlobal, const wchar_t* szRegEx, const wchar_t* szChildname)
{
   IDiaEnumSymbols* pEnumSymbols;

   if (FAILED(pGlobal->findChildren(SymTagNull, szRegEx, nsRegularExpression, &pEnumSymbols))) {
      return false;
   }

   bool bReturn = true;

   IDiaSymbol* pSymbol;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
      PrintGeneric(pSymbol);

      bReturn = DumpSymbolWithChildren(pSymbol, szChildname);

      pSymbol->Release();
   }

   pEnumSymbols->Release();

   return bReturn;
}

////////////////////////////////////////////////////////////
// Dump the information corresponding to a symbol name which
//  is a children of the specified parent symbol
//
bool DumpSymbolWithChildren(IDiaSymbol* pSymbol, const wchar_t* szChildname)
{
   if (szChildname != NULL) {
      IDiaEnumSymbols* pEnumSyms;

      if (FAILED(pSymbol->findChildren(SymTagNull, szChildname, nsRegularExpression, &pEnumSyms))) {
         return false;
      }

      IDiaSymbol* pChild;
      DWORD celt = 1;

      while (SUCCEEDED(pEnumSyms->Next(celt, &pChild, &celt)) && (celt == 1)) {
         PrintGeneric(pChild);
         PrintSymbol(pChild, 0);

         pChild->Release();
      }

      pEnumSyms->Release();
   }

   else {
      // If the specified name is NULL then only the parent symbol data is displayed

      DWORD dwSymTag;

      if ((pSymbol->get_symTag(&dwSymTag) == S_OK) && (dwSymTag == SymTagPublicSymbol)) {
         PrintPublicSymbol(pSymbol);
      }

      else {
         PrintSymbol(pSymbol, 0);
      }
   }

   return true;
}

////////////////////////////////////////////////////////////
// Dump all the type symbols information that matches their
//  names to a specified regular expression string
//
bool DumpType(IDiaSymbol* pGlobal, const wchar_t* szRegEx)
{
   IDiaEnumSymbols* pEnumSymbols;

   if (FAILED(pGlobal->findChildren(SymTagUDT, szRegEx, nsRegularExpression, &pEnumSymbols))) {
      return false;
   }

   IDiaSymbol* pSymbol;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
      PrintTypeInDetail(pSymbol, 0);

      pSymbol->Release();
   }

   pEnumSymbols->Release();

   return true;
}

////////////////////////////////////////////////////////////
// Dump line numbering information for a given file name and
//  an optional line number
//
bool DumpLinesForSourceFile(IDiaSession* pSession, const wchar_t* szFileName, DWORD dwLine)
{
   IDiaEnumSourceFiles* pEnumSrcFiles;

   if (FAILED(pSession->findFile(NULL, szFileName, nsFNameExt, &pEnumSrcFiles))) {
      return false;
   }

   IDiaSourceFile* pSrcFile;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumSrcFiles->Next(1, &pSrcFile, &celt)) && (celt == 1)) {
      IDiaEnumSymbols* pEnumCompilands;

      if (pSrcFile->get_compilands(&pEnumCompilands) == S_OK) {
         IDiaSymbol* pCompiland;

         celt = 0;
         while (SUCCEEDED(pEnumCompilands->Next(1, &pCompiland, &celt)) && (celt == 1)) {
            BSTR bstrName;

            if (pCompiland->get_name(&bstrName) == S_OK) {
               wprintf(L"Compiland = %s\n", bstrName);

               SysFreeString(bstrName);
            }

            else {
               wprintf(L"Compiland = (???)\n");
            }

            IDiaEnumLineNumbers* pLines;

            if (dwLine != 0) {
               if (SUCCEEDED(pSession->findLinesByLinenum(pCompiland, pSrcFile, dwLine, 0, &pLines))) {
                  PrintLines(pLines);

                  pLines->Release();
               }
            }

            else {
               if (SUCCEEDED(pSession->findLines(pCompiland, pSrcFile, &pLines))) {
                  PrintLines(pLines);

                  pLines->Release();
               }
            }

            pCompiland->Release();
         }

         pEnumCompilands->Release();
      }

      pSrcFile->Release();
   }

   pEnumSrcFiles->Release();

   return true;
}

////////////////////////////////////////////////////////////
// Dump public symbol information for a given number of
//  symbols around a given RVA address
//
bool DumpPublicSymbolsSorted(IDiaSession* pSession, DWORD dwRVA, DWORD dwRange, bool bReverse)
{
   IDiaEnumSymbolsByAddr* pEnumSymsByAddr;

   if (FAILED(pSession->getSymbolsByAddr(&pEnumSymsByAddr))) {
      return false;
   }

   IDiaSymbol* pSymbol;

   if (SUCCEEDED(pEnumSymsByAddr->symbolByRVA(dwRVA, &pSymbol))) {
      if (dwRange == 0) {
         PrintPublicSymbol(pSymbol);
      }

      ULONG celt;
      ULONG i;

      if (bReverse) {
         pSymbol->Release();

         i = 0;

         for (pSymbol = NULL; (i < dwRange) && SUCCEEDED(pEnumSymsByAddr->Next(1, &pSymbol, &celt)) && (celt == 1); i++) {
            PrintPublicSymbol(pSymbol);

            pSymbol->Release();
         }
      }

      else {
         PrintPublicSymbol(pSymbol);

         pSymbol->Release();

         i = 1;

         for (pSymbol = NULL; (i < dwRange) && SUCCEEDED(pEnumSymsByAddr->Prev(1, &pSymbol, &celt)) && (celt == 1); i++) {
            PrintPublicSymbol(pSymbol);
         }
      }
   }

   pEnumSymsByAddr->Release();

   return true;
}

////////////////////////////////////////////////////////////
// Dump label symbol information at a given RVA
//
bool DumpLabel(IDiaSession* pSession, DWORD dwRVA)
{
   IDiaSymbol* pSymbol;
   LONG lDisplacement;

   if (FAILED(pSession->findSymbolByRVAEx(dwRVA, SymTagLabel, &pSymbol, &lDisplacement)) || (pSymbol == NULL)) {
      return false;
   }

   wprintf(L"Displacement = 0x%X\n", lDisplacement);

   PrintGeneric(pSymbol);

   pSymbol->Release();

   return true;
}

////////////////////////////////////////////////////////////
// Dump annotation symbol information at a given RVA
//
bool DumpAnnotations(IDiaSession* pSession, DWORD dwRVA)
{
   IDiaSymbol* pSymbol;
   LONG lDisplacement;

   if (FAILED(pSession->findSymbolByRVAEx(dwRVA, SymTagAnnotation, &pSymbol, &lDisplacement)) || (pSymbol == NULL)) {
      return false;
   }

   wprintf(L"Displacement = 0x%X\n", lDisplacement);

   PrintGeneric(pSymbol);

   pSymbol->Release();

   return true;
}

struct OMAP_DATA
{
   DWORD dwRVA;
   DWORD dwRVATo;
};

////////////////////////////////////////////////////////////
//
bool DumpMapToSrc(IDiaSession* pSession, DWORD dwRVA)
{
   IDiaEnumDebugStreams* pEnumStreams;
   IDiaEnumDebugStreamData* pStream;
   ULONG celt;

   if (FAILED(pSession->getEnumDebugStreams(&pEnumStreams))) {
      return false;
   }

   celt = 0;

   for (; SUCCEEDED(pEnumStreams->Next(1, &pStream, &celt)) && (celt == 1); pStream = NULL) {
      BSTR bstrName;

      if (pStream->get_name(&bstrName) != S_OK) {
         bstrName = NULL;
      }

      if (bstrName && wcscmp(bstrName, L"OMAPTO") == 0) {
         OMAP_DATA data, datasav;
         DWORD cbData;
         DWORD dwRVATo = 0;
         unsigned int i;

         datasav.dwRVATo = 0;
         datasav.dwRVA = 0;

         while (SUCCEEDED(pStream->Next(1, sizeof(data), &cbData, (BYTE*)&data, &celt)) && (celt == 1)) {
            if (dwRVA > data.dwRVA) {
               datasav = data;
               continue;
            }

            else if (dwRVA == data.dwRVA) {
               dwRVATo = data.dwRVATo;
            }

            else if (datasav.dwRVATo) {
               dwRVATo = datasav.dwRVATo + (dwRVA - datasav.dwRVA);
            }
            break;
         }

         wprintf(L"image rva = %08X ==> source rva = %08X\n\nRelated OMAP entries:\n", dwRVA, dwRVATo);
         wprintf(L"image rva ==> source rva\n");
         wprintf(L"%08X  ==> %08X\n", datasav.dwRVA, datasav.dwRVATo);

         i = 0;

         do {
            wprintf(L"%08X  ==> %08X\n", data.dwRVA, data.dwRVATo);
         } while ((++i) < 5 && SUCCEEDED(pStream->Next(1, sizeof(data), &cbData, (BYTE*)&data, &celt)) && (celt == 1));
      }

      if (bstrName != NULL) {
         SysFreeString(bstrName);
      }

      pStream->Release();
   }

   pEnumStreams->Release();

   return true;
}

////////////////////////////////////////////////////////////
//
bool DumpMapFromSrc(IDiaSession* pSession, DWORD dwRVA)
{
   IDiaEnumDebugStreams* pEnumStreams;

   if (FAILED(pSession->getEnumDebugStreams(&pEnumStreams))) {
      return false;
   }

   IDiaEnumDebugStreamData* pStream;
   ULONG celt = 0;

   for (; SUCCEEDED(pEnumStreams->Next(1, &pStream, &celt)) && (celt == 1); pStream = NULL) {
      BSTR bstrName;

      if (pStream->get_name(&bstrName) != S_OK) {
         bstrName = NULL;
      }

      if (bstrName && wcscmp(bstrName, L"OMAPFROM") == 0) {
         OMAP_DATA data;
         OMAP_DATA datasav;
         DWORD cbData;
         DWORD dwRVATo = 0;
         unsigned int i;

         datasav.dwRVATo = 0;
         datasav.dwRVA = 0;

         while (SUCCEEDED(pStream->Next(1, sizeof(data), &cbData, (BYTE*)&data, &celt)) && (celt == 1)) {
            if (dwRVA > data.dwRVA) {
               datasav = data;
               continue;
            }

            else if (dwRVA == data.dwRVA) {
               dwRVATo = data.dwRVATo;
            }

            else if (datasav.dwRVATo) {
               dwRVATo = datasav.dwRVATo + (dwRVA - datasav.dwRVA);
            }
            break;
         }

         wprintf(L"source rva = %08X ==> image rva = %08X\n\nRelated OMAP entries:\n", dwRVA, dwRVATo);
         wprintf(L"source rva ==> image rva\n");
         wprintf(L"%08X  ==> %08X\n", datasav.dwRVA, datasav.dwRVATo);

         i = 0;

         do {
            wprintf(L"%08X  ==> %08X\n", data.dwRVA, data.dwRVATo);
         } while ((++i) < 5 && SUCCEEDED(pStream->Next(1, sizeof(data), &cbData, (BYTE*)&data, &celt)) && (celt == 1));
      }

      if (bstrName != NULL) {
         SysFreeString(bstrName);
      }

      pStream->Release();
   }

   pEnumStreams->Release();

   return true;
}

////////////////////////////////////////////////////////////
// Retreive the table that matches the given iid
//
//  A PDB table could store the section contributions, the frame data,
//  the injected sources
//
HRESULT GetTable(IDiaSession* pSession, REFIID iid, void** ppUnk)
{
   IDiaEnumTables* pEnumTables;

   if (FAILED(pSession->getEnumTables(&pEnumTables))) {
      wprintf(L"ERROR - GetTable() getEnumTables\n");

      return E_FAIL;
   }

   IDiaTable* pTable;
   ULONG celt = 0;

   while (SUCCEEDED(pEnumTables->Next(1, &pTable, &celt)) && (celt == 1)) {
      // There's only one table that matches the given IID

      if (SUCCEEDED(pTable->QueryInterface(iid, (void**)ppUnk))) {
         pTable->Release();
         pEnumTables->Release();

         return S_OK;
      }

      pTable->Release();
   }

   pEnumTables->Release();

   return E_FAIL;
}
