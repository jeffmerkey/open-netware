
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  DEBKERN.H
*   DESCRIP  :  Multi-Processing Debugger Command Library for MANOS v1.0
*   DATE     :  September 7, 1998
*
*
***************************************************************************/

#include "types.h"

extern LONG displaySyncHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displaySema(SCREEN *screen, BYTE *commandLine,
			 StackFrame *stackFrame, LONG Exception,
			 DEBUGGER_PARSER *parser);
extern LONG displayMutex(SCREEN *screen, BYTE *commandLine,
			 StackFrame *stackFrame, LONG Exception,
			 DEBUGGER_PARSER *parser);
extern LONG displayRMutex(SCREEN *screen, BYTE *commandLine,
			 StackFrame *stackFrame, LONG Exception,
			 DEBUGGER_PARSER *parser);
extern LONG displayRWLock(SCREEN *screen, BYTE *commandLine,
			 StackFrame *stackFrame, LONG Exception,
			 DEBUGGER_PARSER *parser);
extern LONG displaySpin(SCREEN *screen, BYTE *commandLine,
			 StackFrame *stackFrame, LONG Exception,
			 DEBUGGER_PARSER *parser);

