
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  DEBUGCMD.H
*   DESCRIP  :  Installable Debugger Command Parser for MANOS v1.0
*   DATE     :  July 27, 1998
*
*
***************************************************************************/

#include "types.h"

typedef struct _DEBUGGER_PARSER {
    struct _DEBUGGER_PARSER *debugNext;
    struct _DEBUGGER_PARSER *debugPrior;
    LONG (*DebugCommandParser)(SCREEN *screen,
			       BYTE *commandLine,
			       void *stackFrame,
			       LONG Exception,
			       struct _DEBUGGER_PARSER *parser);
    LONG (*DebugCommandParserHelp)(SCREEN *screen,
				   BYTE *commandLine,
				   void *stackFrame,
				   LONG Exception,
				   struct _DEBUGGER_PARSER *parser);
    LONG parserFlags;
    BYTE *debugCommandName;
    LONG supervisorCommand;
    BYTE *shortHelp;
} DEBUGGER_PARSER;


extern LONG DebugParserRoutine(SCREEN *screen, BYTE *command, BYTE *commandLine,
			StackFrame *stackFrame,	LONG Exception);
extern LONG DebugParserHelpRoutine(SCREEN *screen, BYTE *command, BYTE *commandLine,
			    StackFrame *stackFrame,LONG Exception);
extern LONG AddDebuggerCommandParser(DEBUGGER_PARSER *parser);
extern LONG RemoveDebuggerCommandParser(DEBUGGER_PARSER *parser);
