
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
*   FILE     :  CINIT.C
*   DESCRIP  :  Initialization Code for MANOS v1.0
*   DATE     :  November 2, 1997
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

LONG MajorVersion = MAJOR_VERSION;
LONG MinorVersion = MINOR_VERSION;
LONG BuildVersion = BUILD_VERSION;

void ExitOS(void)
{
    register LONG *pD;
    extern void ExitToDOS(void);
    extern LONG oldZero;

    EventNotify(EVENT_EXIT_OS, 0);

    pD = (LONG *) 0;
    *pD = oldZero;

    ExitToDOS();   //  call does not return
    return;
}

void MANOSMain(DOS_TABLE *dos, PM_TABLE *pm)
{
    extern void UpdateStartupRegistry(void);
    extern void EnterSMPKernel(void);
    extern LONG oldZero;
    register LONG retCode, *pD;

    pD = (LONG *) 0;
    oldZero = *pD;
    *pD = 0xF4CCCCCC;   // three breakpoints and a halt (and a partridge in a pear tree)

    // create startup registry
    UpdateStartupRegistry();

    // startup operating system
    SystemStartupRoutine(dos, pm);

    EnterSMPKernel();
    return;

}

