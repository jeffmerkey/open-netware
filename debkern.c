
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
*   FILE     :  DEBKERN.C
*   DESCRIP  :  Multi-Processing Debugger Command Library for MANOS v1.0
*   DATE     :  September 7, 1998
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
#include "debug.h"
#include "dparse.h"
#include "bus.h"
#include "debkern.h"

LONG displaySyncHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "sema or sema <addr>      - display semaphore(s)\n");
    printfScreenWithAttribute(screen, LTCYAN, "mutex or mutex <addr>    - display sleep lock(s)\n");
    printfScreenWithAttribute(screen, LTCYAN, "rmutex or rmutex <addr>  - display recursive sleep lock(s)\n");
    printfScreenWithAttribute(screen, LTCYAN, "rwlock or rwlock <addr>  - display reader/writer lock(s)\n");
    printfScreenWithAttribute(screen, LTCYAN, "spin or spin <addr>      - display spin lock(s)\n");
    return TRUE;
}

LONG displaySema(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     return TRUE;

}

LONG displayMutex(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     return TRUE;

}

LONG displayRMutex(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     return TRUE;

}

LONG displayRWLock(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     LONG valid = 0;
     register LONG address;
     extern void RW_DISPLAY(SCREEN *, LONG);

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     address = EvaluateExpression(stackFrame, &cmd, &valid);
     if (valid)
     {
	RW_DISPLAY(screen, address);
     }
     return TRUE;

}

LONG displaySpin(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     return TRUE;

}

