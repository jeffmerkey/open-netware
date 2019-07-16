

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  RLOCK.C
*   DESCRIP  :  Recursive Spin Locks for MANOS v1.0
*   DATE     :  June 26, 1998
*
***************************************************************************/

#include "version.h"
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
#include "rlock.h"

extern LONG dspin_lock(LONG *);
extern LONG dspin_try_lock(LONG *);
extern void dspin_unlock(LONG *);

//
//   returns   0 - atomic lock occurred, processor assigned
//             1 - recusive count increased
//

LONG rspin_lock(rlock_t *rlock)
{
   register LONG proc = get_processor_id();
   register LONG retCode;

   if (rlock->lockValue && rlock->processor == (proc + 1))
   {
      rlock->count++;
      retCode = 1;
   }
   else
   {
      dspin_lock(&rlock->lockValue);
      rlock->processor = (proc + 1);
      retCode = 0;
   }

   return retCode;

}

//
//   returns   0 - lock released
//             1 - recusive count decreased
//

LONG rspin_unlock(rlock_t *rlock)
{

   register LONG retCode;

   if (rlock->count)
   {
      rlock->count--;
      retCode = 1;
   }
   else
   {
      rlock->processor = 0;
      dspin_unlock(&rlock->lockValue);
      retCode = 0;
   }

   return retCode;
}

//
//   returns   0 - atomic lock occurred, processor assigned
//             1 - recusive count increased
//            -1 - atomic try lock failed
//

LONG rspin_try_lock(rlock_t *rlock)
{
   register LONG proc = get_processor_id();
   register LONG retCode;

   if (rlock->lockValue && rlock->processor == (proc + 1))
   {
      rlock->count++;
      retCode = 0;
   }
   else
   {
      if (!dspin_try_lock(&rlock->lockValue))
      {
	 rlock->processor = (proc + 1);
	 retCode = 0;
      }
      else
	 retCode = 1;
   }

   return retCode;

}
