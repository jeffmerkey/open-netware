
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  RMUTEX.C
*   DESCRIP  :  Multi-Processing Recursive Mutual Exclusion Locks MANOS v1.0
*   DATE     :  September 7, 1998
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

rmutex_t *rmutex_init(rmutex_t *rmutex, LONG value)
{

   if (rmutex && !rmutex->signature)
   {
      SetData((LONG *) rmutex, 0, sizeof(rmutex_t));
      rmutex->value = value;
      rmutex->signature = LMUTEX_SIGNATURE;
      return (rmutex_t *) rmutex;
   }
   return 0;

}

rmutex_t *rmutex_alloc(LONG value)
{

   register rmutex_t *rmutex;

   rmutex = kmalloc(sizeof(rmutex_t));
   if (rmutex)
   {
      SetData((LONG *) rmutex, 0, sizeof(rmutex_t));
      rmutex->value = value;
      rmutex->signature = MUTEX_SIGNATURE;
      return (rmutex_t *) rmutex;
   }
   return 0;

}

LONG rmutex_free(rmutex_t *rmutex)
{
    if (rmutex->signature == MUTEX_SIGNATURE || rmutex->signature == LMUTEX_SIGNATURE)
    {
       if (!rmutex_release(rmutex))
       {
	  if (rmutex->signature == MUTEX_SIGNATURE)
	     kfree(rmutex);
	  else
	     rmutex->signature = 0;
	  return 0;
       }
       return -2;
    }
    return -1;

}

LONG rmutex_lock(rmutex_t *rmutex)
{
    register PROCESS *p = get_running_process();
    register LONG flags;

    if (rmutex->signature == MUTEX_SIGNATURE || rmutex->signature == LMUTEX_SIGNATURE)
    {
       flags = get_flags();
       p->syncFlag = 0;
       spin_lock(&rmutex->mutex);
       if (rmutex->owner == (LONG) p)
       {
	  rmutex->owner_count++;
	  spin_unlock(&rmutex->mutex);
	  set_flags(flags);
	  return p->syncFlag;
       }
       if (spin_try_lock(&rmutex->value))
       {
	  spin_lock(&p->threadMutex);    // lock current thread
	  put_rmutex_process(rmutex, p);
	  spin_unlock(&rmutex->mutex);
	  p->stackPointer = 0;
	  p->threadState = PS_SLEEP;
	  p->syncObject = rmutex;
	  p->syncState = PROCESS_BLOCKED_SYNC;
	  thread_switch();               // context switch will unlock thread
	  cli();
	  rmutex->owner = (LONG) p;
	  set_flags(flags);
	  return p->syncFlag;
       }
       rmutex->owner = (LONG) p;
       spin_unlock(&rmutex->mutex);
       set_flags(flags);
       return p->syncFlag;
    }
    return -1;

}

LONG rmutex_unlock(rmutex_t *rmutex)
{
    register PROCESS *p = get_running_process();
    register LONG flags;

    if (rmutex->signature == MUTEX_SIGNATURE || rmutex->signature == LMUTEX_SIGNATURE)
    {
       flags = get_flags();
       spin_lock(&rmutex->mutex);
       if (rmutex->owner == (LONG) p && rmutex->owner_count)
       {
	  rmutex->owner_count--;
	  spin_unlock(&rmutex->mutex);
	  set_flags(flags);
	  return 0;
       }
       if (rmutex->head)
       {
	  p = get_rmutex_process(rmutex);
	  if (p)
	  {
	     spin_lock(&p->threadMutex);     // lock thread
	     if (p->threadState == PS_SLEEP)
	     {
		p->threadState = PS_ACTIVE;
		p->syncObject = 0;
		p->syncState = 0;
		p->syncFlag = 0;
		(!p->processorBinding)
		? put_dispatch(p)
		: put_bind_pset(p, (PROCESSOR_SET *)p->processorBinding);
	     }
	     spin_unlock(&p->threadMutex);   // unlock thread
	   }
       }
       else
	  rmutex->owner = rmutex->value = 0;  // no waiters, clear lock and owners
       spin_unlock(&rmutex->mutex);
       set_flags(flags);
       return 0;
    }
    return -1;

}

PROCESS *get_rmutex_process(rmutex_t *rmutex)
{

    register PROCESS *p, *list;

    if (rmutex->head)
    {
       p = rmutex->head;
       list = (PROCESS *) rmutex->head = (void *) p->syncNext;
       if (list)
	  list->syncPrior = NULL;
       else
	  rmutex->tail = NULL;
       return p;
    }
    return 0;

}

void put_rmutex_process(rmutex_t *rmutex, PROCESS *p)
{

    register PROCESS *search, *list;

    search = rmutex->head;
    while (search)
    {
       if (search == p)
	  return;
       search = search->syncNext;
    }

    if (!rmutex->head)
    {
       rmutex->head = rmutex->tail = p;
       p->syncNext = p->syncPrior = 0;
    }
    else
    {
       list = (PROCESS *) rmutex->tail;
       list->syncNext = p;
       p->syncNext = 0;
       p->syncPrior = rmutex->tail;
       rmutex->tail = p;
    }
    return;

}

LONG rmutex_release(rmutex_t *rmutex)
{
    register PROCESS *p, *list;
    register LONG flags;

    if (rmutex->signature == MUTEX_SIGNATURE ||
	rmutex->signature == LMUTEX_SIGNATURE)
    {
       flags = get_flags();
       spin_lock(&rmutex->mutex);
       p = rmutex->head;
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
	     if (rmutex->waiters)
		rmutex->waiters--;
	     (!p->processorBinding)
	     ? put_dispatch(p)
	     : put_bind_pset(p, (PROCESSOR_SET *)p->processorBinding);
	  }
	  spin_unlock(&p->threadMutex);   // unlock thread
	  p = list;
       }
       rmutex->value = rmutex->owner = 0;
       spin_unlock(&rmutex->mutex);
       set_flags(flags);
       return 0;
    }
    return -1;

}
