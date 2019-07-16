

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  TIMER.H
*   DESCRIP  :  Multi-Processing Timer Defines for MANOS v1.0
*   DATE     :  January 10, 1998
*
*
***************************************************************************/


#include "types.h"
#include "version.h"

#define TIMER_SIGNATURE  0x43525556

typedef struct _TIMER {
   struct _TIMER *next;
   struct _TIMER *prior;
   LONG timerSignature;
   LONG waitTime;
   LONG currTime;
   void *parameter;
   void (*timerFunction)(struct _TIMER *);
   timerFlags;
} TIMER;

extern LONG scheduleTimer(TIMER *timer);
extern LONG cancelTimer(TIMER *timer);
extern void timerProcedure(void);

extern TIMER *timerHead;
extern TIMER *timerTail;
extern LONG timerMutex;


