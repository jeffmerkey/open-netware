

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  BUS.C
*   DESCRIP  :  BUS and Machine Specific Code for MANOS v1.0
*   DATE     :  April 13, 1998
*
*   THIS MODULE CURRENTLY SUPPORTS MCA/EISA/PCI
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
#include "os.h"
#include "dos.h"
#include "tss.h"
#include "mps.h"
#include "hal.h"
#include "xcall.h"
#include "window.h"
#include "line.h"
#include "loader.h"
#include "menu.h"
#include "dosfile.h"

extern void *AllocateMemoryBelow1MB(LONG);
extern void FreeMemoryBelow1MB(void *);

// MCA slot/function masks and value locations

#define  MEM_ABOVE_16         0x0E

// EISA slot/function masks and value locations

#define  MEM_PRESENT	     0x00000002
#define  ENTRY_CONT	     0x00000080
#define  MEM_SYSTEM	     0x00000001
#define  MEM_MASK	     0x00000019
#define  FUNCTION_INFO       0x00000022
#define  EFI_FREE_FORM       0x00000073

#define  MAX_EISA_FUNCTIONS  256
#define  MAX_EISA_SLOTS      16

typedef struct _PCI_BIOS {
   BYTE PCISignature[4];
   LONG PCIEntry;
   BYTE StructureRevision;
   BYTE StructureLength;
   BYTE StructureChecksum;
   BYTE PCIReserved[5];
} PCI_BIOS_INFO;

#define  PCI_BIOS_ENTRY_32   0x000FFE6E

//
//  PCI BIOS/BUS SUPPORT FUNCTIONS
//
//  PCI BIOS Present
//
//     ah = 0xB1  test PCI bios present
//     al = 0x01
//
//  Returns:
//
//     BIOS Not Present = carry flag is set
//
//     BIOS Present = carry flag clear
//
//     edx = ' PCI'
//     dl = 'P'
//     dh = 'C'
//     ah = 0
//     al bits 0   =  Host/PCI Bridge Uses Config 1
//     al bits 1   =  Host/PCI Bridge Uses Config 2
//     al bits 2-3 =  Reserved
//     al bits 4   =  Special Cycle Supported Config 1
//     al bits 5   =  Special Cycle Supported Config 2
//     al bits 6-7 =  Reserved
//     bh = BIOS major version BCD
//     bl = BIOS minor version BCD
//     cl = Number Of Last PIC bus in system
//
//  PCI BIOS is present only if:
//
//    A.  EDX contains the name string 'PCI ' (AND)
//    B.  AL contains PCI settings (AND)
//    C.  Carry Flag is Clear
//

LONG PCIBIOSPresent(void)
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

   RealModeBuffer = (LONG) AllocateMemoryBelow1MB(0x1000);
   if (!RealModeBuffer)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return 0;
   }

   SetData((LONG *) inregs, 0, sizeof(REGS));

   p = (LONG) RealModeBuffer;
   inregs->ds = ((LONG)(p >> 4) & (LONG) 0xFFFF);
   inregs->esi = ((LONG) p & (LONG) 0xF);
   inregs->eax = 0xB101;

   retCode = int86x(0x1A, inregs, outregs);

   if (retCode || outregs->flags & 1)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      FreeMemoryBelow1MB((void *)RealModeBuffer);
      return (0);
   }

   if (!outregs->eax >> 8 & 0xFF)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      FreeMemoryBelow1MB((void *)RealModeBuffer);
      return (0);
   }

   if (outregs->edx != 0x20494350)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      FreeMemoryBelow1MB((void *)RealModeBuffer);
      return (0);
   }

   FreeMemoryBelow1MB(inregs);
   FreeMemoryBelow1MB(outregs);
   FreeMemoryBelow1MB((void *)RealModeBuffer);
   return (1);

}


//
//   MICRO-CHANNEL BUS SUPPORT FUNCTIONS
//
//
//   GetMCAMemoryMap
//
//     ah = 0xC7
//     al = 0x00
//     ds:si  address of 4K buffer for MCA memory map
//
//   Returns:
//
//	00h Success  (carry flag clear)
//	80h Function Unsupported  (carry flag set)
//	86h Function Unsupported  (carry flag set)
//
//   [MCAMap + 0x0E] = Memory Above 16MB (1K Blocks)
//   [MCAMap + 0x0E] << 10 = Memory Above 16MB (bytes)
//

LONG GetMCAMemory(void)
{

   register LONG retCode;
   register BYTE *ptr;
   register LONG p, *lp, memory, RealModeBuffer;
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

   RealModeBuffer = (LONG) AllocateMemoryBelow1MB(0x1000);
   if (!RealModeBuffer)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return 0;
   }

   SetData((LONG *) inregs, 0, sizeof(REGS));
   p = (LONG) RealModeBuffer;
   inregs->eax = 0xC700;
   inregs->ds = ((LONG)(p >> 4) & (LONG) 0xFFFF);
   inregs->esi = ((LONG) p & (LONG) 0xF);
   retCode = int86x(0x15, inregs, outregs);
   if (retCode || outregs->flags & 1)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return (0);
   }

   ptr = (BYTE *) RealModeBuffer;

   //  memory above 16 MB returned from the
   //  MCA Memory Map

   lp = (LONG *) &ptr[MEM_ABOVE_16];

   // shift value << 10 to convert 1K blocks to bytes
   // this function assumes if any size is returned
   // for memory above 16MB, then we should add
   // 16MB to any value we get except zero.

   memory = (LONG)*lp << 10;
   if (memory)
   {
      // round to MB boundry
      memory &= 0xFFF00000;
      memory += 0x01000000;
   }

   FreeMemoryBelow1MB(inregs);
   FreeMemoryBelow1MB(outregs);
   FreeMemoryBelow1MB((void *)RealModeBuffer);

   return (memory);

}

//
//   EISA BUS SUPPORT FUNCTIONS
//
//
//   EISA Read Slot Configuration Request:
//
//	ah = 0xD8
//	al = 0x00 16 bit segments in CS/DS
//	     0x80 32 bit segments in CS/DS
//	cl = slot number
//
//
//   EISA Read Slot Configuration Returns:
//
//	 ah = Completion code
//
//	    0x00 Successful (CF is clear)
//	    0x80 Invalid slot number (CF is set)
//	    0x82 Invalid system configuration (CF set)
//	    0x83 Empty slot (CF set)
//	    0x86 Invalid BIOS routine call (CF set)
//	    0x87 Invalid system configuration (CF set)
//
//	 al bits 0-3 = Duplicate ID number
//	 al bits 4-5 = Slot type
//	 al bit 6 = Product ID
//	 al bit 7 = Duplicate ID
//	 bh = Major revision level of configuration utility
//	 bl = Minor revision level of configuration utility
//	 cx = Checksum of configuration file
//	 dh = Number of device functions
//	 dl bit 0 = Slot has function type definitions
//	 dl bit 1 = Slot has memory entries
//	 dl bit 2 = Slot has interrupt entries
//	 dl bit 3 = Slot has DMA entries
//	 dl bit 4 = Slot has I/O port range entries
//	 dl bit 5 = Slot has I/O port initialization entries
//	 dl bit 6-7 = Reserved
//	 ds:si Compressed ID
//
//  Determine which slot/functions have memory
//
//  EISA Read Function Configuration Request
//
//	  ah = 0xD8
//	  al = 0x01 for 16 bit segments in CS and DS
//	       0x81 for 32 bit segments in CS and DS
//	  cl = slot number
//	  ch = function number
//	  ds:si = pointer to 0x140 (320) byte buffer
//
//
//  EISA Read Function Configuration Returns:
//
//	  ah = Completion code
//	     0x00 Successful (CF is clear)
//	     0x80 Invalid slot number (CF is set)
//	     0x81 Invalid function number (CF is set)
//	     0x82 Invalid system configuration (CF set)
//	     0x83 Empty slot (CF set)
//	     0x86 Invalid BIOS routine call (CF set)
//           0x87 Invalid System Configuration
//	  Buffer pointed to by ds:si is filled in with
//	  configuration data block information
//

LONG GetEISAConfigInfo(LONG slot, LONG function, BYTE *EISAData)
{

   register LONG p, RealModeBuffer, retCode;
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

   RealModeBuffer = (LONG) AllocateMemoryBelow1MB(0x1000);
   if (!RealModeBuffer)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return 0;
   }

   SetData((LONG *) inregs, 0, sizeof(REGS));
   p = (LONG) RealModeBuffer;
   inregs->eax = 0xD801;
   inregs->ecx = (function << 8 | slot);
   inregs->ds = ((LONG)(p >> 4) & (LONG) 0xFFFF);
   inregs->esi = ((LONG) p & (LONG) 0xF);

   retCode = int86x(0x15, inregs, outregs);
   if (retCode)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      FreeMemoryBelow1MB((void *)RealModeBuffer);
      return 0;
   }

   if (EISAData)
      CopyDataB((LONG *) RealModeBuffer, (LONG *) EISAData, 320);

   retCode = outregs->eax;
   FreeMemoryBelow1MB(inregs);
   FreeMemoryBelow1MB(outregs);
   FreeMemoryBelow1MB((void *)RealModeBuffer);
   return retCode;

}

LONG GetEISAMemory(void)
{

   register LONG retCode;
   BYTE EISAConfig[512];
   register LONG RealModeBuffer, p, Slot, Function, memory = 0;
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

   RealModeBuffer = (LONG) AllocateMemoryBelow1MB(0x1000);
   if (!RealModeBuffer)
   {
      FreeMemoryBelow1MB(inregs);
      FreeMemoryBelow1MB(outregs);
      return 0;
   }

   SetData((LONG *) inregs, 0, sizeof(REGS));
   p = (LONG) RealModeBuffer;
   inregs->ds = ((LONG)(p >> 4) & (LONG) 0xFFFF);
   inregs->esi = ((LONG) p & (LONG) 0xF);

   // scan each EISA slot, and get config info

   for (Slot=0; Slot < MAX_EISA_SLOTS; Slot++)
   {
      inregs->eax = 0xD800;
      inregs->ecx = Slot;
      retCode = int86x(0x15, inregs, outregs);
      if (retCode || outregs->flags & 1)
      {
	 // if invalid slot number, then exit
	 if (outregs->eax >> 8 == 0x80)
	    break;
      }

      //  if slot reports memory, scan functions for type
      //  of memory

      if (outregs->edx & MEM_PRESENT)
      {
	 // scan functions for a particular slot and read
	 // EISA configuration information

	 for (Function=0; Function < MAX_EISA_FUNCTIONS; Function++)
	 {
	    register BYTE *bp;
	    register LONG *lp;
	    register WORD *wp, wlen;
	    register LONG address, length, retCode;

	    retCode = GetEISAConfigInfo(Slot, Function, &EISAConfig[0]);

	    // if invalid Function number, then try next slot
	    if (retCode >> 8 == 0x81)
	       break;

	    //  see if this slot has memory

	    if (EISAConfig[FUNCTION_INFO] & MEM_PRESENT)
	    {
	       // if this slot has memory, get a pointer
	       // into the EISA config information

	       bp = (BYTE *) &EISAConfig[EFI_FREE_FORM];
	       while (1)
	       {
		  // if this slot reports system memory, extract
		  // values

		  if (((LONG)*bp & MEM_MASK) == MEM_SYSTEM)
		  {
		     lp = (LONG *) &bp[1]; // get address entry
		     address = *lp;
		     address &= 0xFFFFFF00;
		     if (address >= 0x1000000) // if address is above 16MB
		     {
			wp = (WORD *) &bp[5];  // get size entry
			wlen = *wp;
			length = (LONG) wlen;
			length <<= 10;         // convert 1K blocks to bytes
			if (!length)           // if we find a length of 0,
			   length = 0x4000000; // then assume size is 64MB
			memory += length;
		     }
		  }
		  if (*bp & ENTRY_CONT)   // move to next entry in this table
		     bp += 7;             // if flags state there are more
		  else                    // entries
		     break;
	       }
	    }
	 }
      }
   }

   FreeMemoryBelow1MB(inregs);
   FreeMemoryBelow1MB(outregs);
   FreeMemoryBelow1MB((void *)RealModeBuffer);

   // if we found entries above 16MB, then assume a base
   // of 16MB, and add to total

   if (memory)
   {
      // round to MB boundry
      memory &= 0xFFF00000;
      memory += 0x01000000;
   }

   return memory;

}

