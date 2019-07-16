
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
*   FILE     :  SPEED.C
*   DESCRIP  :  Speed Measurement Code for MANOS v1.0
*   DATE     :  August 19, 1998
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
#include "window.h"
#include "timer.h"
#include "line.h"
#include "dosfile.h"
#include "loader.h"
#include "event.h"
#include "bus.h"
#include "start.h"

#define LOCK_SPEED     0
#define CACHE_SPEED    0
#define INST_SPEED     0

TIMER speedTimer;
LONG speedLatch = 1;
LONG cacheCounter = 1;
LONG lockCounter = 1;
LONG instructionCount = 1;
LONG SystemSpeedRating = 0;

extern void RegisterWriteTest(void);
extern void CacheWriteTest(void);
extern void LockTest(void);

void speedCounter(TIMER *timer)
{
    if (timer) {};
    speedLatch = 0;
    return;
}

LONG MeasureSpeedRating(void)
{

    sti();
    unmask_pic_timer();

    speedLatch = 1;
    speedTimer.waitTime = 18;
    speedTimer.timerFunction = speedCounter;
    scheduleTimer(&speedTimer);

    RegisterWriteTest();
    SystemSpeedRating = (instructionCount * 5) >> 12;
    printfScreen(consoleScreen, "Processor Speed Rating:  %d\n", SystemSpeedRating);
    printfScreen(consoleScreen, "(Type SPEED at the system console for an explanation this rating)\n");

#if INST_SPEED
    printfScreen(consoleScreen, "This processor measured at %d instructions/second (register)\n", instructionCount * 5);
#endif

#if CACHE_SPEED
    speedLatch = 1;
    speedTimer.waitTime = 18;
    speedTimer.timerFunction = speedCounter;
    scheduleTimer(&speedTimer);

    sti();
    CachedWriteTest();
    printfScreen(consoleScreen, "This processor measured at %d instructions/second (cached write)\n", cacheCounter * 5);
    cli();
#endif

#if LOCK_SPEED
    speedLatch = 1;
    speedTimer.waitTime = 18;
    speedTimer.timerFunction = speedCounter;
    scheduleTimer(&speedTimer);

    sti();
    LockTest();
    printfScreen(consoleScreen, "This processor measured at %d instructions/second (LOCK# assertion)\n\n", lockCounter * 5);
    cli();
#endif

    return 0;

}