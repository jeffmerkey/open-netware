

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
*   FILE     :  CTIMER.C
*   DESCRIP  :  Multi-Processing Timer for MANOS v1.0
*   DATE     :  January 10, 1998
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
#include "timer.h"

TIMER *timerHead = 0;
TIMER *timerTail = 0;
LONG timerMutex = 0;

void timerProcedure(void)
{
    register LONG flags;
    register TIMER *timerList, *timer;

    flags = get_flags();

    spin_lock(&timerMutex);
    timerList = timerHead;
    timerHead = timerTail = 0;
    spin_unlock(&timerMutex);

    while (timerList)
    {
       timer = timerList;
       timerList = timerList->next;
       timer->next = timer->prior = 0;
       if (timer->timerSignature == TIMER_SIGNATURE)
       {
	  if (timer->currTime++ > timer->waitTime)
	  {
	     timer->timerSignature = 0;
	     if (timer->timerFunction)
	     {
		(timer->timerFunction)(timer);
		cli(); // if someone turns on interrupts, turn them off
	     }
	  }
	  else
	  {
	     timer->timerSignature = TIMER_SIGNATURE;
	     if (!timerHead)
	     {
		timerHead = timer;
		timerTail = timer;
		timer->next = 0;
		timer->prior = 0;
	     }
	     else
	     {
		timerTail->next = timer;
		timer->next = 0;
		timer->prior = timerTail;
		timerTail = timer;
	     }
	  }
       }
    }
    set_flags(flags);
    return;

}

LONG scheduleTimer(TIMER *timer)
{
    register LONG flags;
    register TIMER *timerSearch;

    if (timer->timerSignature == TIMER_SIGNATURE)
    {
       timer->currTime = 0;
       return -1;
    }
    timer->currTime = 0;

    flags = get_flags();
    spin_lock(&timerMutex);
    timerSearch = timerHead;
    while (timerSearch)
    {
       if (timerSearch == timer)  // already scheduled, return error.
       {
	  spin_unlock(&timerMutex);
	  set_flags(flags);
	  return 1;
       }
       timerSearch = timerSearch->next;
    }
    timer->timerSignature = TIMER_SIGNATURE;
    if (!timerHead)
    {
       timerHead = timer;
       timerTail = timer;
       timer->next = 0;
       timer->prior = 0;
    }
    else
    {
       timerTail->next = timer;
       timer->next = 0;
       timer->prior = timerTail;
       timerTail = timer;
    }
    spin_unlock(&timerMutex);
    set_flags(flags);
    return 0;

}


LONG cancelTimer(TIMER *timer)
{
    register LONG flags;
    register TIMER *timerSearch;

    if (timer->timerSignature != TIMER_SIGNATURE)
      return -1;

    flags = get_flags();
    spin_lock(&timerMutex);
    timerSearch = timerHead;
    while (timerSearch)
    {
       if (timerSearch == timer)   // if its on the list, then cancel
       {
	  if (timerHead == timer)
	  {
	     timerHead = (void *) timer->next;
	     if (timerHead)
		timerHead->prior = NULL;
	     else
		timerTail = NULL;
	  }
	  else
	  {
	     timer->prior->next = timer->next;
	     if (timer != timerTail)
		timer->next->prior = timer->prior;
	     else
		timerTail = timer->prior;
	  }
	  timer->next = timer->prior = 0;
	  timer->timerSignature = 0;
	  timer->currTime = 0;
	  spin_unlock(&timerMutex);
	  set_flags(flags);
	  return 0;
       }
       timerSearch = timerSearch->next;
    }
    spin_unlock(&timerMutex);
    set_flags(flags);
    return -1;

}


