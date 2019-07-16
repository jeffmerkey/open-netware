
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  MUTEX.C
*   DESCRIP  :  Multi-Processing Mutual Exclusion Locks MANOS v1.0
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

mutex_t *mutex_init(mutex_t *mutex, LONG value)
{

   if (mutex && !mutex->signature)
   {
      SetData((LONG *) mutex, 0, sizeof(mutex_t));
      mutex->value = value;
      mutex->signature = LMUTEX_SIGNATURE;
      return (mutex_t *) mutex;
   }
   return 0;

}

mutex_t *mutex_alloc(LONG value)
{

   register mutex_t *mutex;

   mutex = kmalloc(sizeof(mutex_t));
   if (mutex)
   {
      SetData((LONG *) mutex, 0, sizeof(mutex_t));
      mutex->value = value;
      mutex->signature = MUTEX_SIGNATURE;
      return (mutex_t *) mutex;
   }
   return 0;

}

LONG mutex_free(mutex_t *mutex)
{
    if (mutex->signature == MUTEX_SIGNATURE || mutex->signature == LMUTEX_SIGNATURE)
    {
       if (!mutex_release(mutex))
       {
	  if (mutex->signature == MUTEX_SIGNATURE)
	     kfree(mutex);
	  else
	     mutex->signature = 0;
	  return 0;
       }
       return -2;
    }
    return -1;

}

LONG mutex_lock(mutex_t *mutex)
{
    register PROCESS *p = get_running_process();
    register LONG flags;

    if (mutex->signature == MUTEX_SIGNATURE || mutex->signature == LMUTEX_SIGNATURE)
    {
       flags = get_flags();
       p->syncFlag = 0;

MutexTryLock:
       if (spin_try_lock(&mutex->value))
       {
	  spin_lock(&mutex->mutex);
	  if (!mutex->value)
	  {
	     spin_unlock(&mutex->mutex);
	     goto MutexTryLock;
	  }
	  spin_lock(&p->threadMutex);    // lock current thread
	  put_mutex_process(mutex, p);
	  p->stackPointer = 0;
	  spin_unlock(&mutex->mutex);

	  p->threadState = PS_SLEEP;
	  p->syncObject = mutex;
	  p->syncState = PROCESS_BLOCKED_SYNC;
	  thread_switch();               // context switch will unlock thread
	  cli();
       }
       mutex->owner = (LONG) p;
       set_flags(flags);
       return p->syncFlag;
    }
    return -1;

}

LONG mutex_unlock(mutex_t *mutex)
{
    register PROCESS *p;
    register LONG flags;

    if (mutex->signature == MUTEX_SIGNATURE || mutex->signature == LMUTEX_SIGNATURE)
    {
       flags = get_flags();
       spin_lock(&mutex->mutex);
       if (mutex->head)
       {
	  p = get_mutex_process(mutex);
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
	  mutex->owner = mutex->value = 0;  // no waiters, clear lock and owners
       spin_unlock(&mutex->mutex);
       set_flags(flags);
       return 0;
    }
    return -1;

}

PROCESS *get_mutex_process(mutex_t *mutex)
{

    register PROCESS *p, *list;

    if (mutex->head)
    {
       p = mutex->head;
       list = (PROCESS *) mutex->head = (void *) p->syncNext;
       if (list)
	  list->syncPrior = NULL;
       else
	  mutex->tail = NULL;
       return p;
    }
    return 0;

}

void put_mutex_process(mutex_t *mutex, PROCESS *p)
{

    register PROCESS *search, *list;

    search = mutex->head;
    while (search)
    {
       if (search == p)
	  return;
       search = search->syncNext;
    }

    if (!mutex->head)
    {
       mutex->head = mutex->tail = p;
       p->syncNext = p->syncPrior = 0;
    }
    else
    {
       list = (PROCESS *) mutex->tail;
       list->syncNext = p;
       p->syncNext = 0;
       p->syncPrior = mutex->tail;
       mutex->tail = p;
    }
    return;

}

LONG mutex_release(mutex_t *mutex)
{
    register PROCESS *p, *list;
    register LONG flags;

    if (mutex->signature == MUTEX_SIGNATURE ||
	mutex->signature == LMUTEX_SIGNATURE)
    {
       flags = get_flags();
       spin_lock(&mutex->mutex);
       p = mutex->head;
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
	     if (mutex->waiters)
		mutex->waiters--;
	     (!p->processorBinding)
	     ? put_dispatch(p)
	     : put_bind_pset(p, (PROCESSOR_SET *)p->processorBinding);
	  }
	  spin_unlock(&p->threadMutex);   // unlock thread
	  p = list;
       }
       mutex->value = mutex->owner = 0;
       spin_unlock(&mutex->mutex);
       set_flags(flags);
       return 0;
    }
    return -1;

}
