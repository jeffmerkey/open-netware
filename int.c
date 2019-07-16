

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
*   FILE     :  INT.C
*   DESCRIP  :  Multi Processing Interrupt Support for MANOS v1.0
*   DATE     :  January 3, 1998
*
***************************************************************************/

#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "string.h"
#include "keyboard.h"
#include "types.h"
#include "kernel.h"
#include "hal.h"
#include "screen.h"
#include "mps.h"
#include "os.h"
#include "emit.h"

#define MAX_INTS      64
#define CHAIN_LENGTH  16

#define EDGE     0
#define LEVEL    1

extern void processor_init(LONG p);
extern void KeyboardInterrupt(void);
extern void TimerInterrupt(void);

LONG (*int_chain[MAX_INTS][CHAIN_LENGTH])(LONG);
LONG int_type[MAX_INTS];
LONG int_entries[MAX_INTS];
LONG systemType = PIC_MODE;

BYTE irq_control[MAX_PICS] = { PIC_0, PIC_1, PIC_2 };
BYTE irq_mask[MAX_PICS] = { MASK_0, MASK_1, MASK_2 };
BYTE mask_value[MAX_PICS] = { 0xF8, 0xFF, 0xFF }; // mask all except
						  // keyboard, timer, cascade
LONG InDebuggerOrISR[MAX_PROCESSORS];

//
//   ISRs return 0 if they did not process the interrupt
//   1 if they did.
//

LONG set_interrupt(LONG intr,
		   LONG (*isr)(LONG),
		   LONG share,
		   LONG mode,
		   LONG proc)
{

   register int i;

   if (proc > MAX_PROCESSORS)
      return 0;

   if (intr > MAX_INTS)
      return 0;

   if (int_type[intr] != share)
      return 0;

   for (i=0; i < CHAIN_LENGTH; i++)
   {
      if (int_chain[intr][i] == isr)
	 return 0;
   }

   int_type[intr] = share;
   for (i=0; i < CHAIN_LENGTH; i++)
   {
      if (!int_chain[intr][i])
      {
	 int_chain[intr][i] = isr;
	 if (!int_entries[intr])
	    (processor_table[get_processor_id()].SetInterrupt)(intr, proc, mode, share);
	 int_entries[intr]++;
	 return 1;
      }
   }
   return 0;

}

LONG clear_interrupt(LONG intr,
		     LONG (*isr)(LONG))
{

   register int i;

   if (intr > MAX_INTS)
      return 0;

   for (i=0; i < CHAIN_LENGTH; i++)
   {
      if (int_chain[intr][i] == isr)
      {
	 int_chain[intr][i] = 0;
	 int_entries[intr]--;
	 if (!int_entries[intr])
	 {
	    int_type[intr] = 0;
	    (processor_table[get_processor_id()].ClearInterrupt)(intr);
	 }
	 return 1;
      }
   }
   return 0;

}

void mask_interrupt(LONG intr)
{

   (processor_table[get_processor_id()].MaskInterrupt)(intr);
   return;

}

void unmask_interrupt(LONG intr)
{

   (processor_table[get_processor_id()].UnmaskInterrupt)(intr);
   return;

}

void eoi_interrupt(LONG intr)
{
   (processor_table[0].EndOfInterrupt)(intr);
   return;
}

void interrupt_entry(LONG intr)
{

     extern LONG inRealModeFlag;
     register LONG processor = get_processor_id();
     register int i, c;

     processor_table[processor].totalInterrupts++;

     InDebuggerOrISR[processor]++;
     (processor_table[processor].MaskTimer)();
     if (int_type[intr] == EDGE)
     {
	for (i=0; i < CHAIN_LENGTH; i++)
	{
	   if (int_chain[intr][i])
	      if ((int_chain[intr][i])(intr))
	      {
		 (processor_table[processor].UnmaskTimer)();
		 InDebuggerOrISR[processor]--;
		 return;
	      }
	}
     }
     else
     if (int_type[intr] == LEVEL)
     {
	for (i=0; i < CHAIN_LENGTH; i++)
	{
	   if (int_chain[intr][i])
	      if ((int_chain[intr][i])(intr))
	      {
		 (processor_table[processor].UnmaskTimer)();
		 InDebuggerOrISR[processor]--;
		 return;
	      }
	}
     }

     pic_eoi(intr);
     (processor_table[processor].UnmaskTimer)();
     InDebuggerOrISR[processor]--;
     return;

}

void InitializeInterrupts(void)
{

    SetData((LONG *) &int_chain[0][0], 0, MAX_INTS * CHAIN_LENGTH);
    SetData((LONG *) &int_type[0], 0, MAX_INTS);
    SetData((LONG *) &int_entries[0], 0, MAX_INTS);

    program_8254(); // program the 8254 timer

}

//
//  8259 PIC HAL Section
//

void unmask_pic(LONG intr)
{

    if (intr > 15)
       return;

    if (intr == 2)
       intr = 9;

    mask_value[0] = (inb(irq_mask[0]) & ~(1 << 2));
    outb(irq_mask[0], mask_value[0]);
    if (intr > 7)
    {
       mask_value[1] = (inb(irq_mask[1]) & ~(1 << (intr - 8)));
       outb(irq_mask[1], mask_value[1]);
    }
    else
    {
       mask_value[0] = (inb(irq_mask[0]) & ~(1 << intr));
       outb(irq_mask[0], mask_value[0]);
    }

}

void mask_pic(LONG intr)
{

    if (intr > 15)
       return;

    if (intr == 2)
       intr = 9;

    if (intr > 7)
    {
       mask_value[1] = (inb(irq_mask[1]) | (1 << (intr - 8)));
       outb(irq_mask[1], mask_value[1]);
    }
    else
    {
       mask_value[0] = (inb(irq_mask[0]) | (1 << intr));
       outb(irq_mask[0], mask_value[0]);
    }

}

void mask_pic_timer(void)
{
    mask_pic(0);
}

void unmask_pic_timer(void)
{
    unmask_pic(0);
}

void mask_system(void)
{

   outb(irq_mask[0], 0xFB);
   outb(irq_mask[1], 0xFF);

}

void unmask_system(void)
{

   outb(irq_mask[0], mask_value[0]);
   outb(irq_mask[1], mask_value[1]);

}


LONG pic_set_int(LONG intr, LONG proc, LONG mode, LONG share)
{

     if (share) {};
     if (mode) {};
     if (proc) {};

     unmask_pic(intr);
     return 0;

}

LONG pic_clear_int(LONG intr)
{

     mask_pic(intr);
     return 0;
}

void pic_eoi(LONG intr)
{

     if (intr == 2)
	intr = 9;

     if (intr < 16)
     {
	if (intr > 7)
	{
	   outb(irq_control[1], PIC_EOI);
	   outb(irq_control[0], PIC_EOI);
	}
	else
	   outb(irq_control[0], PIC_EOI);
     }

}

void MicroDelay(LONG interval15us)
{

   BYTE lc, cc;

   lc = 0;
   while (1)
   {
      cc = inb(0x61);
      cc &= 0x10;
      if (cc == lc)      // check for toggle of refresh 15us bit
	 continue;
      lc = cc;

      if (!interval15us)
	 break;
      interval15us--;
   }
   return;


}

