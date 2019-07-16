
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
*   FILE     :  CMD.H
*   DESCRIP  :  Installable Console Command Parser for MANOS v1.0
*   DATE     :  July 27, 1998
*
*
***************************************************************************/

#include "types.h"

typedef struct _CONSOLE_PARSER
{
    struct _CONSOLE_PARSER *consoleNext;
    struct _CONSOLE_PARSER *consolePrior;
    LONG (*ConsoleCommandParser)(SCREEN *screen, BYTE *commandLine,
				 struct _CONSOLE_PARSER *parser);
    LONG (*ConsoleCommandParserHelp)(SCREEN *screen, BYTE *commandLine,
				 struct _CONSOLE_PARSER *parser);
    LONG parserFlags;
    BYTE *commandName;
    LONG commandNameLength;
    LONG supervisorCommand;
    BYTE *shortHelp;
    void *moduleContext;
} CONSOLE_PARSER;

extern LONG ConsoleParserRoutine(SCREEN *screen, BYTE *command, BYTE *commandLine);
extern LONG ConsoleParserHelpRoutine(SCREEN *screen, BYTE *command, BYTE *commandLine);
extern LONG AddConsoleCommandParser(CONSOLE_PARSER *parser);
extern LONG RemoveConsoleCommandParser(CONSOLE_PARSER *parser);

