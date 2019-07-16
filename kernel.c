
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
*   FILE     :  KERNEL.C
*   DESCRIP  :  Multi-Processing Kernel for MANOS v1.0
*   DATE     :  January 10, 1998
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

PROCESSOR_SET *ptable[MAX_PROCESSORS];
PROCESSOR_SET processorSet[MAX_PROCESSORS];
PROCESS idle_p[MAX_PROCESSORS];
extern LONG NestedInterrupts[MAX_PROCESSORS];
extern LONG PreemptiveEvents;
extern void put_free_process(PROCESS *p);

LONG processorCountArray[MAX_PROCESSORS];
LONG threadCountArray[MAX_PROCESSORS];
LONG idleCountArray[MAX_PROCESSORS];
LONG averageActiveCount = 0;
LONG systemUtilization;
LONG processorUtilization[MAX_PROCESSORS];
LONG preemptOn[MAX_PROCESSORS];
LONG KernelInitialized = 0;

PROCESS *commandP = 0;
LONG LWPCount = 0;
LONG LastLWPCreate;

LONG LWP_PROCESS_COUNT = 20;
LONG WAIT_INTERVAL = 18;
LONG IDLE_COUNT = 200;
LONG SAMPLE_INTERVAL = 72;
LONG DELTA = 20;

LONG SystemTickCounter = 0;
LONG SystemDTSCTimeCounter = 0;
LONG SystemDTSCTimeBase = 0;
LONG SystemCurrentTime = 0;

PROCESS *dispatch_queue_head;
PROCESS *dispatch_queue_tail;
LONG dispatch_queue_mutex;

PROCESS *rundown_queue_head;
PROCESS *rundown_queue_tail;
LONG rundown_queue_mutex;

LWP *lwp_dispatch_queue_head = 0;
LWP *lwp_dispatch_queue_tail = 0;
LONG lwp_dispatch_queue_mutex = 0;

LONG dispatch_queue_count;
LONG lwp_dispatch_queue_count;

LWP *soft_int_head = 0;
LWP *soft_int_tail = 0;
LONG soft_int_lock = 0;

LWP *fast_lwp_head = 0;
LWP *fast_lwp_tail = 0;
LONG fast_lwp_lock = 0;

extern LONG DaysUp;
extern LONG HoursUp;
extern LONG MinutesUp;
extern LONG SecondsUp;
extern LONG TicksUp;

void EnterSMPKernel(void)
{
   idle_loop(0);
}

LONG GetCurrentTime(void)
{
   return (SystemTickCounter);
}

void LWPProcess(LONG proc)
{

    register PROCESS *p;
    register LWP *lwp;

    p = processorSet[proc].running_process;
    for (;;)
    {
       sti();

       if (soft_int_head)
       {
	  register LWP *lwpWork;

	  cli();
	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_soft_int_chain();
	  while (lwp)
	  {
	     lwpWork = lwp;
	     lwp = lwp->next;
	     if (lwpWork->procedureAddress && lwpWork->lwpState == LWP_ACTIVE)
	     {
		lwpWork->lwpState = LWP_RUNNING;
		(lwpWork->procedureAddress)((void *)lwpWork->lwpArgument);
		cli();
	     }
	     lwpWork->lwpState = LWP_INACTIVE;
	  }
	  processorSet[proc].sleepDisallowed = 0;
	  sti();
       }

       if (processorSet[proc].soft_int_head)
       {
	  register LWP *lwpWork;

	  cli();
	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_local_soft_int_chain(proc);
	  while (lwp)
	  {
	     lwpWork = lwp;
	     lwp = lwp->next;
	     if (lwpWork->procedureAddress && lwpWork->lwpState == LWP_ACTIVE)
	     {
		lwpWork->lwpState = LWP_RUNNING;
		(lwpWork->procedureAddress)((void *)lwpWork->lwpArgument);
		cli();
	     }
	     lwpWork->lwpState = LWP_INACTIVE;
	  }
	  processorSet[proc].sleepDisallowed = 0;
	  sti();
       }

       if (fast_lwp_head)
       {
	  register LWP *lwpWork;

	  cli();
	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_fast_lwp_chain();
	  while (lwp)
	  {
	     lwpWork = lwp;
	     lwp = lwp->next;
	     if (lwpWork->procedureAddress && lwpWork->lwpState == LWP_ACTIVE)
	     {
		lwpWork->lwpState = LWP_RUNNING;
		(lwpWork->procedureAddress)((void *)lwpWork->lwpArgument);
		cli();
	     }
	     lwpWork->lwpState = LWP_INACTIVE;
	  }
	  processorSet[proc].sleepDisallowed = 0;
	  sti();
       }

       if (processorSet[proc].fast_lwp_head)
       {
	  register LWP *lwpWork;

	  cli();
	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_local_fast_lwp_chain(proc);
	  while (lwp)
	  {
	     lwpWork = lwp;
	     lwp = lwp->next;
	     if (lwpWork->procedureAddress && lwpWork->lwpState == LWP_ACTIVE)
	     {
		lwpWork->lwpState = LWP_RUNNING;
		(lwpWork->procedureAddress)((void *)lwpWork->lwpArgument);
		cli();
	     }
	     lwpWork->lwpState = LWP_INACTIVE;
	  }
	  processorSet[proc].sleepDisallowed = 0;
	  sti();
       }

       if (processorSet[proc].lwp_queue_head)
       {
	  lwp = get_local_lwp(proc);
	  if (lwp)
	  {
	     if (lwp->procedureAddress && lwp->lwpState == LWP_ACTIVE)
	     {
		p->threadState = PS_ACTIVE;
		lwp->lwpState = LWP_RUNNING;
		(lwp->procedureAddress)((void *)lwp->lwpArgument);
		p->threadState = PS_LWP;
	     }
	     lwp->lwpState = LWP_INACTIVE;
	  }
       }

       if (lwp_dispatch_queue_head)
       {
	  lwp = get_dispatch_lwp();
	  if (lwp)
	  {
	     if (lwp->procedureAddress && lwp->lwpState == LWP_ACTIVE)
	     {
		p->threadState = PS_ACTIVE;
		lwp->lwpState = LWP_RUNNING;
		(lwp->procedureAddress)((void *)lwp->lwpArgument);
		p->threadState = PS_LWP;
	     }
	     lwp->lwpState = LWP_INACTIVE;
	  }
       }

       thread_switch();
    }
}

void MakeLWPThread(LONG proc)
{

   register PROCESS *p;
   BYTE LWPName[100] = { "" };

   if (LastLWPCreate + WAIT_INTERVAL < SystemCurrentTime)
   {
      sprintf(LWPName, "Light Weight Process %i", LWPCount++);
      p = createThreadContext(LWPName, (LONG (*)())LWPProcess, 4096, (void *)proc, proc);
      if (p)
	 put_lwp_thread(p, proc);
      LastLWPCreate = SystemCurrentTime;
   }

}

void process_lwp_dispatch_queue(LONG proc)
{
    register LWP *lwp;

    if (lwp_dispatch_queue_head)
    {
       lwp = get_dispatch_lwp();
       if (lwp)
	  put_local_lwp(lwp, proc);
    }
}

void process_lwp_bind_queue(LONG proc)
{

    register LWP *lwp;

    while (processorSet[proc].lwp_bind_queue_head)
    {
       lwp = get_lwp_bind(proc);
       if (lwp)
	  put_local_lwp(lwp, proc);
    }
}

void process_rundown_queue(void)
{

    register PROCESS *p;

    if (rundown_queue_head)
    {
       p = get_rundown();
       if (p)
       {
	  p->threadState = PS_SHUTDOWN;
	  kfree(p->stackEnd);
	  put_free_process(p);
       }
    }

}

void process_bind_queue(LONG proc)
{

    register PROCESS *p;

    while (processorSet[proc].bind_queue_head)
    {
       p = get_bind(proc);
       if (p)
	  put_local(p, proc);
    }
}

void process_bindsw_queue(LONG proc)
{

    register PROCESS *p;

    while (processorSet[proc].bindsw_queue_head)
    {
       p = get_bindsw(proc);
       if (p)
	  put_local(p, proc);
    }
}

void process_dispatch_queue(LONG proc)
{
    register PROCESS *p;

    if (dispatch_queue_head)
    {
       p = get_dispatch();
       if (p)
	  put_local(p, proc);
    }

}

#if (!OPT_IA32_MANOS)

void idle_loop(LONG proc)
{

    register PROCESS *p;
    register LWP *lwp;
    BYTE LWPName[100];

    for (;;)
    {
       sti();

       if (processor_table[proc].ShutdownSignal)
       {
	  processor_exit(proc);  // this call does not return
       }

       if (processor_table[proc].DebugSignal)
       {
	  processor_table[proc].DebugSignal = 0;
	  EnterDebugger();
       }


       while (processorSet[proc].lwp_bind_queue_head)
       {
	  lwp = get_lwp_bind(proc);
	  if (lwp)
	     put_local_lwp(lwp, proc);
       }

       if ((lwp_dispatch_queue_head             ||
	    soft_int_head                       ||
	    fast_lwp_head                       ||
	    processorSet[proc].soft_int_head    ||
	    processorSet[proc].fast_lwp_head    ||
	    processorSet[proc].lwp_queue_head)  &&
	   !processorSet[proc].lwp_worker_queue_head)
       {
	  if (LastLWPCreate + WAIT_INTERVAL < SystemCurrentTime)
	  {
	     sprintf(LWPName, "Light Weight Process %i", LWPCount++);
	     p = createThreadContext(LWPName, (LONG (*)())LWPProcess, 4096, (void *)proc, proc);
	     if (p)
		put_lwp_thread(p, proc);
	     LastLWPCreate = SystemCurrentTime;
	  }
       }

       if (rundown_queue_head)
       {
	  p = get_rundown();
	  if (p)
	  {
	     p->threadState = PS_SHUTDOWN;
	     kfree(p->stackEnd);
	     put_free_process(p);
	  }
       }

       while (processorSet[proc].bind_queue_head)
       {
	  p = get_bind(proc);
	  if (p)
	     put_local(p, proc);
       }

       while (processorSet[proc].bindsw_queue_head)
       {
	  p = get_bindsw(proc);
	  if (p)
	     put_local(p, proc);
       }

       if (dispatch_queue_head)
       {
	  p = get_dispatch();
	  if (p)
	     put_local(p, proc);
       }

       if (soft_int_head)
       {
	  register LWP *lwpWork;

	  cli();
	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_soft_int_chain();
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
	  processorSet[proc].sleepDisallowed = 0;
	  sti();
       }

       if (processorSet[proc].soft_int_head)
       {
	  register LWP *lwpWork;

	  cli();
	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_local_soft_int_chain(proc);
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
	  processorSet[proc].sleepDisallowed = 0;
	  sti();
       }

       if (fast_lwp_head)
       {
	  register LWP *lwpWork;

	  cli();
	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_fast_lwp_chain();
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
	  processorSet[proc].sleepDisallowed = 0;
	  sti();
       }

       if (processorSet[proc].fast_lwp_head)
       {
	  register LWP *lwpWork;

	  cli();
	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_local_fast_lwp_chain(proc);
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
	  processorSet[proc].sleepDisallowed = 0;
	  sti();
       }

       if (processorSet[proc].lwp_queue_head)
	  thread_switch();

       if (!processorSet[proc].eligibleCount &&
	   (processorUtilization[proc] + DELTA > systemUtilization ||
	    processorSet[proc].local_queue_count > averageActiveCount))
	  thread_switch();

       if (processorSet[proc].local_queue_head)
	  thread_switch();

    }

}

#endif

void thread_switch(void)
{

     register PROCESS *target, *curr, *idle;
     register LWP *lwp;
     register LONG proc;

     if (KernelInitialized != TRUE)
	return;

     cli();
     proc = get_processor_id();

     if (NestedInterrupts[proc])
     {
	panic("process switch during interrupt");
	return;
     }

     if (processorSet[proc].sleepDisallowed)
     {
	panic("process attempted sleep when not allowed");
	return;
     }

     curr = processorSet[proc].running_process;
     idle = processorSet[proc].idle_process;
     curr->refCount++;
     curr->lastProcessor = proc;

     if (soft_int_head)
     {
	  register LWP *lwpWork;

	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_soft_int_chain();
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
	  processorSet[proc].sleepDisallowed = 0;
     }

     if (processorSet[proc].soft_int_head)
     {
	  register LWP *lwpWork;

	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_local_soft_int_chain(proc);
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
	  processorSet[proc].sleepDisallowed = 0;
     }

     if (fast_lwp_head)
     {
	  register LWP *lwpWork;

	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_fast_lwp_chain();
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
	  processorSet[proc].sleepDisallowed = 0;
     }

     if (processorSet[proc].fast_lwp_head)
     {
	  register LWP *lwpWork;

	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_local_fast_lwp_chain(proc);
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
	  processorSet[proc].sleepDisallowed = 0;
     }

     if (processorSet[proc].idleCount++ > IDLE_COUNT)
     {
	// switch to idle
	target = idle;
	if (curr != idle)
	{
	   switch (curr->threadState)
	   {
	      case PS_LWP:
		 put_lwp_thread(curr, proc);
		 break;

	      case PS_SLEEP:
		 break;

	      case PS_HALT:
		 curr->stackPointer = 0;
		 put_rundown(curr);
		 break;

	      default:
		 if (curr->processorBinding)
		    put_local(curr, proc);
		 else
		 {
		    if (!processorSet[proc].eligibleCount &&
			(processorUtilization[proc] + DELTA > systemUtilization ||
			 processorSet[proc].local_queue_count > averageActiveCount))
		    {
		       processorSet[proc].eligibleCount++;
		       curr->stackPointer = 0;
		       put_dispatch(curr);
		    }
		    else
		       put_local(curr, proc);
		 }
		 break;
	   }
	}
	processorSet[proc].idleCount = 0;
	processorSet[proc].running_process = target;
	processorSet[proc].ModuleContext = target->ModuleContext;
	switch_process_context(curr, target);
	sti();
	return;
     }

     while (processorSet[proc].lwp_bind_queue_head)
     {
	lwp = get_lwp_bind(proc);
	if (lwp)
	   put_local_lwp(lwp, proc);
     }

     while (processorSet[proc].bind_queue_head)
     {
	target = get_bind(proc);
	if (target)
	   put_local(target, proc);
     }

     while (processorSet[proc].bindsw_queue_head)
     {
	target = get_bindsw(proc);
	if (target)
	   put_local(target, proc);
     }

     if (lwp_dispatch_queue_head              ||
	 soft_int_head                        ||
	 fast_lwp_head                        ||
	 processorSet[proc].soft_int_head     ||
	 processorSet[proc].fast_lwp_head     ||
	 processorSet[proc].lwp_queue_head)
     {
	target = get_lwp_thread(proc);
	if (target)
	{
	   if (curr != idle)
	   {
	      switch (curr->threadState)
	      {
		 case PS_LWP:
		    put_lwp_thread(curr, proc);
		    break;

		 case PS_SLEEP:
		    break;

		 case PS_HALT:
		    curr->stackPointer = 0;
		    put_rundown(curr);
		    break;

		 default:
		    if (curr->processorBinding)
		       put_local(curr, proc);
		    else
		    {
		       if (!processorSet[proc].eligibleCount &&
			  (processorUtilization[proc] + DELTA > systemUtilization ||
			   processorSet[proc].local_queue_count > averageActiveCount))
		       {
			  processorSet[proc].eligibleCount++;
			  curr->stackPointer = 0;
			  put_dispatch(curr);
		       }
		       else
			  put_local(curr, proc);
		    }
		    break;
	      }
	   }
	   processorSet[proc].running_process = target;
	   processorSet[proc].ModuleContext = target->ModuleContext;
	   switch_process_context(curr, target);
	   sti();
	   return;
	}
     }

     if (dispatch_queue_head)
     {
	target = get_dispatch();
	if (target)
	   put_local(target, proc);
     }

     if (processorSet[proc].local_queue_head)
     {
	target = get_local(proc);
	if (target)
	{
	   if (curr != idle)
	   {
	      switch (curr->threadState)
	      {
		 case PS_LWP:
		    put_lwp_thread(curr, proc);
		    break;

		 case PS_SLEEP:
		    break;

		 case PS_HALT:
		    curr->stackPointer = 0;
		    put_rundown(curr);
		    break;

		 default:
		    if (curr->processorBinding)
		       put_local(curr, proc);
		    else
		    {
		       if (!processorSet[proc].eligibleCount &&
			  (processorUtilization[proc] + DELTA > systemUtilization ||
			   processorSet[proc].local_queue_count > averageActiveCount))
		       {
			  processorSet[proc].eligibleCount++;
			  curr->stackPointer = 0;
			  put_dispatch(curr);
		       }
		       else
			  put_local(curr, proc);
		    }
		    break;
	      }
	   }
	   processorSet[proc].running_process = target;
	   processorSet[proc].ModuleContext = target->ModuleContext;
	   switch_process_context(curr, target);
	   sti();
	   return;
	}
     }

     target = idle;
     if (curr != idle)
     {
	switch (curr->threadState)
	{
	   case PS_LWP:
	      put_lwp_thread(curr, proc);
	      processorSet[proc].running_process = target;
	      processorSet[proc].ModuleContext = target->ModuleContext;
	      switch_process_context(curr, target);
	      sti();
	      return;

	   case PS_SLEEP:
	      processorSet[proc].running_process = target;
	      processorSet[proc].ModuleContext = target->ModuleContext;
	      switch_process_context(curr, target);
	      sti();
	      return;

	   case PS_HALT:
	      curr->stackPointer = 0;
	      put_rundown(curr);
	      processorSet[proc].running_process = target;
	      processorSet[proc].ModuleContext = target->ModuleContext;
	      switch_process_context(curr, target);
	      sti();
	      return;

	   default:
	      sti();
	      return;
	}
     }
     sti();
     return;

}

LONG thread_switch_and_bind(LONG p)
{

     register PROCESS *target, *curr, *idle;
     register LWP *lwp;
     register LONG proc;

     if (KernelInitialized != TRUE)
	return 0;

     // this is a request for unbind (-1)
     if (p == (LONG) -1)
     {
	register LONG flags;

	flags = get_flags();
	curr = get_running_process();
	if (curr->processorBinding)
	{
	   curr->processorBinding = 0;
	   set_flags(flags);
	   return 0;
	}
	else
	{
	   set_flags(flags);
	   return -1;
	}
     }

     if (p >= active_processors)
	return -3;

     cli();
     proc = get_processor_id();
     if (p == proc)
     {
	thread_switch();
	return -1;
     }

     if (NestedInterrupts[proc])
     {
	panic("process switch during interrupt");
	sti();
	return -4;
     }

     if (processorSet[proc].sleepDisallowed)
     {
	panic("process attempted sleep when not allowed");
	sti();
	return -5;
     }

     curr = processorSet[proc].running_process;
     idle = processorSet[proc].idle_process;
     curr->refCount++;
     curr->lastProcessor = proc;

     if (soft_int_head)
     {
	  register LWP *lwpWork;

	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_soft_int_chain();
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
	  processorSet[proc].sleepDisallowed = 0;
     }

     if (processorSet[proc].soft_int_head)
     {
	  register LWP *lwpWork;

	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_local_soft_int_chain(proc);
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
	  processorSet[proc].sleepDisallowed = 0;
     }

     if (fast_lwp_head)
     {
	  register LWP *lwpWork;

	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_fast_lwp_chain();
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
	  processorSet[proc].sleepDisallowed = 0;
     }

     if (processorSet[proc].fast_lwp_head)
     {
	  register LWP *lwpWork;

	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_local_fast_lwp_chain(proc);
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
	  processorSet[proc].sleepDisallowed = 0;
     }

     if (processorSet[proc].idleCount++ > IDLE_COUNT)
     {
	// switch to idle
	target = idle;
	if (curr != idle)
	{
	   switch (curr->threadState)
	   {
	      case PS_LWP:
		 put_lwp_thread(curr, proc);
		 break;

	      case PS_SLEEP:
		 break;

	      case PS_HALT:
		 curr->stackPointer = 0;
		 put_rundown(curr);
		 break;

	      default:
		 curr->processorBinding = (LONG) &processorSet[p];
		 curr->stackPointer = 0;
		 put_bind(curr, p);
		 break;
	   }
	}
	processorSet[proc].idleCount = 0;
	processorSet[proc].running_process = target;
	processorSet[proc].ModuleContext = target->ModuleContext;
	switch_process_context(curr, target);
	sti();
	return 0;
     }

     while (processorSet[proc].lwp_bind_queue_head)
     {
	lwp = get_lwp_bind(proc);
	if (lwp)
	   put_local_lwp(lwp, proc);
     }

     while (processorSet[proc].bind_queue_head)
     {
	target = get_bind(proc);
	if (target)
	   put_local(target, proc);
     }

     while (processorSet[proc].bindsw_queue_head)
     {
	target = get_bindsw(proc);
	if (target)
	   put_local(target, proc);
     }

     if (lwp_dispatch_queue_head             ||
 	 soft_int_head                       ||
	 fast_lwp_head                       ||
	 processorSet[proc].soft_int_head    ||
	 processorSet[proc].fast_lwp_head    ||
	 processorSet[proc].lwp_queue_head)
     {
	target = get_lwp_thread(proc);
	if (target)
	{
	   if (curr != idle)
	   {
	      switch (curr->threadState)
	      {
		 case PS_LWP:
		    put_lwp_thread(curr, proc);
		    break;

		 case PS_SLEEP:
		    break;

		 case PS_HALT:
		    curr->stackPointer = 0;
		    put_rundown(curr);
		    break;

		 default:
		    curr->processorBinding = (LONG) &processorSet[p];
		    curr->stackPointer = 0;
		    put_bind(curr, p);
		    break;
	      }
	   }
	   processorSet[proc].running_process = target;
	   processorSet[proc].ModuleContext = target->ModuleContext;
	   switch_process_context(curr, target);
	   sti();
	   return 0;
	}
     }

     if (dispatch_queue_head)
     {
	target = get_dispatch();
	if (target)
	   put_local(target, proc);
     }

     if (processorSet[proc].local_queue_head)
     {
	target = get_local(proc);
	if (target)
	{
	   if (curr != idle)
	   {
	      switch (curr->threadState)
	      {
		 case PS_LWP:
		    put_lwp_thread(curr, proc);
		    break;

		 case PS_SLEEP:
		    break;

		 case PS_HALT:
		    curr->stackPointer = 0;
		    put_rundown(curr);
		    break;

		 default:
		    curr->processorBinding = (LONG) &processorSet[p];
		    curr->stackPointer = 0;
		    put_bind(curr, p);
		    break;
	      }
	   }
	   processorSet[proc].running_process = target;
	   processorSet[proc].ModuleContext = target->ModuleContext;
	   switch_process_context(curr, target);
	   sti();
	   return 0;
	}
     }

     target = idle;
     if (curr != idle)
     {
	switch (curr->threadState)
	{
	   case PS_LWP:
	      put_lwp_thread(curr, proc);
	      processorSet[proc].running_process = target;
	      processorSet[proc].ModuleContext = target->ModuleContext;
	      switch_process_context(curr, target);
	      sti();
	      return 0;

	   case PS_SLEEP:
	      processorSet[proc].running_process = target;
	      processorSet[proc].ModuleContext = target->ModuleContext;
	      switch_process_context(curr, target);
	      sti();
	      return 0;

	   case PS_HALT:
	      curr->stackPointer = 0;
	      put_rundown(curr);
	      processorSet[proc].running_process = target;
	      processorSet[proc].ModuleContext = target->ModuleContext;
	      switch_process_context(curr, target);
	      sti();
	      return 0;

	   default:
	      sti();
	      return 0;
	}
     }
     sti();
     return 0;

}

LONG preempt_timer(void)
{

     register PROCESS *target;
     register TIMER *timer, *timerList;
     register LONG systemCount, threadCount, idleCount, i;
     register LONG proc;
     register LWP *lwp;
     register PROCESS *curr, *p, *idle;

     // if we are on zero, EOI the 8259 PIC (8254 ExtInt origin)
     // else EOI the local APIC (APIC LVT_TIMER origin)

     cli(); // ints should be off here

     proc = get_processor_id();
     curr = processorSet[proc].running_process;
     curr->lastProcessor = proc;
     curr->utilizationCount++;
     processorSet[proc].utilizationCount++;
     processor_table[proc].totalInterrupts++;

     if (!proc)
     {
	pic_eoi(0); // EOI the 8259
	if (TicksUp++ > 18)
	{

	   TicksUp = 0;
	   if (SecondsUp++ > 59)
	   {
	      SecondsUp = 0;
	      if (MinutesUp++ > 59)
	      {
		 MinutesUp = 0;
		 if (HoursUp++ > 23)
		 {
		    HoursUp = 0;
		    DaysUp++;
		 }
	      }
	   }
	}
     }
     else
	(processor_table[proc].EndOfInterrupt)(0); // EOI the local APIC

     // calculate processor utilization for all processors
     // processor zero will fire on the 8254, the other processors
     // use the local APIC timer to support preemption.  on processor
     // zero, we service system utilization, preempt to idle, and service
     // the timer event queue, the other processors record utilization
     // and preempt to the idle process.

     if (!proc)
     {
	SystemTickCounter++;

	GetSystemDTSC(&SystemDTSCTimeBase, &SystemDTSCTimeCounter);

	SystemCurrentTime++;

	if (processorSet[proc].sampleCount++ > SAMPLE_INTERVAL)
	{
	   processorSet[proc].sampleCount = 0;
	   averageActiveCount = idleCount = 0;
	   for (i=0; i < active_processors; i++)
	   {
	      processorCountArray[i] = processorSet[i].utilizationCount;
	      processorSet[i].utilizationCount = 0;
	      threadCountArray[i] = idleCountArray[i] = 0;
	   }

	   spin_lock(&systemThreadmutex);
	   p = systemThreadHead;
	   while (p)
	   {
	      switch (p->typeID)
	      {
		 case KERNEL_THREAD:
		    if (p->lastProcessor < active_processors)
		    {
		       threadCountArray[p->lastProcessor] += p->utilizationCount;
		    }
		    break;

		 case IDLE_THREAD:
		    if (p->lastProcessor < active_processors)
		    {
		       idleCountArray[p->lastProcessor] += p->utilizationCount;
		    }
		    break;
	      }
	      p = p->kernelNext;
	   }

	   p = systemThreadHead;
	   while (p)
	   {
	      switch (p->typeID)
	      {
		 case KERNEL_THREAD:
		    if (p->lastProcessor < active_processors)
		    {
		       if (threadCountArray[p->lastProcessor])
			  p->threadUtilization = \
			      p->utilizationCount * 100 /
			      processorCountArray[p->lastProcessor];
		       else
			  p->threadUtilization = 0;

		    }
		    break;

		 case IDLE_THREAD:
		    if (p->lastProcessor < active_processors)
		    {
		       if (idleCountArray[p->lastProcessor])
			  p->threadUtilization = \
			      p->utilizationCount * 100 /
			      processorCountArray[p->lastProcessor];
		       else
			  p->threadUtilization = 0;
		    }
		    break;
	      }
	      p->utilizationCount = 0;
	      p = p->kernelNext;

	   }
	   spin_unlock(&systemThreadmutex);

	   for (i=0; i < active_processors; i++)
	   {
	      idleCount += idleCountArray[i];

	      if (processorCountArray[i])
		 processorUtilization[i] = (threadCountArray[i] * 100) / processorCountArray[i];
	      else
		 processorUtilization[i] = 0;

	      if (processorUtilization[i] > 100)
		 processorUtilization[i] = 100;

	      averageActiveCount += processorSet[i].local_queue_count;
	   }

	   for (systemCount = 0, i=0; i < active_processors; i++)
	      systemCount += processorUtilization[i];

	   systemUtilization = systemCount / active_processors;

	   if (systemUtilization > 100)
	      systemUtilization = 100;

	   if (idleCount) {};

	   if (active_processors)
	      averageActiveCount = averageActiveCount / active_processors;

	   for (i=0; i < active_processors; i++)
	      processorSet[i].eligibleCount = 0;
	}
     }

     // now service timer events

     if (!proc)
	timerProcedure();

     if (soft_int_head)
     {
	  register LWP *lwpWork;

	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_soft_int_chain();
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
	  processorSet[proc].sleepDisallowed = 0;
     }

     if (processorSet[proc].soft_int_head)
     {
	  register LWP *lwpWork;

	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_local_soft_int_chain(proc);
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
	  processorSet[proc].sleepDisallowed = 0;
     }

     if (fast_lwp_head)
     {
	  register LWP *lwpWork;

	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_fast_lwp_chain();
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
	  processorSet[proc].sleepDisallowed = 0;
     }

     if (processorSet[proc].fast_lwp_head)
     {
	  register LWP *lwpWork;

	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_local_fast_lwp_chain(proc);
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
	  processorSet[proc].sleepDisallowed = 0;
     }

     //
     //   Now preempt to idle.  We do not preempt if the debugger is active
     //   or if we are inside of an interrupt service routine
     //

/*********

     if (!NestedInterrupts[proc] &&
	 curr->threadPreempt  	 &&
	 preemptOn[proc])
     {
	idle = processorSet[proc].idle_process;
	curr->refCount++;
	curr->lastProcessor = proc;
	if (curr != idle)
	{
	   switch (curr->threadState)
	   {
	      case PS_LWP:
		 put_lwp_thread(curr, proc);
		 break;

	      case PS_SLEEP:
		 break;

	      case PS_HALT:
		 curr->stackPointer = 0;
		 put_rundown(curr);
		 break;

	      default:
		 put_local(curr, proc);
		 break;
	   }
	   PreemptiveEvents++;
	   processorSet[proc].idleCount = 0;
	   processorSet[proc].running_process = idle;
	   processorSet[proc].ModuleContext = idle->ModuleContext;
	   return (LONG) idle;
	}
     }

********/

     return 0;

}

//  queue/de-queue routines

PROCESS *get_dispatch(void)
{

    register PROCESS *p;
    register LONG flags;

    flags = get_flags();
    spin_lock(&dispatch_queue_mutex);

    if (dispatch_queue_head && dispatch_queue_head->stackPointer)
    {
       p = dispatch_queue_head;
       dispatch_queue_head = (void *) p->next;
       if (dispatch_queue_head)
	  dispatch_queue_head->prior = NULL;
       else
	  dispatch_queue_tail = NULL;

       dispatch_queue_count--;

       spin_unlock(&dispatch_queue_mutex);
       set_flags(flags);
       return p;
    }

    spin_unlock(&dispatch_queue_mutex);
    set_flags(flags);
    return 0;

}

void put_dispatch(PROCESS *p)
{

    register LONG flags;

    flags = get_flags();
    spin_lock(&dispatch_queue_mutex);

    if (!dispatch_queue_head)
    {
       dispatch_queue_head = dispatch_queue_tail = p;
       p->next = p->prior = 0;
    }
    else
    {
       dispatch_queue_tail->next = p;
       p->next = 0;
       p->prior = dispatch_queue_tail;
       dispatch_queue_tail = p;
    }

    dispatch_queue_count++;

    spin_unlock(&dispatch_queue_mutex);
    set_flags(flags);
    return;

}

PROCESS *get_local(LONG proc)
{

    register PROCESS *p;
    register LONG flags;

    flags = get_flags();
    if (processorSet[proc].local_queue_head)
    {
       p = processorSet[proc].local_queue_head;
       processorSet[proc].local_queue_head = (void *) p->next;

       if (processorSet[proc].local_queue_head)
	  processorSet[proc].local_queue_head->prior = NULL;
       else
	  processorSet[proc].local_queue_tail = NULL;

       if (processorSet[proc].local_queue_count)
	  processorSet[proc].local_queue_count--;

       set_flags(flags);
       return p;
    }
    set_flags(flags);
    return 0;

}

void put_local(PROCESS *p, LONG proc)
{
    register LONG flags;

    flags = get_flags();

    if (!processorSet[proc].local_queue_head)
    {
       processorSet[proc].local_queue_head = p;
       processorSet[proc].local_queue_tail = p;
       p->next = p->prior = 0;
    }
    else
    {
       processorSet[proc].local_queue_tail->next = p;
       p->next = 0;
       p->prior = processorSet[proc].local_queue_tail;
       processorSet[proc].local_queue_tail = p;
    }
    processorSet[proc].local_queue_count++;

    set_flags(flags);
    return;
}

PROCESS *get_rundown(void)
{

    register PROCESS *p;
    register LONG flags;

    flags = get_flags();
    spin_lock(&rundown_queue_mutex);
    if (rundown_queue_head && rundown_queue_head->stackPointer)
    {
       p = rundown_queue_head;
       rundown_queue_head = (void *) p->next;

       if (rundown_queue_head)
	  rundown_queue_head->prior = NULL;
       else
	  rundown_queue_tail = NULL;

       spin_unlock(&rundown_queue_mutex);
       set_flags(flags);
       return p;
    }
    spin_unlock(&rundown_queue_mutex);
    set_flags(flags);
    return 0;

}

void put_rundown(PROCESS *p)
{

    register LONG flags;

    flags = get_flags();
    spin_lock(&rundown_queue_mutex);
    if (!rundown_queue_head)
    {
       rundown_queue_head = p;
       rundown_queue_tail = p;
       p->next = p->prior = 0;
    }
    else
    {
       rundown_queue_tail->next = p;
       p->next = 0;
       p->prior = rundown_queue_tail;
       rundown_queue_tail = p;
    }
    spin_unlock(&rundown_queue_mutex);
    set_flags(flags);
    return;

}

PROCESS *get_bind(LONG proc)
{

    register PROCESS *p;
    register LONG flags;

    flags = get_flags();
    spin_lock(&processorSet[proc].bind_queue_mutex);

    if (processorSet[proc].bind_queue_head)
    {
       p = processorSet[proc].bind_queue_head;
       processorSet[proc].bind_queue_head = (void *) p->next;

       if (processorSet[proc].bind_queue_head)
	  processorSet[proc].bind_queue_head->prior = NULL;
       else
	  processorSet[proc].bind_queue_tail = NULL;

       spin_unlock(&processorSet[proc].bind_queue_mutex);
       set_flags(flags);
       return p;
    }

    spin_unlock(&processorSet[proc].bind_queue_mutex);
    set_flags(flags);
    return 0;

}

void put_bind(PROCESS *p, LONG proc)
{

    register LONG flags;

    flags = get_flags();
    spin_lock(&processorSet[proc].bind_queue_mutex);

    if (!processorSet[proc].bind_queue_head)
    {
       processorSet[proc].bind_queue_head = p;
       processorSet[proc].bind_queue_tail = p;
       p->next = p->prior = 0;
    }
    else
    {
       processorSet[proc].bind_queue_tail->next = p;
       p->next = 0;
       p->prior = processorSet[proc].bind_queue_tail;
       processorSet[proc].bind_queue_tail = p;
    }

    spin_unlock(&processorSet[proc].bind_queue_mutex);
    set_flags(flags);
    return;

}

void put_bind_pset(PROCESS *p, PROCESSOR_SET *pset)
{

    register LONG flags;

    flags = get_flags();
    spin_lock(&pset->bind_queue_mutex);

    if (!pset->bind_queue_head)
    {
       pset->bind_queue_head = p;
       pset->bind_queue_tail = p;
       p->next = p->prior = 0;
    }
    else
    {
       pset->bind_queue_tail->next = p;
       p->next = 0;
       p->prior = pset->bind_queue_tail;
       pset->bind_queue_tail = p;
    }

    spin_unlock(&pset->bind_queue_mutex);
    set_flags(flags);
    return;

}

PROCESS *get_bindsw(LONG proc)
{

    register PROCESS *p;
    register LONG flags;

    flags = get_flags();
    spin_lock(&processorSet[proc].bindsw_queue_mutex);

    if (processorSet[proc].bindsw_queue_head)
    {
       p = processorSet[proc].bindsw_queue_head;
       processorSet[proc].bindsw_queue_head = (void *) p->next;

       if (processorSet[proc].bindsw_queue_head)
	  processorSet[proc].bindsw_queue_head->prior = NULL;
       else
	  processorSet[proc].bindsw_queue_tail = NULL;

       spin_unlock(&processorSet[proc].bindsw_queue_mutex);
       set_flags(flags);
       return p;
    }

    spin_unlock(&processorSet[proc].bindsw_queue_mutex);
    set_flags(flags);
    return 0;

}

void put_bindsw(PROCESS *p, LONG proc)
{

    register LONG flags;

    flags = get_flags();
    spin_lock(&processorSet[proc].bindsw_queue_mutex);

    if (!processorSet[proc].bindsw_queue_head)
    {
       processorSet[proc].bindsw_queue_head = p;
       processorSet[proc].bindsw_queue_tail = p;
       p->next = p->prior = 0;
    }
    else
    {
       processorSet[proc].bindsw_queue_tail->next = p;
       p->next = 0;
       p->prior = processorSet[proc].bindsw_queue_tail;
       processorSet[proc].bindsw_queue_tail = p;
    }

    spin_unlock(&processorSet[proc].bindsw_queue_mutex);
    set_flags(flags);
    return;

}

PROCESS *get_lwp_thread(LONG proc)
{

    register PROCESS *p;
    register LONG flags;

    flags = get_flags();
    if (processorSet[proc].lwp_worker_queue_head)
    {
       p = processorSet[proc].lwp_worker_queue_head;
       processorSet[proc].lwp_worker_queue_head = (void *) p->next;

       if (processorSet[proc].lwp_worker_queue_head)
	  processorSet[proc].lwp_worker_queue_head->prior = NULL;
       else
	  processorSet[proc].lwp_worker_queue_tail = NULL;

       set_flags(flags);
       return p;
    }
    set_flags(flags);
    return 0;

}

void put_lwp_thread(PROCESS *p, LONG proc)
{

    register LONG flags;

    flags = get_flags();
    if (!processorSet[proc].lwp_worker_queue_head)
    {
       processorSet[proc].lwp_worker_queue_head = p;
       processorSet[proc].lwp_worker_queue_tail = p;
       p->next = p->prior = 0;
    }
    else
    {
       processorSet[proc].lwp_worker_queue_tail->next = p;
       p->next = 0;
       p->prior = processorSet[proc].lwp_worker_queue_tail;
       processorSet[proc].lwp_worker_queue_tail = p;
    }
    set_flags(flags);
    return;

}

LWP *get_lwp_bind(LONG proc)
{

    register LWP *p;
    register LONG flags;

    flags = get_flags();
    spin_lock(&processorSet[proc].lwp_bind_queue_mutex);

    if (processorSet[proc].lwp_bind_queue_head)
    {
       p = processorSet[proc].lwp_bind_queue_head;
       processorSet[proc].lwp_bind_queue_head = (void *) p->next;

       if (processorSet[proc].lwp_bind_queue_head)
	  processorSet[proc].lwp_bind_queue_head->prior = NULL;
       else
	  processorSet[proc].lwp_bind_queue_tail = NULL;

       spin_unlock(&processorSet[proc].lwp_bind_queue_mutex);
       set_flags(flags);
       return p;
    }
    spin_unlock(&processorSet[proc].lwp_bind_queue_mutex);
    set_flags(flags);
    return 0;

}

void put_lwp_bind(LWP *p, LONG proc)
{

    register LONG flags;

    flags = get_flags();
    spin_lock(&processorSet[proc].lwp_bind_queue_mutex);
    if (!processorSet[proc].lwp_bind_queue_head)
    {
       processorSet[proc].lwp_bind_queue_head = p;
       processorSet[proc].lwp_bind_queue_tail = p;
       p->next = p->prior = 0;
    }
    else
    {
       processorSet[proc].lwp_bind_queue_tail->next = p;
       p->next = 0;
       p->prior = processorSet[proc].lwp_bind_queue_tail;
       processorSet[proc].lwp_bind_queue_tail = p;
    }
    spin_unlock(&processorSet[proc].lwp_bind_queue_mutex);
    set_flags(flags);
    return;

}

LWP *get_dispatch_lwp(void)
{

    register LWP *p;
    register LONG flags;

    flags = get_flags();
    spin_lock(&lwp_dispatch_queue_mutex);

    if (lwp_dispatch_queue_head)
    {
       p = lwp_dispatch_queue_head;
       lwp_dispatch_queue_head = (void *) p->next;

       if (lwp_dispatch_queue_head)
	  lwp_dispatch_queue_head->prior = NULL;
       else
	  lwp_dispatch_queue_tail = NULL;

       spin_unlock(&lwp_dispatch_queue_mutex);
       set_flags(flags);
       return p;
    }
    spin_unlock(&lwp_dispatch_queue_mutex);
    set_flags(flags);
    return 0;

}

void put_dispatch_lwp(LWP *p)
{

    register LONG flags;

    flags = get_flags();
    spin_lock(&lwp_dispatch_queue_mutex);
    if (!lwp_dispatch_queue_head)
    {
       lwp_dispatch_queue_head = p;
       lwp_dispatch_queue_tail = p;
       p->next = p->prior = 0;
    }
    else
    {
       lwp_dispatch_queue_tail->next = p;
       p->next = 0;
       p->prior = lwp_dispatch_queue_tail;
       lwp_dispatch_queue_tail = p;
    }
    spin_unlock(&lwp_dispatch_queue_mutex);
    set_flags(flags);
    return;

}

LWP *get_local_lwp(LONG proc)
{

    register LWP *p;
    register LONG flags;

    flags = get_flags();
    if (processorSet[proc].lwp_queue_head)
    {
       p = processorSet[proc].lwp_queue_head;
       processorSet[proc].lwp_queue_head = (void *) p->next;

       if (processorSet[proc].lwp_queue_head)
	  processorSet[proc].lwp_queue_head->prior = NULL;
       else
	  processorSet[proc].lwp_queue_tail = NULL;

       set_flags(flags);
       return p;
    }
    set_flags(flags);
    return 0;

}

void put_local_lwp(LWP *p, LONG proc)
{

    register LONG flags;

    flags = get_flags();
    if (!processorSet[proc].lwp_queue_head)
    {
       processorSet[proc].lwp_queue_head = p;
       processorSet[proc].lwp_queue_tail = p;
       p->next = p->prior = 0;
    }
    else
    {
       processorSet[proc].lwp_queue_tail->next = p;
       p->next = 0;
       p->prior = processorSet[proc].lwp_queue_tail;
       processorSet[proc].lwp_queue_tail = p;
    }
    set_flags(flags);
    return;

}

LWP *get_fast_lwp_chain(void)
{

    register LWP *p;
    register LONG flags;

    flags = get_flags();
    spin_lock(&fast_lwp_lock);
    if (fast_lwp_head)
    {
       p = fast_lwp_head;
       fast_lwp_head = fast_lwp_tail = NULL;
       spin_unlock(&fast_lwp_lock);
       set_flags(flags);
       return p;
    }
    spin_unlock(&fast_lwp_lock);
    set_flags(flags);
    return 0;

}

LWP *get_fast_lwp(void)
{

    register LWP *p;
    register LONG flags;

    flags = get_flags();
    spin_lock(&fast_lwp_lock);
    if (fast_lwp_head)
    {
       p = fast_lwp_head;
       fast_lwp_head = (void *) p->next;

       if (fast_lwp_head)
	  fast_lwp_head->prior = NULL;
       else
	  fast_lwp_tail = NULL;

       spin_unlock(&fast_lwp_lock);
       set_flags(flags);
       return p;
    }
    spin_unlock(&fast_lwp_lock);
    set_flags(flags);
    return 0;

}

void put_fast_lwp(LWP *p)
{

    register LONG flags;

    flags = get_flags();
    spin_lock(&fast_lwp_lock);
    if (!fast_lwp_head)
    {
       fast_lwp_head = p;
       fast_lwp_tail = p;
       p->next = p->prior = 0;
    }
    else
    {
       fast_lwp_tail->next = p;
       p->next = 0;
       p->prior = fast_lwp_tail;
       fast_lwp_tail = p;
    }
    spin_unlock(&fast_lwp_lock);
    set_flags(flags);
    return;

}

LWP *get_local_fast_lwp_chain(LONG proc)
{

    register LWP *p;
    register LONG flags;

    flags = get_flags();
    if (processorSet[proc].fast_lwp_head)
    {
       p = processorSet[proc].fast_lwp_head;
       processorSet[proc].fast_lwp_head = \
	  processorSet[proc].fast_lwp_tail = NULL;
       set_flags(flags);
       return p;
    }
    set_flags(flags);
    return 0;

}
LWP *get_local_fast_lwp(LONG proc)
{

    register LWP *p;
    register LONG flags;

    flags = get_flags();
    if (processorSet[proc].fast_lwp_head)
    {
       p = processorSet[proc].fast_lwp_head;
       processorSet[proc].fast_lwp_head = (void *) p->next;

       if (processorSet[proc].fast_lwp_head)
	  processorSet[proc].fast_lwp_head->prior = NULL;
       else
	  processorSet[proc].fast_lwp_tail = NULL;

       set_flags(flags);
       return p;
    }
    set_flags(flags);
    return 0;

}

void put_local_fast_lwp(LWP *p, LONG proc)
{

    register LONG flags;

    flags = get_flags();
    if (!processorSet[proc].fast_lwp_head)
    {
       processorSet[proc].fast_lwp_head = p;
       processorSet[proc].fast_lwp_tail = p;
       p->next = p->prior = 0;
    }
    else
    {
       processorSet[proc].fast_lwp_tail->next = p;
       p->next = 0;
       p->prior = processorSet[proc].fast_lwp_tail;
       processorSet[proc].fast_lwp_tail = p;
    }
    set_flags(flags);
    return;

}

LWP *get_local_soft_int_chain(LONG proc)
{

    register LWP *p;
    register LONG flags;

    flags = get_flags();
    if (processorSet[proc].soft_int_head)
    {
       p = processorSet[proc].soft_int_head;
       processorSet[proc].soft_int_head = \
	  processorSet[proc].soft_int_tail = NULL;
       set_flags(flags);
       return p;
    }
    set_flags(flags);
    return 0;

}

LWP *get_local_soft_int(LONG proc)
{

    register LWP *p;
    register LONG flags;

    flags = get_flags();
    if (processorSet[proc].soft_int_head)
    {
       p = processorSet[proc].soft_int_head;
       processorSet[proc].soft_int_head = (void *) p->next;

       if (processorSet[proc].soft_int_head)
	  processorSet[proc].soft_int_head->prior = NULL;
       else
	  processorSet[proc].soft_int_tail = NULL;

       set_flags(flags);
       return p;
    }
    set_flags(flags);
    return 0;

}

void put_local_soft_int(LWP *p, LONG proc)
{

    register LONG flags;

    flags = get_flags();
    if (!processorSet[proc].soft_int_head)
    {
       processorSet[proc].soft_int_head = p;
       processorSet[proc].soft_int_tail = p;
       p->next = p->prior = 0;
    }
    else
    {
       processorSet[proc].soft_int_tail->next = p;
       p->next = 0;
       p->prior = processorSet[proc].soft_int_tail;
       processorSet[proc].soft_int_tail = p;
    }
    set_flags(flags);
    return;

}

LWP *get_soft_int_chain(void)
{

    register LWP *p;
    register LONG flags;

    flags = get_flags();
    spin_lock(&soft_int_lock);
    if (soft_int_head)
    {
       p = soft_int_head;
       soft_int_head = soft_int_tail = NULL;
       spin_unlock(&soft_int_lock);
       set_flags(flags);
       return p;
    }
    spin_unlock(&soft_int_lock);
    set_flags(flags);
    return 0;

}

LWP *get_soft_int(void)
{

    register LWP *p;
    register LONG flags;

    flags = get_flags();
    spin_lock(&soft_int_lock);
    if (soft_int_head)
    {
       p = soft_int_head;
       soft_int_head = (void *) p->next;

       if (soft_int_head)
	  soft_int_head->prior = NULL;
       else
	  soft_int_tail = NULL;

       spin_unlock(&soft_int_lock);
       set_flags(flags);
       return p;
    }
    spin_unlock(&soft_int_lock);
    set_flags(flags);
    return 0;

}

void put_soft_int(LWP *p)
{

    register LONG flags;

    flags = get_flags();
    spin_lock(&soft_int_lock);
    if (!soft_int_head)
    {
       soft_int_head = p;
       soft_int_tail = p;
       p->next = p->prior = 0;
    }
    else
    {
       soft_int_tail->next = p;
       p->next = 0;
       p->prior = soft_int_tail;
       soft_int_tail = p;
    }
    spin_unlock(&soft_int_lock);
    set_flags(flags);
    return;

}

PROCESS *get_running_process(void)
{
   return (PROCESS *) processorSet[get_processor_id()].running_process;
}

LONG StartSleepNotAllowed(void)
{
   register LONG proc = get_processor_id();
   register LONG retCode;

   retCode = processorSet[proc].sleepDisallowed;
   processorSet[proc].sleepDisallowed = 1;

   return retCode;
}

LONG EndSleepNotAllowed(void)
{
   register LONG proc = get_processor_id();
   register LONG retCode;

   retCode = processorSet[proc].sleepDisallowed;
   processorSet[proc].sleepDisallowed = 0;

   return retCode;
}

LONG DisablePreemption(void)
{
   register LONG prev;

   prev = preemptOn[get_processor_id()];
   preemptOn[get_processor_id()] = 0;

   return prev;

}

LONG EnablePreemption(void)
{
   register LONG prev;

   prev = preemptOn[get_processor_id()];
   preemptOn[get_processor_id()] = 1;

   return prev;
}

void DisplayKernel(SCREEN *screen, LONG p)
{

     SetPauseMode(screen, screen->nLines - 3);

     printfScreenWithAttribute(screen, BRITEWHITE, "System Global Queues\n");
     printfScreenWithAttribute(screen, LTGREEN, "dispatch head          : %08X\n", dispatch_queue_head);
     printfScreenWithAttribute(screen, LTGREEN, "dispatch tail          : %08X\n", dispatch_queue_tail);
     printfScreenWithAttribute(screen, LTGREEN, "lwp disp head          : %08X\n", lwp_dispatch_queue_head);
     printfScreenWithAttribute(screen, LTGREEN, "lwp disp tail          : %08X\n", lwp_dispatch_queue_tail);
     printfScreenWithAttribute(screen, LTGREEN, "dispatch count         : %08X\n", dispatch_queue_count);
     printfScreenWithAttribute(screen, LTGREEN, "lwp disp count         : %08X\n", lwp_dispatch_queue_count);
     printfScreenWithAttribute(screen, LTGREEN, "rundown head           : %08X\n", rundown_queue_head);
     printfScreenWithAttribute(screen, LTGREEN, "rundown tail           : %08X\n", rundown_queue_tail);
     printfScreenWithAttribute(screen, LTGREEN, "averageActiveCount     : %08X\n", averageActiveCount);

     printfScreenWithAttribute(screen, BRITEWHITE, "Processor State\n");
     printfScreenWithAttribute(screen, LTGREEN, "processor number       : %d\n", processorSet[p].processorNumber);
     printfScreenWithAttribute(screen, LTGREEN, "sleep allowed          : %d\n", processorSet[p].sleepDisallowed);

     printfScreenWithAttribute(screen, BRITEWHITE, "Processor Context\n");
     printfScreenWithAttribute(screen, LTGREEN, "running module         : %08X\n", processorSet[p].ModuleContext);
     printfScreenWithAttribute(screen, LTGREEN, "address space          : %08X\n", processorSet[p].AddressSpace);
     printfScreenWithAttribute(screen, LTGREEN, "idle process           : %08X\n", processorSet[p].idle_process);
     printfScreenWithAttribute(screen, LTGREEN, "running process        : %08X\n", processorSet[p].running_process);

     printfScreenWithAttribute(screen, BRITEWHITE, "Processor Local Queues\n");
     printfScreenWithAttribute(screen, LTGREEN, "local queue head       : %08X\n", processorSet[p].local_queue_head);
     printfScreenWithAttribute(screen, LTGREEN, "local queue tail       : %08X\n", processorSet[p].local_queue_tail);
     printfScreenWithAttribute(screen, LTGREEN, "bind queue head        : %08X\n", processorSet[p].bind_queue_head);
     printfScreenWithAttribute(screen, LTGREEN, "bind queue tail        : %08X\n", processorSet[p].bind_queue_tail);
     printfScreenWithAttribute(screen, LTGREEN, "bind queue mutex       : %d\n", processorSet[p].bind_queue_mutex);
     printfScreenWithAttribute(screen, LTGREEN, "lwp queue head         : %08X\n", processorSet[p].lwp_queue_head);
     printfScreenWithAttribute(screen, LTGREEN, "lwp queue tail         : %08X\n", processorSet[p].lwp_queue_tail);
     printfScreenWithAttribute(screen, LTGREEN, "lwp bind queue head    : %08X\n", processorSet[p].lwp_bind_queue_head);
     printfScreenWithAttribute(screen, LTGREEN, "lwp bind queue tail    : %08X\n", processorSet[p].lwp_bind_queue_tail);
     printfScreenWithAttribute(screen, LTGREEN, "lwp bind queue mutex   : %d\n", processorSet[p].lwp_bind_queue_mutex);
     printfScreenWithAttribute(screen, LTGREEN, "soft int head          : %08X\n", processorSet[p].soft_int_head);
     printfScreenWithAttribute(screen, LTGREEN, "soft int tail          : %08X\n", processorSet[p].soft_int_tail);
     printfScreenWithAttribute(screen, LTGREEN, "soft int mutex         : %d\n", processorSet[p].soft_int_mutex);
     printfScreenWithAttribute(screen, LTGREEN, "fast lwp head          : %08X\n", processorSet[p].fast_lwp_head);
     printfScreenWithAttribute(screen, LTGREEN, "fast lwp tail          : %08X\n", processorSet[p].fast_lwp_tail);
     printfScreenWithAttribute(screen, LTGREEN, "fast lwp mutex         : %d\n", processorSet[p].fast_lwp_mutex);
     printfScreenWithAttribute(screen, LTGREEN, "lwp worker queue head  : %08X\n", processorSet[p].lwp_worker_queue_head);
     printfScreenWithAttribute(screen, LTGREEN, "lwp worker queue tail  : %08X\n", processorSet[p].lwp_worker_queue_tail);
     printfScreenWithAttribute(screen, LTGREEN, "bindsw queue head      : %08X\n", processorSet[p].bindsw_queue_head);
     printfScreenWithAttribute(screen, LTGREEN, "bindsw queue tail      : %08X\n", processorSet[p].bindsw_queue_tail);
     printfScreenWithAttribute(screen, LTGREEN, "bindsw_queue_mutex     : %d\n", processorSet[p].bindsw_queue_mutex);

     printfScreenWithAttribute(screen, BRITEWHITE, "Processor Counters\n");
     printfScreenWithAttribute(screen, LTGREEN, "local queue count      : %d\n", processorSet[p].local_queue_count);
     printfScreenWithAttribute(screen, LTGREEN, "bind queue count       : %d\n", processorSet[p].bind_queue_count);
     printfScreenWithAttribute(screen, LTGREEN, "lwp queue count        : %d\n", processorSet[p].lwp_queue_count);
     printfScreenWithAttribute(screen, LTGREEN, "lwp execute count      : %d\n", processorSet[p].lwpExecCount);
     printfScreenWithAttribute(screen, LTGREEN, "lwp idle count         : %d\n", processorSet[p].idleCount);
     printfScreenWithAttribute(screen, LTGREEN, "lwp utilization count  : %d\n", processorSet[p].utilizationCount);
     printfScreenWithAttribute(screen, LTGREEN, "sampling count         : %d\n", processorSet[p].sampleCount);
     printfScreenWithAttribute(screen, LTGREEN, "processor utilization  : %d\n", processorSet[p].processorUtilization);
     printfScreenWithAttribute(screen, LTGREEN, "eligible count         : %d\n", processorSet[p].eligibleCount);
     printfScreenWithAttribute(screen, LTGREEN, "active count           : %d\n", processorSet[p].activeCount);
     printfScreenWithAttribute(screen, LTGREEN, "lwp process counter    : %d\n", processorSet[p].lwpProcessCounter);

     ClearPauseMode(screen);

}

void InitializeKernel(void)
{
    register LONG i, stackSize;
    register BYTE *stack;
    BYTE buf[100];
    extern MODULE_HANDLE serverModuleHandle;
    extern LONG OSPDE;

    dispatch_queue_head = 0;
    dispatch_queue_tail = 0;
    dispatch_queue_mutex = 0;
    rundown_queue_head = 0;
    rundown_queue_tail = 0;
    rundown_queue_mutex = 0;
    lwp_dispatch_queue_head = 0;
    lwp_dispatch_queue_tail = 0;
    lwp_dispatch_queue_mutex = 0;
    dispatch_queue_count = 0;
    lwp_dispatch_queue_count = 0;
    systemUtilization = 0;

    for (i=0; i < MAX_PROCESSORS; i++)
    {
       preemptOn[i] = 1;
       ptable[i] = (PROCESSOR_SET *) &processorSet[i];
       processorCountArray[i] = threadCountArray[i] = 0;
       idleCountArray[i] = processorUtilization[i] = 0;

       stack = (BYTE *) initial_stack_array[i];
       stackSize = initial_stack_size_array[i];
       SetData((LONG *) &idle_p[i], 0x00000000, sizeof(PROCESS));
       sprintf((void *)&idle_p[i].processName, "idle process (%i)", i);
       idle_p[i].resourceFlag = KERNEL_MEMORY;
       idle_p[i].processSignature = PROCESS_SIGNATURE;
       idle_p[i].lastProcessor = i;
       idle_p[i].processID = (LONG) &idle_p[i];
       idle_p[i].typeID = IDLE_THREAD;
       idle_p[i].threadState = PS_ACTIVE;

       idle_p[i].stackPointer = (LONG *) &stack[stackSize - (32 * 4)];
       idle_p[i].stackLimit = (LONG *) &stack[stackSize];
       idle_p[i].stackEnd = (LONG *) &stack[0];

       idle_p[i].startParm = (void *) i;
       idle_p[i].startAddress = (LONG (*)()) idle_loop;
       idle_p[i].ModuleContext = &serverModuleHandle;
       idle_p[i].AddressSpace = (void *) OSPDE;

       SetData((LONG *) &processorSet[i], 0, sizeof(PROCESSOR_SET));
       processorSet[i].idle_process = (PROCESS *) &idle_p[i];
       processorSet[i].running_process = (PROCESS *) &idle_p[i];
       processorSet[i].processorNumber = i;
       processorSet[i].ModuleContext = &serverModuleHandle;
       processorSet[i].AddressSpace = (void *) OSPDE;
    }
    KernelInitialized = TRUE;

}

void GetSystemDTSC(LONG *base, LONG *counter)
{
    if (DosDataTable->CPU_TYPE >= 5)
       ReadDTSC(base, counter);
    else
    {
       (*base)++;
       (*counter)++;
    }
}

void process_soft_ints(void)
{
   register LONG proc = get_processor_id();
   register LWP *lwp;

   if (soft_int_head)
   {
      register LWP *lwpWork;

      processorSet[proc].sleepDisallowed = 1;
      lwp = get_soft_int_chain();
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
      processorSet[proc].sleepDisallowed = 0;
   }

}

void process_local_soft_ints(void)
{
   register LONG proc = get_processor_id();
   register LWP *lwp;

   if (processorSet[proc].soft_int_head)
   {
      register LWP *lwpWork;

      processorSet[proc].sleepDisallowed = 1;
      lwp = get_local_soft_int_chain(proc);
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
      processorSet[proc].sleepDisallowed = 0;
   }

}

LONG preempt_worker(void)
{

     register LONG proc = get_processor_id();
     register LWP *lwp;
     register PROCESS *curr, *p, *idle;

     curr = processorSet[proc].running_process;
     curr->lastProcessor = proc;

     if ((soft_int_head                       ||
	 fast_lwp_head                        ||
	 processorSet[proc].soft_int_head     ||
	 processorSet[proc].fast_lwp_head     ||
	 processorSet[proc].lwp_queue_head)   &&
	 curr->threadPreempt  		      &&
	 preemptOn[proc])
     {
	idle = processorSet[proc].idle_process;
	curr->refCount++;
	curr->lastProcessor = proc;
	if (curr != idle)
	{
	   switch (curr->threadState)
	   {
	      case PS_LWP:
		 put_lwp_thread(curr, proc);
		 break;

	      case PS_SLEEP:
		 break;

	      case PS_HALT:
		 curr->stackPointer = 0;
		 put_rundown(curr);
		 break;

	      default:
		 put_local(curr, proc);
		 break;
	   }
	   PreemptiveEvents++;
	   processorSet[proc].idleCount = 0;
	   processorSet[proc].running_process = idle;
	   return (LONG) idle;
	}
     }

     return 0;
}

LONG sleep_current_process(void)
{
     register LONG proc;
     register PROCESS *curr, *idle;

     proc = get_processor_id();
     curr = processorSet[proc].running_process;
     idle = processorSet[proc].idle_process;

     curr->lastProcessor = proc;
     curr->refCount++;
     curr->lastProcessor = proc;
     curr->threadState = PS_SLEEP;
     if (curr != idle)
     {
	processorSet[proc].idleCount = 0;
	processorSet[proc].running_process = idle;
	return (LONG) idle;
     }
     return 0;

}

void process_fast_lwp(LONG proc)
{

     register LWP *lwp;

     if (fast_lwp_head)
     {
	  register LWP *lwpWork;

	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_fast_lwp_chain();
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
	  processorSet[proc].sleepDisallowed = 0;
     }

}

void process_local_fast_lwp(LONG proc)
{

     register LWP *lwp;

     if (processorSet[proc].fast_lwp_head)
     {
	  register LWP *lwpWork;

	  processorSet[proc].sleepDisallowed = 1;
	  lwp = get_local_fast_lwp_chain(proc);
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
	  processorSet[proc].sleepDisallowed = 0;
     }

}


