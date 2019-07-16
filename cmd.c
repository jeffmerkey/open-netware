
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
*   FILE     :  CMD.C
*   DESCRIP  :  Installable Console Command Parser for MANOS v1.0
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
#include "cmd.h"

LONG consoleParserLock = 0;
CONSOLE_PARSER *consoleParserHead = 0;
CONSOLE_PARSER *consoleParserTail = 0;

LONG ConsoleParserRoutine(SCREEN *screen,
			  BYTE *command,
			  BYTE *commandLine)
{
    register CONSOLE_PARSER *consoleParser;
    register LONG flags, retCode;

    flags = get_flags();
    spin_lock(&consoleParserLock);
    consoleParser = consoleParserHead;
    while (consoleParser)
    {
       if (consoleParser->parserFlags &&
	   consoleParser->ConsoleCommandParser &&
	   strlen(consoleParser->commandName) == strlen(command) &&
	   !stricmp(consoleParser->commandName, command))
       {
	  retCode = (consoleParser->ConsoleCommandParser)(screen, commandLine,
							  consoleParser);
	  if (retCode)
	  {
	     spin_unlock(&consoleParserLock);
	     set_flags(flags);
	     return 1;
	  }
	  cli();
       }
       consoleParser = consoleParser->consoleNext;
    }
    spin_unlock(&consoleParserLock);
    set_flags(flags);
    return 0;
}

LONG ConsoleParserHelpRoutine(SCREEN *screen, BYTE *command, BYTE *commandLine)
{
    register CONSOLE_PARSER *consoleParser;
    register LONG retCode;

    SetPauseMode(screen, screen->nLines - 3);
    consoleParser = consoleParserHead;
    if (*command)  // if we were passed a command string
    {
       while (consoleParser)
       {
	  if (consoleParser->parserFlags &&
	      strlen(consoleParser->commandName) == strlen(command) &&
	      !stricmp(consoleParser->commandName, command))
	  {
	     if (consoleParser->ConsoleCommandParserHelp)
	     {
		printfScreen(screen, "Help for Command %s\n", consoleParser->commandName);
		(consoleParser->ConsoleCommandParserHelp)(screen, commandLine, consoleParser);
		ClearPauseMode(screen);
		return 1;
	     }
	     printfScreen(screen, "Help for Command %s\n", consoleParser->commandName);
	     ClearPauseMode(screen);
	     return 1;
	  }
	  consoleParser = consoleParser->consoleNext;
       }
       printfScreen(screen, "Help for Command [%s] not found\n", strupr(command));
       ClearPauseMode(screen);
       return 1;
    }
    else
    {
       printfScreenWithAttribute(screen, WHITE, "Console Command(s)\n");
       while (consoleParser)
       {
	  if (consoleParser->parserFlags && consoleParser->commandName &&
	      !consoleParser->supervisorCommand)
	     if (printfScreen(screen, "%15s    - %55s\n",
			  consoleParser->commandName, consoleParser->shortHelp))
	     {
		ClearPauseMode(screen);
		return 1;
	     }
	  consoleParser = consoleParser->consoleNext;
       }
    }
    ClearPauseMode(screen);
    return 0;
}

CONSOLE_PARSER *insertConsoleParser(CONSOLE_PARSER *i, CONSOLE_PARSER *top)
{
    CONSOLE_PARSER *old, *p;

    if (!consoleParserTail)
    {
       i->consoleNext = i->consolePrior = NULL;
       consoleParserTail = i;
       return i;
    }
    p = top;
    old = NULL;
    while (p)
    {
       if (strcmp(p->commandName, i->commandName) < 0)
       {
	  old = p;
	  p = p->consoleNext;
       }
       else
       {
	  if (p->consolePrior)
	  {
	     p->consolePrior->consoleNext = i;
	     i->consoleNext = p;
	     i->consolePrior = p->consolePrior;
	     p->consolePrior = i;
	     return top;
	  }
	  i->consoleNext = p;
	  i->consolePrior = NULL;
	  p->consolePrior = i;
	  return i;
       }
    }
    old->consoleNext = i;
    i->consoleNext = NULL;
    i->consolePrior = old;
    consoleParserTail = i;
    return consoleParserHead;

}

LONG AddConsoleCommandParser(CONSOLE_PARSER *parser)
{
    register MODULE_HANDLE *module = parser->moduleContext;
    register CONSOLE_PARSER *consoleParser;
    register LONG flags;

    if (module->ModuleSignature != MODULE_SIGNATURE)
       return -3;

    flags = get_flags();
    spin_lock(&consoleParserLock);
    consoleParser = consoleParserHead;
    while (consoleParser)
    {
       if (consoleParser == parser ||
	  (strlen(parser->commandName) == strlen(consoleParser->commandName) &&
	  (!stricmp(parser->commandName, consoleParser->commandName))))
       {
	  spin_unlock(&consoleParserLock);  // already exists or command
	  set_flags(flags);                 // is already taken
	  return 1;
       }
       consoleParser = consoleParser->consoleNext;
    }
    parser->parserFlags = -1;
    parser->commandNameLength = strlen(parser->commandName);
    consoleParserHead = insertConsoleParser(parser, consoleParserHead);
    spin_unlock(&consoleParserLock);
    set_flags(flags);
    return 0;
}

LONG RemoveConsoleCommandParser(CONSOLE_PARSER *parser)
{
    register CONSOLE_PARSER *consoleParser;
    register LONG flags;

    flags = get_flags();
    spin_lock(&consoleParserLock);
    consoleParser = consoleParserHead;
    while (consoleParser)
    {
       if (consoleParser == parser)   // found, remove from list
       {
	  if (consoleParserHead == parser)
	  {
	     consoleParserHead = (void *) parser->consoleNext;
	     if (consoleParserHead)
		consoleParserHead->consolePrior = NULL;
	     else
		consoleParserTail = NULL;
	  }
	  else
	  {
	     parser->consolePrior->consoleNext = parser->consoleNext;
	     if (parser != consoleParserTail)
		parser->consoleNext->consolePrior = parser->consolePrior;
	     else
		consoleParserTail = parser->consolePrior;
	  }
	  parser->consoleNext = parser->consolePrior = 0;
	  parser->parserFlags = 0;
	  spin_unlock(&consoleParserLock);
	  set_flags(flags);
	  return 0;
       }
       consoleParser = consoleParser->consoleNext;
    }
    spin_unlock(&consoleParserLock);
    set_flags(flags);
    return -1;
}




