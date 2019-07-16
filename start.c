
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  START.C
*   DESCRIP  :  Installable Startup Routine System for MANOS v1.0
*   DATE     :  July 28, 1998
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
#include "start.h"

LONG startupLock = 0;
STARTUP_NODE *startupHead = 0;
STARTUP_NODE *startupTail = 0;

LONG SystemStartupRoutine(DOS_TABLE *dos, PM_TABLE *pm)
{
    register STARTUP_NODE *startup;
    register LONG flags, retCode;

    flags = get_flags();
    spin_lock(&startupLock);
    startup = startupHead;
    while (startup)
    {
       if (startup->startupRoutine)
	  (startup->startupRoutine)(startup, dos, pm);
       cli();
       startup = startup->startupNext;
    }
    spin_unlock(&startupLock);
    set_flags(flags);
    return 0;
}

LONG AddStartupRoutine(STARTUP_NODE *startup_node)
{
    register STARTUP_NODE *startup;
    register LONG flags;

    flags = get_flags();
    spin_lock(&startupLock);
    startup = startupHead;
    while (startup)
    {
       if (startup == startup_node)
       {
	  spin_unlock(&startupLock);  // already exists
	  set_flags(flags);
	  return 1;
       }
       startup = startup->startupNext;
    }
    if (!startupHead)
    {
       startupHead = startup_node;
       startupTail = startup_node;
       startup_node->startupNext = 0;
       startup_node->startupPrior = 0;
    }
    else
    {
       startupTail->startupNext = startup_node;
       startup_node->startupNext = 0;
       startup_node->startupPrior = startupTail;
       startupTail = startup_node;
    }
    spin_unlock(&startupLock);
    set_flags(flags);
    return 0;
}

LONG RemoveStartupRoutine(STARTUP_NODE *startup_node)
{
    register STARTUP_NODE *startup;
    register LONG flags;

    flags = get_flags();
    spin_lock(&startupLock);
    startup = startupHead;
    while (startup)
    {
       if (startup == startup_node)   // found, remove from list
       {
	  if (startupHead == startup_node)
	  {
	     startupHead = (void *) startup_node->startupNext;
	     if (startupHead)
		startupHead->startupPrior = NULL;
	     else
		startupTail = NULL;
	  }
	  else
	  {
	     startup_node->startupPrior->startupNext = startup_node->startupNext;
	     if (startup_node != startupTail)
		startup_node->startupNext->startupPrior = startup_node->startupPrior;
	     else
		startupTail = startup_node->startupPrior;
	  }
	  startup_node->startupNext = startup_node->startupPrior = 0;
	  spin_unlock(&startupLock);
	  set_flags(flags);
	  return 0;
       }
       startup = startup->startupNext;
    }
    spin_unlock(&startupLock);
    set_flags(flags);
    return -1;
}




