
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
*   FILE     :  ACCEL.C
*   DESCRIP  :  Screen Keystroke Accelerator for MANOS v1.0
*   DATE     :  August 11, 1998
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

LONG AccelRoutine(SCREEN *screen, LONG key, void *p)
{
    register ACCELERATOR *accel;
    register LONG flags, retCode;

    flags = get_flags();
    spin_lock(&screen->accelLock);
    accel = screen->accelHead;
    while (accel)
    {
       if (accel->accelFlags && accel->accelRoutine &&
	   accel->key == key)
       {
	  retCode = (accel->accelRoutine)(screen, key, p, accel);
	  if (retCode)
	  {
	     spin_unlock(&screen->accelLock);
	     set_flags(flags);
	     return retCode;
	  }
	  cli();
       }
       accel = accel->accelNext;
    }
    spin_unlock(&screen->accelLock);
    set_flags(flags);
    return 0;
}

void ShowDebuggerAccelerators(SCREEN *screen)
{
   register ACCELERATOR *accel;

   if (printfScreenWithAttribute(screen, BRITEWHITE, "\nDebugger Accelerator(s)\n"))
      return;
   accel = debugScreen->accelHead;
   while (accel)
   {
      if (accel->shortHelp)
	 if (printfScreenWithAttribute(screen, LTCYAN, "%s\n", accel->shortHelp))
	    break;
      accel = accel->accelNext;
   }
   return;
}

LONG IsAccelerator(SCREEN *screen, LONG key)
{
    register ACCELERATOR *accel;
    register LONG flags, retCode;

    if (screen) {};

    flags = get_flags();
    spin_lock(&screen->accelLock);
    accel = screen->accelHead;
    while (accel)
    {
       if (accel->accelFlags && accel->accelRoutine &&
	   accel->key == key)
       {
	  spin_unlock(&screen->accelLock);
	  set_flags(flags);
	  return 1;
       }
       accel = accel->accelNext;
    }
    spin_unlock(&screen->accelLock);
    set_flags(flags);
    return 0;
}

LONG AccelHelpRoutine(SCREEN *screen, LONG key)
{
    register ACCELERATOR *accel;
    register LONG retCode;

    SetPauseMode(screen, screen->nLines - 3);
    accel = screen->accelHead;
    if (key)  // if we were passed a command string
    {
       while (accel)
       {
	  if (accel->accelFlags && accel->key == key)
	  {
	     if (accel->accelRoutineHelp)
	     {
		printfScreen(screen, "Accelerator %08X\n", accel->key);
		(accel->accelRoutineHelp)(screen, key, accel);
		ClearPauseMode(screen);
		return 1;
	     }
	     printfScreen(screen, "Accelerator %08X\n", accel->key);
	     ClearPauseMode(screen);
	     return 1;
	  }
	  accel = accel->accelNext;
       }
       printfScreen(screen, "Help for Accelerator [%08X] not found\n", key);
       ClearPauseMode(screen);
       return 1;
    }
    else
    {
       printfScreenWithAttribute(screen, WHITE, "Accelerator(s)\n");
       while (accel)
       {
	  if (accel->accelFlags && accel->key && !accel->supervisorCommand)
	     if (printfScreen(screen, "%08X         - %55s\n",
			  accel->key, accel->shortHelp))
	     {
		ClearPauseMode(screen);
		return 1;
	     }
	  accel = accel->accelNext;
       }
    }
    ClearPauseMode(screen);
    return 0;
}

ACCELERATOR *insertAccel(SCREEN *screen, ACCELERATOR *i, ACCELERATOR *top)
{
    ACCELERATOR *old, *p;

    if (!screen->accelTail)
    {
       i->accelNext = i->accelPrior = NULL;
       screen->accelTail = i;
       return i;
    }
    p = top;
    old = NULL;
    while (p)
    {
       if (p->key < i->key)
       {
	  old = p;
	  p = p->accelNext;
       }
       else
       {
	  if (p->accelPrior)
	  {
	     p->accelPrior->accelNext = i;
	     i->accelNext = p;
	     i->accelPrior = p->accelPrior;
	     p->accelPrior = i;
	     return top;
	  }
	  i->accelNext = p;
	  i->accelPrior = NULL;
	  p->accelPrior = i;
	  return i;
       }
    }
    old->accelNext = i;
    i->accelNext = NULL;
    i->accelPrior = old;
    screen->accelTail = i;
    return screen->accelHead;

}

LONG AddAccelRoutine(SCREEN *screen, ACCELERATOR *newAccel)
{
    register MODULE_HANDLE *module = newAccel->moduleContext;
    register ACCELERATOR *accel;
    register LONG flags;

    if (module->ModuleSignature != MODULE_SIGNATURE)
       return -3;

    flags = get_flags();
    spin_lock(&screen->accelLock);
    accel = screen->accelHead;
    while (accel)
    {
       if (accel == newAccel || accel->key == newAccel->key)
       {
	  spin_unlock(&screen->accelLock);  // already exists or command
	  set_flags(flags);                 // is already taken
	  return 1;
       }
       accel = accel->accelNext;
    }
    newAccel->accelFlags = -1;
    screen->accelHead = insertAccel(screen, newAccel, screen->accelHead);
    spin_unlock(&screen->accelLock);
    set_flags(flags);
    return 0;
}

LONG RemoveAccelRoutine(SCREEN *screen, ACCELERATOR *newAccel)
{
    register ACCELERATOR *accel;
    register LONG flags;

    flags = get_flags();
    spin_lock(&screen->accelLock);
    accel = screen->accelHead;
    while (accel)
    {
       if (accel == newAccel)   // found, remove from list
       {
	  if (screen->accelHead == newAccel)
	  {
	     screen->accelHead = (void *) newAccel->accelNext;
	     if (screen->accelHead)
		screen->accelHead->accelPrior = NULL;
	     else
		screen->accelTail = NULL;
	  }
	  else
	  {
	     newAccel->accelPrior->accelNext = newAccel->accelNext;
	     if (newAccel != screen->accelTail)
		newAccel->accelNext->accelPrior = newAccel->accelPrior;
	     else
		screen->accelTail = newAccel->accelPrior;
	  }
	  newAccel->accelNext = newAccel->accelPrior = 0;
	  newAccel->accelFlags = 0;
	  spin_unlock(&screen->accelLock);
	  set_flags(flags);
	  return 0;
       }
       accel = accel->accelNext;
    }
    spin_unlock(&screen->accelLock);
    set_flags(flags);
    return -1;
}




