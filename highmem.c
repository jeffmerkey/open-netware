

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
*   FILE     :  HIGHMEM.C
*   DESCRIP  :  High Memory Fixed Allocator for MANOS v1.0
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

typedef struct _HighMemory {
    LONG pageCount;
    BYTE *address;
    LONG chainID;
    LONG useCount;
} HIGH_MEMORY;

LONG TotalHighMemoryPages = 0;
LONG ValidHighMemoryPages = 0;
LONG HighMemoryTableSize = 0;

HIGH_MEMORY *HighMemoryTableStart = 0;
BYTE *HighMemoryStart = 0;
LONG HighMemoryLock = 0;

void InitializeMemoryBelow16MB(LONG start, LONG length)
{

    register BYTE *HighMemory;
    register LONG i;

    TotalHighMemoryPages = length / 4096;
    HighMemoryTableSize = TotalHighMemoryPages * sizeof(HIGH_MEMORY);
    ValidHighMemoryPages = TotalHighMemoryPages - ((HighMemoryTableSize + (4096 - 1)) / 4096);

    HighMemoryTableStart = (HIGH_MEMORY *) start;
    HighMemoryStart = (BYTE *)(((start + HighMemoryTableSize) + 0xFFF) & 0xFFFFF000);

    HighMemory = HighMemoryStart;
    for (i=0; i < ValidHighMemoryPages; i++)
    {
       HighMemoryTableStart[i].useCount = (LONG) -1;
       HighMemoryTableStart[i].chainID = (LONG) -1;
       HighMemoryTableStart[i].pageCount = 0;
       HighMemoryTableStart[i].address = HighMemory;
       HighMemory = (BYTE *)((LONG) HighMemory + (LONG) 4096);
    }

}

void *AllocateMemoryBelow16MB(LONG size)
{

    register void *p = 0;
    register LONG pages;
    register LONG i, count, j;
    register LONG flags;

    pages = (size + (4096 - 1)) / 4096;

    flags = get_flags();
    spin_lock(&HighMemoryLock);

    for (i=0; i < ValidHighMemoryPages; i++)
    {
       if (HighMemoryTableStart[i].useCount == (LONG) -1)
       {
	  register LONG allocated = 0;

	  for (count = 0, j = i; count < pages; count++, j++)
	  {
	     if (HighMemoryTableStart[j].useCount != (LONG) -1)
	     {
		allocated = 1;
	     }
	  }
	  if (!allocated)
	  {
	     for (count = 0, j = i; count < pages; count++, j++)
	     {
		HighMemoryTableStart[j].pageCount = pages;
		HighMemoryTableStart[j].useCount = 0;
		HighMemoryTableStart[j].chainID = i;
	     }
	     p = (void *) HighMemoryTableStart[i].address;
	     break;
	  }
       }
    }

    spin_unlock(&HighMemoryLock);
    set_flags(flags);

    return p;

}

LONG FreeMemoryBelow16MB(void *address)
{

    register LONG i, count, j, pages;
    register LONG flags;

    flags = get_flags();
    spin_lock(&HighMemoryLock);

    for (i=0; i < ValidHighMemoryPages; i++)
    {
       if (HighMemoryTableStart[i].address == address &&
	   HighMemoryTableStart[i].useCount != (LONG) -1)
       {
	  pages = HighMemoryTableStart[i].pageCount;
	  for (count = 0, j = i; count < pages; count++, j++)
	  {
	     HighMemoryTableStart[j].pageCount = 0;
	     HighMemoryTableStart[j].useCount = (LONG) -1;
	     HighMemoryTableStart[j].chainID = (LONG) -1;
	  }
	  break;
       }
    }

    spin_unlock(&HighMemoryLock);
    set_flags(flags);

    return 0;

}
