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

////////////////////////////////////////////////////////////
//
int wmain(int argc, wchar_t* argv[])
{
   IDiaDataSource* g_pDiaDataSource;
   auto g_szFilename = argv[argc - 1];

   if (!LoadDataFromPdb(g_szFilename, &g_pDiaDataSource, &g_pDiaSession, &g_pGlobalSymbol)) {
      return -1;
   }

   DumpAllPdbInfo(g_pDiaSession, g_pGlobalSymbol);

   Cleanup();

   return 0;
}
