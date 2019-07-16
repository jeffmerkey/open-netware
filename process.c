
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
*   FILE     :  PROCESS.C
*   DESCRIP  :  Multi-Processing Kernel for MANOS v1.0
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
#include "malloc.h"
#include "free.h"
#include "event.h"

#define DEFAULT_PREEMPT_VALUE   0
#define VERBOSE_KERNEL          0

PROCESS *systemThreadHead = 0;
PROCESS *systemThreadTail = 0;
LONG systemThreadCount = 0;
LONG systemThreadmutex = 0;
PROCESS *pFreeListHead = 0;
PROCESS *pFreeListTail = 0;
LONG pFreeListMutex = 0;

PROCESS *createThread(BYTE *name, LONG (*start)(), LONG stackSize, void *parm, LONG bind)
{
    register PROCESS *p;
    register BYTE *stack;
    extern LONG get_current_flags(void);

    if (bind != (LONG) -1 && bind > active_processors - 1)
       return 0;

    p = get_free_process();
    if (!p)
    {
       p = (PROCESS *) kmalloc(sizeof(PROCESS));
       if (!p)
	  return 0;
    }

    stack = (BYTE *) kmalloc(stackSize);
    if (!stack)
    {
       put_free_process(p);
       return 0;
    }

    SetData((LONG *) p, 0x00000000, sizeof(PROCESS));
    sprintf((void *) p->processName, name);

    sema_init(&p->thread_sema, 0);

    p->processID = (LONG) p;
    p->typeID = KERNEL_THREAD;
    p->resourceFlag = KERNEL_MEMORY;
    p->threadState = PS_ACTIVE;
    p->processSignature = PROCESS_SIGNATURE;
    p->stackPointer = (LONG *) &stack[stackSize - (32 * 4)];
    p->stackLimit = (LONG *) &stack[stackSize];
    p->stackEnd = (LONG *) &stack[0];
    p->startParm = parm;
    p->startAddress = (LONG (*)()) start;
    p->threadPreempt = DEFAULT_PREEMPT_VALUE;

    (bind == (LONG) -1)
    ? (p->processorBinding = 0)
    : (p->processorBinding = (LONG) &processorSet[bind]);

    p->stackPointer[0] = 0x10;   // fs   // OSDATA
    p->stackPointer[1] = 0x10;   // es   // OSDATA
    p->stackPointer[2] = 0x10;   // ds   // OSDATA
    p->stackPointer[3] = 0;      // edi
    p->stackPointer[4] = 0;      // esi
    p->stackPointer[5] = 0;      // ebp
    p->stackPointer[6] = 0;      // edx
    p->stackPointer[7] = 0;      // ecx
    p->stackPointer[8] = 0;      // ebx
    p->stackPointer[9] = 0;      // eax
    p->stackPointer[10] = (LONG) threadStart;
    p->stackPointer[11] = (LONG) 0x08;   // OSCODE
    p->stackPointer[12] = (LONG) get_current_flags();
    p->stackPointer[13] = 0;
    p->stackPointer[14] = (LONG) p;
    p->stackPointer[15] = (LONG) STACK_END;

    p->next = p->prior = 0;
    p->kernelNext = p->kernelPrior = 0;

    insert_process(p);

    (bind == (LONG) -1)
    ? put_dispatch(p)
    : put_bind(p, bind);

    return p;

}

PROCESS *createThreadContext(BYTE *name, LONG (*start)(), LONG stackSize, void *parm, LONG bind)
{
    register PROCESS *p;
    register BYTE *stack;
    extern LONG get_current_flags(void);

    if (bind != (LONG) -1 && bind > active_processors - 1)
       return 0;

    p = get_free_process();
    if (!p)
    {
       p = (PROCESS *) kmalloc(sizeof(PROCESS));
       if (!p)
	  return 0;
    }
    stack = (BYTE *) kmalloc(stackSize);
    if (!stack)
    {
       put_free_process(p);
       return 0;
    }

    SetData((LONG *) p, 0x00000000, sizeof(PROCESS));
    sprintf((void *) p->processName, name);

    sema_init(&p->thread_sema, 0);

    p->processID = (LONG) p;
    p->typeID = KERNEL_THREAD;
    p->resourceFlag = KERNEL_MEMORY;
    p->threadState = PS_ACTIVE;
    p->processSignature = PROCESS_SIGNATURE;
    p->stackPointer = (LONG *) &stack[stackSize - (32 * 4)];
    p->stackLimit = (LONG *) &stack[stackSize];
    p->stackEnd = (LONG *) &stack[0];
    p->startParm = parm;
    p->startAddress = (LONG (*)()) start;
    p->threadPreempt = DEFAULT_PREEMPT_VALUE;

    (bind == (LONG) -1)
    ? (p->processorBinding = 0)
    : (p->processorBinding = (LONG) &processorSet[bind]);

    p->stackPointer[0] = 0x10;   // fs   // OSDATA
    p->stackPointer[1] = 0x10;   // es   // OSDATA
    p->stackPointer[2] = 0x10;   // ds   // OSDATA
    p->stackPointer[3] = 0;      // edi
    p->stackPointer[4] = 0;      // esi
    p->stackPointer[5] = 0;      // ebp
    p->stackPointer[6] = 0;      // edx
    p->stackPointer[7] = 0;      // ecx
    p->stackPointer[8] = 0;      // ebx
    p->stackPointer[9] = 0;      // eax
    p->stackPointer[10] = (LONG) threadStart;
    p->stackPointer[11] = (LONG) 0x08;   // OSCODE
    p->stackPointer[12] = (LONG) get_current_flags();
    p->stackPointer[13] = 0;
    p->stackPointer[14] = (LONG) p;
    p->stackPointer[15] = (LONG) STACK_END;
    p->next = p->prior = 0;

    p->kernelNext = p->kernelPrior = 0;
    insert_process(p);

    return p;

}

PROCESS *sleepThread(void)
{
    register PROCESS *p = get_running_process();

    if (p->processSignature != PROCESS_SIGNATURE)
       return 0;

    sema_wait(&p->thread_sema);

    return p;

}

LONG rescheduleThread(PROCESS *p)
{
    if (p == get_running_process())
       return -1;

    if (p->processSignature != PROCESS_SIGNATURE)
       return -3;

    sema_post(&p->thread_sema);

    return 0;

}

LONG killThread(PROCESS *p)
{

    if (p->processSignature != PROCESS_SIGNATURE)
       return -3;

    spin_lock(&p->threadMutex);     // lock thread
    switch (p->threadState)
    {
       // need to handle blocked on sema case

       case PS_SLEEP:
	  if (p->delayStamp == PROCESS_DELAYED)
	  {
	     cancelTimer((TIMER *) &p->delayNext);
	     p->delayStamp = PROCESS_AWAKENED;
	  }
	  remove_process(p);
	  put_rundown(p);
	  spin_unlock(&p->threadMutex);
	  break;

       case PS_ACTIVE:
	  if (p->delayStamp == PROCESS_DELAYED)
	  {
	     cancelTimer((TIMER *) &p->delayNext);
	     p->delayStamp = PROCESS_AWAKENED;
	  }
	  p->threadState = PS_HALT;
	  remove_process(p);
	  thread_switch();      // will unlock thread

	  while (p->threadState == PS_HALT)
	     thread_switch();

	  break;

       case PS_SWAPPED:
       case PS_INIT:
       case PS_HALT:
       case PS_SHUTDOWN:
       default:
	  spin_unlock(&p->threadMutex);     // unlock thread
	  break;

    }
    return 1;

}

LONG awakenThread(TIMER *timer)
{
    register PROCESS *p = (PROCESS *) timer->parameter;

    if (p->processSignature != PROCESS_SIGNATURE)
       return -3;

    p->delayStamp = PROCESS_AWAKENED;

    sema_post(&p->thread_sema);

    return 0;

}

LONG delayThread(LONG delay)
{
    register PROCESS *p = get_running_process();

    if (p->processSignature != PROCESS_SIGNATURE)
       return -3;

    p->delayStamp = PROCESS_DELAYED;
    p->delayWaitTime = delay;
    p->delayParameter = p;
    p->delayFunction = (void (*)(void *)) awakenThread;
    scheduleTimer((TIMER *) &p->delayNext);

    sema_wait(&p->thread_sema);

    if (p->delayStamp != PROCESS_AWAKENED)
    {
       printf("delayThread - process %08X was prematurely awakened\n", p);
       return -4;
    }
    return 0;

}

//
//  Light Weight Process and Soft Interrupt Routines
//

LONG scheduleLocalLWP(LWP *lwp)
{
   register LWP *search;
   register LONG proc = get_processor_id();
   register LONG flags;

   if (lwp->lwpState == LWP_ACTIVE)
      return -1;

   flags = get_flags();
   search = processorSet[proc].lwp_queue_head;
   while (search)
   {
      if (search == lwp)
      {
	 set_flags(flags);
	 return 1;
      }
      search = search->next;
   }
   lwp->lwpState = LWP_ACTIVE;
   if (!processorSet[proc].lwp_queue_head)
   {
      processorSet[proc].lwp_queue_head = lwp;
      processorSet[proc].lwp_queue_tail = lwp;
      lwp->next = lwp->prior = 0;
   }
   else
   {
      processorSet[proc].lwp_queue_tail->next = lwp;
      lwp->next = 0;
      lwp->prior = processorSet[proc].lwp_queue_tail;
      processorSet[proc].lwp_queue_tail = lwp;
   }
   set_flags(flags);
   return 0;

}

LONG cancelLocalLWP(LWP *lwp)
{
   register LWP *search;
   register LONG proc = get_processor_id();
   register LONG flags;
   register LONG state;

   if (lwp->lwpState == LWP_INACTIVE)
      return 0;

   flags = get_flags();
   lwp->lwpState = LWP_INACTIVE;
   search = processorSet[proc].lwp_queue_head;
   while (search)
   {
      if (search == lwp)
      {
	 if (processorSet[proc].lwp_queue_head == lwp)
	 {
	    processorSet[proc].lwp_queue_head = (void *) lwp->next;
	    if (processorSet[proc].lwp_queue_head)
	       processorSet[proc].lwp_queue_head->prior = NULL;
	    else
	       processorSet[proc].lwp_queue_tail = NULL;
	 }
	 else
	 {
	    lwp->prior->next = lwp->next;
	    if (lwp != processorSet[proc].lwp_queue_tail)
	       lwp->next->prior = lwp->prior;
	    else
	       processorSet[proc].lwp_queue_tail = lwp->prior;
	 }
	 set_flags(flags);
	 return 1;
      }
      search = search->next;
   }
   set_flags(flags);
   return 0;
}

//
//
//

LONG scheduleBoundLWP(LWP *lwp, LONG proc)
{
   register LWP *search;
   register LONG flags;

   if (lwp->lwpState == LWP_ACTIVE)
      return -1;

   flags = get_flags();
   spin_lock(&processorSet[proc].lwp_bind_queue_mutex);
   search = processorSet[proc].lwp_bind_queue_head;
   while (search)
   {
      if (search == lwp)
      {
	 set_flags(flags);
	 spin_unlock(&processorSet[proc].lwp_bind_queue_mutex);
	 return 1;
      }
      search = search->next;
   }
   lwp->lwpState = LWP_ACTIVE;
   if (!processorSet[proc].lwp_bind_queue_head)
   {
      processorSet[proc].lwp_bind_queue_head = lwp;
      processorSet[proc].lwp_bind_queue_tail = lwp;
      lwp->next = lwp->prior = 0;
   }
   else
   {
      processorSet[proc].lwp_bind_queue_tail->next = lwp;
      lwp->next = 0;
      lwp->prior = processorSet[proc].lwp_bind_queue_tail;
      processorSet[proc].lwp_bind_queue_tail = lwp;
   }
   spin_unlock(&processorSet[proc].lwp_bind_queue_mutex);
   set_flags(flags);
   return 0;

}

LONG cancelBoundLWP(LWP *lwp)
{
   register LWP *search;
   register LONG flags;
   register LONG proc;
   register LONG state;

   if (lwp->lwpState == LWP_INACTIVE)
      return 0;

   flags = get_flags();
   lwp->lwpState = LWP_INACTIVE;
   for (proc=0; proc < active_processors; proc++)
   {
      spin_lock(&processorSet[proc].lwp_bind_queue_mutex);
      search = processorSet[proc].lwp_bind_queue_head;
      while (search)
      {
	 if (search == lwp)
	 {
	    if (processorSet[proc].lwp_bind_queue_head == lwp)
	    {
	       processorSet[proc].lwp_bind_queue_head = (void *) lwp->next;
	       if (processorSet[proc].lwp_bind_queue_head)
		  processorSet[proc].lwp_bind_queue_head->prior = NULL;
	       else
		  processorSet[proc].lwp_bind_queue_tail = NULL;
	    }
	    else
	    {
	       lwp->prior->next = lwp->next;
	       if (lwp != processorSet[proc].lwp_bind_queue_tail)
		  lwp->next->prior = lwp->prior;
	       else
		  processorSet[proc].lwp_bind_queue_tail = lwp->prior;
	    }
	    spin_unlock(&processorSet[proc].lwp_bind_queue_mutex);
	    set_flags(flags);
	    return 1;
	 }
	 search = search->next;
      }
      spin_unlock(&processorSet[proc].lwp_bind_queue_mutex);
   }
   set_flags(flags);
   return 0;
}

//
//
//

LONG scheduleLWP(LWP *lwp)
{
   register LWP *search;
   register LONG flags;
   extern LWP *lwp_dispatch_queue_head;
   extern LWP *lwp_dispatch_queue_tail;
   extern LONG lwp_dispatch_queue_mutex;

   if (lwp->lwpState == LWP_ACTIVE)
      return -1;

   flags = get_flags();
   spin_lock(&lwp_dispatch_queue_mutex);
   search = lwp_dispatch_queue_head;
   while (search)
   {
      if (search == lwp)
      {
	 spin_unlock(&lwp_dispatch_queue_mutex);
	 set_flags(flags);
	 return 0;
      }
      search = search->next;
   }
   lwp->lwpState = LWP_ACTIVE;
   if (!lwp_dispatch_queue_head)
   {
       lwp_dispatch_queue_head = lwp;
       lwp_dispatch_queue_tail = lwp;
       lwp->next = lwp->prior = 0;
   }
   else
   {
       lwp_dispatch_queue_tail->next = lwp;
       lwp->next = 0;
       lwp->prior = lwp_dispatch_queue_tail;
       lwp_dispatch_queue_tail = lwp;
   }
   spin_unlock(&lwp_dispatch_queue_mutex);
   set_flags(flags);
   return 0;
}

LONG cancelLWP(LWP *lwp)
{
   register LWP *search;
   register LONG flags;
   register LONG state;
   extern LONG lwp_dispatch_queue_mutex;
   extern LWP *lwp_dispatch_queue_head;
   extern LWP *lwp_dispatch_queue_tail;

   if (lwp->lwpState == LWP_INACTIVE)
      return 0;

   flags = get_flags();
   spin_lock(&lwp_dispatch_queue_mutex);

   lwp->lwpState = LWP_INACTIVE;
   search = lwp_dispatch_queue_head;
   while (search)
   {
      if (search == lwp)
      {
	 if (lwp_dispatch_queue_head == lwp)
	 {
	    lwp_dispatch_queue_head = (void *) lwp->next;
	    if (lwp_dispatch_queue_head)
	       lwp_dispatch_queue_head->prior = NULL;
	    else
	       lwp_dispatch_queue_tail = NULL;
	 }
	 else
	 {
	    lwp->prior->next = lwp->next;
	    if (lwp != lwp_dispatch_queue_tail)
	       lwp->next->prior = lwp->prior;
	    else
	       lwp_dispatch_queue_tail = lwp->prior;
	 }
	 spin_unlock(&lwp_dispatch_queue_mutex);
	 set_flags(flags);
	 return 1;
      }
      search = search->next;
   }
   spin_unlock(&lwp_dispatch_queue_mutex);
   set_flags(flags);
   return 0;
}

//
//
//

LONG scheduleFastLWP(LWP *lwp)
{
   register LWP *search;
   register LONG proc = get_processor_id();
   register LONG flags;

   if (lwp->lwpState == LWP_ACTIVE)
      return -1;

   flags = get_flags();
   search = processorSet[proc].fast_lwp_head;
   while (search)
   {
      if (search == lwp)
      {
	 set_flags(flags);
	 return 0;
      }
      search = search->next;
   }
   lwp->lwpState = LWP_ACTIVE;
   if (!processorSet[proc].fast_lwp_head)
   {
      processorSet[proc].fast_lwp_head = lwp;
      processorSet[proc].fast_lwp_tail = lwp;
      lwp->next = lwp->prior = 0;
   }
   else
   {
      processorSet[proc].fast_lwp_tail->next = lwp;
      lwp->next = 0;
      lwp->prior = processorSet[proc].fast_lwp_tail;
      processorSet[proc].fast_lwp_tail = lwp;
   }
   set_flags(flags);
   return 0;

}

LONG cancelFastLWP(LWP *lwp)
{
   register LWP *search;
   register LONG proc = get_processor_id();
   register LONG flags;
   register LONG state;

   if (lwp->lwpState == LWP_INACTIVE)
      return 0;

   flags = get_flags();
   lwp->lwpState = LWP_INACTIVE;
   search = processorSet[proc].fast_lwp_head;
   while (search)
   {
      if (search == lwp)
      {
	 if (processorSet[proc].fast_lwp_head == lwp)
	 {
	    processorSet[proc].fast_lwp_head = (void *) lwp->next;
	    if (processorSet[proc].fast_lwp_head)
	       processorSet[proc].fast_lwp_head->prior = NULL;
	    else
	       processorSet[proc].fast_lwp_tail = NULL;
	 }
	 else
	 {
	    lwp->prior->next = lwp->next;
	    if (lwp != processorSet[proc].fast_lwp_tail)
	       lwp->next->prior = lwp->prior;
	    else
	       processorSet[proc].fast_lwp_tail = lwp->prior;
	 }
	 set_flags(flags);
	 return 1;
      }
      search = search->next;
   }
   set_flags(flags);
   return 0;
}

//
//
//

LONG scheduleSoftInt(LWP *lwp)
{
   register LWP *search;
   register LONG flags;
   extern LONG soft_int_lock;
   extern LWP *soft_int_head;
   extern LWP *soft_int_tail;

   if (lwp->lwpState == LWP_ACTIVE)
      return -1;

   flags = get_flags();
   spin_lock(&soft_int_lock);
   search = soft_int_head;
   while (search)
   {
      if (search == lwp)
      {
	 spin_unlock(&soft_int_lock);
	 set_flags(flags);
	 return 0;
      }
      search = search->next;
   }
   lwp->lwpState = LWP_ACTIVE;
   if (!soft_int_head)
   {
      soft_int_head = lwp;
      soft_int_tail = lwp;
      lwp->next = lwp->prior = 0;
   }
   else
   {
      soft_int_tail->next = lwp;
      lwp->next = 0;
      lwp->prior = soft_int_tail;
      soft_int_tail = lwp;
   }
   spin_unlock(&soft_int_lock);
   set_flags(flags);
   return 0;
}

LONG cancelSoftInt(LWP *lwp)
{
   register LWP *search;
   register LONG flags;
   register LONG state;
   extern LONG soft_int_lock;
   extern LWP *soft_int_head;
   extern LWP *soft_int_tail;

   if (lwp->lwpState == LWP_INACTIVE)
      return 0;

   flags = get_flags();
   spin_lock(&soft_int_lock);

   lwp->lwpState = LWP_INACTIVE;
   search = soft_int_head;
   while (search)
   {
      if (search == lwp)
      {
	 if (soft_int_head == lwp)
	 {
	    soft_int_head = (void *) lwp->next;
	    if (soft_int_head)
	       soft_int_head->prior = NULL;
	    else
	       soft_int_tail = NULL;
	 }
	 else
	 {
	    lwp->prior->next = lwp->next;
	    if (lwp != soft_int_tail)
	       lwp->next->prior = lwp->prior;
	    else
	       soft_int_tail = lwp->prior;
	 }
	 spin_unlock(&soft_int_lock);
	 set_flags(flags);
	 return 1;
      }
      search = search->next;
   }
   spin_unlock(&soft_int_lock);
   set_flags(flags);
   return 0;
}

//
//  thread startup/cleaup routines
//

void threadCleanup(PROCESS *p)
{
   if (p->processSignature != PROCESS_SIGNATURE)
      return;

   spin_lock(&p->threadMutex);     // lock current thread
   p->threadState = PS_HALT;
   remove_process(p);
   thread_switch();                // context_switch will unlock

}

void threadStart(PROCESS *p)
{
    PROCESS *process = p;
    (process->startAddress)(process->startParm);
    threadCleanup(process);
}


//
//  list and process link routines
//

PROCESS *get_free_process(void)
{
    register LONG flags;
    register PROCESS *p = 0;

    if (pFreeListHead)
    {
       flags = get_flags();
       spin_lock(&pFreeListMutex);
       if (pFreeListHead)
       {
	  p = pFreeListHead;
	  pFreeListHead = (void *) p->next;
	  if (pFreeListHead)
	     pFreeListHead->prior = NULL;
	  else
	     pFreeListTail = NULL;
       }
       spin_unlock(&pFreeListMutex);
       set_flags(flags);
    }
    return p;

}

void put_free_process(PROCESS *p)
{

    register LONG flags;

    flags = get_flags();
    spin_lock(&pFreeListMutex);

    p->processID = 0;
    p->typeID = KERNEL_THREAD;
    p->resourceFlag = KERNEL_MEMORY;
    p->threadState = PS_SHUTDOWN;
    p->processSignature = 0;
    p->stackPointer = 0;
    p->stackLimit = 0;
    p->stackEnd = 0;

    if (!pFreeListHead)
    {
       pFreeListHead = p;
       pFreeListTail = p;
       p->next = p->prior = 0;
    }
    else
    {
       pFreeListTail->next = p;
       p->next = 0;
       p->prior = pFreeListTail;
       pFreeListTail = p;
    }
    spin_unlock(&pFreeListMutex);
    set_flags(flags);
    return;

}

void remove_process(PROCESS *p)
{

    register PROCESS *process;
    register LONG flags;

    flags = get_flags();
    spin_lock(&systemThreadmutex);
    process = systemThreadHead;
    while (process)
    {
       if (process == p)
       {
	  if (systemThreadHead == p)
	  {
	     systemThreadHead = (void *) p->kernelNext;
	     if (systemThreadHead)
		systemThreadHead->kernelPrior = NULL;
	     else
		systemThreadTail = NULL;
	  }
	  else
	  {
	     p->kernelPrior->kernelNext = p->kernelNext;
	     if (p != systemThreadTail)
		p->kernelNext->kernelPrior = p->kernelPrior;
	     else
		systemThreadTail = p->kernelPrior;
	  }
       }
       process = process->kernelNext;
    }
    if (systemThreadCount)
       systemThreadCount--;
    spin_unlock(&systemThreadmutex);
    set_flags(flags);

    EventNotify(EVENT_DESTROY_THREAD, (LONG) p);

    return;

}

void insert_process(PROCESS *p)
{
    register PROCESS *process;
    register LONG flags;

    EventNotify(EVENT_CREATE_THREAD, (LONG) p);

    flags = get_flags();
    spin_lock(&systemThreadmutex);
    process = systemThreadHead;
    while (process)
    {
       if (process == p)
       {
	  spin_unlock(&systemThreadmutex);
	  set_flags(flags);
	  return;
       }
       process = process->kernelNext;
    }
    if (!systemThreadHead)
    {
       systemThreadHead = p;
       systemThreadTail = p;
       p->kernelNext = p->kernelPrior = 0;
    }
    else
    {
       systemThreadTail->kernelNext = p;
       p->kernelNext = 0;
       p->kernelPrior = systemThreadTail;
       systemThreadTail = p;
    }
    systemThreadCount++;
    spin_unlock(&systemThreadmutex);
    set_flags(flags);
    return;

}



