
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
*   FILE     :  EXPORTS.H
*   DESCRIP  :  Exports Library for MANOS v1.0
*   DATE     :  August 22, 1998
*
*
***************************************************************************/

#include "types.h"

// export list types

#define EXPORT_PROC   0x00000001
#define EXPORT_DATA   0x00000002
#define EXPORT_RPC    0x00000004
#define EXPORT_EVENT  0x00000008

// symbol format types

#define FORMAT_UNKNOWN     0x0000
#define FORMAT_BORLAND     0x0001
#define FORMAT_COFF        0x0002
#define FORMAT_CODEVIEW    0x0003
#define FORMAT_FPO         0x0004
#define FORMAT_MISC        0x0005
#define FORMAT_EXCEPTION   0x0006
#define FORMAT_FIXUP       0x0007

typedef struct _EXPORT {
   struct _EXPORT *next;
   struct _EXPORT *prior;
   struct _EXPORT *addressNext;    // address forward search link
   struct _EXPORT *addressPrior;   // address previous search link
   struct _EXPORT *nameNext;       // name forward search link
   struct _EXPORT *namePrior;      // name previous search link
   LONG ExportOrdinal;
   LONG ExportType;
   LONG ExportPDE;
   BYTE *ExportName;
   LONG ExportNameLength;
   LONG ExportJumpAddress;
   LONG ExportRPCHandle;
   LONG *ExportRPCHead;
   LONG *ExportRPCTail;
   LONG ExportRPCMutex;
   BYTE *SymbolLink;       // 64 bytes = 64 exports / 4096 bytes
   LONG ExportID;
   LONG SymbolFormat;
   MODULE_HANDLE *module;
} EXPORT;

extern EXPORT *insertExport(EXPORT *i, EXPORT *top);
extern EXPORT *insertExportName(EXPORT *i, EXPORT *top);
extern EXPORT *insertExportAddress(EXPORT *i, EXPORT *top);
extern LONG AddPublicExport(EXPORT *NewExport);
extern LONG RemovePublicExport(EXPORT *NewExport);
extern void InitializeExportList(MODULE_HANDLE *module);
extern void RemoveModuleExports(MODULE_HANDLE *module);
extern void DisplayExportsMatch(SCREEN *screen, LONG attr, BYTE *match);
extern LONG ImportExportedAddress(BYTE *name, BYTE *moduleName, MODULE_HANDLE **module,
			   EXPORT **exportRecord);
extern LONG GetGlobalExportOrdinal(BYTE *name, BYTE *moduleName);
extern LONG RegisterGlobalExportAddress(BYTE *name, BYTE *moduleName, LONG address,
				 LONG Ordinal, LONG type);
extern LONG RegisterModuleExport(MODULE_HANDLE *module, BYTE *name, LONG address,
			  LONG Ordinal, LONG type);


