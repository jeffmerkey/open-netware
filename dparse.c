
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
*   FILE     :  DPARSE.C
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
#include "dparse.h"

LONG debugParserLock = 0;
DEBUGGER_PARSER *debugParserHead = 0;
DEBUGGER_PARSER *debugParserTail = 0;

LONG DebuggerParserRoutine(SCREEN *screen,
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
	     return retCode;
	  }
	  cli();
       }
       debugParser = debugParser->debugNext;
    }
    spin_unlock(&debugParserLock);
    set_flags(flags);
    return 0;
}

LONG DebuggerParserHelpRoutine(SCREEN *screen, BYTE *command, BYTE *commandLine)
{
    extern void ShowDebuggerAccelerators(SCREEN *screen);
    register DEBUGGER_PARSER *debugParser;
    register LONG retCode;

    SetPauseMode(screen, screen->nLines - 3);
    debugParser = debugParserHead;
    if (*command)  // if we were passed a command string
    {
       while (debugParser)
       {
	  if (debugParser->parserFlags &&
	      strlen(debugParser->debugCommandName) == strlen(command) &&
	      !stricmp(debugParser->debugCommandName, command))
	  {
	     if (debugParser->DebugCommandParserHelp)
	     {
		printfScreenWithAttribute(screen, BRITEWHITE, "Help for Command %s\n", debugParser->debugCommandName);
		(debugParser->DebugCommandParserHelp)(screen, commandLine, debugParser);
		ClearPauseMode(screen);
		return 1;
	     }
	     printfScreenWithAttribute(screen, BRITEWHITE, "Help for Command %s\n", debugParser->debugCommandName);
	     ClearPauseMode(screen);
	     return 1;
	  }
	  debugParser = debugParser->debugNext;
       }
       printfScreenWithAttribute(screen, BRITEWHITE, "Help for Command [%s] not found\n", strupr(command));
       ClearPauseMode(screen);
       return 1;
    }
    else
    {
       printfScreenWithAttribute(screen, BRITEWHITE, "Debugger Command(s)\n");
       while (debugParser)
       {
	  if (debugParser->parserFlags && debugParser->debugCommandName &&
	      !debugParser->supervisorCommand)
	     if (printfScreenWithAttribute(screen, LTCYAN, "%15s    - %55s\n",
			  debugParser->debugCommandName, debugParser->shortHelp))
	     {
		ClearPauseMode(screen);
		return 1;
	     }
	  debugParser = debugParser->debugNext;
       }
       ShowDebuggerAccelerators(screen);
    }
    ClearPauseMode(screen);
    return 0;
}

DEBUGGER_PARSER *insertDebuggerParser(DEBUGGER_PARSER *i, DEBUGGER_PARSER *top)
{
    DEBUGGER_PARSER *old, *p;

    if (!debugParserTail)
    {
       i->debugNext = i->debugPrior = NULL;
       debugParserTail = i;
       return i;
    }
    p = top;
    old = NULL;
    while (p)
    {
       if (strcmp(p->debugCommandName, i->debugCommandName) < 0)
       {
	  old = p;
	  p = p->debugNext;
       }
       else
       {
	  if (p->debugPrior)
	  {
	     p->debugPrior->debugNext = i;
	     i->debugNext = p;
	     i->debugPrior = p->debugPrior;
	     p->debugPrior = i;
	     return top;
	  }
	  i->debugNext = p;
	  i->debugPrior = NULL;
	  p->debugPrior = i;
	  return i;
       }
    }
    old->debugNext = i;
    i->debugNext = NULL;
    i->debugPrior = old;
    debugParserTail = i;
    return debugParserHead;

}

LONG AddDebuggerCommandParser(DEBUGGER_PARSER *parser)
{
    register MODULE_HANDLE *module = parser->moduleContext;
    register DEBUGGER_PARSER *debugParser;
    register LONG flags;

    if (module->ModuleSignature != MODULE_SIGNATURE)
       return -3;

    flags = get_flags();
    spin_lock(&debugParserLock);
    debugParser = debugParserHead;
    while (debugParser)
    {
       if (debugParser == parser ||
	  (strlen(parser->debugCommandName) == strlen(debugParser->debugCommandName) &&
	  (!stricmp(parser->debugCommandName, debugParser->debugCommandName))))
       {
	  spin_unlock(&debugParserLock);  // already exists or command
	  set_flags(flags);                 // is already taken
	  return 1;
       }
       debugParser = debugParser->debugNext;
    }
    parser->parserFlags = -1;
    parser->debugCommandNameLength = strlen(parser->debugCommandName);

//    if (!debugParserHead)
//    {
//       debugParserHead = parser;
//       debugParserTail = parser;
//       parser->debugNext = 0;
//       parser->debugPrior = 0;
//    }
//    else
//    {
//       debugParserTail->debugNext = parser;
//       parser->debugNext = 0;
//       parser->debugPrior = debugParserTail;
//       debugParserTail = parser;
//    }

    debugParserHead = insertDebuggerParser(parser, debugParserHead);

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




