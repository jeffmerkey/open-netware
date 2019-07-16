

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  SEMA.C
*   DESCRIP  :  Multi-Processing Semaphores for MANOS v1.0
*   DATE     :  January 19, 1998
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

sema_t *sema_init(sema_t *sema, LONG value)
{

   if (sema && !sema->sema_signature)
   {
      SetData((LONG *) sema, 0, sizeof(sema_t));
      sema->sema_value = value;
      sema->sema_signature = LSEMA_SIGNATURE;
      return (sema_t *) sema;
   }
   return 0;

}

sema_t *sema_alloc(LONG value)
{

   register sema_t *sema;

   sema = kmalloc(sizeof(sema_t));
   if (sema)
   {
      SetData((LONG *) sema, 0, sizeof(sema_t));
      sema->sema_value = value;
      sema->sema_signature = SEMA_SIGNATURE;
      return (sema_t *) sema;
   }
   return 0;

}

LONG sema_free(sema_t *sema)
{
    if (sema->sema_signature == SEMA_SIGNATURE ||
	sema->sema_signature == LSEMA_SIGNATURE)
    {
       if (!sema_release(sema))
       {
	  if (sema->sema_signature == SEMA_SIGNATURE)
	     kfree(sema);
	  else
	     sema->sema_signature = 0;
	  return 0;
       }
       return -2;
    }
    return -1;

}

void sema_timer_event(TIMER *timer)
{

    register sema_t *sema;
    register PROCESS *p;
    register LONG flags;

    sema = (sema_t *) timer->parameter;

    if (sema->sema_signature == SEMA_SIGNATURE ||
	sema->sema_signature == LSEMA_SIGNATURE)
    {
       flags = get_flags();
       spin_lock(&sema->sema_mutex);
       sema->sema_value = 0;
       if (sema->sema_head)
       {
	  p = get_sema_process(sema);
	  if (p)
	  {
	     spin_lock(&p->threadMutex);     // lock thread
	     if (p->threadState == PS_SLEEP)
	     {
		p->threadState = PS_ACTIVE;
		p->syncObject = 0;
		p->syncState = 0;
		p->syncFlag = 0;
		if (sema->sema_waiters)
		   sema->sema_waiters--;
		(!p->processorBinding)
		? put_dispatch(p)
		: put_bind_pset(p, (PROCESSOR_SET *)p->processorBinding);
	     }
	     spin_unlock(&p->threadMutex);   // unlock thread
	  }
       }
       spin_unlock(&sema->sema_mutex);
       set_flags(flags);
       return;
    }
    return;
}

LONG sema_wait(sema_t *sema)
{

    register PROCESS *p = get_running_process();
    register LONG flags;

    if (sema->sema_signature == SEMA_SIGNATURE ||
	sema->sema_signature == LSEMA_SIGNATURE)
    {
       flags = get_flags();
       spin_lock(&sema->sema_mutex);
       sema->sema_value--;
       p->syncFlag = 0;
       if (sema->sema_value < 0)
       {
	  spin_lock(&p->threadMutex);    // lock current thread
	  put_sema_process(sema, p);
	  spin_unlock(&sema->sema_mutex);
	  p->stackPointer = 0;
	  p->threadState = PS_SLEEP;
	  p->syncObject = sema;
	  p->syncState = PROCESS_BLOCKED_SYNC;
	  sema->sema_waiters++;
	  thread_switch();               // context switch will unlock
	  cli();
       }
       else
	  spin_unlock(&sema->sema_mutex);

       set_flags(flags);
       return p->syncFlag;
    }
    return -1;

}

LONG sema_timed_wait(sema_t *sema, LONG delay)
{

    register PROCESS *p = get_running_process();
    register LONG retCode;
    register LONG flags;
    TIMER timer;

    if (sema->sema_signature == SEMA_SIGNATURE ||
	sema->sema_signature == LSEMA_SIGNATURE)
    {
       timer.waitTime = delay;
       timer.parameter = sema;
       timer.timerFunction = sema_timer_event;

       flags = get_flags();
       spin_lock(&sema->sema_mutex);
       sema->sema_value--;
       p->syncFlag = 0;
       if (sema->sema_value < 0)
       {
	  spin_lock(&p->threadMutex);    // lock current thread
	  put_sema_process(sema, p);
	  scheduleTimer(&timer);
	  spin_unlock(&sema->sema_mutex);
	  p->stackPointer = 0;
	  p->threadState = PS_SLEEP;
	  p->syncObject = sema;
	  p->syncState = PROCESS_BLOCKED_SYNC;
	  sema->sema_waiters++;
	  thread_switch();               // context switch will unlock
	  cli();
	  cancelTimer(&timer);
	  set_flags(flags);
	  return p->syncFlag;
       }
       else
       {
	  spin_unlock(&sema->sema_mutex);
	  set_flags(flags);
	  return p->syncFlag;
       }
    }
    return -1;

}

LONG sema_post(sema_t *sema)
{

    register PROCESS *p;
    register LONG flags;

    if (sema->sema_signature == SEMA_SIGNATURE ||
	sema->sema_signature == LSEMA_SIGNATURE)
    {
       flags = get_flags();
       spin_lock(&sema->sema_mutex);
       sema->sema_value++;
       if (sema->sema_value >= 0 && sema->sema_head)
       {
	  p = get_sema_process(sema);
	  if (p)
	  {
	     spin_lock(&p->threadMutex);     // lock thread
	     if (p->threadState == PS_SLEEP)
	     {
		p->threadState = PS_ACTIVE;
		p->syncObject = 0;
		p->syncState = 0;
		p->syncFlag = 0;
		if (sema->sema_waiters)
		   sema->sema_waiters--;
		(!p->processorBinding)
		? put_dispatch(p)
		: put_bind_pset(p, (PROCESSOR_SET *)p->processorBinding);
	     }
	     spin_unlock(&p->threadMutex);   // unlock thread
	   }
       }
       spin_unlock(&sema->sema_mutex);
       set_flags(flags);
       return 0;
    }
    return -1;

}


LONG sema_release(sema_t *sema)
{
    register PROCESS *p, *list;
    register LONG flags;

    if (sema->sema_signature == SEMA_SIGNATURE ||
	sema->sema_signature == LSEMA_SIGNATURE)
    {
       flags = get_flags();
       spin_lock(&sema->sema_mutex);
       p = sema->sema_head;
       while (p)
       {
	  list = p->syncNext;
	  spin_lock(&p->threadMutex);     // lock thread
	  if (p->threadState == PS_SLEEP)
	  {
	     p->threadState = PS_ACTIVE;
	     p->syncObject = 0;
	     p->syncState = 0;
	     p->syncFlag = -2;
	     if (sema->sema_waiters)
		sema->sema_waiters--;
	     (!p->processorBinding)
	     ? put_dispatch(p)
	     : put_bind_pset(p, (PROCESSOR_SET *)p->processorBinding);
	  }
	  spin_unlock(&p->threadMutex);   // unlock thread
	  p = list;
       }
       sema->sema_value = 0;
       spin_unlock(&sema->sema_mutex);
       set_flags(flags);
       return 0;
    }
    return -1;

}


PROCESS *get_sema_process(sema_t *sema)
{

    register PROCESS *p, *list;

    if (sema->sema_head)
    {
       p = sema->sema_head;
       list = (PROCESS *) sema->sema_head = (void *) p->syncNext;
       if (list)
	  list->syncPrior = NULL;
       else
	  sema->sema_tail = NULL;
       return p;
    }
    return 0;

}

void put_sema_process(sema_t *sema, PROCESS *p)
{

    register PROCESS *search, *list;

    search = sema->sema_head;
    while (search)
    {
       if (search == p)
	  return;
       search = search->syncNext;
    }
    if (!sema->sema_head)
    {
       sema->sema_head = sema->sema_tail = p;
       p->syncNext = p->syncPrior = 0;
    }
    else
    {
       list = (PROCESS *) sema->sema_tail;
       list->syncNext = p;
       p->syncNext = 0;
       p->syncPrior = sema->sema_tail;
       sema->sema_tail = p;
    }
    return;

}

