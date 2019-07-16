

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
*   FILE     :  RLOCK.H
*   DESCRIP  :  Recursive Spin Lock defines for MANOS v1.0
*   DATE     :  June 26, 1998
*
***************************************************************************/

#include "types.h"

typedef struct _RLOCK_ {
   LONG exclLock;
   LONG lockValue;
   LONG processor;
   LONG count;
} rlock_t;

//
//   returns   0 - atomic lock occurred, processor assigned
//             1 - recusive count increased
//
extern LONG rspin_lock(rlock_t *rlock);

//
//   returns   0 - lock released
//             1 - recusive count decreased
//
extern LONG rspin_unlock(rlock_t *rlock);

//
//   returns   0 - atomic lock occurred, processor assigned
//             1 - recusive count increased
//            -1 - atomic try lock failed
//
extern LONG rspin_try_lock(rlock_t *rlock);
