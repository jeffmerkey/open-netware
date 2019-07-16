
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  LOADER.H
*   DESCRIP  :  Loader Module Defines for MANOS v1.0
*   DATE     :  August 1, 1998
*
*
***************************************************************************/

#include "types.h"

#define  FILE_NOT_FOUND     10
#define  FILE_INCOMPAT      11
#define  FILE_SEEK_ERROR    12
#define  FILE_READ_ERROR    13
#define  FILE_MODULO_ERROR  14
#define  FILE_MEMORY_ERROR  15
#define  FILE_CORRUPT_ERROR 16
#define  FILE_DUP_ERROR     17
#define  FILE_UNRESOLVED    18
#define  FILE_PROCESS_ERROR 19
#define  FILE_LINK_ERROR    20

#define  PE_FILE_DEFAULT_BASE  0x400000

#define  MODULE_SIGNATURE    0xFEEDC0ED
#define  MAX_OS_SEGMENTS     32

typedef struct _IMPORT {
     struct _IMPORT *providerNext;
     struct _IMPORT *providerPrior;
     struct _IMPORT *importedNext;
     struct _IMPORT *importedPrior;
     void *providerModuleContext;
     void *importedModuleContext;
     void *referenceObject;
     LONG referenceAddress;
} IMPORT;

typedef struct _LOADED_MODULE {
     struct _LOADED_MODULE *next;
     struct _LOADED_MODULE *prior;
     LONG moduleMutex;
     LONG ModuleID;
     LONG ModuleSignature;
     BYTE ModuleName[256];
     BYTE ModuleNameLength;
     BYTE ModuleShortName[32];
     LONG ModuleShortNameLength;
     LONG ModuleStart;
     LONG ModuleEnd;
     LONG PEBase;
     LONG EntryRVA;
     LONG SectionNumber;
     LONG BaseAddress;
     LONG BaseSize;
     LONG StackSize;
     LONG HeapSize;
     LONG HeaderSize;
     LONG CodeSegmentPtr;
     LONG CodeSegmentSize;
     LONG CodeSegmentFlags;
     LONG DataSegmentPtr;
     LONG DataSegmentSize;
     LONG DataSegmentFlags;
     LONG RelocSegmentPtr;
     LONG RelocSegmentSize;
     LONG RelocSegmentFlags;
     LONG DebugSegmentPtr;
     LONG DebugSegmentSize;
     LONG DebugSegmentFlags;
     LONG DebugSymbolType;
     LONG InitSegmentPtr;
     LONG InitSegmentSize;
     LONG InitSegmentFlags;
     LONG ImportDirectory;
     LONG ImportDirectorySize;
     LONG ImportSegmentPtr;
     LONG ImportSegmentSize;
     LONG ImportSegmentFlags;
     LONG ExportDirectory;
     LONG ExportDirectorySize;
     LONG ExportSegmentPtr;
     LONG ExportSegmentSize;
     LONG ExportSegmentFlags;
     LONG SharedSegmentPtr;
     LONG SharedSegmentSize;
     LONG SharedSegmentFlags;
     LONG RPCSegmentPtr;
     LONG RPCSegmentSize;
     LONG RPCSegmentFlags;
     LONG ResourceSegmentPtr;
     LONG ResourceSegmentSize;
     LONG ResourceSegmentFlags;

     LONG (*entryPoint)(LONG, BYTE **, LONG);
     LONG (*exitProcedure)(struct _LOADED_MODULE *);
     LONG (*unloadProcedure)(struct _LOADED_MODULE *);
     LONG (*warnProcedure)(struct _LOADED_MODULE *);

     LONG sourceIndex;
     LONG SourceNumber;
     LONG ModuleNumber;
     LONG SourceIndex;
     LONG ModuleIndex;
     LONG NameSection;
     LONG NumberOfSymbols;
     LONG NumberOfNames;
     LONG NumberOfSections;
     LONG DebugSymbolVersion;
     LONG GlobalSymbolTable;
     LONG GlobalTypeTable;
     LONG DebugBaseAddress;
     LONG SegmentIndexTable[MAX_OS_SEGMENTS];
     LONG SegmentTypeTable[MAX_OS_SEGMENTS];
     LONG SegmentLengthTable[MAX_OS_SEGMENTS];

     LONG *ModuleIndexTable;
     LONG *SourceIndexTable;
     LONG *NameIndexTable;
     LONG *SymbolIndexTable;
     LONG *SourceCodeIndex;
     LONG *SourceLineIndex;
     LONG *SourceSizeIndex;
     NAME_CROSS_INDEX *crossIndex;
     PROCESS *defaultProcess;
     BYTE commandLine[256];

     void *resourceHead;
     void *resourceTail;
     LONG resourceLock;
     LONG resourceNum;

     IMPORT *extHead;
     IMPORT *extTail;
     LONG ExternalReferenceLock;
     LONG ExternalReferenceCount;

     IMPORT *intHead;
     IMPORT *intTail;
     LONG InternalReferenceLock;
     LONG InternalReferenceCount;

} MODULE_HANDLE;


extern void DumpOSSymbolTable(SCREEN *screen);
extern void DisplayClosestSymbols(SCREEN *screen, LONG address);
extern void DisplayClosestSymbol(SCREEN *screen, LONG address);
extern BYTE *GetSymbolFromValue(LONG value);

extern LONG GetLineInfoFromValue(LONG address, SOURCE_LINE_INFO *info,
				 MODULE_HANDLE **module,
				 LONG *totalLines,
				 LONG *debugInfoPresent,
				 LONG *exact);

extern LONG GetValueFromLine(LONG ModuleIndex,
			     LONG FileIndex,
			     LONG SegmentIndex,
			     LONG LineNumber,
			     MODULE_HANDLE *module,
			     LONG *debugInfoPresent);

extern LONG InitializeSymbolTable(MODULE_HANDLE *module);

extern MODULE_HANDLE serverModuleHandle;
extern MODULE_HANDLE loaderModuleHandle;
extern MODULE_HANDLE *moduleListHead;
extern MODULE_HANDLE *moduleListTail;
extern LONG moduleListMutex;
extern LONG moduleIDs;
extern LONG NumberOfModules;
extern void remove_module(MODULE_HANDLE *module);
extern void insert_module(MODULE_HANDLE *module);

extern MODULE_HANDLE *GetModuleHandleFromValue(LONG value);
extern MODULE_HANDLE *GetModuleHandleFromName(BYTE *name);

extern LONG ModuleInitializeSymbolTable(MODULE_HANDLE *module);
extern LONG ModuleFreeSymbolTable(MODULE_HANDLE *module);


