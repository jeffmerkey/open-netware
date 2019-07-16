
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   This program is an unpublished work of TRG, Inc. and contains
*   trade secrets and other proprietary information.  Unauthorized
*   use, copying, disclosure, or distribution of this file without the
*   consent of TRG, Inc. can subject the offender to severe criminal
*   and/or civil penalities.
*
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  DLL.C
*   DESCRIP  :  Microsoft DLL Explicit Loader for MANOS v1.0
*   DATE     :  February 24, 1998
*
***************************************************************************/

#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "string.h"
#include "kernel.h"
#include "keyboard.h"
#include "screen.h"
#include "types.h"
#include "emit.h"
#include "dos.h"
#include "tss.h"
#include "os.h"
#include "mps.h"
#include "hal.h"
#include "timer.h"
#include "dosfile.h"
#include "peexe.h"
#include "extrnvar.h"
#include "line.h"
#include "loader.h"
#include "exports.h"

extern LONG ImportExportedAddress(BYTE *name, BYTE *moduleName,
				  MODULE_HANDLE **module, EXPORT **export);
extern LONG LoadFile(BYTE *filename, MODULE_HANDLE **newModule);

MODULE_HANDLE *LoadLibrary(BYTE *name)
{
   register LONG retCode;
   MODULE_HANDLE *module = 0;

   retCode = LoadFile(name, &module);
   if (retCode)
      module = 0;

   return module;

}

LONG GetProcAddress(MODULE_HANDLE *module, BYTE *function)
{

    MODULE_HANDLE *newModule;
    EXPORT *export;
    register LONG retCode;

    retCode = ImportExportedAddress(function, module->ModuleShortName,
				     &newModule, &export);
    if (retCode == (LONG) -1)
       retCode = 0;

    return retCode;

}

