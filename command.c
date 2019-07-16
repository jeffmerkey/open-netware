

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
*   FILE     :  COMMAND.C
*   DESCRIP  :  System Console Command Executive for MANOS v1.0
*   DATE     :  January 10, 1998
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
#include "window.h"
#include "dosfile.h"
#include "timer.h"
#include "line.h"
#include "loader.h"
#include "cmd.h"
#include "xcall.h"

void CommandProcess(void);
BYTE ServerName[100] = { "MANOS_PRIME" };
BYTE ServerNetworkNumber[100] = { "" };
LONG ServerNetworkAddress = 0;
LONG ConnectionsAllowed = 10000;
LONG ConnectionsOpen = 0;

LONG endTest = 0;
void ThreadTest(void)
{
   while (!endTest)
   {
      thread_switch();
   }
   ConsolePrintf("test thread %08X has been killed\n", get_running_process());
   return;
}

LONG displayThreadTestHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "create a test thread\n");
    return TRUE;
}

LONG createTestThread(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    register PROCESS *process;

    if (screen) {};
    if (commandLine) {};
    if (parser) {};

    endTest = 0;
    process = createThread("test thread", (LONG (*)())ThreadTest, 8192, 0, -1);
    if (process)
       ConsolePrintf("test thread %08X has been started\n", process);
    return TRUE;

}
LONG displayKillThreadTestHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "kill all test threads\n");
    return TRUE;
}

LONG killTestThreads(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{

    if (screen) {};
    if (commandLine) {};
    if (parser) {};

    endTest = TRUE;
    return TRUE;
}

void CreateConsoleProcess(void)
{
    insert_process(&idle_p[0]);
    commandP = createThread("system console process",
			    (LONG (*)()) CommandProcess,
			    8192 * 2, 0, 0);
}

void GetServerInfo(void)
{
    register LONG retCode;
    LONG valid;

    while (1)
    {
       register BYTE *p = ServerName;
       register LONG nameInvalid = 0;

       printfScreen(consoleScreen, "\nEnter File Server Name: ");
       ScreenInputFromKeyboard(consoleScreen, &ServerName[0], 0, 80, 7);
       strupr(ServerName);
       if (ServerName[0])
       {
	  while (*p)
	  {
	     switch (*p)
	     {
		case '>': case '<': case ',': case '+':
		case '|': case ']': case '[': case '=':
		case ' ': case '.': case '*': case '?':
		   printfScreen(consoleScreen, "\nInvalid Name \n");
		   nameInvalid = TRUE;
		   break;

		default:
		   p++;
		   break;
	     }
	     if (nameInvalid)
		break;
	  }
	  if (!nameInvalid)
	     break;
       }
    }

    while (1)
    {
       BYTE *p = ServerNetworkNumber;

       printfScreen(consoleScreen, "\nEnter Internal Network Address: ");
       ScreenInputFromKeyboard(consoleScreen, &ServerNetworkNumber[0], 0, 80, 7);
       ServerNetworkAddress = EvaluateExpression(0, &p, (LONG *) &valid);
       if (valid)
       {
	  strupr(ServerNetworkNumber);
	  break;
       }
    }


}

LONG displayMemoryHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "displays the memory configuration for this system\n");
    return TRUE;
}

LONG displayMemoryInfo(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    BYTE displayBuffer[100] = { "" };
    extern LONG TotalSystemMemory;
    extern LONG HighMemoryLength;
    extern LONG LowMemoryLength;

    if (commandLine) {};
    if (parser) {};

    sprintf_comma(displayBuffer, "%d", TotalSystemMemory);
    printfScreen(screen, "Total System Memory        : %s bytes\n", displayBuffer);
    sprintf_comma(displayBuffer, "%d", HighMemoryLength);
    printfScreen(screen, "Available Memory Above 1MB : %s bytes\n", displayBuffer);
    sprintf_comma(displayBuffer, "%d", LowMemoryLength);
    printfScreen(screen, "Available Memory Below 1MB : %s bytes\n", displayBuffer);

    return TRUE;

}

LONG displaySpeedHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "displays the current processor speed rating for this system\n");
    return TRUE;
}

LONG displaySpeedInfo(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     extern LONG SystemSpeedRating;

     if (commandLine) {};
     if (parser) {};

     printfScreenWithAttribute(screen, WHITE,
	     "Processor Speed Rating:  %d\n", SystemSpeedRating);
     printfScreenWithAttribute(screen, WHITE,
	     "\nTypical Speed Ratings are:\n");
     printfScreenWithAttribute(screen, WHITE,
	     "Intel 80386SX-16 Mhz ......... rating is 466\n");
     printfScreenWithAttribute(screen, WHITE,
	     "Intel 80386DX-33 Mhz ......... rating is 1250\n");
     printfScreenWithAttribute(screen, WHITE,
	     "Intel 80486DX2-66 Mhz ........ rating is 5058\n");
     printfScreenWithAttribute(screen, WHITE,
	     "Intel Pentium(R)-133 Mhz ..... rating is 24346\n");
     printfScreenWithAttribute(screen, WHITE,
	     "Intel Pentium(R)-166 Mhz ..... rating is 35556\n");
     printfScreenWithAttribute(screen, WHITE,
	     "Intel PentiumPro-200 Mhz ..... rating is 51126\n");
     printfScreenWithAttribute(screen, WHITE,
	     "Intel PentiumII-266 Mhz ...... rating is 69709\n");
     return TRUE;
}

LONG displayVersionHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "displays the current operating system version\n");
    return TRUE;
}

LONG displayVersionInfo(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    extern LONG MajorVersion;
    extern LONG MinorVersion;
    extern LONG BuildVersion;

    if (commandLine) {};
    if (parser) {};

    printfScreen(screen, "Metropolitan Area Network Operating System\n");
    printfScreen(screen, "v%02d.%02d.%02d\n", MajorVersion, MinorVersion, BuildVersion);
    printfScreen(screen, "Copyright (C) 1997, 1998 Timpanogas Research Group, Inc.\n");
    printfScreen(screen, "All Rights Reserved.\n");
    return TRUE;
}

LONG displayMonitorHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "activates the system monitor console\n");
    return TRUE;
}

LONG activateSystemMonitor(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     register PROCESS *process;
     extern LONG MonitorActive;
     extern void MonitorProcess(void);

     if (commandLine) {};
     if (parser) {};

     if (MonitorActive)
     {
	printfScreen(screen, "MONITOR is already active\n");
	return TRUE;
     }
     process = createThread("System Monitor",
			    (LONG (*)()) MonitorProcess,
			    8192 * 2,
			    0,
			    -1);
     if (!process)
	printfScreen(screen, "could not exec monitor process\n");
     return TRUE;

}

LONG displayExportsHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "displays all application module exported functions\n");
    return TRUE;
}

LONG displayExports(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    extern void DisplayExportsMatch(SCREEN *, LONG, BYTE *);
    extern void DisplayExports(SCREEN *, LONG);

    if (parser) {};

    commandLine = &commandLine[parser->commandNameLength];
    while (*commandLine && *commandLine == ' ') commandLine++;

    if (*commandLine)
       DisplayExportsMatch(screen, WHITE, commandLine);
    else
       DisplayExports(screen, WHITE);
    return TRUE;
}

LONG displayModulesHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "displays all loaded application modules\n");
    return TRUE;
}

LONG displayModules(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    extern void DisplayModuleMap(SCREEN *, LONG, LONG);

    if (commandLine) {};
    if (parser) {};

    DisplayModuleMap(screen, WHITE, 0);
    return TRUE;
}

LONG loadFileModuleHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "loads and activates an application module\n");
    printfScreen(screen, "i.e.  LOAD \\path\\filename\n");
    return TRUE;
}

LONG loadFileModule(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     register LONG retCode;
     MODULE_HANDLE *newModule;
     extern LONG LoadFile(BYTE *, MODULE_HANDLE **);

     if (parser) {};

     commandLine = &commandLine[parser->commandNameLength];
     while (*commandLine && *commandLine == ' ') commandLine++;

     strupr(commandLine);
     retCode = LoadFile(commandLine, &newModule);
     switch(retCode)
     {
	 case 0:
	    printfScreen(screen, "%s Loaded\n", newModule->ModuleShortName);
	    break;

	 case FILE_NOT_FOUND:
	    printfScreen(screen, "File %s not found\n", commandLine);
	    break;

	 case FILE_INCOMPAT:
	    printfScreen(screen, "File %s is not a Microsoft(R) PE32 Format. Load Failed\n", commandLine);
	    break;

	 case FILE_SEEK_ERROR:
	    printfScreen(screen, "Seek Error for File %s. Load Failed\n", commandLine);
	    break;

	 case FILE_READ_ERROR:
	    printfScreen(screen, "Read Error for File %s.  Load Failed\n", commandLine);
	    break;

	 case FILE_MODULO_ERROR:
	    printfScreen(screen, "File %s had a MODULO 512 Segment Error.  Load Failed\n", commandLine);
	    break;

	 case FILE_MEMORY_ERROR:
	    printfScreen(screen, "System Out of Memory.  File %s not Loaded\n", commandLine);
	    break;

	 case FILE_CORRUPT_ERROR:
	    printfScreen(screen, "File %s is corrupted.  Load Failed\n", commandLine);
	    break;

	 case FILE_DUP_ERROR:
	    printfScreen(screen, "File %s is already loaded\n", commandLine);
	    break;

	 case FILE_UNRESOLVED:
	    printfScreen(screen, "File %s had Unresolved Exernals.  Load Failed\n", commandLine);
	    break;

	 case FILE_PROCESS_ERROR:
	    printfScreen(screen, "Default Process not created.  File %s Load Failed\n", commandLine);
	    break;

	 case FILE_LINK_ERROR:
	    printfScreen(screen, "File %s had multiple data/code segments or bad relocation table.\n", commandLine);
	    break;

	 default:
	    printfScreen(screen, "Error Loading File (%d)\n", retCode);
	    break;
     }
     return TRUE;
}

LONG unloadFileModuleHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "unloads an active application module\n");
    printfScreen(screen, "i.e.  UNLOAD module\n");
    return TRUE;
}

LONG unloadFileModule(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     register LONG retCode;
     extern LONG UnloadFile(BYTE *);

     if (parser) {};

     commandLine = &commandLine[parser->commandNameLength];
     while (*commandLine && *commandLine == ' ') commandLine++;

     retCode = UnloadFile(commandLine);
     switch (retCode)
     {
	case 0:
	   printfScreen(screen, "Module %s Unloaded\n", strupr(commandLine));
	   break;

	case -1:
	   printfScreen(screen, "Module %s not Loaded\n", strupr(commandLine));
	   break;

	case -2:
	   break;

	case -3:
	   printfScreen(screen, "Module %s unload cancelled by user\n", strupr(commandLine));
	   break;

	default:
	   printfScreen(screen, "Module not Unloaded\n");
	   break;
     }
     return TRUE;

}

LONG dumpModuleHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "dumps a file in binary mode\n");
    printfScreen(screen, "i.e.  pedump \\path\\filename\n");
    return TRUE;
}

LONG dumpModule(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     register LONG retCode;
     extern LONG DumpFile(SCREEN *screen, BYTE *);

     if (parser) {};

     commandLine = &commandLine[parser->commandNameLength];
     while (*commandLine && *commandLine == ' ') commandLine++;

     retCode = DumpFile(screen, commandLine);
     switch (retCode)
     {
	case 0:
	   break;

	case -1:
	default:
	   printfScreen(screen, "file not found\n");
	   break;
     }
     return TRUE;
}

LONG pedumpModuleHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "dumps a Microsoft Portable Executable (PE) module\n");
    printfScreen(screen, "i.e.  pedump \\path\\filename\n");

    return TRUE;
}

LONG pedumpModule(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     register LONG retCode;
     extern LONG DumpPEFile(SCREEN *screen, BYTE *);

     if (parser) {};

     commandLine = &commandLine[parser->commandNameLength];
     while (*commandLine && *commandLine == ' ') commandLine++;

     SetPauseMode(screen, screen->nLines - 4);
     retCode = DumpPEFile(screen, commandLine);
     switch (retCode)
     {
	case 0:
	   break;

	case -1:
	default:
	   printfScreen(screen, "file not found\n");
	   break;
     }
     ClearPauseMode(screen);
     return TRUE;
}

LONG exitSystemHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "exits the system and returns to MS-DOS\n");
    return TRUE;
}

LONG exitSystem(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     register LONG key;

     if (commandLine) {};
     if (parser) {};

     printfScreen(screen, "Exit To MS-DOS? <y/n> ");
     key = ReadKeyboard(screen);
     printfScreen(screen, "%c\n", (BYTE) key & 0xFF);
     if (toupper(key & 0xFF) == 'Y')
     {
	printfScreen(screen, "Exiting ... \n");
	ExitOS();  // this call does not return
     }
     return TRUE;
}

LONG clearConsoleScreenHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "clears the system console screen\n");
    return TRUE;
}

LONG clearConsoleScreen(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     if (commandLine) {};
     if (parser) {};

     ClearScreen(screen);
     return TRUE;
}

LONG displayUtilizationHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "displays active SMP utilization per processor, and for the system\n");
    return TRUE;
}

LONG displayUtilization(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     register LONG i;

     if (commandLine) {};
     if (parser) {};

     printfScreen(screen, "system utilization is : %d%%\n", systemUtilization);
     for (i=0; i < active_processors; i++)
     {
	printfScreen(screen, "processor (%02i) utilization is : %03d%%\n",
		     i, processorUtilization[i]);
     }
     return TRUE;
}

LONG onlineProcessorHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "activates a specified processor\n");
    printfScreen(screen, "i.e.  online [p#]\n");
    return TRUE;
}

LONG onlineProcessor(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     register LONG value;
     LONG valid;

     if (parser) {};

     commandLine = &commandLine[parser->commandNameLength];
     while (*commandLine && *commandLine == ' ') commandLine++;

     value = EvaluateExpression(0, &commandLine, &valid);
     if (valid && value < num_procs)
     {
	activate_processor(value)
	? printfScreen(screen, "Processor (%i) has been activated\n", value)
	: printfScreen(screen, "Processor (%i) did not activate\n", value);
	return TRUE;
     }
     printfScreen(screen, "invalid processor specified\n");
     return TRUE;
}

LONG offlineProcessorHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "deactivates a specified processor\n");
    printfScreen(screen, "i.e.  offline [p#]\n");
    return TRUE;
}

LONG offlineProcessor(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     register LONG value;
     LONG valid;

     if (parser) {};

     commandLine = &commandLine[parser->commandNameLength];
     while (*commandLine && *commandLine == ' ') commandLine++;

     value = EvaluateExpression(0, &commandLine, &valid);
     if (valid && value < num_procs)
     {
	close_processor(value)
	? printfScreen(screen, "Processor (%i) could not be halted\n", value)
	: printfScreen(screen, "Processor (%i) has been shutdown\n", value);
	return TRUE;
     }
     printfScreen(screen, "invalid processor specified\n");
     return TRUE;
}

LONG displayProcessorStatusHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "displays active SMP processors and their current state\n");
    return TRUE;
}

LONG displayProcessorStatus(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     register LONG i;
     extern BYTE *procState[];

     if (commandLine) {};
     if (parser) {};

     for (i=0; i < num_procs; i++)
     {
	printfScreenWithAttribute(screen, WHITE, "Processor: (%i)  State:  %s\n",
				 i, procState[processor_table[i].ProcessorState & 0xF]);
     }
     return TRUE;
}

LONG displayHelpHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "displays general help for all commands, or help for a specific command\n");
    printfScreen(screen, "HELP         <enter>  - list all commands\n");
    printfScreen(screen, "HELP command <enter>  - help for a specific command\n");
    return TRUE;
}

LONG displayHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{

    register LONG count;
    BYTE verbBuffer[100];
    register BYTE *verb, *pp, *vp;

    commandLine = &commandLine[parser->commandNameLength];
    while (*commandLine && *commandLine == ' ') commandLine++;

    count = 0;
    pp = commandLine;
    vp = verb = &verbBuffer[0];
    while (*pp && *pp == ' ' && count++ < 80)
       pp++;

    while (*pp && *pp != ' ' && count++ < 80)
       *vp++ = *pp++;
    *vp = '\0';

    while (*pp && *pp == ' ' && count++ < 80)
       pp++;
    ConsoleParserHelpRoutine(screen, verb, commandLine);

    return TRUE;
}

LONG displayProcessHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "displays active SMP processes and threads\n");
    return TRUE;
}

LONG displayProcessInfo(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     register PROCESS *p;
     register LONG flags, i;
     extern BYTE *processState[];

     if (commandLine) {};
     if (parser) {};

     flags = get_flags();
     spin_lock(&systemThreadmutex);
     p = systemThreadHead;
     while (p)
     {
	i = p->lastProcessor;
	printfScreenWithAttribute(screen, WHITE,
				    "%08X:%08X | %s | p%02i | %s | %s\n",
				    p,
				    p->stackPointer,
				    processState[p->threadState & 0xF],
				    p->lastProcessor,
				    (p == processorSet[i].running_process) ? "RUN " : "IDLE",
				    p->processName);

	p = p->kernelNext;
     }
     spin_unlock(&systemThreadmutex);
     set_flags(flags);
     return TRUE;
}

LONG changeScreenSizeHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "changes the line settings for the system screen\n");
    printfScreen(screen, "LINES 25  - set to 25 line mode\n");
    printfScreen(screen, "LINES 43  - set to 43 line mode\n");
    printfScreen(screen, "LINES 50  - set to 50 line mode\n");
    return TRUE;
}

LONG changeScreenSize(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     register LONG value;
     extern LONG ChangeSystemScreenSize(LONG);

     if (parser) {};

     commandLine = &commandLine[parser->commandNameLength];
     while (*commandLine && *commandLine == ' ') commandLine++;

     value = atol(commandLine);
     switch (value)
     {
	   case 25:
	      ChangeSystemScreenSize(25);
	      return TRUE;

	   case 43:
	      ChangeSystemScreenSize(43);
	      return TRUE;

	   case 50:
	      ChangeSystemScreenSize(50);
	      return TRUE;

	   default:
	      printfScreen(screen, "invalid line mode specified\n");
	      return TRUE;
     }

}

LONG nmiDebugEntryHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "send a directed nmi to debug a specified processor\n");
    printfScreen(screen, "i.e.  nmidebug [p#]\n");
    return TRUE;
}

LONG nmiDebugEntry(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     register LONG value;
     LONG valid;

     if (parser) {};

     commandLine = &commandLine[parser->commandNameLength];
     while (*commandLine && *commandLine == ' ') commandLine++;

     value = EvaluateExpression(0, &commandLine, &valid);
     if (valid &&
	 value < num_procs &&
	 processor_table[value].ProcessorState != PROCESSOR_INACTIVE)
     {
	extern LONG xcall_processor(LONG, LONG, LONG, LONG, LONG);

	xcall_processor(value, XCALL_DIRECTED, 0, 0, 0);
	return TRUE;
     }
     printfScreen(screen, "invalid processor specified\n");
     return TRUE;
}

LONG debugEntryHelp(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreen(screen, "signal an INT3 to debug a specified processor\n");
    printfScreen(screen, "i.e.  debug [p#]\n");
    return TRUE;
}

LONG debugEntry(SCREEN *screen, BYTE *commandLine, CONSOLE_PARSER *parser)
{
     register LONG value;
     LONG valid;

     if (parser) {};

     commandLine = &commandLine[parser->commandNameLength];
     while (*commandLine && *commandLine == ' ') commandLine++;

     value = EvaluateExpression(0, &commandLine, &valid);
     if (valid &&
	 value < num_procs &&
	 processor_table[value].ProcessorState != PROCESSOR_INACTIVE)
     {
	processor_table[value].DebugSignal = TRUE;
	close_processor(value);
	return TRUE;
     }
     printfScreen(screen, "invalid processor specified\n");
     return TRUE;
}

CONSOLE_PARSER createTestThreadParser = {
  0, 0, killTestThreads, displayKillThreadTestHelp, 0, "STOP", 0, 0,
 "kill all SMP test threads", &serverModuleHandle };

CONSOLE_PARSER killTestThreadsParser = {
  0, 0, createTestThread, displayThreadTestHelp, 0, "TEST", 0, 0,
 "create an SMP test thread", &serverModuleHandle };

CONSOLE_PARSER displayMemoryInfoParser = {
  0, 0, displayMemoryInfo, displayMemoryHelp, 0, "MEMORY", 0, 0,
 "display current system memory totals", &serverModuleHandle };

CONSOLE_PARSER displaySpeedInfoParser = {
  0, 0, displaySpeedInfo, displaySpeedHelp, 0, "SPEED", 0, 0,
 "display processor speed ratings", &serverModuleHandle };

CONSOLE_PARSER displayVersionInfoParser = {
  0, 0, displayVersionInfo, displayVersionHelp, 0, "VERSION", 0, 0,
 "display current version", &serverModuleHandle };

CONSOLE_PARSER activateSystemMonitorParser = {
  0, 0, activateSystemMonitor, displayMonitorHelp, 0, "MONITOR", 0, 0,
 "activate system monitor", &serverModuleHandle };

CONSOLE_PARSER displayExportsParser = {
  0, 0, displayExports, displayExportsHelp, 0, "EXPORTS", 0, 0,
 "display exported functions", &serverModuleHandle };

CONSOLE_PARSER displayModulesParser = {
  0, 0, displayModules, displayModulesHelp, 0, "MODULES", 0, 0,
 "display loaded modules", &serverModuleHandle };

CONSOLE_PARSER loadFileModuleParser = {
  0, 0, loadFileModule, loadFileModuleHelp, 0, "LOAD", 0, 0,
 "load a module", &serverModuleHandle };

CONSOLE_PARSER unloadFileModuleParser = {
  0, 0, unloadFileModule, unloadFileModuleHelp, 0, "UNLOAD", 0, 0,
 "unload a module", &serverModuleHandle };

CONSOLE_PARSER displayProcessInfoParser = {
  0, 0, displayProcessInfo, displayProcessHelp, 0, "PROCESS", 0, 0,
 "display all active kernel processes", &serverModuleHandle };

CONSOLE_PARSER exitSystemParser = {
  0, 0, exitSystem, exitSystemHelp, 0, "EXIT", 0, 0,
 "shutdown and exit the operating system", &serverModuleHandle };

CONSOLE_PARSER quitSystemParser = {
  0, 0, exitSystem, exitSystemHelp, 0, "Q", 0, 0,
 "shutdown and exit the operating system", &serverModuleHandle };

CONSOLE_PARSER clearConsoleScreenParser = {
  0, 0, clearConsoleScreen, clearConsoleScreenHelp, 0, "CLS", 0, 0,
 "clear the console screen", &serverModuleHandle };

CONSOLE_PARSER displayUtilizationParser = {
  0, 0, displayUtilization, displayUtilizationHelp, 0, "UTIL", 0, 0,
 "display processor and system utilization", &serverModuleHandle };

CONSOLE_PARSER onlineProcessorParser = {
  0, 0, onlineProcessor, onlineProcessorHelp, 0, "ONLINE", 0, 0,
 "activate a processor", &serverModuleHandle };

CONSOLE_PARSER offlineProcessorParser = {
  0, 0, offlineProcessor, offlineProcessorHelp, 0, "OFFLINE", 0, 0,
 "deactivate a processor", &serverModuleHandle };

CONSOLE_PARSER displayProcessorStatusParser = {
  0, 0, displayProcessorStatus, displayProcessorStatusHelp, 0, "PROCESSORS", 0, 0,
 "display status of all processors", &serverModuleHandle };

CONSOLE_PARSER displayHelpParser = {
  0, 0, displayHelp, displayHelpHelp, 0, "HELP", 0, 0,
 "this help screen (HELP <command> for specific help)", &serverModuleHandle };

CONSOLE_PARSER displayQuestionMarkParser = {
  0, 0, displayHelp, displayHelpHelp, 0, "?", 0, 0,
 "this help screen", &serverModuleHandle };

CONSOLE_PARSER dumpModuleParser = {
  0, 0, dumpModule, dumpModuleHelp, 0, "DUMP", 0, 0,
 "dumps a file in binary mode", &serverModuleHandle };

CONSOLE_PARSER pedumpModuleParser = {
  0, 0, pedumpModule, pedumpModuleHelp, 0, "PEDUMP", 0, 0,
 "dumps a Microsoft Portable Executable (PE) file module", &serverModuleHandle };

CONSOLE_PARSER changeScreenSizeParser = {
  0, 0, changeScreenSize,
  changeScreenSizeHelp, 0, "LINES", 0, 0,
 "changes the number of screen lines", &serverModuleHandle };

CONSOLE_PARSER nmiDebugEntryParser = {
  0, 0, nmiDebugEntry, nmiDebugEntryHelp, 0, "NMIDEBUG", 0, 0,
 "signal a processor (w/NMI) to enter the debugger", &serverModuleHandle };

CONSOLE_PARSER debugEntryParser = {
  0, 0, debugEntry, debugEntryHelp, 0, "DEBUG", 0, 0,
 "signal a processor (w/INT3) to enter the debugger", &serverModuleHandle };

CONSOLE_PARSER debugHotKeyParser = {
  0, 0, 0, 0, 0, "ALT-D", 0, 0,
 "enter system debugger from keyboard int", &serverModuleHandle };

CONSOLE_PARSER selectScreenHotKeyParser = {
  0, 0, 0, 0, 0, "CTRL-ESC", 0, 0,
 "select an active screen to view", &serverModuleHandle };

void CommandProcess(void)
{

    register LONG retCode, count;
    BYTE buffer[100];
    register BYTE *commandLine;
    BYTE verbBuffer[100];
    register BYTE *verb, *pp, *vp;

    AddConsoleCommandParser(&clearConsoleScreenParser);
    AddConsoleCommandParser(&changeScreenSizeParser);
    AddConsoleCommandParser(&displayMemoryInfoParser);
    AddConsoleCommandParser(&displaySpeedInfoParser);
    AddConsoleCommandParser(&displayVersionInfoParser);
    AddConsoleCommandParser(&activateSystemMonitorParser);
    AddConsoleCommandParser(&displayExportsParser);
    AddConsoleCommandParser(&displayModulesParser);
    AddConsoleCommandParser(&loadFileModuleParser);
    AddConsoleCommandParser(&unloadFileModuleParser);
    AddConsoleCommandParser(&pedumpModuleParser);
    AddConsoleCommandParser(&dumpModuleParser);
    AddConsoleCommandParser(&displayProcessInfoParser);
    AddConsoleCommandParser(&exitSystemParser);
    AddConsoleCommandParser(&quitSystemParser);
    AddConsoleCommandParser(&displayUtilizationParser);
    AddConsoleCommandParser(&onlineProcessorParser);
    AddConsoleCommandParser(&offlineProcessorParser);
    AddConsoleCommandParser(&displayProcessorStatusParser);
    AddConsoleCommandParser(&nmiDebugEntryParser);
    AddConsoleCommandParser(&debugEntryParser);
    AddConsoleCommandParser(&debugHotKeyParser);
    AddConsoleCommandParser(&selectScreenHotKeyParser);
    AddConsoleCommandParser(&displayHelpParser);
    AddConsoleCommandParser(&displayQuestionMarkParser);
    AddConsoleCommandParser(&createTestThreadParser);
    AddConsoleCommandParser(&killTestThreadsParser);

    printfScreen(consoleScreen, "\n");
    printfScreen(consoleScreen, "Timpanogas Research Group, Inc. (R) Metropolitan Area Network OS\n");
    printfScreen(consoleScreen, "Copyright (C) 1997, 1998  Timpanogas Research Group, Inc. \n");
    printfScreen(consoleScreen, "All Rights Reserved.\n");
    printfScreen(consoleScreen, "\n");

    while (1)
    {
       printfScreen(consoleScreen, "%s: ", ServerName);
       SetData((LONG *)&buffer[0], 0, 80);
       commandLine = &buffer[0];
       ScreenInputFromKeyboard(consoleScreen, commandLine, 0, 80, 7);
       if (*commandLine)
       {
	  count = 0;
	  pp = commandLine;
	  vp = verb = &verbBuffer[0];
	  while (*pp && *pp == ' ' && count++ < 80)
	     pp++;

	  while (*pp && *pp != ' ' && *pp != '=' && count++ < 80)
	     *vp++ = *pp++;
	  *vp = '\0';

	  while (*pp && *pp == ' ' && count++ < 80)
	     pp++;

	  retCode = ConsoleParserRoutine(consoleScreen, verb, commandLine);
	  if (!retCode)
	     printfScreen(consoleScreen, "??? Unknown Command ???\n");
       }
    }
}


