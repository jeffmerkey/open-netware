

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
*   FILE     :  PROC.C
*   DESCRIP  :  Multi-Processing Startup for MANOS v1.0
*   DATE     :  November 2, 1997
*
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
#include "event.h"
#include "dos.h"

extern LONG StartupMemory;
LONG active_processors = 1;
extern LONG startup_number;
extern void set_processor_id(LONG);
extern LONG get_processor_id(void);

LONG InitializeProcessorStartup(DOS_TABLE *dos)
{

    extern LONG JUMPAbsolute;
    register BYTE *pB;
    register LONG *pD;
    LONG segment, offset, delta, address;

    // fixup processor startup code in low memory for SMP

    pD = (LONG *) dos->STARTUP_JUMP;
    *pD = (LONG) &JUMPAbsolute;
    delta = dos->STARTUP_CODE - dos->STARTUP_SEGMENT;
    segment = (dos->STARTUP_SEGMENT >> 4) & 0xFFFF;
    offset = (dos->STARTUP_SEGMENT & 0xF) + delta;
    address = ((segment << 16) | (offset));
    pB = (BYTE *) StartupMemory;
    pD = (LONG *) (StartupMemory + 1);
    *pB = 0xEA;  // real mode far jump
    *pD = (LONG) address;

    set_processor_id(0);

    return 0;
}

void ExitOSProcessorShutdown(LONG processor)
{

    register LONG i, retCode, count;

    if (processor) {};

    cli();
    for (i=1; i < active_processors; i++)
    {
       count = 0;
       retCode = close_processor(i);
       while (retCode)
       {
	  if (count++ > 5)
	  {
	     printfScreen(consoleScreen, "\nprocessor %d could not be halted", i);
	     break;
	  }
	  retCode = close_processor(i);
       }
    }
    return;

}

LONG get_physical_processor(void)
{
   return (processor_table[0].GetCPUID)();
}

LONG close_processor(LONG p)
{
    LONG count = 0;

    if (p != get_processor_id())
    {
       if (processor_table[p].ProcessorState != PROCESSOR_INACTIVE)
       {
	  processor_table[p].ShutdownSignal = 1;
	  while (processor_table[p].ProcessorState != PROCESSOR_INACTIVE)
	  {
	     if (count++ > 0x1FFFFFF)
		return -1;
	  }
       }
       EventNotify(EVENT_PROCESSOR_OFFLINE, p);
    }
    return 0;

}

void processor_init(LONG p)
{

     BYTE buf[100];

     set_processor_id(p);

     processor_table[p].ProcessorState = PROCESSOR_ACTIVE;
     (processor_table[p].InitializeProcessorHardware)(p);
     (processor_table[p].StartTimer)();

     active_processors++;

     insert_process(&idle_p[p]);
     idle_loop(p);  // this call does not return

     return;
}

LONG activate_processor(LONG p)
{

    LONG count = 0;

    startup_number = p;

    if (p < num_procs && p != get_processor_id())
    {
       if (processor_table[p].ProcessorState == PROCESSOR_INACTIVE)
       {
	  processor_table[p].ShutdownSignal = 0;
	  (processor_table[p].OnlineProcessor)(p, StartupMemory);
	  while (processor_table[p].ProcessorState != PROCESSOR_ACTIVE)
	  {
	     if (count++ > 0x1FFFFFF)
		return 0;
	  }
	  EventNotify(EVENT_PROCESSOR_ONLINE, p);
	  return 1;
       }
    }
    return 0;

}

void processor_exit(LONG p)
{

     PROCESS *local;

     cli();
     if (processorSet[p].soft_int_head)
     {
	register LWP *lwpWork, *lwp;

	processorSet[p].sleepDisallowed = 1;
	lwp = get_local_soft_int_chain(p);
	while (lwp)
	{
	   lwpWork = lwp;
	   lwp = lwp->next;
	   if (lwpWork->procedureAddress && lwpWork->lwpState == LWP_ACTIVE)
	   {
	      (lwpWork->procedureAddress)((void *)lwpWork->lwpArgument);
	      cli();
	   }
	   lwpWork->lwpState = LWP_INACTIVE;
	}
	processorSet[p].sleepDisallowed = 0;
     }

     if (processorSet[p].fast_lwp_head)
     {
	register LWP *lwpWork, *lwp;

	cli();
	processorSet[p].sleepDisallowed = 1;
	lwp = get_local_fast_lwp_chain(p);
	while (lwp)
	{
	   lwpWork = lwp;
	   lwp = lwp->next;
	   if (lwpWork->procedureAddress && lwpWork->lwpState == LWP_ACTIVE)
	   {
	      (lwpWork->procedureAddress)((void *)lwpWork->lwpArgument);
	      cli();
	   }
	   lwpWork->lwpState = LWP_INACTIVE;
	}
	processorSet[p].sleepDisallowed = 0;
     }

     while (processorSet[p].local_queue_head)
     {
	local = get_local(p);
	if (local)
	{
	   if (local->processorBinding)
	   {
	      remove_process(local);
	      put_rundown(local);
	   }
	   else
	      put_dispatch(local);
	}
     }

     while (processorSet[p].bind_queue_head)
     {
	local = get_bind(p);
	if (local)
	{
	   remove_process(local);
	   put_rundown(local);
	}
     }

     while (processorSet[p].lwp_worker_queue_head)
     {
	local = get_lwp_thread(p);
	if (local)
	{
	   remove_process(local);
	   put_rundown(local);
	}
     }

     while (processorSet[p].lwp_queue_head)
     {
	register LWP *lwp;

	lwp = get_local_lwp(p);
	if (lwp)
	   put_dispatch_lwp(lwp);
     }

     while (processorSet[p].lwp_bind_queue_head)
     {
	register LWP *lwp;

	lwp = get_lwp_bind(p);
	if (lwp)
	   put_dispatch_lwp(lwp);
     }

     remove_process(&idle_p[p]);

     if (active_processors > 1)
	active_processors--;

     (processor_table[p].StopTimer)();
     (processor_table[p].ShutdownProcessorHardware)(p);
     processor_table[p].ProcessorState = PROCESSOR_INACTIVE;

     HALT();


}





