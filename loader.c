

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
*   FILE     :  LOADER.C
*   DESCRIP  :  Microsoft Portable Executable Loader for MANOS v1.0
*   DATE     :  February 13, 1998
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
#include "peexe.h"
#include "extrnvar.h"
#include "line.h"
#include "loader.h"
#include "malloc.h"
#include "free.h"
#include "dosfile.h"
#include "ifs.h"
#include "event.h"
#include "exports.h"

#define VERBOSE             0
#define MULTI_LOAD          1
#define MAX_ARGUMENTS      64
#define COMMANDLINE_LEN   256

extern LONG BaseSegment;
extern LONG CodeSegment;
extern LONG CodeRVA;
extern LONG DataSegment;
extern LONG RelocSegment;
extern LONG DebugSegment;
extern LONG DebugSize;
extern LONG ExportSegment;
extern LONG ExportSize;
extern LONG ExportRVA;
extern LONG LoaderCodeSegment;
extern LONG LoaderDataSegment;
extern LONG StartOfHighMemory;
extern LONG HighMemoryLength;
extern LONG StartupMemory;
extern LONG StartupLength;
extern LONG StartOfLowMemory;
extern LONG LowMemoryLength;

MODULE_HANDLE serverModuleHandle;
MODULE_HANDLE loaderModuleHandle;

MODULE_HANDLE *moduleListHead = 0;
MODULE_HANDLE *moduleListTail = 0;
LONG moduleListMutex = 0;
LONG moduleIDs = 1;
LONG NumberOfModules = 0;

void remove_module(MODULE_HANDLE *module);
void insert_module(MODULE_HANDLE *module);

void RemoveModuleReferences(MODULE_HANDLE *module);

#define FN_DRIVE       0x01
#define FN_DIRECTORY   0x02
#define FN_EXTENSION   0x04
#define FN_WILDCARDS   0x08
#define FN_FILENAME    0x10

BYTE *max_ptr(BYTE *p1, BYTE *p2)
{
    if (p1 > p2)
      return p1;
    else
      return p2;
}

LONG filenameSplit(BYTE *path, BYTE *drive, BYTE *dir, BYTE *name, BYTE *ext)
{

    register LONG flags = 0, len;
    BYTE *pp, *pe;

    pp = path;
    if ((isalpha(*pp) || strchr("[\\]^_`", *pp)) && (pp[1] == ':'))
    {
       flags |= FN_DRIVE;
       if (drive)
       {
	  strncpy(drive, pp, 2);
	  drive[2] = '\0';
       }
       pp += 2;
    }

    pe = max_ptr(strrchr(pp, '\\'), strrchr(pp, '/'));
    if (pe)
    {
       flags |= FN_DIRECTORY;
       pe++;
       len = pe - pp;
       if (dir)
       {
	 strncpy(dir, pp, len);
	 dir[len] = '\0';
       }
       pp = pe;
    }
    else
       pe = pp;

    //  Special case: "c:/path/." or "c:/path/.."
    //  These mean FILENAME, not EXTENSION.

    while (*pp == '.')
       ++pp;

    if (pp > pe)
    {
       flags |= FN_FILENAME;
       if (name)
       {
	  len = pp - pe;
	  strncpy(name, pe, len);
	  name[len] = '\0';
       }
    }

    pe = strrchr(pp, '.');
    if (pe)
    {
       flags |= FN_EXTENSION;
       if (ext)
	  strcpy(ext, pe);
    }
    else
      pe = strchr( pp, '\0');

    if (pp != pe)
    {
       flags |= FN_FILENAME;
       len = pe - pp;
       if (name)
       {
	  strncpy(name, pp, len);
	  name[len] = '\0';
       }
    }

    if (strcspn(path, "*?[") < strlen(path))
       flags |= FN_WILDCARDS;

    return flags;

}


LONG ExitModule(MODULE_HANDLE *module)
{

    EXPORT *export, *exportList;

    if (module->exitProcedure)
       (module->exitProcedure)(module);

    RemoveModuleReferences(module);
    RemoveModuleExports(module);

    if (module->DebugSegmentPtr)
	ModuleFreeSymbolTable(module);

    remove_module(module);

    if (module->BaseAddress)
       free((void *)module->BaseAddress);

    kfree((void *)module);

    return 0;

}

void _moduleInit(MODULE_HANDLE *module)
{

   register BYTE *p;
   register LONG argc, count;
   BYTE *argv[MAX_ARGUMENTS];

   count = argc = 0;
   argv[0] = (BYTE *) 0;
   p = (BYTE *) module->commandLine;

   while (*p && argc < MAX_ARGUMENTS && count < COMMANDLINE_LEN)
   {
      while (*p && *p == ' ' && count < COMMANDLINE_LEN)
      {
	 p++;
	 count++;
      }
      if (*p)
      {
	 argv[argc++] = p;
	 while (*p && *p != ' ' && count < COMMANDLINE_LEN)
	 {
	    p++;
	    count++;
	 }
	 *p++ = '\0';
      }
   }

   (module->entryPoint)(argc, argv, (LONG)module);
   ExitModule(module);

}

MODULE_HANDLE *GetModuleHandleFromValue(LONG value)
{

   register LONG i;
   register MODULE_HANDLE *module;

   module = moduleListHead;
   for (i=0; i < NumberOfModules && module; i++)
   {
      if (value >= module->ModuleStart && value <= module->ModuleEnd)
	 return module;
      module = module->next;
      if (!module)
	 break;
   }
   return 0;

}

MODULE_HANDLE *GetPreviousModuleHandle(MODULE_HANDLE *module)
{

   register LONG i, value;
   register MODULE_HANDLE *previousModule;

   previousModule = moduleListHead;
   value = ((LONG)module->ModuleStart - 1);
   for (i=0; i < NumberOfModules && previousModule; i++)
   {
      if (value >= previousModule->ModuleStart &&
	  value <= previousModule->ModuleEnd)
	 return previousModule;
      previousModule = previousModule->next;
      if (!previousModule)
	 break;
   }

   return module;

}

MODULE_HANDLE *GetNextModuleHandle(MODULE_HANDLE *module)
{

   register LONG i, value;
   register MODULE_HANDLE *nextModule;

   nextModule = moduleListHead;
   value = ((LONG)module->ModuleEnd + 1);
   for (i=0; i < NumberOfModules && nextModule; i++)
   {
      if (value >= nextModule->ModuleStart &&
	  value <= nextModule->ModuleEnd)
	 return nextModule;
      nextModule = nextModule->next;
      if (!nextModule)
	 break;
   }

   return module;

}

MODULE_HANDLE *GetModuleHandleFromName(BYTE *name)
{

   register LONG i;
   register MODULE_HANDLE *module;

   module = moduleListHead;
   for (i=0; i < NumberOfModules && module; i++)
   {
      if (!stricmp(module->ModuleShortName, name))
	 return module;
      module = module->next;
      if (!module)
	 break;
   }

   return 0;

}

void InitializeModuleList(void)
{

    MODULE_HANDLE *module = &serverModuleHandle;

    SetData((LONG *) module, 0, sizeof(MODULE_HANDLE));
    module->ModuleID = moduleIDs++;
    module->ModuleSignature = MODULE_SIGNATURE;
    strcpy(module->ModuleName, "MANOS Operating System");
    strcpy(module->ModuleShortName, "MANOS32.DLL");
    module->ModuleNameLength = strlen(module->ModuleName);
    module->ModuleShortNameLength = strlen(module->ModuleShortName);
    module->PEBase = BaseSegment;
    module->BaseAddress = BaseSegment;
    module->BaseSize = StartOfHighMemory - CodeSegment;
    module->ModuleStart = module->BaseAddress;
    module->ModuleEnd = module->BaseAddress + module->BaseSize;
    module->CodeSegmentPtr = CodeSegment;
    module->CodeSegmentSize = DataSegment - CodeSegment;
    module->DataSegmentPtr = DataSegment;
    module->DataSegmentSize = RelocSegment - DataSegment;
    module->RelocSegmentPtr = RelocSegment;
    module->RelocSegmentSize = DebugSegment - RelocSegment;
    module->ExportSegmentPtr = ExportSegment;
    module->ExportDirectory = ExportSegment;
    module->ExportSegmentSize = ExportSize;
    module->DebugSegmentPtr = DebugSegment;
    module->DebugSegmentSize = DebugSize;

    module->InitSegmentPtr = 0;
    module->InitSegmentSize = 0;
    module->ImportSegmentPtr = 0;
    module->ImportSegmentSize = 0;
    module->entryPoint = (LONG (*)(LONG, BYTE **, LONG)) CodeSegment;
    module->exitProcedure = (LONG (*)(MODULE_HANDLE *)) ExitOS;
    module->unloadProcedure = (LONG (*)(MODULE_HANDLE *)) ExitOS;
    module->defaultProcess = (PROCESS *) &idle_p[0];

    // inititlalize debugger segment index table for OS

    module->NumberOfSections = module->SectionNumber = 4;
    module->SegmentIndexTable[1] = module->CodeSegmentPtr;
    module->SegmentIndexTable[2] = module->DataSegmentPtr;
    module->SegmentIndexTable[3] = module->RelocSegmentPtr;
    module->SegmentIndexTable[4] = module->DebugSegmentPtr;
    module->SegmentTypeTable[1] = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ ;
    module->SegmentTypeTable[2] = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ ;
    module->SegmentTypeTable[3] = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_SHARED | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE;
    module->SegmentTypeTable[4] = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_SHARED | IMAGE_SCN_MEM_READ;
    module->SegmentLengthTable[1] = module->CodeSegmentSize;
    module->SegmentLengthTable[2] = module->DataSegmentSize;
    module->SegmentLengthTable[3] = module->RelocSegmentSize;
    module->SegmentLengthTable[4] = module->DebugSegmentSize;
    module->resourceHead = module->resourceTail = 0;
    module->resourceLock = module->resourceNum = 0;

    module->extHead = module->extTail = 0;
    module->ExternalReferenceLock = module->ExternalReferenceCount = 0;
    module->intHead = module->intTail = 0;
    module->InternalReferenceLock = module->InternalReferenceCount = 0;

    insert_module(module);

    return;

}


void remove_module(MODULE_HANDLE *module)
{

    register MODULE_HANDLE *search;
    register LONG flags;

    flags = get_flags();
    spin_lock(&moduleListMutex);
    search = moduleListHead;
    while (search)
    {
       if (search == module)
       {
	  if (moduleListHead == module)
	  {
	     moduleListHead = (void *) module->next;
	     if (moduleListHead)
		moduleListHead->prior = NULL;
	     else
		moduleListTail = NULL;
	  }
	  else
	  {
	     module->prior->next = module->next;
	     if (module != moduleListTail)
		module->next->prior = module->prior;
	     else
		moduleListTail = module->prior;
	  }
       }
       search = search->next;
    }
    if (NumberOfModules)
       NumberOfModules--;

    spin_unlock(&moduleListMutex);
    set_flags(flags);

    EventNotify(EVENT_UNLOAD_MODULE, (LONG) module);

    return;

}

void insert_module(MODULE_HANDLE *module)
{

    register MODULE_HANDLE *search;
    register LONG flags;

    EventNotify(EVENT_LOAD_MODULE, (LONG) module);

    flags = get_flags();
    spin_lock(&moduleListMutex);
    search = moduleListHead;
    while (search)
    {
       if (search == module)
       {
	  spin_unlock(&moduleListMutex);
	  set_flags(flags);
	  return;
       }
       search = search->next;
    }
    if (!moduleListHead)
    {
       moduleListHead = module;
       moduleListTail = module;
       module->next = module->prior = 0;
    }
    else
    {
       moduleListTail->next = module;
       module->next = 0;
       module->prior = moduleListTail;
       moduleListTail = module;
    }
    NumberOfModules++;

    spin_unlock(&moduleListMutex);
    set_flags(flags);
    return;

}

void DisplayModule(SCREEN *screen, LONG attr, LONG address)
{
   register LONG retCode;
   register MODULE_HANDLE *module;
   register LONG i, j;
   typedef struct
   {
      DWORD	flag;
      LPSTR	name;
   } DWORD_FLAG_DESCRIPTIONS;
   DWORD_FLAG_DESCRIPTIONS SectionCharacteristics[] =
   {
      { IMAGE_SCN_CNT_CODE, "CODE" },
      { IMAGE_SCN_CNT_INITIALIZED_DATA, "INITIALIZED_DATA" },
      { IMAGE_SCN_CNT_UNINITIALIZED_DATA, "UNINITIALIZED_DATA" },
      { IMAGE_SCN_LNK_INFO, "LNK_INFO" },
      { IMAGE_SCN_LNK_OVERLAY, "LNK_OVERLAY" },
      { IMAGE_SCN_LNK_REMOVE, "LNK_REMOVE" },
      { IMAGE_SCN_LNK_COMDAT, "LNK_COMDAT" },
      { IMAGE_SCN_MEM_DISCARDABLE, "MEM_DISCARDABLE" },
      { IMAGE_SCN_MEM_NOT_CACHED, "MEM_NOT_CACHED" },
      { IMAGE_SCN_MEM_NOT_PAGED, "MEM_NOT_PAGED" },
      { IMAGE_SCN_MEM_SHARED, "MEM_SHARED" },
      { IMAGE_SCN_MEM_EXECUTE, "MEM_EXECUTE" },
      { IMAGE_SCN_MEM_READ, "MEM_READ" },
      { IMAGE_SCN_MEM_WRITE, "MEM_WRITE" },
   };
   #define NUMBER_SECTION_CHARACTERISTICS  (sizeof(SectionCharacteristics) / sizeof(DWORD_FLAG_DESCRIPTIONS))
   extern LONG validate_address(LONG);
   retCode = validate_address(address);
   if (retCode)
   {
      switch (retCode)
      {
	 case (LONG) -1:
	    printfScreenWithAttribute(screen, attr, "invalid address given\n");
	    return;

	 case 1:
	    printfScreenWithAttribute(screen, attr, "address paged out\n");
	    return;

	 default:
	    printfScreenWithAttribute(screen, attr, "invalid address (%d)\n", retCode);
	    return;
      }
   }
   module = (MODULE_HANDLE *) address;
   if (module->ModuleSignature != MODULE_SIGNATURE)
   {
      printfScreenWithAttribute(screen, attr, "invalid module signature\n");
      return;
   }

   SetPauseMode(screen, screen->nLines - 3);

   printfScreenWithAttribute(screen,
				  attr,
				  "%s  ::  %08X\n",
				  module->ModuleName, module);
   printfScreenWithAttribute(screen,
				  attr,
				  "   Short Name : %s   Entry : %08X  Size : %08X\n",
				  module->ModuleShortName,
				  module->entryPoint,
				  module->BaseSize);
   printfScreenWithAttribute(screen,
				  attr,
				  "   Range      : %08X-%08X   BaseRVA    : %08X\n",
				  module->ModuleStart,
				  module->ModuleEnd,
				  module->PEBase);

   printfScreenWithAttribute(screen,
				  attr,
				  "   Import Directory  : %08X   Export Directory  : %08X\n",
				  module->ImportDirectory,
				  module->ExportDirectory);

   if (module->CodeSegmentSize)
	   printfScreenWithAttribute(screen,
				  attr,
				  "   Code Segment   : %08X  Code Size   : (%08X) %d bytes\n",
				  module->CodeSegmentPtr,
				  module->CodeSegmentSize,
				  module->CodeSegmentSize);

   if (module->DataSegmentSize)
	   printfScreenWithAttribute(screen,
				  attr,
				  "   Data Segment   : %08X  Data Size   : (%08X) %d bytes\n",
				  module->DataSegmentPtr,
				  module->DataSegmentSize,
				  module->DataSegmentSize);

   if (module->RelocSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   Reloc Segment  : %08X  Reloc Size  : (%08X) %d bytes\n",
				  module->RelocSegmentPtr,
				  module->RelocSegmentSize,
				  module->RelocSegmentSize);

   if (module->InitSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   Init Segment   : %08X  Init Size   : (%08X) %d bytes\n",
				  module->InitSegmentPtr,
				  module->InitSegmentSize,
				  module->InitSegmentSize);

   if (module->ImportSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   Import Segment : %08X  Import Size : (%08X) %d bytes\n",
				  module->ImportSegmentPtr,
				  module->ImportSegmentSize,
				  module->ImportSegmentSize);

   if (module->ExportSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   Export Segment : %08X  Export Size : (%08X) %d bytes\n",
				  module->ExportSegmentPtr,
				  module->ExportSegmentSize,
				  module->ExportSegmentSize);

   if (module->SharedSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   Shared Segment : %08X  Shared Size : (%08X) %d bytes\n",
				  module->SharedSegmentPtr,
				  module->SharedSegmentSize,
				  module->SharedSegmentSize);


   if (module->RPCSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   RPC Segment    : %08X  RPC Size    : (%08X) %d bytes\n",
				  module->RPCSegmentPtr,
				  module->RPCSegmentSize,
				  module->RPCSegmentSize);


   if (module->ResourceSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   Resrce Segment : %08X  Resrce Size : (%08X) %d bytes\n",
				  module->ResourceSegmentPtr,
				  module->ResourceSegmentSize,
				  module->ResourceSegmentSize);

   if (module->DebugSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   Debug Segment  : %08X  Debug Size  : (%08X) %d bytes\n",
				  module->DebugSegmentPtr,
				  module->DebugSegmentSize,
				  module->DebugSegmentSize);

   printfScreenWithAttribute(screen,
			     attr,
			     "\n-------------- Section Table ----------------\n");
   for (i=0; i < module->SectionNumber && i < 32; i++)
   {
      printfScreenWithAttribute(screen,
				attr,
				"Section %i : %08X Len: %08X ",
				i,
				module->SegmentIndexTable[i + 1],
				module->SegmentLengthTable[i + 1]);

      for (j=0; j < NUMBER_SECTION_CHARACTERISTICS; j++)
      {
	 if (module->SegmentTypeTable[i + 1] & SectionCharacteristics[j].flag)
	      printfScreenWithAttribute(screen, attr, " %s", SectionCharacteristics[j].name);
      }
      printfScreen(screen, "\n");
   }

   printfScreenWithAttribute(screen,
			     attr,
			     "\n--------------- Source Debug Table ----------------\n");

   printfScreenWithAttribute(screen, attr, "sourceIndex        -> %08X  ", module->sourceIndex);
   printfScreenWithAttribute(screen, attr, "SourceNumber       -> %08X\n", module->SourceNumber);
   printfScreenWithAttribute(screen, attr, "ModuleNumber       -> %08X  ", module->ModuleNumber);
   printfScreenWithAttribute(screen, attr, "SourceIndex        -> %08X\n", module->SourceIndex);
   printfScreenWithAttribute(screen, attr, "ModuleIndex        -> %08X  ", module->ModuleIndex);
   printfScreenWithAttribute(screen, attr, "NameSection        -> %08X\n", module->NameSection);
   printfScreenWithAttribute(screen, attr, "NumberOfSymbols    -> %08X  ", module->NumberOfSymbols);
   printfScreenWithAttribute(screen, attr, "NumberOfNames      -> %08X\n", module->NumberOfNames);
   printfScreenWithAttribute(screen, attr, "NumberOfSections   -> %08X  ", module->NumberOfSections);
   printfScreenWithAttribute(screen, attr, "DebugSymbolVersion -> %08X\n", module->DebugSymbolVersion);
   printfScreenWithAttribute(screen, attr, "GlobalSymbolTable  -> %08X  ", module->GlobalSymbolTable);
   printfScreenWithAttribute(screen, attr, "GlobalTypeTable    -> %08X\n", module->GlobalTypeTable);
   printfScreenWithAttribute(screen, attr, "DebugBaseAddress   -> %08X  ", module->DebugBaseAddress);
   printfScreenWithAttribute(screen, attr, "ModuleIndexTable   -> %08X\n", module->ModuleIndexTable);
   printfScreenWithAttribute(screen, attr, "SourceIndexTable   -> %08X  ", module->SourceIndexTable);
   printfScreenWithAttribute(screen, attr, "NameIndexTable     -> %08X\n", module->NameIndexTable);
   printfScreenWithAttribute(screen, attr, "SymbolIndexTable   -> %08X  ", module->SymbolIndexTable);
   printfScreenWithAttribute(screen, attr, "SourceCodeIndex    -> %08X\n", module->SourceCodeIndex);
   printfScreenWithAttribute(screen, attr, "SourceLineIndex    -> %08X  ", module->SourceLineIndex);
   printfScreenWithAttribute(screen, attr, "SourceSizeIndex    -> %08X\n", module->SourceSizeIndex);
   printfScreenWithAttribute(screen, attr, "crossIndex         -> %08X  ", module->crossIndex);
   printfScreenWithAttribute(screen, attr, "defaultProcess     -> %08X\n", module->defaultProcess);

   printfScreenWithAttribute(screen,
			     attr,
			     "\n--------------- Import Reference Lists ----------------\n");

   printfScreenWithAttribute(screen, attr, "external head      -> %08X  ", module->extHead);
   printfScreenWithAttribute(screen, attr, "external tail      -> %08X\n", module->extTail);
   printfScreenWithAttribute(screen, attr, "external lock      -> %08X  ", module->ExternalReferenceLock);
   printfScreenWithAttribute(screen, attr, "external count     -> %08X\n", module->ExternalReferenceCount);
   printfScreenWithAttribute(screen, attr, "internal head      -> %08X  ", module->intHead);
   printfScreenWithAttribute(screen, attr, "internal tail      -> %08X\n", module->intTail);
   printfScreenWithAttribute(screen, attr, "external lock      -> %08X  ", module->InternalReferenceLock);
   printfScreenWithAttribute(screen, attr, "external count     -> %08X\n", module->InternalReferenceCount);

   ClearPauseMode(screen);
   return;

}

void DisplayModuleMap(SCREEN *screen, LONG attr, LONG extendedInfo)
{

     MODULE_HANDLE *module;

     SetPauseMode(screen, screen->nLines - 3);
     module = moduleListHead;
     while (module)
     {
	if (printfScreenWithAttribute(screen,
				  attr,
				  "%s  ::  %08X\n",
				  module->ModuleName, module))
	   break;
	printfScreenWithAttribute(screen,
				  attr,
				  "   Short Name : %s   Entry : %08X  Size : %08X\n",
				  module->ModuleShortName,
				  module->entryPoint,
				  module->BaseSize);
	printfScreenWithAttribute(screen,
				  attr,
				  "   Range      : %08X-%08X   BaseRVA    : %08X\n",
				  module->ModuleStart,
				  module->ModuleEnd,
				  module->PEBase);

	printfScreenWithAttribute(screen,
				  attr,
				  "   Import Directory  : %08X   Export Directory  : %08X\n",
				  module->ImportDirectory,
				  module->ExportDirectory);

	if (module->CodeSegmentSize)
	   printfScreenWithAttribute(screen,
				  attr,
				  "   Code Segment   : %08X  Code Size   : (%08X) %d bytes\n",
				  module->CodeSegmentPtr,
				  module->CodeSegmentSize,
				  module->CodeSegmentSize);

	if (module->DataSegmentSize)
	   printfScreenWithAttribute(screen,
				  attr,
				  "   Data Segment   : %08X  Data Size   : (%08X) %d bytes\n",
				  module->DataSegmentPtr,
				  module->DataSegmentSize,
				  module->DataSegmentSize);

	if (extendedInfo)
	{
	   if (module->RelocSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   Reloc Segment  : %08X  Reloc Size  : (%08X) %d bytes\n",
				  module->RelocSegmentPtr,
				  module->RelocSegmentSize,
				  module->RelocSegmentSize);

	   if (module->InitSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   Init Segment   : %08X  Init Size   : (%08X) %d bytes\n",
				  module->InitSegmentPtr,
				  module->InitSegmentSize,
				  module->InitSegmentSize);

	   if (module->ImportSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   Import Segment : %08X  Import Size : (%08X) %d bytes\n",
				  module->ImportSegmentPtr,
				  module->ImportSegmentSize,
				  module->ImportSegmentSize);

	   if (module->ExportSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   Export Segment : %08X  Export Size : (%08X) %d bytes\n",
				  module->ExportSegmentPtr,
				  module->ExportSegmentSize,
				  module->ExportSegmentSize);

	   if (module->SharedSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   Shared Segment : %08X  Shared Size : (%08X) %d bytes\n",
				  module->SharedSegmentPtr,
				  module->SharedSegmentSize,
				  module->SharedSegmentSize);


	   if (module->RPCSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   RPC Segment    : %08X  RPC Size    : (%08X) %d bytes\n",
				  module->RPCSegmentPtr,
				  module->RPCSegmentSize,
				  module->RPCSegmentSize);


	   if (module->ResourceSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   Resrce Segment : %08X  Resrce Size : (%08X) %d bytes\n",
				  module->ResourceSegmentPtr,
				  module->ResourceSegmentSize,
				  module->ResourceSegmentSize);

	   if (module->DebugSegmentSize)
	      printfScreenWithAttribute(screen,
				  attr,
				  "   Debug Segment  : %08X  Debug Size  : (%08X) %d bytes\n",
				  module->DebugSegmentPtr,
				  module->DebugSegmentSize,
				  module->DebugSegmentSize);

	}
	module = module->next;
     }
     ClearPauseMode(screen);

}

void RemoveModuleReferences(MODULE_HANDLE *module)
{

    register MODULE_HANDLE *importModule;
    register IMPORT *importList, *import;
    register LONG flags;

    flags = get_flags();
    spin_lock(&module->InternalReferenceLock);
    import = module->intHead;
    while (import)
    {
       importList = import->importedNext;
       importModule = import->providerModuleContext;

       if (module->intHead == import)
       {
	  module->intHead = (void *) import->importedNext;
	  if (module->intHead)
	     module->intHead->importedPrior = NULL;
	  else
	     module->intTail = NULL;
       }
       else
       {
	  import->importedPrior->importedNext = import->importedNext;
	  if (import != module->intTail)
	     import->importedNext->importedPrior = import->importedPrior;
	  else
	     module->intTail = import->importedPrior;
       }
       if (module->InternalReferenceCount)
	  module->InternalReferenceCount--;

       spin_lock(&importModule->ExternalReferenceLock);
       if (importModule->extHead == import)
       {
	  importModule->extHead = (void *) import->providerNext;
	  if (importModule->extHead)
	     importModule->extHead->providerPrior = NULL;
	  else
	     importModule->extTail = NULL;
       }
       else
       {
	  import->providerPrior->providerNext = import->providerNext;
	  if (import != importModule->extTail)
	     import->providerNext->providerPrior = import->providerPrior;
	  else
	     importModule->extTail = import->providerPrior;
       }
       if (importModule->ExternalReferenceCount)
	  importModule->ExternalReferenceCount--;
       spin_unlock(&importModule->ExternalReferenceLock);

       kfree(import);
       import = importList;
    }
    spin_unlock(&module->InternalReferenceLock);
    set_flags(flags);

    return;

}

LONG AddImportReference(MODULE_HANDLE *module,
			MODULE_HANDLE *importModule,
			EXPORT *exportRecord,
			LONG exportValue)
{

    register LONG retCode, flags;
    IMPORT *import;

    import = (IMPORT *) kmalloc(sizeof(IMPORT));
    if (!import)
       return (-1);

    SetData((LONG *)import, 0, sizeof(IMPORT));
    import->providerModuleContext = importModule;
    import->importedModuleContext = module;
    import->referenceObject = exportRecord;
    import->referenceAddress = exportValue;

    flags = get_flags();
    spin_lock(&module->InternalReferenceLock);
    if (!module->intHead)
    {
       module->intHead = import;
       module->intTail = import;
       import->importedNext = import->importedPrior = 0;
    }
    else
    {
       module->intTail->importedNext = import;
       import->importedNext = 0;
       import->importedPrior = module->intTail;
       module->intTail = import;
    }
    module->InternalReferenceCount++;
    spin_unlock(&module->InternalReferenceLock);

    spin_lock(&importModule->ExternalReferenceLock);
    if (!importModule->extHead)
    {
       importModule->extHead = import;
       importModule->extTail = import;
       import->providerNext = import->providerPrior = 0;
    }
    else
    {
       importModule->extTail->providerNext = import;
       import->providerNext = 0;
       import->providerPrior = importModule->extTail;
       importModule->extTail = import;
    }
    importModule->ExternalReferenceCount++;
    spin_unlock(&importModule->ExternalReferenceLock);
    set_flags(flags);

    return (LONG) 0;

}

LONG CheckModuleReferences(SCREEN *screen, MODULE_HANDLE *module)
{
    register LONG i, flags;
    register IMPORT *import;
    register MODULE_HANDLE *moduleTarget, *lastModule = 0;

    flags = get_flags();
    spin_lock(&module->ExternalReferenceLock);
    if (module->ExternalReferenceCount)
    {
       import = module->extHead;
       while (import)
       {
	  moduleTarget = import->importedModuleContext;
	  if (moduleTarget != lastModule)
	  {
	     printfScreen(screen,
			  "Module %s is being referenced by Module %s\n",
			  module->ModuleShortName,
			  moduleTarget->ModuleShortName);
	     lastModule = moduleTarget;
	  }
	  import = import->providerNext;
       }
       spin_unlock(&module->ExternalReferenceLock);
       set_flags(flags);
       return (LONG) -2;
    }
    spin_unlock(&module->ExternalReferenceLock);
    set_flags(flags);
    return (LONG) 0;

}

LONG UnloadFile(BYTE *name)
{

    register SCREEN *screen = activeScreen;
    register LONG key, retCode;
    MODULE_HANDLE *module;
    EXPORT *export, *exportList;

    module = GetModuleHandleFromName(name);
    if (!module)
       return -1;

    retCode = CheckModuleReferences(screen, module);
    if (retCode)
       return (retCode);

    if (module->warnProcedure)
    {
       if ((module->warnProcedure)(module))
       {
	 printfScreen(screen, "\nUnload Module? <y/n> ");
	 key = ReadKeyboard(screen);
	 printfScreen(screen, "%c\n", key);
	 if (toupper(key & 0xFF) == 'N')
	    return -3;
       }
    }

    if (module->unloadProcedure)
       (module->unloadProcedure)(module);

// DARREN AND JEFF STILL HAVE SOME QUESTIONS ???
//    if (module->exitProcedure)
//       (module->exitProcedure)(module);

    killThread(module->defaultProcess);

    RemoveModuleReferences(module);
    RemoveModuleExports(module);

    if (module->DebugSegmentPtr)
	ModuleFreeSymbolTable(module);

    remove_module(module);

    if (module->BaseAddress)
       free((void *)module->BaseAddress);

    kfree((void *)module);

    return 0;

}


//
//  PE Format section directory by compiler/linker type
//
//  .text  - Microsoft C code segment
//  .data  - Microsoft C data segment
//   CODE  - Borland C++ code segment
//   DATA  - Borland C++ data segment
//  .reloc - relocation table
//  .debug - debug symbol table
//  .rdata - Microsoft C Debug Symbol Table
//  .edata - Microsoft C exports section
//  .idata - Microsoft C imports section
//  .icode - Borland C++ imports thunk section
//   INIT  - Microsoft C initialization segment
//  .sdata - Microsoft C shared data segment
//  .orpc  - Microsoft C remote procedure call segment
//  .rsrc  - Microsoft C resource segment
//

LONG LoadFile(BYTE *filename, MODULE_HANDLE **newModule)
{

     register LONG i, r, j, retCode, ImportError;
     LONG fd, fp;
     LONG *functions;
     WORD *ordinals;
     LONG *name;
     LONG bytesRead;
     BYTE *section_ptr, *p;
     LONG section_raw_size, section_virt_size;
     LONG section_rva, section_blocks, section_raw_offset;
     LONG export_rva, import_rva;
     MODULE_HANDLE *module, *referenceModule, *importModule;
     IMAGE_DATA_DIRECTORY dataDir;
     IMAGE_DOS_HEADER dosHeader;
     IMAGE_NT_HEADERS pNTHeader;
     IMAGE_SECTION_HEADER section;
     PBASE_RELOCATION baseReloc;
     PIMAGE_EXPORT_DIRECTORY exportDir;
     EXPORT *export, *exportRecord;
     BYTE namePointer[100]={""};
     BYTE extPointer[100]={""};

#if (!MULTI_LOAD)
     module = GetModuleHandleFromName(filename);
     if (module)
	return (FILE_DUP_ERROR);
#endif

     fd = IFSOpenFile(filename, O_RDONLY | O_DENYNONE);
     if (!fd)
	   return (FILE_NOT_FOUND);

     // set stream to SEEK_SET(0)
     IFSFileSeek(fd, 0, SEEK_SET);

     // read DOS MZ header
     IFSReadFile(fd, (BYTE *)&dosHeader, sizeof(IMAGE_DOS_HEADER));

     // if no PE header LFA, exit with error
     if (!dosHeader.e_lfanew)
	   return (FILE_INCOMPAT);

     // seek to PE header
     IFSFileSeek(fd, (LONG)dosHeader.e_lfanew, SEEK_SET);

     // read PE header
     IFSReadFile(fd, (BYTE *)&pNTHeader, sizeof(IMAGE_NT_HEADERS));

     // if not an NT executable, exit with error
     if (pNTHeader.Signature != IMAGE_NT_SIGNATURE)
     {
	   IFSCloseFile(fd);
	   return (FILE_INCOMPAT);
     }

     // clone the file handle for section loading

     fp = DOSOpen(filename, O_RDONLY | O_DENYNONE);
     if (!fd)
     {
	   IFSCloseFile(fd);
	   return (FILE_NOT_FOUND);
     }

     module = (MODULE_HANDLE *) kmalloc(sizeof(MODULE_HANDLE));
     if (!module)
     {
	   IFSCloseFile(fd);
	   IFSCloseFile(fp);
	   return (FILE_MEMORY_ERROR);
     }

     SetData((LONG *) module, 0, sizeof(MODULE_HANDLE));
     module->ModuleSignature = MODULE_SIGNATURE;
     module->PEBase = pNTHeader.OptionalHeader.ImageBase;
     if (!module->PEBase)
	   module->PEBase = PE_FILE_DEFAULT_BASE;

     for (p = filename, i=0; p[i] && i < COMMANDLINE_LEN; i++)
	   module->commandLine[i] = p[i];

     module->EntryRVA = pNTHeader.OptionalHeader.AddressOfEntryPoint;
     module->BaseSize = pNTHeader.OptionalHeader.SizeOfImage;
     module->NumberOfSections = module->SectionNumber = pNTHeader.FileHeader.NumberOfSections;
     module->resourceHead = module->resourceTail = 0;
     module->resourceLock = module->resourceNum = 0;

     module->extHead = module->extTail = 0;
     module->ExternalReferenceLock = module->ExternalReferenceCount = 0;
     module->intHead = module->intTail = 0;
     module->InternalReferenceLock = module->InternalReferenceCount = 0;

     if (!module->BaseSize || !module->SectionNumber)
     {
	   kfree((void *)module);
	   IFSCloseFile(fd);
	   IFSCloseFile(fp);
	   return (FILE_CORRUPT_ERROR);
     }

     module->BaseAddress = (LONG) malloc(module->BaseSize);
     if (!module->BaseAddress)
     {
	   kfree((void *)module);
	   IFSCloseFile(fd);
	   IFSCloseFile(fp);
	   return (FILE_MEMORY_ERROR);
     }

     SetData((LONG *)module->BaseAddress, 0, module->BaseSize);

     // strip path string from filename
     retCode = filenameSplit(filename, 0, 0, namePointer, extPointer);
     if (retCode & FN_FILENAME && retCode & FN_EXTENSION)
     {
	strcat(namePointer, extPointer);
	strupr(namePointer);
	strcpy(module->ModuleName, namePointer);
	strcpy(module->ModuleShortName, namePointer);
     }
     else
     {
	strupr(filename);
	strcpy(module->ModuleName, filename);
	strcpy(module->ModuleShortName, filename);
     }
     module->ModuleNameLength = strlen(module->ModuleName);
     module->ModuleShortNameLength = strlen(module->ModuleShortName);

     dataDir = pNTHeader.OptionalHeader.DataDirectory[IMAGE_FILE_IMPORT_DIRECTORY];
     module->ImportDirectory = module->BaseAddress + dataDir.VirtualAddress;
     module->ImportDirectorySize = dataDir.Size;
     dataDir = pNTHeader.OptionalHeader.DataDirectory[IMAGE_FILE_EXPORT_DIRECTORY];
     module->ExportDirectory = module->BaseAddress + dataDir.VirtualAddress;
     module->ExportDirectorySize = dataDir.Size;

     module->ModuleStart = module->BaseAddress;
     module->ModuleEnd = module->BaseAddress + module->BaseSize;

     IFSReadFile(fd, (BYTE *)&section, sizeof(IMAGE_SECTION_HEADER));
     for (i=0; i < pNTHeader.FileHeader.NumberOfSections; i++)
     {
	   if (i >= 32)
	   {
	      free((void *)module->BaseAddress);
	      kfree((void *)module);
	      IFSCloseFile(fd);
	      IFSCloseFile(fp);
	      return FILE_LINK_ERROR;
	   }
	   section_rva = section.VirtualAddress;
	   section_raw_offset = section.PointerToRawData;
	   section_raw_size = section.SizeOfRawData;
	   section_virt_size = section.Misc.VirtualSize;
	   section_blocks = (section_raw_size + 4095) / 4096;
	   module->SegmentIndexTable[i + 1] = module->BaseAddress + section_rva;
	   module->SegmentTypeTable[i + 1] = section.Characteristics;
	   module->SegmentLengthTable[i + 1] = section_virt_size;
	   if (!strnicmp(section.Name, ".text", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      if (module->CodeSegmentPtr)
	      {
		    free((void *)module->BaseAddress);
		    kfree((void *)module);
		    IFSCloseFile(fd);
		    IFSCloseFile(fp);
		    return FILE_LINK_ERROR;
	      }
	      module->CodeSegmentPtr = module->BaseAddress + section_rva;
	      module->CodeSegmentSize = section_virt_size;
	   }
	   else
	   if (!strnicmp(section.Name, "CODE", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      if (module->CodeSegmentPtr)
	      {
		    free((void *)module->BaseAddress);
		    kfree((void *)module);
		    IFSCloseFile(fd);
		    IFSCloseFile(fp);
		    return FILE_LINK_ERROR;
	      }
	      module->CodeSegmentPtr = module->BaseAddress + section_rva;
	      module->CodeSegmentSize = section_virt_size;
	   }
	   else
	   if (!strnicmp(section.Name, ".data", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      if (module->DataSegmentPtr)
	      {
		    free((void *)module->BaseAddress);
		    kfree((void *)module);
		    IFSCloseFile(fd);
		    IFSCloseFile(fp);
		    return FILE_LINK_ERROR;
	      }
	      module->DataSegmentPtr = module->BaseAddress + section_rva;
	      module->DataSegmentSize = section_virt_size;
	   }
	   else
	   if (!strnicmp(section.Name, "DATA", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      if (module->DataSegmentPtr)
	      {
		    free((void *)module->BaseAddress);
		    kfree((void *)module);
		    IFSCloseFile(fd);
		    IFSCloseFile(fp);
		    return FILE_LINK_ERROR;
	      }
	      module->DataSegmentPtr = module->BaseAddress + section_rva;
	      module->DataSegmentSize = section_virt_size;
	   }
	   else
	   if (!strnicmp(section.Name, ".reloc", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      module->RelocSegmentPtr = module->BaseAddress + section_rva;
	      module->RelocSegmentSize = section_virt_size;
	   }
	   else
	   if (!strnicmp(section.Name, ".debug", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      module->DebugSegmentPtr = module->BaseAddress + section_rva;
	      module->DebugSegmentSize = section_raw_size;
	      module->DebugSymbolType = FORMAT_BORLAND;
	   }
	   else
	   if (!strnicmp(section.Name, ".rdata", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      module->DebugSegmentPtr = module->BaseAddress + section_rva;
	      module->DebugSegmentSize = section_raw_size;
	      module->DebugSymbolType = FORMAT_BORLAND;
	   }
	   else
	   if (!strnicmp(section.Name, ".idata", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      module->ImportSegmentPtr = module->BaseAddress + section_rva;
	      module->ImportSegmentSize = section_virt_size;
	   }
	   else
	   if (!strnicmp(section.Name, ".icode", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      module->ImportSegmentPtr = module->BaseAddress + section_rva;
	      module->ImportSegmentSize = section_virt_size;
	   }
	   else
	   if (!strnicmp(section.Name, ".edata", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      module->ExportSegmentPtr = module->BaseAddress + section_rva;
	      module->ExportSegmentSize = section_virt_size;
	   }
	   else
	   if (!strnicmp(section.Name, "INIT", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      module->InitSegmentPtr = module->BaseAddress + section_rva;
	      module->InitSegmentSize = section_virt_size;
	   }
	   else
	   if (!strnicmp(section.Name, ".init", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      module->InitSegmentPtr = module->BaseAddress + section_rva;
	      module->InitSegmentSize = section_virt_size;
	   }
	   else
	   if (!strnicmp(section.Name, ".sdata", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      module->SharedSegmentPtr = module->BaseAddress + section_rva;
	      module->SharedSegmentSize = section_virt_size;
	   }
	   else
	   if (!strnicmp(section.Name, ".orpc", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      module->RPCSegmentPtr = module->BaseAddress + section_rva;
	      module->RPCSegmentSize = section_virt_size;
	   }
	   else
	   if (!strnicmp(section.Name, ".rsrc", IMAGE_SIZEOF_SHORT_NAME))
	   {
	      module->ResourceSegmentPtr = module->BaseAddress + section_rva;
	      module->ResourceSegmentSize = section_virt_size;
	   }

	   if (section_raw_offset)
	   {
	      IFSFileSeek(fp, section_raw_offset, SEEK_SET);
	      section_ptr = (BYTE *) module->BaseAddress + section_rva;
	      for (r=0; r < section_blocks; r++)
	      {
		 bytesRead = IFSReadFile(fp, section_ptr, 4096);
		 if (!bytesRead)
		 {
		    free((void *)module->BaseAddress);
		    kfree((void *)module);
		    IFSCloseFile(fd);
		    IFSCloseFile(fp);
		    return FILE_READ_ERROR;
		 }
		 section_ptr = (BYTE *) ((LONG) section_ptr + 4096);
	      }
	   }
	   IFSReadFile(fd, (BYTE *)&section, sizeof(IMAGE_SECTION_HEADER));
     }
     IFSCloseFile(fd);
     IFSCloseFile(fp);

     // perform relocation fixup of all sections

     baseReloc = (PBASE_RELOCATION) module->RelocSegmentPtr;
     while (baseReloc && baseReloc->SizeOfBlock != 0)
     {
	LONG i, cEntries;
	PWORD pEntry;
	LONG ImageOffset, *ImagePtr;
	LONG ImageEntry;
	WORD relocType;

	cEntries = (baseReloc->SizeOfBlock - sizeof(BASE_RELOCATION))/sizeof(WORD);
	pEntry = MakePtr(PWORD, baseReloc, sizeof(BASE_RELOCATION));
	for ( i=0; i < cEntries; i++ )
	{
	   relocType = (*pEntry & 0xF000) >> 12;
	   switch (relocType)
	   {
	     case 0:    // ABSOLUTE fixup entry.  ingore absolute fixups
		break;  // on Intel.  Inserted for Alignment

	     case 3:   // HIGHLOW Intel IA32 fixup entry
		ImageOffset = (LONG)(*pEntry & 0x0FFF) + baseReloc->VirtualAddress;
		ImageOffset += module->BaseAddress;
		ImagePtr = (LONG *) ImageOffset;
		ImageEntry = *ImagePtr;
		ImageEntry -= module->PEBase;
		ImageEntry += module->BaseAddress;
		*ImagePtr = ImageEntry;
		break;

	     default:
		break;
	   }
	   pEntry++;	// Advance to next relocation entry
	}
	baseReloc = MakePtr(PBASE_RELOCATION, baseReloc, baseReloc->SizeOfBlock);
     }

     // resolve imported functions

     SetPauseMode(consoleScreen, consoleScreen->nLines - 3);

     ImportError = 0;
     if (module->ImportDirectory)
     {
	PIMAGE_IMPORT_DESCRIPTOR importDesc;
	PIMAGE_THUNK_DATA ImportNameTable;
	PIMAGE_IMPORT_BY_NAME pOrdinalName;
	PBYTE moduleName;
	LONG ExportAddress;

	importDesc = (PIMAGE_IMPORT_DESCRIPTOR) module->ImportDirectory;
	while (importDesc->Name != 0)
	{
	   if (importDesc->u.Characteristics)
	   {
	      ImportNameTable = (PIMAGE_THUNK_DATA)((LONG)importDesc->u.Characteristics + module->BaseAddress);
	      moduleName = (PBYTE)(importDesc->Name) + module->BaseAddress;
	      referenceModule = GetModuleHandleFromName(moduleName);
	      if (!referenceModule)
	      {
		 MODULE_HANDLE *newModule;
		 if (LoadFile(moduleName, &newModule))
		 {
		    printf("could not load file %s\n", strupr(moduleName));
		    ImportError = 1;
		 }
		 else
		    printf("%s Loaded\n", newModule->ModuleShortName);
	      }
#if (VERBOSE)
	      printf("%s\n", (PBYTE)(importDesc->Name) + module->BaseAddress);
	      printf("Import Table    : %08X\n", ImportNameTable);
#endif

	      while (ImportNameTable->u1.AddressOfData != 0)
	      {
		 if (ImportNameTable->u1.Ordinal & 0x80000000)
		 {
		    printf("[%08X] ordinal %s|%04X.%04X is Unresolved\n",
			    ImportNameTable,
			    (BYTE *)((LONG)importDesc->Name) + module->BaseAddress,
			    (LONG)(ImportNameTable->u1.Ordinal >> 16) & 0xFFFF,
			    (LONG)ImportNameTable->u1.Ordinal & 0xFFFF);
		    ImportError = 1;
		 }
		 else
		 {
		    pOrdinalName = (PIMAGE_IMPORT_BY_NAME)((LONG)ImportNameTable->u1.AddressOfData +
				   module->BaseAddress);
#if (VERBOSE)
		    printf("[%08X] ordinal %04x %s\n",
			    ImportNameTable,
			    pOrdinalName->Hint,
			    pOrdinalName->Name);
#endif
		    ExportAddress = ImportExportedAddress(pOrdinalName->Name,
							  moduleName,
							  &importModule,
							  &exportRecord);
#if (VERBOSE)
		    printf("GetExportAddress(%s, %s)=%08X\n",
			    pOrdinalName->Name,
			    moduleName,
			    ExportAddress);
#endif
		    if (ExportAddress == (LONG)-1)
		    {
		       printf("Unresolved External: %s|%s\n",
			     (BYTE *)((LONG)importDesc->Name) + module->BaseAddress,
			     pOrdinalName->Name);
		       ImportError = 1;
		    }
		    else
		    {
		       retCode = AddImportReference(module, importModule,
						    exportRecord,
						    ExportAddress);
		       if (!retCode)
		       {
			  EventNotify(EVENT_IMPORT_SYMBOL, (LONG) pOrdinalName->Name);
			  ImportNameTable->u1.Function = (LPDWORD)ExportAddress;
		       }
		       else
			  ImportError = 1;
		    }
		 }
		 ImportNameTable++;
	      }
	   }
	   else if (importDesc->FirstThunk)
	   {
	      ImportNameTable = (PIMAGE_THUNK_DATA)((LONG)importDesc->FirstThunk + module->BaseAddress);
	      moduleName = (PBYTE)(importDesc->Name) + module->BaseAddress;
	      referenceModule = GetModuleHandleFromName(moduleName);
	      if (!referenceModule)
	      {
		 MODULE_HANDLE *newModule;

		 if (LoadFile(moduleName, &newModule))
		 {
		    printf("could not load file %s\n", strupr(moduleName));
		    ImportError = 1;
		 }
		 else
		    printf("%s Loaded\n", newModule->ModuleShortName);
	      }

#if (VERBOSE)
	      printf("%s\n", (PBYTE)(importDesc->Name) + module->BaseAddress);
	      printf("Import Table    : %08X\n", ImportNameTable);
#endif
	      while (ImportNameTable->u1.AddressOfData != 0)
	      {
		 if (ImportNameTable->u1.Ordinal & 0x80000000)
		 {
		    printf("[%08X] ordinal %s|%04X.%04X\n",
			    ImportNameTable,
			    (BYTE *)((LONG)importDesc->Name) + module->BaseAddress,
			    (LONG)(ImportNameTable->u1.Ordinal >> 16) & 0xFFFF,
			    (LONG)ImportNameTable->u1.Ordinal & 0xFFFF);
		    ImportError = 1;
		 }
		 else
		 {
		    pOrdinalName = (PIMAGE_IMPORT_BY_NAME)((LONG)ImportNameTable->u1.AddressOfData +
				   module->BaseAddress);
#if (VERBOSE)
		    printf("[%08X] ordinal %04x %s\n",
			    ImportNameTable,
			    pOrdinalName->Hint,
			    pOrdinalName->Name);
#endif
		    ExportAddress = ImportExportedAddress(pOrdinalName->Name,
							  moduleName,
							  &importModule,
							  &exportRecord);
#if (VERBOSE)
		    printf("GetExportAddress(%s, %s)=%08X\n",
			    pOrdinalName->Name,
			    moduleName,
			    ExportAddress);
#endif
		    if (ExportAddress == (LONG)-1)
		    {
		       printf("Unresolved External: %s|%s\n",
			     (BYTE *)((LONG)importDesc->Name) + module->BaseAddress,
			     pOrdinalName->Name);
		       ImportError = 1;
		    }
		    else
		    {
		       retCode = AddImportReference(module, importModule,
						    exportRecord,
						    ExportAddress);
		       if (!retCode)
		       {
			  EventNotify(EVENT_IMPORT_SYMBOL, (LONG) pOrdinalName->Name);
			  ImportNameTable->u1.Function = (LPDWORD)ExportAddress;
		       }
		       else
			  ImportError = 1;
		    }
		 }
		 ImportNameTable++;
	      }
	   }
	   importDesc++;
	}
	if (ImportError)
	{
	   RemoveModuleReferences(module);
	   ClearPauseMode(consoleScreen);
	   free((void *)module->BaseAddress);
	   kfree((void *)module);
	   return (FILE_UNRESOLVED);
	}
     }

     // post exported functions

     if (module->ExportDirectory)
     {
	exportDir = (PIMAGE_EXPORT_DIRECTORY) module->ExportDirectory;

#if (VERBOSE)
	printf("Exported Functions\n");
	printf("Name Address Table  : %08X\n", ((LONG)exportDir->AddressOfNames + module->BaseAddress));
	printf("Function Table      : %08X\n", ((LONG)exportDir->AddressOfFunctions + module->BaseAddress));
	printf("Ordinal Table       : %08X\n", ((LONG)exportDir->AddressOfNameOrdinals + module->BaseAddress));
#endif

	functions = (LONG *)((LONG)exportDir->AddressOfFunctions + module->BaseAddress);
	ordinals = (WORD *)((LONG)exportDir->AddressOfNameOrdinals + module->BaseAddress);
	name = (LONG *)((LONG)exportDir->AddressOfNames + module->BaseAddress);

	for (j=0; j < exportDir->NumberOfNames; j++)
	{

	   register LONG ExportOrdinal;
	   register BYTE *ExportName;
	   register LONG ExportAddress;

	   ExportOrdinal = *ordinals + exportDir->Base;
	   ExportName = (BYTE *) *name + module->BaseAddress;
	   ExportAddress = functions[ExportOrdinal - 1] + module->BaseAddress;
	   retCode = RegisterModuleExport(module, ExportName, ExportAddress,
					  ExportOrdinal, EXPORT_PROC);
	   switch (retCode)
	   {
	      case 0:   // success
		 break;

	      case -1:  // out of memory
		 RemoveModuleReferences(module);
		 RemoveModuleExports(module);
		 ClearPauseMode(consoleScreen);
		 free((void *)module->BaseAddress);
		 kfree((void *)module);
		 return (FILE_MEMORY_ERROR);

	      case -2:  // export already exists
		 printfScreen(consoleScreen,
			      "Export: %s in Module: %s already exists\n",
			      (BYTE *) *name + module->BaseAddress,
			      module->ModuleShortName);
		 break;

	   }
	   name++;
	   ordinals++;
	}
     }

     ClearPauseMode(consoleScreen);

     if (module->DebugSegmentPtr)
	ModuleInitializeSymbolTable(module);

     module->entryPoint = (LONG (*)(LONG, BYTE **, LONG))((LONG)module->BaseAddress + \
					      (LONG)module->EntryRVA);


     // spawn initialization thread

     module->defaultProcess = createThread(filename,
			  (LONG (*)())_moduleInit,
			  pNTHeader.OptionalHeader.SizeOfStackCommit > 0
			  ? pNTHeader.OptionalHeader.SizeOfStackCommit
			  : 8192,
			  module,
			  -1);

     if (!module->defaultProcess)
     {
	RemoveModuleReferences(module);
	RemoveModuleExports(module);
	free((void *)module->BaseAddress);
	kfree((void *)module);
	return (FILE_PROCESS_ERROR);
     }

     // link module into global list
     insert_module(module);

     if (newModule)
	*newModule = module;


     return 0;

}

LONG RegisterUnloadProcedure(MODULE_HANDLE *module, LONG (*unloadProc)(MODULE_HANDLE *))
{
    if (!module->unloadProcedure)
    {
       module->unloadProcedure = (LONG (*)(MODULE_HANDLE *)) unloadProc;
       return 0;
    }
    return -1;
}

LONG RegisterExitProcedure(MODULE_HANDLE *module, LONG (*exitProc)(MODULE_HANDLE *))
{
    if (!module->exitProcedure)
    {
       module->exitProcedure = (LONG (*)(MODULE_HANDLE *)) exitProc;
       return 0;
    }
    return -1;
}

LONG RegisterWarnProcedure(MODULE_HANDLE *module, LONG (*warnProc)(MODULE_HANDLE *))
{
    if (!module->warnProcedure)
    {
       module->warnProcedure = (LONG (*)(MODULE_HANDLE *)) warnProc;
       return 0;
    }
    return -1;
}


