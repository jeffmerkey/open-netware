

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
*   FILE     :  LOWMEM.C
*   DESCRIP  :  Low Memory Allocator for MANOS v1.0
*   DATE     :  November 2, 1997
*
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
#include "bus.h"

typedef struct _LowMemory {
    LONG pageCount;
    BYTE *address;
    LONG chainID;
    LONG useCount;
} LOW_MEMORY;

LONG TotalLowMemoryPages = 0;
LONG ValidLowMemoryPages = 0;
LONG LowMemoryTableSize = 0;

LOW_MEMORY *LowMemoryTableStart = 0;
BYTE *LowMemoryStart = 0;
LONG LowMemoryLock = 0;

DOS_TABLE *DosDataTable;
LONG BaseSegment;
LONG CodeSegment;
LONG CodeRVA;
LONG DataSegment;
LONG RelocSegment;
LONG DebugSegment;
LONG DebugSize;
LONG ExportSegment;
LONG ExportSize;
LONG ExportRVA;
LONG LoaderCodeSegment;
LONG LoaderDataSegment;
LONG StartOfHighMemory;
LONG HighMemoryLength;
LONG StartupMemory;
LONG StartupLength;
LONG FloppyDMAPage1;
LONG FloppyDMAPage2;
LONG StartOfLowMemory;
LONG LowMemoryLength;
LONG TotalSystemMemory;
LONG MCAMemory;
LONG EISAMemory;
LONG PCIMemory;
LONG oldZero;
LONG keyboard_lock;

void InitializeMemoryBelow1MB(LONG start, LONG length)
{

    register BYTE *lowMemory;
    register LONG i;

    TotalLowMemoryPages = length / 4096;
    LowMemoryTableSize = TotalLowMemoryPages * sizeof(LOW_MEMORY);
    ValidLowMemoryPages = TotalLowMemoryPages - ((LowMemoryTableSize + (4096 - 1)) / 4096);

    LowMemoryTableStart = (LOW_MEMORY *) start;
    LowMemoryStart = (BYTE *)(((start + LowMemoryTableSize) + 0xFFF) & 0xFFFFF000);

    lowMemory = LowMemoryStart;
    for (i=0; i < ValidLowMemoryPages; i++)
    {
       LowMemoryTableStart[i].useCount = (LONG) -1;
       LowMemoryTableStart[i].chainID = (LONG) -1;
       LowMemoryTableStart[i].pageCount = 0;
       LowMemoryTableStart[i].address = lowMemory;
       lowMemory = (BYTE *)((LONG) lowMemory + (LONG) 4096);
    }

}

void *AllocateMemoryBelow1MB(LONG size)
{

    register void *p = 0;
    register LONG pages;
    register LONG i, count, j;
    register LONG flags;

    pages = (size + (4096 - 1)) / 4096;

    flags = get_flags();
    spin_lock(&LowMemoryLock);

    for (i=0; i < ValidLowMemoryPages; i++)
    {
       if (LowMemoryTableStart[i].useCount == (LONG) -1)
       {
	  register LONG allocated = 0;

	  for (count = 0, j = i; count < pages; count++, j++)
	  {
	     if (LowMemoryTableStart[j].useCount != (LONG) -1)
	     {
		allocated = 1;
	     }
	  }
	  if (!allocated)
	  {
	     for (count = 0, j = i; count < pages; count++, j++)
	     {
		LowMemoryTableStart[j].pageCount = pages;
		LowMemoryTableStart[j].useCount = 0;
		LowMemoryTableStart[j].chainID = i;
	     }
	     p = (void *) LowMemoryTableStart[i].address;
	     break;
	  }
       }
    }

    spin_unlock(&LowMemoryLock);
    set_flags(flags);

    return p;

}

LONG FreeMemoryBelow1MB(void *address)
{

    register LONG i, count, j, pages;
    register LONG flags;

    flags = get_flags();
    spin_lock(&LowMemoryLock);

    for (i=0; i < ValidLowMemoryPages; i++)
    {
       if (LowMemoryTableStart[i].address == address &&
	   LowMemoryTableStart[i].useCount != (LONG) -1)
       {
	  pages = LowMemoryTableStart[i].pageCount;
	  for (count = 0, j = i; count < pages; count++, j++)
	  {
	     LowMemoryTableStart[j].pageCount = 0;
	     LowMemoryTableStart[j].useCount = (LONG) -1;
	     LowMemoryTableStart[j].chainID = (LONG) -1;
	  }
	  break;
       }
    }

    spin_unlock(&LowMemoryLock);
    set_flags(flags);

    return 0;

}

LONG InitializeBusMemory(DOS_TABLE *dos, PM_TABLE *pm)
{
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
    extern LONG FloppyDMAPage1;
    extern LONG FloppyDMAPage2;
    extern LONG StartOfLowMemory;
    extern LONG LowMemoryLength;
    extern LONG TotalSystemMemory;
    extern LONG MCAMemory;
    extern LONG EISAMemory;
    extern LONG PCIMemory;
    extern LONG OSPAETable;
    extern LONG OSPageTable;
    extern LONG OSPageTableSize;
    extern LONG OSPageTableEnd;
    extern LONG OSPDE;
    extern LONG DebuggerPAETable;
    extern LONG DebuggerPageTable;
    extern LONG DebuggerPageTableSize;
    extern LONG DebuggerPageTableEnd;
    extern LONG DebuggerPDE;
    extern LONG DefaultPageSize;
    extern LONG GetMCAMemory(void);
    extern LONG GetEISAMemory(void);
    extern void SetSystemPageSize(DOS_TABLE *dos, LONG *mem_start, LONG *mem_length);
    extern void InitializeBasePageTables(LONG SystemMemorySize);

    BaseSegment = pm->BaseSegment;
    CodeSegment = pm->CodeSegment;
    CodeRVA = pm->CodeRVA;
    DataSegment = pm->DataSegment;
    RelocSegment = pm->RelocSegment;
    DebugSegment = pm->DebugSegment;
    DebugSize = pm->DebugSize;
    ExportSegment = pm->ExportSegment;
    ExportSize = pm->ExportSize;
    ExportRVA = pm->ExportRVA;
    LoaderCodeSegment = pm->LoaderCodeSegment;
    LoaderDataSegment = pm->LoaderDataSegment;
    StartOfLowMemory = pm->StartOfLowMemory;
    LowMemoryLength = pm->LowMemoryLength;
    InitializeMemoryBelow1MB(StartOfLowMemory, LowMemoryLength);

    StartupLength = 0x1000;
    StartupMemory = (LONG) AllocateMemoryBelow1MB(StartupLength);
    if (!StartupMemory)
       return -1;

    FloppyDMAPage1 = (LONG) AllocateMemoryBelow1MB(0x1000);
    if (!FloppyDMAPage1)
       return -1;

    FloppyDMAPage2 = (LONG) AllocateMemoryBelow1MB(0x1000);
    if (!FloppyDMAPage2)
       return -1;

    //
    //  BIOS and/or XMS may not have reported all of system memory
    //  the next two functions verify system memory
    //  for MCA and EISA by scanning the EISA bus configuration and/or
    //  the MCA memory map.
    //

    if (DosDataTable->DOS_SYSTEM_BUS_TYPE & MCA)
       MCAMemory = GetMCAMemory();

    if (DosDataTable->DOS_SYSTEM_BUS_TYPE & EISA)
       EISAMemory = GetEISAMemory();

    TotalSystemMemory = (dos->MEMORY_HIGH_START + dos->MEMORY_HIGH) & 0xFFF00000;

    // if EISA or MCA report more memory than was reported
    // by the system BIOS, then adjust our total amount of memory

    if (TotalSystemMemory < MCAMemory)
       TotalSystemMemory = MCAMemory;

    if (TotalSystemMemory < EISAMemory)
       TotalSystemMemory = EISAMemory;

    StartOfHighMemory = pm->StartOfHighMemory;
    HighMemoryLength = TotalSystemMemory - StartOfHighMemory;

    SetSystemPageSize(dos, &StartOfHighMemory, &HighMemoryLength);
    InitializeBasePageTables(dos->MEMORY_HIGH);

    return 0;

}

LONG displaySystemMemoryInfo(void)
{

    extern LONG TotalSystemMemory;
    extern LONG HighMemoryLength;
    extern LONG LowMemoryLength;
    extern LONG CodeSegment;
    extern LONG DataSegment;
    extern LONG OSPageTable;
    extern LONG OSPageTableSize;
    extern LONG DebuggerPageTable;
    extern LONG DebuggerPageTableSize;
    BYTE displayBuffer[100] = { "" };

    sprintf_comma(displayBuffer, "%d", TotalSystemMemory);
    printfScreen(consoleScreen, "\nTotal System Memory        : %s bytes", displayBuffer);

    sprintf_comma(displayBuffer, "%d", HighMemoryLength);
    printfScreen(consoleScreen, "\nAvailable Memory Above 1MB : %s bytes", displayBuffer);

    sprintf_comma(displayBuffer, "%d", LowMemoryLength);
    printfScreen(consoleScreen, "\nAvailable Memory Below 1MB : %s bytes", displayBuffer);

    printfScreen(consoleScreen, "\nSystem Page Table          : %08X      System Table Size     : %08X ",
	   OSPageTable, OSPageTableSize);
    printfScreen(consoleScreen, "\nException Page Table       : %08X      Exception Table Size  : %08X ",
	   DebuggerPageTable, DebuggerPageTableSize);
    printfScreen(consoleScreen, "\n\nPE Code : %08X  Data : %08X\n\n", CodeSegment, DataSegment);

    return 0;

}
