
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
*   FILE     :  DEBUGCMD.C
*   DESCRIP  :  Installable Debugger Command Parser for MANOS v1.0
*   DATE     :  July 27, 1998
*
*
***************************************************************************/

#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "string.h"
#include "keyboard.h"
#include "screen.h"
#include "types.h"
#include "emit.h"
#include "os.h"
#include "dos.h"
#include "tss.h"
#include "kernel.h"
#include "mps.h"
#include "hal.h"
#include "xcall.h"
#include "window.h"
#include "source.h"
#include "loader.h"
#include "menu.h"
#include "rlock.h"
#include "event.h"
#include "debugcmd.h"

LONG debugParserLock = 0;
DEBUGGER_PARSER *debugParserHead = 0;
DEBUGGER_PARSER *debugParserTail = 0;

LONG DebugParserRoutine(SCREEN *screen,
			BYTE *command,
			BYTE *commandLine,
			StackFrame *stackFrame,
			LONG Exception)
{
    register DEBUGGER_PARSER *debugParser;
    register LONG flags, retCode;

    flags = get_flags();
    spin_lock(&debugParserLock);
    debugParser = debugParserHead;
    while (debugParser)
    {
       if (debugParser->parserFlags &&
	   debugParser->DebugCommandParser &&
	   strlen(debugParser->debugCommandName) == strlen(command) &&
	   !stricmp(debugParser->debugCommandName, command))
       {
	  retCode = (debugParser->DebugCommandParser)(screen,
						      commandLine,
						      stackFrame,
						      Exception,
						      debugParser);
	  if (retCode)
	  {
	     spin_unlock(&debugParserLock);
	     set_flags(flags);
	     return 1;
	  }
	  cli();
       }
       debugParser = debugParser->debugNext;
    }
    spin_unlock(&debugParserLock);
    set_flags(flags);
    return 0;
}

LONG DebugParserHelpRoutine(SCREEN *screen,
			    BYTE *command,
			    BYTE *commandLine,
			    StackFrame *stackFrame,
			    LONG Exception)
{
    register DEBUGGER_PARSER *debugParser;
    register LONG flags, retCode;

    SetPauseMode(screen, screen->nLines - 2);
    debugParser = debugParserHead;
    if (*command)
    {
       while (debugParser)
       {
	  if (debugParser->parserFlags &&
	      strlen(debugParser->debugCommandName) == strlen(command) &&
	      !stricmp(debugParser->debugCommandName, command))
	  {
	     if (debugParser->DebugCommandParserHelp)
	     {
		printfScreen(screen, "\nHelp for Command %s\n", debugParser->debugCommandName);
		(debugParser->DebugCommandParserHelp)(screen, commandLine, stackFrame,
						      Exception, debugParser);
	     }
	     printfScreen(screen, "\nHelp for Command %s\n", debugParser->debugCommandName);
	     return 1;
	  }
	  debugParser = debugParser->debugNext;
       }
       printfScreen(screen, "\nHelp for Command [%s] not found\n", strupr(command));
       return 1;
    }
    else
    {
       while (debugParser)
       {
	  if (debugParser->parserFlags && debugParser->debugCommandName &&
	      !debugParser->supervisorCommand)
	     printfScreen(screen, "\n%15s    - %s",
			  debugParser->debugCommandName, debugParser->shortHelp);
	  debugParser = debugParser->debugNext;
       }
    }
    ClearPauseMode(screen);
    return 0;
}

LONG AddDebuggerCommandParser(DEBUGGER_PARSER *parser)
{
    register DEBUGGER_PARSER *debugParser;
    register LONG flags;

    flags = get_flags();
    spin_lock(&debugParserLock);
    debugParser = debugParserHead;
    while (debugParser)
    {
       if (debugParser == parser ||
	  (strlen(parser->debugCommandName) == strlen(debugParser->debugCommandName) &&
	  (!stricmp(parser->debugCommandName, debugParser->debugCommandName))))
       {
	  spin_unlock(&debugParserLock);  // already exists
	  set_flags(flags);
	  return 1;
       }
       debugParser = debugParser->debugNext;
    }
    if (!debugParserHead)
    {
       debugParserHead = parser;
       debugParserTail = parser;
       parser->debugNext = 0;
       parser->debugPrior = 0;
    }
    else
    {
       debugParserTail->debugNext = parser;
       parser->debugNext = 0;
       parser->debugPrior = debugParserTail;
       debugParserTail = parser;
    }
    spin_unlock(&debugParserLock);
    set_flags(flags);
    return 0;
}

LONG RemoveDebuggerCommandParser(DEBUGGER_PARSER *parser)
{
    register DEBUGGER_PARSER *debugParser;
    register LONG flags;

    flags = get_flags();
    spin_lock(&debugParserLock);
    debugParser = debugParserHead;
    while (debugParser)
    {
       if (debugParser == parser)   // found, remove from list
       {
	  if (debugParserHead == parser)
	  {
	     debugParserHead = (void *) parser->debugNext;
	     if (debugParserHead)
		debugParserHead->debugPrior = NULL;
	     else
		debugParserTail = NULL;
	  }
	  else
	  {
	     parser->debugPrior->debugNext = parser->debugNext;
	     if (parser != debugParserTail)
		parser->debugNext->debugPrior = parser->debugPrior;
	     else
		debugParserTail = parser->debugPrior;
	  }
	  parser->debugNext = parser->debugPrior = 0;
	  parser->parserFlags = 0;
	  spin_unlock(&debugParserLock);
	  set_flags(flags);
	  return 0;
       }
       debugParser = debugParser->debugNext;
    }
    spin_unlock(&debugParserLock);
    set_flags(flags);
    return -1;
}




