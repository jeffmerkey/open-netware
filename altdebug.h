
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  ALTDEBUG.H
*   DESCRIP  :  Alternate Debugger Interface for MANOS v1.0
*   DATE     :  July 27, 1998
*
*
***************************************************************************/

#include "types.h"

typedef struct _ALT_DEBUGGER {
    struct _ALT_DEBUGGER *altDebugNext;
    struct _ALT_DEBUGGER *altDebugPrior;
    LONG (*AlternateDebugger)(SCREEN *screen, StackFrame *stackFrame,
			      LONG Exception, LONG processor);
    void *ModuleContext;
} ALT_DEBUGGER;

extern LONG AlternateDebuggerRoutine(SCREEN *screen, StackFrame *stackFrame,
			      LONG Exception, LONG processor);
extern LONG AddAlternateDebugger(ALT_DEBUGGER *Debugger);
extern LONG RemoveAlternateDebugger(ALT_DEBUGGER *Debugger);

