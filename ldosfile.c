
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
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

#define MESSAGE_ON  0

long OutputText(BYTE *s)
{
   register LONG p, index;
   REGS inregs, outregs;

   SetData((LONG *) &inregs, 0, sizeof(REGS));
   index = strlen(s);
   s[index] = '$';
   p = (LONG) s;
   inregs.eax = 0x0900;
   inregs.ds = ((LONG)(p >> 4) & (LONG) 0x0000FFFF);
   inregs.edx = ((LONG) p & (LONG) 0xF);
   int86x(0x21, &inregs, &outregs);

   return (outregs.flags);
}

void DOSFlush(void)
{

   REGS inregs, outregs;

#if (MESSAGE_ON)
   printf("\nDOSFlush Entered");
#endif

   SetData((LONG *) &inregs, 0, sizeof(REGS));
   inregs.eax = 0x0D00;
   int86x(0x21, &inregs, &outregs);
#if (MESSAGE_ON)
   printf("\nDOSFlush Exit");
#endif
   return;

}

LONG DOSLseek(LONG fd, LONG offset, LONG flags)
{

   REGS inregs, outregs;

#if (MESSAGE_ON)
   printf("\nDOSLseek Entered");
#endif
   SetData((LONG *) &inregs, 0, sizeof(REGS));
   inregs.eax = ((LONG) 0x4200 | ((LONG) flags & 0xF));
   inregs.ebx = fd;
   inregs.ecx = ((offset >> 16) & 0x0000FFFF);
   inregs.edx = (offset & 0x0000FFFF);
   int86x(0x21, &inregs, &outregs);
#if (MESSAGE_ON)
   printf("\nDOSLseek Exit");
#endif
   if (outregs.flags & 1)
      return (0);
   else
      return (LONG)((outregs.edx << 16) | (outregs.eax & 0x0000FFFF));

}


LONG DOSRead(LONG fd, void *buf, LONG size)
{

   LONG p;
   REGS inregs, outregs;

#if (MESSAGE_ON)
   printf("\nDOSRead Entered");
#endif

   if (size > 4096)
      return -1;

   p = (LONG) buf;
   SetData((LONG *) &inregs, 0, sizeof(REGS));
   inregs.eax = 0x3F00;
   inregs.ds = ((LONG)(p >> 4) & (LONG) 0x0000FFFF);
   inregs.edx = ((LONG) p & (LONG) 0xF);
   inregs.ebx = fd;
   inregs.ecx = size;
   int86x(0x21, &inregs, &outregs);
#if (MESSAGE_ON)
   printf("\nDOSRead Exit");
#endif
   if (outregs.flags & 1)
      return (0);
   else
     return (outregs.eax);
}


LONG DOSWrite(LONG fd, void *buf, LONG size)
{

   LONG p;
   REGS inregs, outregs;

#if (MESSAGE_ON)
   printf("\nDOSWrite Entered");
#endif

   if (size > 4096)
      return -1;

   p = (LONG) buf;
   SetData((LONG *) &inregs, 0, sizeof(REGS));
   inregs.eax = 0x4000;
   inregs.ds = ((LONG)(p >> 4) & (LONG) 0x0000FFFF);
   inregs.edx = ((LONG) p & (LONG) 0xF);
   inregs.ebx = fd;
   inregs.ecx = size;
   int86x(0x21, &inregs, &outregs);
#if (MESSAGE_ON)
   printf("\nDOSWrite Exit");
#endif
   if (outregs.flags & 1)
      return (0);
   else
      return (outregs.ecx);

}


LONG DOSCreate(BYTE *path, LONG flags)
{

   register LONG retCode;
   LONG p;
   REGS inregs, outregs;

#if (MESSAGE_ON)
   printf("\nDOSCreate Entered");
#endif
   p = (LONG) path;
   SetData((LONG *) &inregs, 0, sizeof(REGS));
   inregs.eax = 0x3C00;
   inregs.ds = ((LONG)(p >> 4) & (LONG) 0x0000FFFF);
   inregs.edx = ((LONG) p & (LONG) 0xF);
   inregs.ecx = (flags & 0x0000FFFF);
   int86x(0x21, &inregs, &outregs);
#if (MESSAGE_ON)
   printf("\nDOSCreate Exit");
#endif
   if (outregs.flags & 1)
      return (0);
   else
      return (outregs.eax & 0x0000FFFF);

}

LONG DOSOpen(BYTE *path, LONG flags)
{

   LONG p;
   REGS inregs, outregs;

#if (MESSAGE_ON)
   printf("\nDOSOpen Entered");
#endif
   p = (LONG) path;
   SetData((LONG *) &inregs, 0, sizeof(REGS));
   inregs.eax = (0x3D00 | (flags & 0xFF));
   inregs.ds = ((LONG)(p >> 4) & (LONG) 0x0000FFFF);
   inregs.edx = ((LONG) p & (LONG) 0xF);
   int86x(0x21, &inregs, &outregs);
#if (MESSAGE_ON)
   printf("\nDOSOpen Exit");
#endif
   if (outregs.flags & 1)
      return (0);
   else
      return (outregs.eax & 0x0000FFFF);

}

LONG DOSClose(LONG fd)
{

   REGS inregs, outregs;

#if (MESSAGE_ON)
   printf("\nDOSClose Entered");
#endif
   SetData((LONG *) &inregs, 0, sizeof(REGS));
   inregs.eax = 0x3E00;
   inregs.ebx = fd;
   int86x(0x21, &inregs, &outregs);
#if (MESSAGE_ON)
   printf("\nDOSClose Exit");
#endif
   if (outregs.flags & 1)
      return (0);
   else
      return (1);

}


LONG DOSDelete(BYTE *path)
{

   LONG p;
   REGS inregs, outregs;

#if (MESSAGE_ON)
   printf("\nDOSDelete Entered");
#endif
   SetData((LONG *) &inregs, 0, sizeof(REGS));
   p = (LONG) path;
   SetData((LONG *) &inregs, 0, sizeof(REGS));
   inregs.ds = ((LONG)(p >> 4) & (LONG) 0x0000FFFF);
   inregs.edx = ((LONG) p & (LONG) 0xF);
   inregs.eax = 0x4100;
   int86x(0x21, &inregs, &outregs);
#if (MESSAGE_ON)
   printf("\nDOSDelete Exit");
#endif
   if (outregs.flags & 1)
      return (0);
   else
      return (1);

}


