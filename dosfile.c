

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  DOSFILE.C
*   DESCRIP  :  MSDOS Real Mode File System Interface for MANOS v1.0
*   DATE     :  January 11, 1998
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
#include "peexe.h"
#include "window.h"
#include "dosfile.h"
#include "event.h"

extern LONG oldZero;
extern void *AllocateMemoryBelow1MB(LONG);
extern void FreeMemoryBelow1MB(void *);
extern void dspin_lock(LONG *);
extern void dspin_unlock(LONG *);

LONG inRealModeFlag = 0;
extern LONG tlb_sync;

LONG int86x(LONG intn, REGS *inregs, REGS *outregs)
{

   register LONG proc = get_processor_id();
   register LONG *low;

   if (proc)
   {
      panic("DOS access atempted by processor other than 0");
      return -1;
   }

   EventNotify(EVENT_ENTER_REAL_MODE, 0);

   dspin_lock(&tlb_sync);
   inRealModeFlag++;
   low = (LONG *) 0;
   *low = oldZero;
   ((void (*)(LONG, REGS *, REGS *))DosDataTable->REAL_MODE_INT)
				    (intn, inregs, outregs);
   *low = 0xF4CCCCCC; // three breakpoints and a halt
   inRealModeFlag--;
   dspin_unlock(&tlb_sync);

   EventNotify(EVENT_RETURN_REAL_MODE, 0);

   return 0;

}

long OutputText(BYTE *s)
{
   BYTE *op;
   register LONG p, RealModeBuffer, index;
   REGS *inregs, *outregs;

   inregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!inregs)
      return 0;

   outregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!outregs)
   {
      FreeMemoryBelow1MB(inregs);
      return 0;
   }

   RealModeBuffer = (LONG) AllocateMemoryBelow1MB(strlen(s) + 16);
   if (!RealModeBuffer)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return 0;
   }


   strcpy((char *)RealModeBuffer, s);
   SetData((LONG *) &inregs, 0, sizeof(REGS));
   op = (BYTE *) RealModeBuffer;
   index = strlen(op);
   op[index] = '$';
   p = (LONG) RealModeBuffer;
   inregs->eax = 0x0900;
   inregs->ds = ((LONG)(p >> 4) & (LONG) 0x0000FFFF);
   inregs->edx = ((LONG) p & (LONG) 0xF);

   int86x(0x21, inregs, outregs);

   FreeMemoryBelow1MB(inregs);
   FreeMemoryBelow1MB(outregs);
   FreeMemoryBelow1MB((void *)RealModeBuffer);

   return (outregs->flags);

}


LONG DOSFileSize(LONG fd)
{
    register LONG size, curr;

    curr = DOSLseek(fd, 0, SEEK_CUR);
    size = DOSLseek(fd, 0L, SEEK_END);
    DOSLseek(fd, curr, SEEK_SET);

    return size;
}

void DOSFlush(void)
{
   register REGS *inregs, *outregs;

   inregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!inregs)
      return;

   outregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!outregs)
   {
      FreeMemoryBelow1MB(inregs);
      return;
   }

   SetData((LONG *) inregs, 0, sizeof(REGS));
   inregs->eax = 0x0D00;
   int86x(0x21, inregs, outregs);

   FreeMemoryBelow1MB(inregs);
   FreeMemoryBelow1MB(outregs);

   return;

}

/***********************************************************************
*
*   flags  SEEK_SET = 0
*          SEEK_CUR = 1
*          SEEK_END = 2
*
*   returns  -1       - failed
*            offset   - current offset within file
*
************************************************************************/

LONG DOSLseek(LONG fd, LONG offset, LONG flags)
{
   register LONG retCode;
   register REGS *inregs, *outregs;

   inregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!inregs)
      return 0;

   outregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!outregs)
   {
      FreeMemoryBelow1MB(inregs);
      return 0;
   }

   SetData((LONG *) inregs, 0, sizeof(REGS));
   inregs->eax = ((LONG) 0x4200 | ((LONG) flags & 0xF));
   inregs->ebx = fd;
   inregs->ecx = ((offset >> 16) & 0x0000FFFF);
   inregs->edx = (offset & 0x0000FFFF);

   retCode = int86x(0x21, inregs, outregs);
   if (retCode || outregs->flags & 1)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return (-1);
   }
   else
   {
      retCode = ((outregs->edx << 16) | (outregs->eax & 0x0000FFFF));
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return (LONG) retCode;
   }
}

/*************************************************************************
*
*
*
*
*
*
*
*************************************************************************/

LONG DOSRead(LONG fd, void *buf, LONG size)
{

   register LONG p, retCode, RealModeBuffer;
   register REGS *inregs, *outregs;

   inregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!inregs)
      return 0;

   outregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!outregs)
   {
      FreeMemoryBelow1MB(inregs);
      return 0;
   }

   RealModeBuffer = (LONG) AllocateMemoryBelow1MB(size);
   if (!RealModeBuffer)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return 0;
   }

   SetData((LONG *) inregs, 0, sizeof(REGS));
   p = (LONG) RealModeBuffer;
   inregs->eax = 0x3F00;
   inregs->ds = ((LONG)(p >> 4) & (LONG) 0x0000FFFF);
   inregs->edx = ((LONG) p & (LONG) 0xF);
   inregs->ebx = fd;
   inregs->ecx = size;

   retCode = int86x(0x21, inregs, outregs);
   if (retCode || outregs->flags & 1)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      FreeMemoryBelow1MB((void *)RealModeBuffer);
      return (0);
   }
   else
   {
      CopyDataB((LONG *) RealModeBuffer, (LONG *) buf, outregs->eax);
      retCode = outregs->eax;
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      FreeMemoryBelow1MB((void *)RealModeBuffer);
      return retCode;
   }
}


LONG DOSWrite(LONG fd, void *buf, LONG size)
{

   register LONG p, retCode, RealModeBuffer;
   register REGS *inregs, *outregs;

   inregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!inregs)
      return 0;

   outregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!outregs)
   {
      FreeMemoryBelow1MB(inregs);
      return 0;
   }

   RealModeBuffer = (LONG) AllocateMemoryBelow1MB(size);
   if (!RealModeBuffer)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return 0;
   }

   CopyDataB((LONG *)buf, (LONG *)RealModeBuffer, size);

   SetData((LONG *) inregs, 0, sizeof(REGS));
   p = (LONG) RealModeBuffer;
   inregs->eax = 0x4000;
   inregs->ds = ((LONG)(p >> 4) & (LONG) 0x0000FFFF);
   inregs->edx = ((LONG) p & (LONG) 0xF);
   inregs->ebx = fd;
   inregs->ecx = size;
   retCode = int86x(0x21, inregs, outregs);

   if (retCode || outregs->flags & 1)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      FreeMemoryBelow1MB((void *)RealModeBuffer);
      return (0);
   }
   else
   {
      retCode = outregs->ecx;
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      FreeMemoryBelow1MB((void *)RealModeBuffer);
      return retCode;
   }
}

/*************************************************************************
*
*    flags:
*
*    bit 0 - read only idea
*    bit 1 - hidden file
*    bit 2 - system file
*    bit 3 - volume label
*    bit 4 - reserved (0) - directory
*    bit 5 - archive bit
*    bit 6 - 15 reserved (0)
*
*    returns   0        -  fail
*              handle   -  handle to newly created file.
*
*************************************************************************/

LONG DOSCreate(BYTE *path, LONG flags)
{

   register LONG p, retCode, RealModeBuffer;
   register REGS *inregs, *outregs;

   inregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!inregs)
      return 0;

   outregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!outregs)
   {
      FreeMemoryBelow1MB(inregs);
      return 0;
   }

   RealModeBuffer = (LONG) AllocateMemoryBelow1MB(strlen(path) + 16);
   if (!RealModeBuffer)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return 0;
   }
   strcpy((char *)RealModeBuffer, path);

   SetData((LONG *) inregs, 0, sizeof(REGS));
   p = (LONG) RealModeBuffer;
   inregs->eax = 0x3C00;
   inregs->ds = ((LONG)(p >> 4) & (LONG) 0x0000FFFF);
   inregs->edx = ((LONG) p & (LONG) 0xF);
   inregs->ecx = (flags & 0x0000FFFF);
   retCode = int86x(0x21, inregs, outregs);
   if (retCode || outregs->flags & 1)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      FreeMemoryBelow1MB((void *)RealModeBuffer);
      return (0);
   }
   else
   {
      retCode = outregs->eax & 0x0000FFFF;
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      FreeMemoryBelow1MB((void *)RealModeBuffer);
      return retCode;
   }

}

LONG DOSOpen(BYTE *path, LONG flags)
{

   register LONG p, retCode, RealModeBuffer;
   register REGS *inregs, *outregs;

   inregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!inregs)
      return 0;

   outregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!outregs)
   {
      FreeMemoryBelow1MB(inregs);
      return 0;
   }

   RealModeBuffer = (LONG) AllocateMemoryBelow1MB(strlen(path) + 16);
   if (!RealModeBuffer)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return 0;
   }

   strcpy((char *)RealModeBuffer, path);

   SetData((LONG *) inregs, 0, sizeof(REGS));
   p = (LONG) RealModeBuffer;
   inregs->eax = (0x3D00 | (flags & 0xFF));
   inregs->ds = ((LONG)(p >> 4) & (LONG) 0x0000FFFF);
   inregs->edx = ((LONG) p & (LONG) 0xF);
   retCode = int86x(0x21, inregs, outregs);
   if (retCode || outregs->flags & 1)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      FreeMemoryBelow1MB((void *)RealModeBuffer);
      return (0);
   }
   else
   {
      retCode = outregs->eax & 0x0000FFFF;
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      FreeMemoryBelow1MB((void *)RealModeBuffer);
      return retCode;
   }

}

/*************************************************************************
*
*   returns   0    - success
*	      -1   - failed
*
**************************************************************************/

LONG DOSClose(LONG fd)
{
   register LONG retCode;
   register REGS *inregs, *outregs;

   inregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!inregs)
      return 0;

   outregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!outregs)
   {
      FreeMemoryBelow1MB(inregs);
      return 0;
   }

   SetData((LONG *) inregs, 0, sizeof(REGS));
   inregs->eax = 0x3E00;
   inregs->ebx = fd;
   retCode = int86x(0x21, inregs, outregs);
   if (retCode || outregs->flags & 1)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return (-1);
   }
   else
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return (0);
   }

}

LONG DOSDelete(BYTE *path)
{

   register LONG retCode;
   register LONG p, RealModeBuffer;
   register REGS *inregs, *outregs;

   inregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!inregs)
      return 0;

   outregs = AllocateMemoryBelow1MB(sizeof(REGS));
   if (!outregs)
   {
      FreeMemoryBelow1MB(inregs);
      return 0;
   }

   RealModeBuffer = (LONG) AllocateMemoryBelow1MB(strlen(path) + 16);
   if (!RealModeBuffer)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return 0;
   }

   strcpy((char *)RealModeBuffer, path);

   SetData((LONG *) inregs, 0, sizeof(REGS));
   p = (LONG) RealModeBuffer;
   inregs->ds = ((LONG)(p >> 4) & (LONG) 0x0000FFFF);
   inregs->edx = ((LONG) p & (LONG) 0xF);
   inregs->eax = 0x4100;
   retCode = int86x(0x21, inregs, outregs);
   if (retCode || outregs->flags & 1)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      FreeMemoryBelow1MB((void *)RealModeBuffer);
      return (-1);
   }
   else
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      FreeMemoryBelow1MB((void *)RealModeBuffer);
      return (0);
   }

}

