
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
*   FILE     :  ALTDEBUG.C
*   DESCRIP  :  Alternate Debugger Interface for MANOS v1.0
*   DATE     :  July 27, 1998
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
#include "altdebug.h"

LONG altDebugLock = 0;
ALT_DEBUGGER *altDebugHead = 0;
ALT_DEBUGGER *altDebugTail = 0;

LONG AlternateDebuggerRoutine(SCREEN *screen, StackFrame *stackFrame,
			      LONG Exception, LONG processor)
{
    register ALT_DEBUGGER *altDebug;
    register LONG flags, retCode;

    flags = get_flags();
    spin_lock(&altDebugLock);
    altDebug = altDebugHead;
    while (altDebug)
    {
       if (altDebug->AlternateDebugger)
       {
	  retCode = (altDebug->AlternateDebugger)(screen, stackFrame, Exception, processor);
	  if (retCode)
	  {
	     spin_unlock(&altDebugLock);
	     set_flags(flags);
	     return retCode;
	  }
       }
       altDebug = altDebug->altDebugNext;
    }
    spin_unlock(&altDebugLock);
    set_flags(flags);
    return 0;
}

LONG AddAlternateDebugger(ALT_DEBUGGER *Debugger)
{
    register ALT_DEBUGGER *altDebug;
    register LONG flags;

    flags = get_flags();
    spin_lock(&altDebugLock);
    altDebug = altDebugHead;
    while (altDebug)
    {
       if (altDebug == Debugger)
       {
	  spin_unlock(&altDebugLock);  // already exists
	  set_flags(flags);
	  return 1;
       }
       altDebug = altDebug->altDebugNext;
    }
    if (!altDebugHead)
    {
       altDebugHead = Debugger;
       altDebugTail = Debugger;
       Debugger->altDebugNext = 0;
       Debugger->altDebugPrior = 0;
    }
    else
    {
       altDebugTail->altDebugNext = Debugger;
       Debugger->altDebugNext = 0;
       Debugger->altDebugPrior = altDebugTail;
       altDebugTail = Debugger;
    }
    spin_unlock(&altDebugLock);
    set_flags(flags);
    return 0;
}

LONG RemoveAlternateDebugger(ALT_DEBUGGER *Debugger)
{
    register ALT_DEBUGGER *altDebug;
    register LONG flags;

    flags = get_flags();
    spin_lock(&altDebugLock);
    altDebug = altDebugHead;
    while (altDebug)
    {
       if (altDebug == Debugger)   // found, remove from list
       {
	  if (altDebugHead == Debugger)
	  {
	     altDebugHead = (void *) Debugger->altDebugNext;
	     if (altDebugHead)
		altDebugHead->altDebugPrior = NULL;
	     else
		altDebugTail = NULL;
	  }
	  else
	  {
	     Debugger->altDebugPrior->altDebugNext = Debugger->altDebugNext;
	     if (Debugger != altDebugTail)
		Debugger->altDebugNext->altDebugPrior = Debugger->altDebugPrior;
	     else
		altDebugTail = Debugger->altDebugPrior;
	  }
	  Debugger->altDebugNext = Debugger->altDebugPrior = 0;
	  spin_unlock(&altDebugLock);
	  set_flags(flags);
	  return 0;
       }
       altDebug = altDebug->altDebugNext;
    }
    spin_unlock(&altDebugLock);
    set_flags(flags);
    return -1;
}




