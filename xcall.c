
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
*   FILE     :  XCALL.C
*   DESCRIP  :  Multi-Processing XCALL Layer for MANOS v1.0
*   DATE     :  December 28, 1997
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
#include "rlock.h"
#include "event.h"
#include "debug.h"
#include "dparse.h"
#include "bus.h"

LONG NestedNMI[MAX_PROCESSORS];
LONG tlb_sync = 0;

//
//   returns    0  -  TLB Shootdown successfully completed
//             -1  -  command timeout/ack not received
//             -2  -  bad or invalid processor number
//             -3  -  NMI hardware reports busy
//             -4  -  NMI hardware parameter error
//

LONG TLBShootDown(void)
{

   register LONG i, retCode = 0, count = 0;
   register LONG self = get_processor_id();
   register LONG flags;
   BYTE XCallMessage[100]= { "" };

   flags = get_flags();
   dspin_lock(&tlb_sync);

   for (i=0; i < active_processors; i++)
   {
      if (i != self)
      {
	 if (processor_table[i].ProcessorState == PROCESSOR_ACTIVE)
	 {
	    processor_table[i].xcall_ack = 0;
	    retCode = xcall_processor((LONG) i, XCALL_FLUSH_TLB, 0, 0, 0);
	    if (retCode)
	       goto exit_shootdown;

	    while (processor_table[i].xcall_ack != XCALL_FLUSH_TLB)
	    {
	       if (count++ > XCALL_RETRY_COUNT)
	       {
		  printfScreenWithAttribute(debugScreen,
					       LTGREEN,
					       "\n xcall FLUSH_TLB failure PROCESSOR: %d",
					       i);

		  sprintf(XCallMessage, "xcall FLUSH_TLB failure PROCESSOR: %d", i);
		  panic(XCallMessage);
		  retCode = -1;
		  goto exit_shootdown;
	       }
	    }
	 }
      }
      else
      {
	 FlushTLB();
      }
   }

exit_shootdown:;
   dspin_unlock(&tlb_sync);
   set_flags(flags);

   return (retCode);

}

LONG VMFlushTLB(void)
{

    register LONG retCode, count = 0;

    retCode = TLBShootDown();
    while (retCode)
    {
       if (count++ > 3)
	  break;

       retCode = TLBShootDown();
    }
    return retCode;

}

LONG xcall_processor(LONG p, LONG cmd, LONG arg1, LONG arg2, LONG arg3)
{

    register LONG retCode;
    register LONG count = 0;
    register LONG realModeCount = 0;

    if (p < active_processors)
    {
       while (inRealModeFlag)
       {
	  realModeCount++;
	  if (realModeCount) {};
       }

       while (count++ < XCALL_RETRY_COUNT)
       {
	  if (!processor_table[p].xcall_command)
	  {
	     EventNotify(EVENT_XCALL_INT, p);

	     processor_table[p].xcall_command = cmd;
	     processor_table[p].xcall_arg1 = arg1;
	     processor_table[p].xcall_arg2 = arg2;
	     processor_table[p].xcall_arg3 = arg3;

	     retCode = (processor_table[p].DirectedNMI)(p);
	     return (LONG) retCode;
	  }
       }
       processor_table[p].xcall_busy++;

       return (LONG) -1;
    }
    return (LONG) -2;

}

LONG nmiSetup(LONG processor, LONG Exception, StackFrame *stackFrame, BYTE *panicMsg)
{

     LONG nmiStatus;
     LONG command;

     //
     //  clear any NMI state for standard and extended NMI (EISA) systems
     //  by reading the NMI status ports and -Or-ing them together.
     //  is this is not done for an NMI interrupt, some systems will
     //  continue to re-assert the NMI after receiving a valid IRET or IRETD
     //

     NestedNMI[processor]++;

     // THIS SHOULD NEVER HAPPEN, BUT DOES ON SOME HARDWARE

     if (NestedNMI[processor] > 1)
	printfScreenWithAttribute(debugScreen, LTGREEN, "\nNMI Nesting depth -> %d",
				  NestedNMI[processor]);

     nmiStatus  = (LONG)(inb(NMI_PORT) & 0xC0);
     if (DosDataTable->DOS_SYSTEM_BUS_TYPE & EISA)
     {
	nmiStatus |= (LONG)((inb(EXT_NMI_PORT) & 0xF0) << 8);
	nmiStatus |= (LONG)((inb(NMI_CONTROL_PORT) & 0xF0) << 16);
     }

     //
     // if we got an actual hardware NMI, then enter the debugger.
     // this indicates a hardware failure of some type and is not
     // an xcall NMI interrupt.
     //

     if (nmiStatus)
     {
	debugger_setup(processor, Exception, stackFrame, panicMsg);
	return 0;
     }

     EventNotify(EVENT_XCALL_INT, processor);

     processor_table[processor].xcall_total++;
     command = processor_table[processor].xcall_command;
     processor_table[processor].xcall_command = 0;
     processor_table[processor].xcall_arg1 = 0;
     processor_table[processor].xcall_arg2 = 0;
     processor_table[processor].xcall_arg3 = 0;

     switch(command)
     {
	case XCALL_DEBUG:
	      processor_table[processor].ProcessorState = PROCESSOR_SUSPEND;
	      while (1)
	      {
		 if (processor_table[processor].ProcessorState != PROCESSOR_SUSPEND)
		    break;

		 if (processor_table[processor].ProcessorState == PROCESSOR_SHUTDOWN)
		 {
		    if (processor)
		       processor_exit(processor);
		 }
	      };
	      break;

	case XCALL_DIRECTED:
	      debugger_setup(processor, 21, stackFrame, panicMsg);
	      break;

	case XCALL_FLUSH_TLB:
	      FlushTLB();
	      processor_table[processor].TLBShootdowns++;
	      processor_table[processor].xcall_ack = XCALL_FLUSH_TLB;
	      break;

	case XCALL_INVL_PAGE:
	case XCALL_RELOAD_DEBUG:
	case XCALL_SHUTDOWN:
	case XCALL_SERVICE:
	      break;

	default:
	      printfScreenWithAttribute(debugScreen,
					LTGREEN,
					"\n xcall failure COMMAND: %d PROCESSOR: %d",
					command,
					processor);
	      processor_table[processor].xcall_fail++;
	      break;
     }


     NestedNMI[processor]--;

     return 0;

}


LONG StopProcessorsExclSelf(LONG self)
{

   register LONG failed = 0;
   register LONG count;
   register int i;

   for (i=0; i < active_processors; i++)
   {
      if (i != self)
      {
	 if (processor_table[i].ProcessorState == PROCESSOR_ACTIVE)
	 {
	    xcall_processor((LONG) i, XCALL_DEBUG, 0, 0, 0);
	    while (processor_table[i].ProcessorState != PROCESSOR_SUSPEND)
	    {
	       if (count++ > 0x1FFFFF)
	       {
		  failed++;
		  printfScreenWithAttribute(debugScreen, BRITEWHITE, "\nProcessor %i could not be halted\n", i);
		  break;
	       }
	    }
	 }
      }
   }
   return (LONG) failed;

}

LONG FreeProcessorsExclSelf(LONG self)
{

   register LONG failed = 0;
   register int i;

   for (i=0; i < active_processors; i++)
   {
      if (i != self)
	 processor_table[i].ProcessorState = PROCESSOR_RESUME;
   }
   return (LONG) failed;

}

