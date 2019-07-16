
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  DEBCMD.C
*   DESCRIP  :  Multi-Processing Debugger Command Library for MANOS v1.0
*   DATE     :  August 6, 1998
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
#include "debcmd.h"

LONG debug_deref = 0;
LONG full_deref_toggle = 0;
LONG general_toggle = TRUE;
LONG line_info_toggle = TRUE;
LONG control_toggle = 0;
LONG segment_toggle = 0;
LONG numeric_toggle = 0;
LONG reason_toggle = TRUE;

LONG enterKeyACC(SCREEN *screen, LONG key, void *stackFrame,
		     ACCELERATOR *accel)
{
    BYTE verbBuffer[100];
    register BYTE *verb, *pp, *vp;
    register LONG i, count;
    extern LONG repeatCommand;
    extern BYTE lastCommand;
    extern BYTE lastDebugCommand[];
    extern BYTE debugCommand[];

    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    if (!debugCommand[0])
    {
       count = 0;
       pp = lastDebugCommand;
       vp = verb = &verbBuffer[0];
       while (*pp && *pp == ' ' && count++ < 80)
	  pp++;

       while (*pp && *pp != ' ' && count++ < 80)
	  *vp++ = *pp++;
       *vp = '\0';

       while (*pp && *pp == ' ' && count++ < 80)
	  pp++;

       if (!strcmp(strupr(verb), "P") || lastCommand == F8)
	  strcpy(debugCommand, "P");
       else
       if (!strcmp(strupr(verb), "T") || lastCommand == F7)
	  strcpy(debugCommand, "T");
       else
       if (!strcmp(strupr(verb), "W") || !strcmp(strupr(verb), "D")  ||
	   !strcmp(strupr(verb), "DB")|| !strcmp(strupr(verb), "DW") ||
	   !strcmp(strupr(verb), "DD")|| !strcmp(strupr(verb), "DDS")||
	   !strcmp(strupr(verb), "DS")|| !strcmp(strupr(verb), "DL") ||
	   !strcmp(strupr(verb), "U") || !strcmp(strupr(verb), "UU"))
       {
	  strcpy(debugCommand, verb);
	  repeatCommand = TRUE;
       }
    }
    return 0;

}

LONG activateRegisterDisplayACC(SCREEN *screen, LONG key, void *stackFrame,
				ACCELERATOR *accel)
{
    extern LONG registerDisplayActive;
    extern LONG lastCR0;
    extern LONG lastCR2;
    extern LONG lastCR4;
    extern StackFrame lastStackFrame;

    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    if (registerDisplayActive)
    {
       close_window(0);
       registerDisplayActive = 0;
    }
    else
    {
       registerDisplayActive = TRUE;
       lastCR0 = ReadCR0();
       lastCR2 = ReadCR2();
       lastCR4 = ReadCR4();
       CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
			sizeof(StackFrame));

       activate_window(0);
       WindowRegisterFrame(0, stackFrame);
    }
    printfScreen(screen, "\r");
    return TRUE;

}

LONG displayDebuggerHelpHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "displays general help for all commands, or help for a specific command\n");
    printfScreenWithAttribute(screen, LTCYAN, "HELP         <enter>  - list all commands\n");
    printfScreenWithAttribute(screen, LTCYAN, "HELP command <enter>  - help for a specific command\n");

    return TRUE;
}

LONG displayDebuggerHelp(SCREEN *screen, BYTE *commandLine,
			 StackFrame *stackFrame, LONG Exception,
			 DEBUGGER_PARSER *parser)
{

    register LONG count;
    BYTE verbBuffer[100];
    register BYTE *verb, *pp, *vp;

    if (stackFrame) {};
    if (Exception) {};

    commandLine = &commandLine[parser->debugCommandNameLength];
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

    DebuggerParserHelpRoutine(screen, verb, commandLine);
    return TRUE;

}

void DisplayLoaderMap(SCREEN *debugScreen)
{

     printfScreenWithAttribute(debugScreen, LTCYAN, "Loader Code Segment   :  %08X    Loader Code Length     : %d bytes\n",
			 LoaderCodeSegment, DosDataTable->CODE_VIRT_SIZE);
     printfScreenWithAttribute(debugScreen, LTCYAN, "Loader Data Segment   :  %08X    Loader Data Length     : %d bytes\n",
			 LoaderDataSegment, DosDataTable->DATA_VIRT_SIZE);
     printfScreenWithAttribute(debugScreen, LTCYAN, "Loader Code End (V)   :  %08X    Loader Data End (V)    : %08X\n",
			 LoaderCodeSegment + DosDataTable->CODE_VIRT_SIZE,
			 LoaderDataSegment + DosDataTable->DATA_VIRT_SIZE);
     printfScreenWithAttribute(debugScreen, LTCYAN, "Reloc Table Start     :  %08X    Debug Table Start      : %08X\n",
			 DosDataTable->RELOC_OFFSET, DosDataTable->DEBUG_TABLE);
     printfScreenWithAttribute(debugScreen, LTCYAN, "Reloc Table End       :  %08X    Debug Table End        : %08X\n",
			 DosDataTable->RELOC_OFFSET + DosDataTable->RELOC_SIZE,
			 DosDataTable->DEBUG_TABLE + DosDataTable->DEBUG_SIZE);
     printfScreenWithAttribute(debugScreen, LTCYAN, "High Memory Start     :  %08X    High Memory Length     : %d bytes\n",
			 StartOfHighMemory,
			 HighMemoryLength);
     printfScreenWithAttribute(debugScreen, LTCYAN, "Low Memory Start      :  %08X    Low Memory Length      : %d bytes\n",
			 StartOfLowMemory,
			 LowMemoryLength);
     printfScreenWithAttribute(debugScreen, LTCYAN, "OS Code Segment       :  %08X    OS Data Segment        : %08X\n",
			 CodeSegment,
			 DataSegment);
     printfScreenWithAttribute(debugScreen, LTCYAN, "OS Debug Segment      :  %08X    OS Debug Size          : %08X\n",
			 DebugSegment,
			 DebugSize);
     printfScreenWithAttribute(debugScreen, LTCYAN, "Startup Segment       :  %08X    Startup Length         : %08X\n",
			 StartupMemory,
			 StartupLength);

}

void DisplayDOSTable(SCREEN *debugScreen, DOS_TABLE *dos)
{

   register int i, j;

   SetPauseMode(debugScreen, debugScreen->nLines - 3);

   printfScreenWithAttribute(debugScreen, BRITEWHITE, "MS-DOS System Data Table\n");
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DOS TABLE:         ::  %8X\n", dos->DOS_TABLE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "GDT TABLE          ::  %8X\n", dos->GDT_TABLE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "GDT POINTER        ::  %8X\n", dos->GDT_POINTER);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DOS IDT TABLE      ::  %8X\n", dos->DOS_IDT_TABLE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DOS CriticalError  ::  %4X:%04X\n", dos->DOS_CE_SEGMENT, dos->DOS_CE_OFFSET);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DOS ControlC       ::  %4X:%04X\n", dos->DOS_CC_SEGMENT, dos->DOS_CC_OFFSET);
   printfScreenWithAttribute(debugScreen, LTGREEN, "\nDOS Segments\n");
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DOS DS:            ::  %8X\n", dos->DOS_DATA_SEGMENT);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DOS SS:            ::  %8X\n", dos->DOS_STACK_SEGMENT);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DOS ES:            ::  %8X\n", dos->DOS_EXTRA_SEGMENT);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DOS FS:            ::  %8X\n", dos->DOS_FS_SEGMENT);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DOS SP             ::  %8X\n", dos->DOS_STACK_OFFSET);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "SYSTEM BUS TYPE    ::  %8X  ", dos->DOS_SYSTEM_BUS_TYPE);
   printfScreenWithAttribute(debugScreen, LTGREEN, "(");
   if (dos->DOS_SYSTEM_BUS_TYPE & ISA)
      printfScreenWithAttribute(debugScreen, LTGREEN, " ISA ");
   if (dos->DOS_SYSTEM_BUS_TYPE & MCA)
      printfScreenWithAttribute(debugScreen, LTGREEN, " MCA ");
   if (dos->DOS_SYSTEM_BUS_TYPE & PS2)
      printfScreenWithAttribute(debugScreen, LTGREEN, " PS2 ");
   if (dos->DOS_SYSTEM_BUS_TYPE & EISA)
      printfScreenWithAttribute(debugScreen, LTGREEN, " EISA ");
   if (dos->DOS_SYSTEM_BUS_TYPE & PCI)
      printfScreenWithAttribute(debugScreen, LTGREEN, " PCI ");
   if (dos->DOS_SYSTEM_BUS_TYPE & PCMCIA)
      printfScreenWithAttribute(debugScreen, LTGREEN, " PCMCIA ");
   printfScreenWithAttribute(debugScreen, LTGREEN, ")\n");
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DOS CR0 VALUE      ::  %8X\n", dos->DOS_CR0);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "REAL MODE INT      ::  %8X\n", dos->REAL_MODE_INT);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DOS EXIT           ::  %8X\n", dos->DOS_EXIT);

   printfScreenWithAttribute(debugScreen, LTGREEN, "CPU VENDOR ID      ::  ");
   for (i=0; i < 12; i++)
      printfScreenWithAttribute(debugScreen, LTGREEN, "%c", dos->VENDOR_ID[i]);
   printfScreen(debugScreen, "\n");

   printfScreenWithAttribute(debugScreen, LTGREEN,
      "CPU TYPE           ::  %8X\n", dos->CPU_TYPE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "CPU MODEL          ::  %8X\n", dos->CPU_MODEL);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "CPU STEPPING       ::  %8X\n", dos->STEPPING);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "CPU ID_FLAG        ::  %8X\n", dos->ID_FLAG);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "CPU FEATURES       ::  %8X\n", dos->FEATURE_FLAGS);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "XMS ENTRY POINT    ::  %8X\n", dos->XMS_FUNCTION);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "XMS START ADDRESS  ::  %8X\n", dos->XMS_MEMORY);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "XMS BLOCK SIZE     ::  %8X  (%d)\n", dos->XMS_SIZE, dos->XMS_SIZE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "XMS HANDLE         ::  %8X\n", dos->XMS_HANDLE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "XMS BASE ADDRESS   ::  %8X\n", dos->XMS_BASE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "TOTAL HIGH MEMORY  ::  %8X  (%d)\n", dos->MEMORY_HIGH, dos->MEMORY_HIGH);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "MEMORY HIGH START  ::  %8X\n", dos->MEMORY_HIGH_START);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "TOTAL LOW MEMORY   ::  %8X  (%d)\n", dos->MEMORY_LOW, dos->MEMORY_LOW);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "MEMORY LOW START   ::  %8X\n", dos->MEMORY_LOW_START);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "EXTENDED MEM LEN   ::  %8X  (%d)\n", dos->EXTENDED_MEMORY_LEN, dos->EXTENDED_MEMORY_LEN);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "EXTENDED MEM ADDR  ::  %8X\n", dos->EXTENDED_MEMORY_ADDR);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "FP STATUS          ::  %8X\n", dos->FP_STATUS);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DOS DEFAULT DRIVE  ::  %8X\n", dos->DOS_DEFAULT_DRIVE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "MASK PIC1 (21h)    ::  %8X\n", dos->MASK_8259_A);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "MASK PIC2 (A1h)    ::  %8X\n", dos->MASK_8259_B);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "FPU TYPE           ::  %8X\n", dos->FPU_TYPE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "INTEL PROC         ::  %8X\n", dos->INTEL_PROC);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "RESERVED           ::  %8X\n", dos->RESERVED);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "LINE 20 ON         ::  %8X\n", dos->LINE_20_ON);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "LINE 20 OFF        ::  %8X\n", dos->LINE_20_OFF);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "PM STACK           ::  %8X\n", dos->PM_STACK);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "PM CODE SEGMENT    ::  %8X\n", dos->PM_CODE_SEGMENT);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "PM DATA SEGMENT    ::  %8X\n", dos->PM_DATA_SEGMENT);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "JUMP16 SEGMENT     ::  %8X\n", dos->JUMP16_SEGMENT);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "CREATE FILE        ::  %8X\n", dos->MSDOS_CREATE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "OPEN FILE          ::  %8X\n", dos->MSDOS_OPEN);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "LSEEK FILE         ::  %8X\n", dos->MSDOS_LSEEK);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "READ FILE          ::  %8X\n", dos->MSDOS_READ);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "WRITE FILE         ::  %8X\n", dos->MSDOS_WRITE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "CLOSE FILE         ::  %8X\n", dos->MSDOS_CLOSE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DELETE FILE        ::  %8X\n", dos->MSDOS_UNLINK);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "PE HEADER ADDR     ::  %8X\n", dos->PE_HEADER_ADDR);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "RELOC OFFSET       ::  %8X\n", dos->RELOC_OFFSET);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "RELOC32 OFFSET     ::  %8X\n", dos->RELOC32_OFFSET);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "CODE RVA           ::  %8X\n", dos->CODE_RVA);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DATA RVA           ::  %8X\n", dos->DATA_RVA);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "CODE SIZE          ::  %8X\n", dos->CODE_SIZE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DATA SIZE          ::  %8X\n", dos->DATA_SIZE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "VIDEO ADDRESS      ::  %8X\n", dos->VIDEO_ADDRESS);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "VIDEO CURSOR MODE  ::  %8X\n", dos->VIDEO_CURSOR_MODE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "VIDEO PORT ADDRESS ::  %8X\n", dos->VIDEO_PORT_ADDRESS);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "SCREEN TYPE        ::  %8X\n", dos->VIDEO_SCREEN_TYPE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "COLOR FLAG         ::  %8X\n", dos->VIDEO_COLOR_FLAG);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "START_CODE_16      ::  %8X\n", dos->START_CODE_16);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "END_CODE_16        ::  %8X\n", dos->END_CODE_16);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "CODE_16_ENTRY      ::  %8X\n", dos->CODE_16_ENTRY);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DEBUG_TABLE        ::  %8X\n", dos->DEBUG_TABLE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DEBUG_SIZE         ::  %8X\n", dos->DEBUG_SIZE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "STARTUP_SEGMENT    ::  %8X\n", dos->STARTUP_SEGMENT);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "STARTUP_CODE       ::  %8X\n", dos->STARTUP_CODE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "STARTUP_JUMP       ::  %8X\n", dos->STARTUP_JUMP);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DEFAULT_DIRECTORY  ::  %8X\n", dos->CURRENT_DIRECTORY);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DISPLAY CODE       ::  %8X\n", dos->DISPLAY_CODE);
   printfScreenWithAttribute(debugScreen, LTGREEN,
      "DISPLAY STATE      ::  %8X\n", &dos->DISPLAY_STATE[0]);
   for (i=0; i < 64; i++)
   {
      printfScreenWithAttribute(debugScreen, LTGREEN, "%02X", dos->DISPLAY_STATE[i]);
   }
   printfScreen(debugScreen, "\n");

   ClearPauseMode(debugScreen);
   return;

}




void DisplayASCIITable(SCREEN *screen)
{

    register LONG i;
    union bhex
    {
       unsigned int i;
       struct btemp {
	     unsigned one : 1;
	     unsigned two : 1;
	     unsigned three : 1;
	     unsigned four : 1;
	     unsigned five : 1;
	     unsigned six : 1;
	     unsigned seven : 1;
	     unsigned eight : 1;
       } b;
    } val;

    SetPauseMode(screen, screen->nLines - 3);

    printfScreenWithAttribute(screen, BRITEWHITE, "ASCII Table\n");
    for (i=0; i < 256; i++)
    {
       val.i = i;
       switch (i)
       {

	  case 0:
	     if (printfScreenWithAttribute(screen, LTCYAN, "|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  | NULL  |", i, i,
				       val.b.eight, val.b.seven, val.b.six, val.b.five,
				       val.b.four, val.b.three, val.b.two, val.b.one))
		return;
	     break;

	  case 8:
	     if (printfScreenWithAttribute(screen, LTCYAN, "|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  | BKSP  |", i, i,
					      val.b.eight, val.b.seven, val.b.six, val.b.five,
					      val.b.four, val.b.three, val.b.two, val.b.one))
		return;
	     break;

	  case 9:
	     if (printfScreenWithAttribute(screen, LTCYAN, "|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  | TAB   |", i, i,
					      val.b.eight, val.b.seven, val.b.six, val.b.five,
					      val.b.four, val.b.three, val.b.two, val.b.one))
		break;
	     break;

	  case 10:
	     if (printfScreenWithAttribute(screen, LTCYAN, "|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  | <CR>  |", i, i,
					      val.b.eight, val.b.seven, val.b.six, val.b.five,
					      val.b.four, val.b.three, val.b.two, val.b.one))
		return;
	     break;

	  case 13:
	     if (printfScreenWithAttribute(screen, LTCYAN, "|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  | <LF>  |", i, i,
					      val.b.eight, val.b.seven, val.b.six, val.b.five,
					      val.b.four, val.b.three, val.b.two, val.b.one))
		return;
	     break;

	  case 32:
	     if (printfScreenWithAttribute(screen, LTCYAN, "|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  | SPACE |", i, i,
					      val.b.eight, val.b.seven, val.b.six, val.b.five,
					      val.b.four, val.b.three, val.b.two, val.b.one))
		return;
	     break;

	  default:
	     if (printfScreenWithAttribute(screen, LTCYAN, "|  %3i  |  (0x%02X)  |  (%1d%1d%1d%1d%1d%1d%1d%1db)  |  %c    |", i, i,
					      val.b.eight, val.b.seven, val.b.six, val.b.five,
					      val.b.four, val.b.three, val.b.two, val.b.one, (BYTE) i))
		return;
	     break;

       }
       if (printfScreen(screen, "\n"))
	  return;
    }

    ClearPauseMode(screen);

}

LONG ascTableHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "a                        - display ASCII Table\n");
    return TRUE;
}

// A

LONG displayASCTable(SCREEN *screen, BYTE *cmd,
		     StackFrame *stackFrame, LONG Exception,
		     DEBUGGER_PARSER *parser)
{

     register SCREEN *displayScreen;
     register LONG address;
     LONG valid;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     DisplayASCIITable(screen);
     return TRUE;

}


LONG disassemble(SCREEN *screen, StackFrame *stackFrame, LONG p, LONG count, LONG attr, LONG use)
{

    register LONG i;
    LONG exact = 0;
    BYTE *symbolName;
    LONG debugInfoPresent;
    MODULE_HANDLE *executeModuleHandle;
    SOURCE_LINE_INFO lineInfo;
    extern LONG totalLines;
    extern LONG line_info_toggle;

    for (i=0; i < count; i++)
    {
       GetLineInfoFromValue(p, &lineInfo, &executeModuleHandle, &totalLines,
			     &debugInfoPresent, &exact);

       if (line_info_toggle && exact)
       {
	  if (lineInfo.SourcePresent && lineInfo.SourceLine)
	  {
	     register LONG length = strlen(lineInfo.SourceLine);

	     i = length > screen->nCols - 1 ? i + 1 + (length / screen->nCols - 1) : i + 1;
	     printfScreenWithAttribute(screen, LTGREEN, "%s (%s : line %d)\n",
				 lineInfo.SourceLine, lineInfo.ModuleName,
				 lineInfo.LineNumber);

#if (SYMBOL_DEBUG)
	     printfScreenWithAttribute(screen, LTGREEN, "seg: %d  Mndx: %d tbl: %08X addr: %08X \noff: %08X sline: %08X mod: %s line: %d\n",
				 lineInfo.Segment, lineInfo.ModuleIndex,
				 lineInfo.LineTable, lineInfo.LineAddress,
				 lineInfo.Offset, lineInfo.SourceLine,
				 lineInfo.ModuleName, lineInfo.LineNumber);
#endif

	  }
	  else if (line_info_toggle && lineInfo.LineNumber)
	  {
	     i++;
	     printfScreenWithAttribute(screen, BRITEWHITE, "file %s  line %d\n",
				 lineInfo.ModuleName, lineInfo.LineNumber);
	  }
       }

       if (i >= count && count != 1)
	  break;

       symbolName = GetSymbolFromValue(p);
       if (symbolName)
       {
	  i++;
	  printfScreenWithAttribute(screen, BRITEWHITE, "%s:\n", symbolName);
       }
       if (i >= count && count != 1)
	  break;
       p = unassemble(screen, stackFrame, p, use, attr);
    }

    return p;

}

BYTE *dump(SCREEN *screen, BYTE *p, LONG count)
{

   BYTE *symbolName;
   register LONG i, r, total;
   BYTE str[9];

   printfScreenWithAttribute(screen, BRITEWHITE, "           0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

   SetPauseMode(screen, screen->nLines - 3);

   for (r=0; r < count; r++)
   {
      symbolName = GetSymbolFromValue((LONG) p);
      if (symbolName)
      {
	 printfScreenWithAttribute(screen, LTCYAN, "%s:\n", symbolName);
	 if (r++ >= count && count != 1)
	    break;
      }
      printfScreenWithAttribute(screen, BRITEWHITE, "%08X ", (LONG) p);
      for (total = 0, i=0; i < 16; i++, total++)
      {
	 printfScreenWithAttribute(screen, LTCYAN, " %02X", (BYTE) p[i]);
      }
      printfScreen(screen, "  ");
      for (i=0; i < total; i++)
      {
	 if (p[i] < 32 || p[i] > 126) printfScreenWithAttribute(screen, BRITEWHITE, ".");
	 else printfScreenWithAttribute(screen, BRITEWHITE, "%c", p[i]);
      }
      printfScreen(screen, "\n");

      p = (void *)((LONG) p + (LONG) total);
   }

   ClearPauseMode(screen);

   return p;

}

BYTE *dumpWord(SCREEN *screen, BYTE *p, LONG count)
{

   register int i, r;
   WORD *wp;
   BYTE *symbolName;
   char str[9];

   SetPauseMode(screen, screen->nLines - 3);

   wp = (WORD *) p;
   for (r=0; r < count; r++)
   {
      symbolName = GetSymbolFromValue((LONG) p);
      if (symbolName)
      {
	 printfScreenWithAttribute(screen, LTCYAN, "%s:\n", symbolName);
	 if (r++ >= count && count != 1)
	    break;
      }
      printfScreenWithAttribute(screen, BRITEWHITE, "%08X ", (LONG) p);
      for (i=0; i < (16 / 2); i++)
      {
	 printfScreenWithAttribute(screen, LTCYAN, " %04X", wp[i]);
      }
      printfScreen(screen, "  ");
      for (i=0; i < 16; i++)
      {
	 if (p[i] < 32 || p[i] > 126) printfScreenWithAttribute(screen, BRITEWHITE, ".");
	 else printfScreenWithAttribute(screen, BRITEWHITE, "%c", p[i]);
      }
      printfScreen(screen, "\n");

      p = (void *)((LONG) p + (LONG) 16);
      wp = (WORD *) p;
   }

   ClearPauseMode(screen);

   return p;

}

BYTE *dumpDouble(SCREEN *screen, BYTE *p, LONG count)
{

   register int i, r;
   LONG *lp;
   BYTE *symbolName;
   char str[9];

   SetPauseMode(screen, screen->nLines - 3);

   lp = (LONG *) p;

   for (r=0; r < count; r++)
   {
      symbolName = GetSymbolFromValue((LONG) p);
      if (symbolName)
      {
	 printfScreenWithAttribute(screen, LTCYAN, "%s:\n", symbolName);
	 if (r++ >= count && count != 1)
	    break;
      }
      printfScreenWithAttribute(screen, BRITEWHITE, "%08X ", (LONG) p);
      for (i=0; i < (16 / 4); i++)
      {
	 printfScreenWithAttribute(screen, LTCYAN, " %08X", (LONG) lp[i]);
      }
      printfScreen(screen, "  ");
      for (i=0; i < 16; i++)
      {
	 if (p[i] < 32 || p[i] > 126) printfScreenWithAttribute(screen, BRITEWHITE, ".");
	 else printfScreenWithAttribute(screen, BRITEWHITE, "%c", p[i]);
      }
      printfScreen(screen, "\n");

      p = (void *)((LONG) p + (LONG) 16);
      lp = (LONG *) p;
   }

   ClearPauseMode(screen);

   return p;

}

BYTE *dumpLinkedList(SCREEN *screen, BYTE *p, LONG count)
{

   register int i, r;
   LONG *lp;
   char str[9];

   lp = (LONG *) p;

   printfScreenWithAttribute(screen, BRITEWHITE, "           0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
   printfScreenWithAttribute(screen, BRITEWHITE, "Linked List -> [%08X] = %08X\n", lp, (*lp));

   SetPauseMode(screen, screen->nLines - 3);

   for (r=0; r < count; r++)
   {
      printfScreenWithAttribute(screen, BRITEWHITE, "%08X ", (LONG) p);
      for (i=0; i < 16; i++)
      {
	 printfScreenWithAttribute(screen, LTCYAN, " %02X", (BYTE) p[i]);
      }
      printfScreen(screen, "  ");
      for (i=0; i < 16; i++)
      {
	 if (p[i] < 32 || p[i] > 126) printfScreenWithAttribute(screen, BRITEWHITE, ".");
	 else printfScreenWithAttribute(screen, BRITEWHITE, "%c", p[i]);
      }
      printfScreen(screen, "\n");

      p = (void *)((LONG) p + (LONG) 16);
   }

   ClearPauseMode(screen);

   return (BYTE *) (*lp);

}

BYTE *dumpDoubleStack(SCREEN *screen, StackFrame *stackFrame, BYTE *p, LONG count)
{

   register int i, r;
   LONG *lp;
   char str[9];

   SetPauseMode(screen, screen->nLines - 3);

   lp = (LONG *) p;

   printfScreenWithAttribute(screen, BRITEWHITE, "Stack SS:ESP = %04lX:%08X\n", stackFrame->tSS, p);

   for (r=0; r < count; r++)
   {
      printfScreenWithAttribute(screen, BRITEWHITE, "%04X:", (WORD) stackFrame->tSS);
      printfScreenWithAttribute(screen, BRITEWHITE, "%08X ", (LONG) p);
      for (i=0; i < (16 / 4); i++)
      {
	 printfScreenWithAttribute(screen, LTCYAN, " %08X", (LONG) lp[i]);
      }
      printfScreen(screen, "  ");
      for (i=0; i < 16; i++)
      {
	 if (p[i] < 32 || p[i] > 126) printfScreenWithAttribute(screen, BRITEWHITE, ".");
	 else printfScreenWithAttribute(screen, BRITEWHITE, "%c", p[i]);
      }
      printfScreen(screen, "\n");

      p = (void *)((LONG) p + (LONG) 16);
      lp = (LONG *) p;
   }

   ClearPauseMode(screen);

   return p;

}

BYTE *dumpStack(SCREEN *screen, StackFrame *stackFrame, BYTE *p, LONG count)
{

   register int i, r;
   LONG *lp;
   char str[9];

   SetPauseMode(screen, screen->nLines - 3);

   lp = (LONG *) p;

   printfScreenWithAttribute(screen, BRITEWHITE, "Stack SS:ESP = %04X:%08X\n", stackFrame->tSS, p);

   for (r=0; r < count; r++)
   {
      printfScreenWithAttribute(screen, BRITEWHITE, "%08X ", (LONG) p);
      printfScreenWithAttribute(screen, LTCYAN, "%08X ", (LONG) *lp);
      DisplayClosestSymbol(screen, *lp);
      printfScreen(screen, "\n");

      p = (void *)((LONG) p + (LONG) 4);
      lp = (LONG *) p;
   }

   ClearPauseMode(screen);

   return p;

}

LONG displayScreenStructHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, ".s or .s <address>       - display screen structure(s)\n");
    return TRUE;
}

// .S

LONG DisplayScreenStructure(SCREEN *screen, BYTE *cmd,
			    StackFrame *stackFrame, LONG Exception,
			    DEBUGGER_PARSER *parser)
{

     extern SCREEN *oldScreen;
     register SCREEN *displayScreen;
     register LONG address;
     LONG valid;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     address = EvaluateExpression(stackFrame, &cmd, &valid);
     if (valid)
	DisplayScreen(screen, (SCREEN *) address);
     else
     {
	SetPauseMode(screen, screen->nLines - 3);
	printfScreenWithAttribute(screen, BRITEWHITE, "Keyboard Owner: %08X\n",
				  keyboardOwner);
	printfScreenWithAttribute(screen, BRITEWHITE, "Screen List\n");
	displayScreen = screenListHead;
	while (displayScreen)
	{
	   if (printfScreenWithAttribute(screen, YELLOW, "Screen: %08X  %s  %s\n", displayScreen,
					    displayScreen->screenName,
					    displayScreen == oldScreen
					    ? "(RUNNING_ACTIVE)"
					    : displayScreen == activeScreen
					    ? "(DEBUG_ACTIVE)"
					    : ""))
	      break;
	   displayScreen = displayScreen->next;
	}
	ClearPauseMode(screen);
     }
     return TRUE;

}


LONG displayToggleHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, ".tc                      - toggles control registers (ON | OFF)\n");
    printfScreenWithAttribute(screen, LTCYAN, ".tn                      - toggles coprocessor registers (ON | OFF)\n");
    printfScreenWithAttribute(screen, LTCYAN, ".ts                      - toggles segment registers (ON | OFF)\n");
    printfScreenWithAttribute(screen, LTCYAN, ".tg                      - toggles general registers (ON | OFF)\n");
    printfScreenWithAttribute(screen, LTCYAN, ".tr                      - toggles display of break reason (ON | OFF)\n");
    printfScreenWithAttribute(screen, LTCYAN, ".td                      - toggles full dereference display (ON | OFF)\n");
    printfScreenWithAttribute(screen, LTCYAN, ".tl                      - toggles source line display (ON | OFF)\n");
    printfScreenWithAttribute(screen, LTCYAN, ".tu                      - toggles unasm debug display (ON | OFF)\n");
    printfScreenWithAttribute(screen, LTCYAN, ".t or .t <address>       - display task state segment (tss)\n");
    return TRUE;
}

// .TU

LONG ProcessTUToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     (debug_deref)
     ? (debug_deref = 0)
     : (debug_deref = 1);
     printfScreenWithAttribute(screen, BRITEWHITE, "toggle unasm debug display (%s)\n",
				  debug_deref ? "ON" : "OFF");
     return TRUE;
}

// .TD

LONG ProcessTDToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     (full_deref_toggle)
     ? (full_deref_toggle = 0)
     : (full_deref_toggle = 1);
     printfScreenWithAttribute(screen, BRITEWHITE, "toggle full dereferencing info (%s) \n",
					    full_deref_toggle ? "ON" : "OFF");
     return TRUE;
}


// .TL

LONG ProcessTLToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     (line_info_toggle)
     ? (line_info_toggle = 0)
     : (line_info_toggle = 1);
     printfScreenWithAttribute(screen, BRITEWHITE, "toggle source line info (%s) \n",
					    line_info_toggle ? "ON" : "OFF");
     return TRUE;

}

// .TG

LONG ProcessTGToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     (general_toggle)
     ? (general_toggle = 0)
     : (general_toggle = 1);
     printfScreenWithAttribute(screen, BRITEWHITE, "toggle general registers (%s) \n",
					    general_toggle ? "ON" : "OFF");
     return TRUE;

}

// .TC

LONG ProcessTCToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     (control_toggle)
     ? (control_toggle = 0)
     : (control_toggle = 1);
     printfScreenWithAttribute(screen, BRITEWHITE, "toggle control registers (%s) \n",
					    control_toggle ? "ON" : "OFF");
     return TRUE;

}

// .TN

LONG ProcessTNToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     (numeric_toggle)
     ? (numeric_toggle = 0)
     : (numeric_toggle = 1);
     printfScreenWithAttribute(screen, BRITEWHITE, "toggle coprocessor registers (%s) \n",
					    numeric_toggle ? "ON" : "OFF");
     return TRUE;

}

// .TR

LONG ProcessTRToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     (reason_toggle)
     ? (reason_toggle = 0)
     : (reason_toggle = 1);
     printfScreenWithAttribute(screen, BRITEWHITE, "toggle display break reason (%s) \n",
					    reason_toggle ? "ON" : "OFF");
     return TRUE;

}

// .TS

LONG ProcessTSToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     (segment_toggle)
     ? (segment_toggle = 0)
     : (segment_toggle = 1);
     printfScreenWithAttribute(screen, BRITEWHITE, "toggle segment registers (%s) \n",
					    segment_toggle ? "ON" : "OFF");
     return TRUE;

}

// .TA

LONG ProcessTAToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     (general_toggle)
     ? (general_toggle = 0)
     : (general_toggle = 1);
     printfScreenWithAttribute(screen, BRITEWHITE, "toggle general registers (%s) \n",
					    general_toggle ? "ON" : "OFF");
     (control_toggle)
     ? (control_toggle = 0)
     : (control_toggle = 1);
     printfScreenWithAttribute(screen, BRITEWHITE, "toggle control registers (%s) \n",
					    control_toggle ? "ON" : "OFF");
     (segment_toggle)
     ? (segment_toggle = 0)
     : (segment_toggle = 1);
     printfScreenWithAttribute(screen, BRITEWHITE, "toggle segment registers (%s) \n",
					    segment_toggle ? "ON" : "OFF");
     return TRUE;

}

// .T

LONG ProcessTToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG address;
     LONG valid;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     address = EvaluateExpression(stackFrame, &cmd, &valid);
     if (!valid)
	DisplayTSS(screen, stackFrame);
     else
	DisplayTSS(screen, (StackFrame *) address);

     return TRUE;
}

LONG displayDebuggerVersionHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, ".v                       - display version info\n");
    return TRUE;
}

// .V

LONG DisplayDebuggerVersion(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     extern LONG MajorVersion;
     extern LONG MinorVersion;
     extern LONG BuildVersion;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     printfScreenWithAttribute(screen, LTCYAN, "Metropolitan Area Network Operating System\n");
     printfScreenWithAttribute(screen, LTCYAN, "v%02d.%02d.%02d\n", MajorVersion, MinorVersion, BuildVersion);
     printfScreenWithAttribute(screen, LTCYAN, "Copyright (C) 1997, 1998 Timpanogas Research Group, Inc.\n");
     printfScreenWithAttribute(screen, LTCYAN, "All Rights Reserved.\n");

     return TRUE;
}

LONG displayKernelProcessHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, ".kp or .kp <address>     - display kernel processes or a selected process\n");
    printfScreenWithAttribute(screen, LTCYAN, ".p  or .p  <address>     - display kernel processes or a selected process\n");
    return TRUE;

}

// .KP

LONG displayKernelProcess(SCREEN *screen, BYTE *cmd,
			  StackFrame *stackFrame, LONG Exception,
			  DEBUGGER_PARSER *parser)
{

     register PROCESS *process;
     register LONG value;
     LONG valid;
     extern BYTE *processState[];

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     value = EvaluateExpression(stackFrame, &cmd, &valid);
     if (valid)
     {
	process = (PROCESS *) value;
	if (process->processSignature != PROCESS_SIGNATURE)
	{
	   printfScreenWithAttribute(screen, BRITEWHITE,
					"process %08X has an invalid signature\n", process);
	   return TRUE;
	}
	printfScreenWithAttribute(screen, LTGREEN,
				    "Process: %08X  %s (%s p:%i)\n",
				    process,
				    process->processName,
				    processState[process->threadState & 0xF],
				    process->lastProcessor);

	printfScreenWithAttribute(screen, LTGREEN,
				    "stackPointer : %08X    startAddress : %08X\n",
				    process->stackPointer, process->startAddress);
	printfScreenWithAttribute(screen, LTGREEN,
				    "stackEnd     : %08X    stackLimit   : %08X\n",
				    process->stackEnd, process->stackLimit);
	printfScreenWithAttribute(screen, LTGREEN,
				    "refCount     : %08X    utilization  : %08X\n",
				    process->refCount, process->utilizationCount);
	printfScreenWithAttribute(screen, BRITEWHITE, "Stack SS:ESP = %04lX:%08X (byte)\n", stackFrame->tSS,
				  stackFrame->tESP);
	dump(screen, (BYTE *)process->stackPointer, 7);
	printfScreenWithAttribute(screen, BRITEWHITE, "Stack SS:ESP = %04lX:%08X (double)\n", stackFrame->tSS,
				  stackFrame->tESP);
	dumpDouble(screen, (BYTE *)process->stackPointer, 7);
     }
     else
     {
	SetPauseMode(screen, screen->nLines - 3);
	process = systemThreadHead;
	while (process)
	{
	   LONG p;

	   p = process->lastProcessor;

	   if (printfScreenWithAttribute(screen, LTGREEN,
				    "%08X:%08X | %s | p%02i | %s | %s\n",
				    process,
				    process->stackPointer,
				    processState[process->threadState & 0xF],
				    process->lastProcessor,
				    (process == processorSet[p].running_process) ? "RUN " : "IDLE",
				    process->processName))
	      break;
	   process = process->kernelNext;
	}
	ClearPauseMode(screen);
     }
     return TRUE;

}

LONG displayKernelQueueHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, ".k or .k [p#]            - display kernel queues or a processor queue\n");
    return TRUE;
}

// .K

LONG displayKernelQueue(SCREEN *screen, BYTE *cmd,
			StackFrame *stackFrame, LONG Exception,
			DEBUGGER_PARSER *parser)
{
     register LONG pnum;
     LONG valid;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     pnum = EvaluateExpression(stackFrame, &cmd, &valid);
     if (valid && pnum < num_procs)
	DisplayKernel(screen, pnum);
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid processor specified\n");
     return TRUE;

}

LONG displaySymbolsHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, ".z or .z [%%s]            - display symbol(s)\n");
    return TRUE;
}

// .Z

LONG displaySymbols(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     if (*cmd)
	DumpOSSymbolTableMatch(screen, cmd);
     else
	DumpOSSymbolTable(screen);

     return TRUE;
}


LONG displayLoaderMapHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, ".l                       - display loader memory map\n");
    return TRUE;
}

// .L

LONG displayLoaderMap(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     DisplayLoaderMap(screen);
     return TRUE;
}


LONG displayModuleHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, ".m or .m <address>       - display loaded module(s) or a specific module\n");
    return TRUE;
}

// .M

LONG displayModuleInfo(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser)
{
     register LONG address;
     LONG valid;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     address = EvaluateExpression(stackFrame, &cmd, &valid);
     if (valid)
	DisplayModule(screen, LTCYAN, address);
     else
	DisplayModuleMap(screen, LTCYAN, 1);

     return TRUE;
}

LONG displayProcessesHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, ".kp or .kp <address>     - display kernel processes or a selected process\n");
    printfScreenWithAttribute(screen, LTCYAN, ".p  or .p  <address>     - display kernel processes or a selected process\n");
    return TRUE;
}

// .P

LONG displayProcesses(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser)
{

     register PROCESS *process;
     register LONG value;
     LONG valid;
     extern BYTE *processState[];

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     value = EvaluateExpression(stackFrame, &cmd, &valid);
     if (valid)
     {
	process = (PROCESS *) value;
	if (process->processSignature != PROCESS_SIGNATURE)
	{
	   printfScreenWithAttribute(screen, BRITEWHITE,
					"process %08X has an invalid signature\n", process);
	   return TRUE;
	}
	printfScreenWithAttribute(screen, LTGREEN,
				    "Process: %08X  %s (%s p:%i)\n",
				    process,
				    process->processName,
				    processState[process->threadState & 0xF],
				    process->lastProcessor);

	printfScreenWithAttribute(screen, LTGREEN,
				    "stackPointer : %08X    startAddress : %08X\n",
				    process->stackPointer, process->startAddress);
	printfScreenWithAttribute(screen, LTGREEN,
				    "stackEnd     : %08X    stackLimit   : %08X\n",
				    process->stackEnd, process->stackLimit);
	printfScreenWithAttribute(screen, LTGREEN,
				    "refCount     : %08X    utilization  : %08X\n",
				    process->refCount, process->utilizationCount);
	printfScreenWithAttribute(screen, BRITEWHITE, "Stack SS:ESP = %04lX:%08X (byte)\n", stackFrame->tSS,
				  stackFrame->tESP);
	dump(screen, (BYTE *)process->stackPointer, 7);
	printfScreenWithAttribute(screen, BRITEWHITE, "Stack SS:ESP = %04lX:%08X (double)\n", stackFrame->tSS,
				  stackFrame->tESP);
	dumpDouble(screen, (BYTE *)process->stackPointer, 7);
     }
     else
     {
	SetPauseMode(screen, screen->nLines - 3);
	process = systemThreadHead;
	while (process)
	{
	   LONG p;

	   p = process->lastProcessor;

	   if (printfScreenWithAttribute(screen, LTGREEN,
				    "%08X:%08X | %s | p%02i | %s | %s\n",
				    process,
				    process->stackPointer,
				    processState[process->threadState & 0xF],
				    process->lastProcessor,
				    (process == processorSet[p].running_process) ? "RUN " : "IDLE",
				    process->processName))
	      break;
	   process = process->kernelNext;
	}
	ClearPauseMode(screen);
     }
     return TRUE;

}

LONG displayRegistersHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "r                        - display registers for a processor\n");
    printfScreenWithAttribute(screen, LTCYAN, "rc                       - display control registers \n");
    printfScreenWithAttribute(screen, LTCYAN, "rs                       - display segment registers \n");
    printfScreenWithAttribute(screen, LTCYAN, "rg                       - display general registers \n");
    printfScreenWithAttribute(screen, LTCYAN, "ra                       - display all registers\n");
    printfScreenWithAttribute(screen, LTCYAN, "rn                       - display coprocessor registers\n");

    return TRUE;
}

// RC

LONG displayControlRegisters(SCREEN *screen, BYTE *cmd,
			     StackFrame *stackFrame, LONG Exception,
			     DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     printfScreenWithAttribute(screen, BRITEWHITE, "Control Registers\n");
     DisplayControlRegisters(screen, get_processor_id(), stackFrame);
     return TRUE;

}

// RA

LONG displayAllRegisters(SCREEN *screen, BYTE *cmd,
			     StackFrame *stackFrame, LONG Exception,
			     DEBUGGER_PARSER *parser)
{
     register LONG processor = get_processor_id();

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     printfScreenWithAttribute(screen, BRITEWHITE, "Control Registers\n");
     DisplayControlRegisters(screen, processor, stackFrame);
     if (DosDataTable->FPU_TYPE)
     {
	printfScreenWithAttribute(screen, BRITEWHITE, "Coprocessor Registers\n");
	DisplayNPXRegisters(screen, processor);
     }
     else
     {
	printfScreenWithAttribute(screen, BRITEWHITE, "Coprocessor Not Present\n");
     }
     printfScreenWithAttribute(screen, BRITEWHITE, "Segment Registers\n");
     DisplaySegmentRegisters(screen, stackFrame);
     printfScreenWithAttribute(screen, BRITEWHITE, "General Registers\n");
     DisplayGeneralRegisters(screen, stackFrame);
     return TRUE;

}


// RS

LONG displaySegmentRegisters(SCREEN *screen, BYTE *cmd,
			     StackFrame *stackFrame, LONG Exception,
			     DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     printfScreenWithAttribute(screen, BRITEWHITE, "Segment Registers\n");
     DisplaySegmentRegisters(screen, stackFrame);
     return TRUE;

}

// RN

LONG displayNumericRegisters(SCREEN *screen, BYTE *cmd,
			     StackFrame *stackFrame, LONG Exception,
			     DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     if (DosDataTable->FPU_TYPE)
     {
	printfScreenWithAttribute(screen, BRITEWHITE, "Coprocessor Registers\n");
	DisplayNPXRegisters(screen, get_processor_id());
     }
     else
     {
	printfScreenWithAttribute(screen, BRITEWHITE, "Coprocessor Not Present\n");
     }
     return TRUE;

}

// RG

LONG displayGeneralRegisters(SCREEN *screen, BYTE *cmd,
			     StackFrame *stackFrame, LONG Exception,
			     DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     printfScreenWithAttribute(screen, BRITEWHITE, "General Registers\n");
     DisplayGeneralRegisters(screen, stackFrame);
     return TRUE;


}

// R

LONG displayDefaultRegisters(SCREEN *screen, BYTE *cmd,
			     StackFrame *stackFrame, LONG Exception,
			     DEBUGGER_PARSER *parser)
{
     extern BYTE *ExceptionDescription[];

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     printfScreenWithAttribute(screen, LTGREEN, "Break at 0x%08X due to - %s\n", stackFrame->tEIP, ExceptionDescription[Exception]);
     DisplayGeneralRegisters(screen, stackFrame);
     disassemble(screen, stackFrame, stackFrame->tEIP, 1, BRITEWHITE, 1);
     return TRUE;

}

LONG displayAPICHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "apic                     - display local apic regs\n");
    printfScreenWithAttribute(screen, LTCYAN, "apic [p#]                - display remote apic regs\n");
    return TRUE;
}

// APIC

LONG displayAPICInfo(SCREEN *screen, BYTE *cmd,
		     StackFrame *stackFrame, LONG Exception,
		     DEBUGGER_PARSER *parser)
{
     register LONG value;
     LONG valid;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     if (mps_present)
     {
	value = EvaluateExpression(stackFrame, &cmd, &valid);
	if (valid && value >= num_procs)
	{
	   printfScreenWithAttribute(screen, BRITEWHITE, "processor not found\n");
	   return TRUE;
	}
	if (valid && value != get_processor_id())
	   dump_remote_apic(screen, value);
	else
	   dump_local_apic(screen);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "mps device not found\n");
     return TRUE;
}

// LP

LONG listProcessors(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser)
{
     register LONG i;
     extern BYTE *procState[];

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     SetPauseMode(screen, screen->nLines - 3);
     for (i=0; i < num_procs; i++)
     {
	printfScreenWithAttribute(screen, YELLOW, "Processor: (%i)  State:  %s\n",
				 i, procState[processor_table[i].ProcessorState & 0xF]);
     }
     ClearPauseMode(screen);
     return TRUE;

}

// LR

LONG listProcessorFrame(SCREEN *screen, BYTE *cmd,
			StackFrame *stackFrame, LONG Exception,
			DEBUGGER_PARSER *parser)
{
     register LONG pnum;
     LONG valid;
     extern StackFrame *CurrentFrame[];

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     pnum = EvaluateExpression(stackFrame, &cmd, &valid);
     if (valid && pnum < num_procs && CurrentFrame[pnum])
     {
	printfScreenWithAttribute(screen,
				  BRITEWHITE,
				  "Processor Frame %d -> (%08X)\n",
				  pnum, CurrentFrame[pnum]);
	DisplayTSS(screen, (StackFrame *)CurrentFrame[pnum]);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE,
				 CurrentFrame[pnum]
				 ? "invalid processor\n"
				 : "invalid frame\n");
     return TRUE;

}

LONG displayMPSHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "mps                      - display MPS tables\n");
    return TRUE;
}

// MPS

LONG displayMPS(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     displayMPSTables(screen);
     return TRUE;

}

LONG clearScreenHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "cls                      - clear the screen\n");
    return TRUE;
}

// CLS

LONG clearDebuggerScreen(SCREEN *screen, BYTE *cmd,
			 StackFrame *stackFrame, LONG Exception,
			 DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     ClearScreen(screen);
     return TRUE;

}

LONG SearchMemoryHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "s                        - search for bytes at address\n");
    printfScreenWithAttribute(screen, LTCYAN, "sb                       - search for bytes at address\n");
    printfScreenWithAttribute(screen, LTCYAN, "sw                       - search for words at address\n");
    printfScreenWithAttribute(screen, LTCYAN, "sd                       - search for dwords at address\n");
    return TRUE;
}

// S

LONG SearchMemory(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     BYTE changeBuffer[100] = {""};
     BYTE searchBuffer[100] = {""};
     LONG maxlen = sizeof(searchBuffer);
     register BYTE *changeB;
     BYTE *pB;
     register LONG address, r, value, count, len, i;
     LONG valid, x, y;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     SetData((LONG *)searchBuffer, 0, sizeof(searchBuffer));
     count = 0;
     changeB = (BYTE *) searchBuffer;
     changeBuffer[0] = '\0';
     printfScreenWithAttribute(screen, BRITEWHITE, "enter bytes to search for, '.' to end input\n");
     while (changeBuffer[0] != '.' && count < maxlen)
     {
	for (r=0; r < 8; r++)
	{
	   printfScreenWithAttribute(screen, WHITE, "0x");
	   convertScreenMode(screen, SUPRESS_CRLF);
	   ScreenInputFromKeyboard(screen, &changeBuffer[0], 0, 2, WHITE);
	   convertScreenMode(screen, NORMAL_CRLF);

	   if ((changeBuffer[0] == '.') || (changeBuffer[1] == '.'))
	      break;

	   pB = (BYTE *) &changeBuffer[0];
	   len = strlen(pB);
	   for (i=0; i < len; i++)
	      printfScreen(screen, "\b");

	   value = EvaluateExpression(0, &pB, &valid);
	   if (valid)
	      *changeB = (BYTE) value;
	   printfScreenWithAttribute(screen, YELLOW, "%02X ", (BYTE) *changeB);

	   changeB++;
	   if (count++ > maxlen)
	      break;
	}
	printfScreen(screen, "\n");
     }

     if (count)
     {
	printfScreenWithAttribute(screen, BRITEWHITE, "enter start address for search:  ");
	ScreenInputFromKeyboard(screen, &changeBuffer[0], 0, 8, WHITE);
	pB = (BYTE *) &changeBuffer[0];
	address = EvaluateExpression(0, &pB, &valid);
	if (valid)
	{
	   extern long memcmp(void *s, void *d, LONG len);
	   extern LONG TotalSystemMemory;
	   register LONG key;

	   printfScreenWithAttribute(screen, BRITEWHITE, "start address = [%08X]\n", address);
	   printfScreenWithAttribute(screen, BRITEWHITE, "\nsearching ");
	   while (address < TotalSystemMemory)
	   {
	      if (!memcmp(searchBuffer, (void *)address, count))
	      {
		 printfScreenWithAttribute(screen, BRITEWHITE, "\nmatch at address [%08X]\n", address);
		 dump(screen, (BYTE *)address, 4);
		 printfScreenWithAttribute(screen, BRITEWHITE, "<enter> to continue or ESC to halt search");
		 key = GetKey();
		 if (key == ESC)
		    break;
		 printfScreenWithAttribute(screen, BRITEWHITE, "\r                                         \r");
		 printfScreenWithAttribute(screen, BRITEWHITE, "\nsearching ");
	      }
	      address++;
	      if (!(address % 0x100000))
		  printfScreenWithAttribute(screen, BRITEWHITE, ".");
	   }
	   printfScreenWithAttribute(screen, BRITEWHITE, "\nsearch completed.\n");
	   return TRUE;
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid start address\n");
	return TRUE;
     }
     printfScreenWithAttribute(screen, BRITEWHITE, "no search pattern\n");
     return TRUE;

}

// SB

LONG SearchMemoryB(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     BYTE changeBuffer[100] = {""};
     BYTE searchBuffer[100] = {""};
     LONG maxlen = sizeof(searchBuffer);
     register BYTE *changeB;
     BYTE *pB;
     register LONG address, r, value, count, len, i;
     LONG valid, x, y;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     SetData((LONG *)searchBuffer, 0, sizeof(searchBuffer));
     count = 0;
     changeB = (BYTE *) searchBuffer;
     changeBuffer[0] = '\0';
     printfScreenWithAttribute(screen, BRITEWHITE, "enter bytes to search for, '.' to end input\n");
     while (changeBuffer[0] != '.' && count < maxlen)
     {
	for (r=0; r < 8; r++)
	{
	   printfScreenWithAttribute(screen, WHITE, "0x");
	   convertScreenMode(screen, SUPRESS_CRLF);
	   ScreenInputFromKeyboard(screen, &changeBuffer[0], 0, 2, WHITE);
	   convertScreenMode(screen, NORMAL_CRLF);

	   if ((changeBuffer[0] == '.') || (changeBuffer[1] == '.'))
	      break;

	   pB = (BYTE *) &changeBuffer[0];
	   len = strlen(pB);
	   for (i=0; i < len; i++)
	      printfScreen(screen, "\b");

	   value = EvaluateExpression(0, &pB, &valid);
	   if (valid)
	      *changeB = (BYTE) value;
	   printfScreenWithAttribute(screen, YELLOW, "%02X ", (BYTE) *changeB);

	   changeB++;
	   if (count++ > maxlen)
	      break;
	}
	printfScreen(screen, "\n");
     }

     if (count)
     {
	printfScreenWithAttribute(screen, BRITEWHITE, "enter start address for search:  ");
	ScreenInputFromKeyboard(screen, &changeBuffer[0], 0, 8, WHITE);
	pB = (BYTE *) &changeBuffer[0];
	address = EvaluateExpression(0, &pB, &valid);
	if (valid)
	{
	   extern long memcmp(void *s, void *d, LONG len);
	   extern LONG TotalSystemMemory;
	   register LONG key;

	   printfScreenWithAttribute(screen, BRITEWHITE, "start address = [%08X]\n", address);
	   printfScreenWithAttribute(screen, BRITEWHITE, "\nsearching ");
	   while (address < TotalSystemMemory)
	   {
	      if (!memcmp(searchBuffer, (void *)address, count))
	      {
		 printfScreenWithAttribute(screen, BRITEWHITE, "\nmatch at address [%08X]\n", address);
		 dump(screen, (BYTE *)address, 4);
		 printfScreenWithAttribute(screen, BRITEWHITE, "<enter> to continue or ESC to halt search");
		 key = GetKey();
		 if (key == ESC)
		    break;
		 printfScreenWithAttribute(screen, BRITEWHITE, "\r                                         \r");
		 printfScreenWithAttribute(screen, BRITEWHITE, "\nsearching ");
	      }
	      address++;
	      if (!(address % 0x100000))
		 printfScreenWithAttribute(screen, BRITEWHITE, ".");
	   }
	   printfScreenWithAttribute(screen, BRITEWHITE, "\nsearch completed.\n");
	   return TRUE;
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid start address\n");
	return TRUE;
     }
     printfScreenWithAttribute(screen, BRITEWHITE, "no search pattern\n");
     return TRUE;
}

// SW

LONG SearchMemoryW(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     BYTE changeBuffer[100] = {""};
     WORD searchBuffer[10];
     LONG maxlen = sizeof(searchBuffer) / sizeof(WORD);
     register WORD *changeW;
     BYTE *pB;
     register LONG address, r, value, count, len, i;
     LONG valid, x, y;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     SetData((LONG *)searchBuffer, 0, sizeof(searchBuffer));
     count = 0;
     changeW = (WORD *) searchBuffer;
     changeBuffer[0] = '\0';
     printfScreenWithAttribute(screen, BRITEWHITE, "enter words to search for, '.' to end input\n");
     while (changeBuffer[0] != '.' && count < maxlen)
     {
	for (r=0; r < 4; r++)
	{
	   printfScreenWithAttribute(screen, WHITE, "0x");
	   convertScreenMode(screen, SUPRESS_CRLF);
	   ScreenInputFromKeyboard(screen, &changeBuffer[0], 0, 4, WHITE);
	   convertScreenMode(screen, NORMAL_CRLF);

	   if ((changeBuffer[0] == '.') || (changeBuffer[1] == '.') ||
	       (changeBuffer[2] == '.') || (changeBuffer[3] == '.'))
	      break;

	   pB = (BYTE *) &changeBuffer[0];
	   len = strlen(pB);
	   for (i=0; i < len; i++)
	      printfScreen(screen, "\b");

	   value = EvaluateExpression(0, &pB, &valid);
	   if (valid)
	      *changeW = value;
	   printfScreenWithAttribute(screen, YELLOW, "%04X ", *changeW);

	   changeW++;
	   if (count++ > maxlen)
	      break;
	}
	printfScreen(screen, "\n");
     }

     if (count)
     {
	printfScreenWithAttribute(screen, BRITEWHITE, "enter start address for search:  ");
	ScreenInputFromKeyboard(screen, &changeBuffer[0], 0, 8, WHITE);
	pB = (BYTE *) &changeBuffer[0];
	address = EvaluateExpression(0, &pB, &valid);
	if (valid)
	{
	   extern long memcmp(void *s, void *d, LONG len);
	   extern LONG TotalSystemMemory;
	   register LONG key;

	   printfScreenWithAttribute(screen, BRITEWHITE, "start address = [%08X]\n", address);
	   printfScreenWithAttribute(screen, BRITEWHITE, "\nsearching ");
	   while (address < TotalSystemMemory)
	   {
	      if (!memcmp(searchBuffer, (void *)address, count * sizeof(WORD)))
	      {
		 printfScreenWithAttribute(screen, BRITEWHITE, "\nmatch at address [%08X]\n", address);
		 dumpWord(screen, (BYTE *)address, 4);
		 printfScreenWithAttribute(screen, BRITEWHITE, "<enter> to continue or ESC to halt search");
		 key = GetKey();
		 if (key == ESC)
		    break;
		 printfScreenWithAttribute(screen, BRITEWHITE, "\r                                         \r");
		 printfScreenWithAttribute(screen, BRITEWHITE, "\nsearching ");
	      }
	      address++;
	      if (!(address % 0x100000))
		 printfScreenWithAttribute(screen, BRITEWHITE, ".");
	   }
	   printfScreenWithAttribute(screen, BRITEWHITE, "\nsearch completed.\n");
	   return TRUE;
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid start address\n");
	return TRUE;
     }
     printfScreenWithAttribute(screen, BRITEWHITE, "no search pattern\n");
     return TRUE;
}

// SD

LONG SearchMemoryD(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     BYTE changeBuffer[100] = {""};
     LONG searchBuffer[10];
     LONG maxlen = sizeof(searchBuffer) / sizeof(LONG);
     register LONG *changeD;
     BYTE *pB;
     register LONG address, r, value, count, len, i;
     LONG valid, x, y;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     SetData((LONG *)searchBuffer, 0, sizeof(searchBuffer));
     count = 0;
     changeD = (LONG *) searchBuffer;
     changeBuffer[0] = '\0';
     printfScreenWithAttribute(screen, BRITEWHITE, "enter dwords to search for, '.' to end input\n");
     while (changeBuffer[0] != '.' && count < maxlen)
     {
	for (r=0; r < 2; r++)
	{
	   printfScreenWithAttribute(screen, WHITE, "0x");
	   convertScreenMode(screen, SUPRESS_CRLF);
	   ScreenInputFromKeyboard(screen, &changeBuffer[0], 0, 8, WHITE);
	   convertScreenMode(screen, NORMAL_CRLF);

	   if ((changeBuffer[0] == '.') || (changeBuffer[1] == '.') ||
	       (changeBuffer[2] == '.') || (changeBuffer[3] == '.') ||
	       (changeBuffer[4] == '.') || (changeBuffer[5] == '.') ||
	       (changeBuffer[6] == '.') || (changeBuffer[7] == '.'))
	      break;

	   pB = (BYTE *) &changeBuffer[0];
	   len = strlen(pB);
	   for (i=0; i < len; i++)
	      printfScreen(screen, "\b");

	   value = EvaluateExpression(0, &pB, &valid);
	   if (valid)
	      *changeD = value;
	   printfScreenWithAttribute(screen, YELLOW, "%08X ", *changeD);

	   changeD++;
	   if (count++ > maxlen)
	      break;
	}
	printfScreen(screen, "\n");
     }

     if (count)
     {
	printfScreenWithAttribute(screen, BRITEWHITE, "enter start address for search:  ");
	ScreenInputFromKeyboard(screen, &changeBuffer[0], 0, 8, WHITE);
	pB = (BYTE *) &changeBuffer[0];
	address = EvaluateExpression(0, &pB, &valid);
	if (valid)
	{
	   extern long memcmp(void *s, void *d, LONG len);
	   extern LONG TotalSystemMemory;
	   register LONG key;

	   printfScreenWithAttribute(screen, BRITEWHITE, "start address = [%08X]\n", address);
	   printfScreenWithAttribute(screen, BRITEWHITE, "\nsearching ");
	   while (address < TotalSystemMemory)
	   {
	      if (!memcmp(searchBuffer, (void *)address, count * sizeof(LONG)))
	      {
		 printfScreenWithAttribute(screen, BRITEWHITE, "\nmatch at address [%08X]\n", address);
		 dumpDouble(screen, (BYTE *)address, 4);
		 printfScreenWithAttribute(screen, BRITEWHITE, "<enter> to continue or ESC to halt search");
		 key = GetKey();
		 if (key == ESC)
		    break;
		 printfScreenWithAttribute(screen, BRITEWHITE, "\r                                         \r");
		 printfScreenWithAttribute(screen, BRITEWHITE, "\nsearching ");
	      }
	      address++;
	      if (!(address % 0x100000))
		  printfScreenWithAttribute(screen, BRITEWHITE, ".");
	   }
	   printfScreenWithAttribute(screen, BRITEWHITE, "\nsearch completed.\n");
	   return TRUE;
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid start address\n");
	return TRUE;
     }
     printfScreenWithAttribute(screen, BRITEWHITE, "no search pattern\n");
     return TRUE;
}


LONG changeMemoryHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "c   <address>            - change bytes at address\n");
    printfScreenWithAttribute(screen, LTCYAN, "cb  <address>            - change bytes at address\n");
    printfScreenWithAttribute(screen, LTCYAN, "cw  <address>            - change words at address\n");
    printfScreenWithAttribute(screen, LTCYAN, "cd  <address>            - change dwords at address\n");
    return TRUE;
}

// CW

LONG changeWordValue(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     BYTE changeBuffer[100] = {""};
     register WORD *changeW, oldW, *pW;
     BYTE *pB;
     register LONG address, r, value, len, i;
     LONG valid, x, y;

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
	changeW = (WORD *) address;
	changeBuffer[0] = '\0';
	printfScreenWithAttribute(screen, BRITEWHITE, "enter new value, <enter> to skip, or '.' to exit\n");
	while (changeBuffer[0] != '.')
	{
	   printfScreenWithAttribute(screen, LTCYAN, "[%08X] ", changeW);
	   for (r=0; r < 4; r++)
	   {
	      printfScreenWithAttribute(screen, BRITEWHITE, "(%04X)=", (WORD) *changeW);
	      oldW = (WORD) *changeW;

	      convertScreenMode(screen, SUPRESS_CRLF);
	      ScreenInputFromKeyboard(screen, &changeBuffer[0], 0, 4, WHITE);
	      convertScreenMode(screen, NORMAL_CRLF);

	      if ((changeBuffer[0] == '.') || (changeBuffer[1] == '.') ||
		  (changeBuffer[2] == '.') || (changeBuffer[3] == '.'))
		 break;
	      pB = (BYTE *) &changeBuffer[0];
	      len = strlen(pB);
	      for (i=0; i < len; i++)
		 printfScreen(screen, "\b");

	      value = EvaluateExpression(0, &pB, &valid);
	      if (valid)
		 *changeW = (WORD) value;
	      printfScreenWithAttribute(screen,
				    (oldW == *changeW) ? BRITEWHITE : YELLOW,
				    "%04X ", (WORD) *changeW);
	      changeW++;
	   }
	   printfScreen(screen, "\n");
	}
	return TRUE;
     }
     printfScreenWithAttribute(screen, BRITEWHITE, "invalid change (word) address\n");
     return TRUE;
}

// CD

LONG changeDoubleValue(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     BYTE changeBuffer[100] = {""};
     register LONG *changeD, oldD, *pD;
     register LONG address, r, value, len, i;
     BYTE *pB;
     LONG valid, x, y;

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
	changeD = (LONG *) address;
	changeBuffer[0] = '\0';
	printfScreenWithAttribute(screen, BRITEWHITE, "enter new value, <enter> to skip, or '.' to exit\n");
	while (changeBuffer[0] != '.')
	{
	   printfScreenWithAttribute(screen, LTCYAN, "[%08X] ", changeD);
	   for (r=0; r < 2; r++)
	   {
	      printfScreenWithAttribute(screen, BRITEWHITE, "(%08X)=", (LONG) *changeD);
	      oldD = (LONG) *changeD;

	      convertScreenMode(screen, SUPRESS_CRLF);
	      ScreenInputFromKeyboard(screen, &changeBuffer[0], 0, 8, WHITE);
	      convertScreenMode(screen, NORMAL_CRLF);

	      if ((changeBuffer[0] == '.') || (changeBuffer[1] == '.') ||
		  (changeBuffer[2] == '.') || (changeBuffer[3] == '.') ||
		  (changeBuffer[4] == '.') || (changeBuffer[5] == '.') ||
		  (changeBuffer[6] == '.') || (changeBuffer[7] == '.'))
		 break;
	      pB = (BYTE *) &changeBuffer[0];
	      len = strlen(pB);
	      for (i=0; i < len; i++)
		 printfScreen(screen, "\b");

	      value = EvaluateExpression(0, &pB, &valid);
	      if (valid)
		 *changeD = (LONG) value;
	      printfScreenWithAttribute(screen, (oldD == *changeD) ? BRITEWHITE : YELLOW,
				       "%08X ", (LONG) *changeD);
	      changeD++;
	   }
	   printfScreen(screen, "\n");
	}
	return TRUE;
     }
     printfScreenWithAttribute(screen, BRITEWHITE, "invalid change (dword) address\n");
     return TRUE;
}

// CB

LONG changeByteValue(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     BYTE changeBuffer[100] = {""};
     register BYTE *changeB, oldB;
     BYTE *pB;
     register LONG address, r, value, len, i;
     LONG valid, x, y;

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
	changeB = (BYTE *) address;
	changeBuffer[0] = '\0';
	printfScreenWithAttribute(screen, BRITEWHITE, "enter new value, <enter> to skip, or '.' to exit\n");
	while (changeBuffer[0] != '.')
	{
	   printfScreenWithAttribute(screen, LTCYAN, "[%08X] ", changeB);
	   for (r=0; r < 8; r++)
	   {
	      printfScreenWithAttribute(screen, BRITEWHITE, "(%02X)=", (BYTE) *changeB);
	      oldB = (BYTE) *changeB;

	      convertScreenMode(screen, SUPRESS_CRLF);
	      ScreenInputFromKeyboard(screen, &changeBuffer[0], 0, 2, WHITE);
	      convertScreenMode(screen, NORMAL_CRLF);

	      if ((changeBuffer[0] == '.') || (changeBuffer[1] == '.'))
		 break;

	      pB = (BYTE *) &changeBuffer[0];
	      len = strlen(pB);
	      for (i=0; i < len; i++)
		 printfScreen(screen, "\b");

	      value = EvaluateExpression(0, &pB, &valid);
	      if (valid)
		 *changeB = (BYTE) value;
	      printfScreenWithAttribute(screen, (oldB == *changeB) ? BRITEWHITE : YELLOW,
				       "%02X ", (BYTE) *changeB);
	      changeB++;
	   }
	   printfScreen(screen, "\n");
	}
	return TRUE;
     }
     printfScreenWithAttribute(screen, BRITEWHITE, "invalid change (byte) address\n");
     return TRUE;
}

// C

LONG changeDefaultValue(SCREEN *screen, BYTE *cmd,
			StackFrame *stackFrame, LONG Exception,
			DEBUGGER_PARSER *parser)
{
     BYTE changeBuffer[100] = {""};
     register BYTE *changeB, oldB;
     BYTE *pB;
     register LONG address, r, value, len, i;
     LONG valid, x, y;

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
	changeB = (BYTE *) address;
	changeBuffer[0] = '\0';
	printfScreenWithAttribute(screen, BRITEWHITE, "enter new value, <enter> to skip, or '.' to exit\n");
	while (changeBuffer[0] != '.')
	{
	   printfScreenWithAttribute(screen, LTCYAN, "[%08X] ", changeB);
	   for (r=0; r < 8; r++)
	   {
	      printfScreenWithAttribute(screen, BRITEWHITE, "(%02X)=", (BYTE) *changeB);
	      oldB = (BYTE) *changeB;

	      convertScreenMode(screen, SUPRESS_CRLF);
	      ScreenInputFromKeyboard(screen, &changeBuffer[0], 0, 2, WHITE);
	      convertScreenMode(screen, NORMAL_CRLF);

	      if ((changeBuffer[0] == '.') || (changeBuffer[1] == '.'))
		 break;

	      pB = (BYTE *) &changeBuffer[0];
	      len = strlen(pB);
	      for (i=0; i < len; i++)
		 printfScreen(screen, "\b");

	      value = EvaluateExpression(0, &pB, &valid);
	      if (valid)
		 *changeB = (BYTE) value;
	      printfScreenWithAttribute(screen, (oldB == *changeB) ? BRITEWHITE : YELLOW,
				       "%02X ", (BYTE) *changeB);
	      changeB++;
	   }
	   printfScreen(screen, "\n");
	}
	return TRUE;
     }
     printfScreenWithAttribute(screen, BRITEWHITE, "invalid change (byte) address\n");
     return TRUE;

}

LONG displayCloseHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "?   <address>            - display closest symbols to <address>\n");
    return TRUE;

}

// ?

LONG displayCloseSymbols(SCREEN *screen, BYTE *cmd,
			 StackFrame *stackFrame, LONG Exception,
			 DEBUGGER_PARSER *parser)
{
     register LONG oldD;
     LONG valid;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     oldD = EvaluateExpression(stackFrame, &cmd, &valid);
     if (!valid)
	oldD = stackFrame->tEIP;
     DisplayClosestSymbols(screen, oldD);
     return TRUE;

}

LONG displayINTRHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "intr                     - system interrupt table\n");
    return TRUE;
}

// INTR

LONG displayInterruptTable(SCREEN *screen, BYTE *cmd,
			   StackFrame *stackFrame, LONG Exception,
			   DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     dump_int_table(screen);
     return TRUE;

}

LONG viewScreensHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "v                        - view screens\n");
    return TRUE;
}

// V

LONG displayScreenList(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser)
{
     register SCREEN *displayScreen;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     displayScreen = screenListHead;
     while (displayScreen)
     {
	if (displayScreen != screen)
	{
	   SetVideoOwner(displayScreen);
	   GetKey();
	}
	displayScreen = displayScreen->next;
     }
     SetVideoOwner(screen);
     return TRUE;

}

LONG displayIOAPICHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "ioapic [#]               - display specified ioapic [#] regs\n");
    return TRUE;
}

// IOAPIC

LONG displayIOAPICInfo(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser)
{
     register LONG value;
     LONG valid;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     if (mps_present)
     {
	value = EvaluateExpression(stackFrame, &cmd, &valid);
	if (valid && value >= num_ioapics)
	{
	   printfScreenWithAttribute(screen, BRITEWHITE, "ioapic not found\n");
	   return TRUE;
	}
	if (valid)
	   dump_ioapic(screen, value);
	else
	   dump_ioapic(screen, 0);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "mps device not found\n");
     return TRUE;

}

LONG debuggerWalkStack(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser)
{
     LONG valid;
     extern BYTE *lastDumpAddress;
     extern LONG repeatCommand;
     extern LONG displayLength;
     extern LONG lastDisplayLength;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     if (repeatCommand)
     {
	lastDumpAddress = dumpStack(screen, stackFrame, lastDumpAddress, lastDisplayLength);
	return TRUE;
     }
     lastDumpAddress = (BYTE *) EvaluateExpression(stackFrame, &cmd, &valid);
     if (!valid)
	lastDumpAddress = (BYTE *) stackFrame->tESP;
     displayLength = EvaluateExpression(stackFrame, &cmd, &valid);
     if (!displayLength || displayLength > screen->nLines - 5)
	displayLength = screen->nLines - 5;
     lastDumpAddress = dumpStack(screen, stackFrame, lastDumpAddress, displayLength);
     return TRUE;

}

LONG displayDumpHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "d   <address> <#lines>   - dump memory as bytes\n");
    printfScreenWithAttribute(screen, LTCYAN, "dw  <address> <#lines>   - dump memory as words\n");
    printfScreenWithAttribute(screen, LTCYAN, "dd  <address> <#lines>   - dump memory as double words\n");
    printfScreenWithAttribute(screen, LTCYAN, "dl  <address> <#lines>   - dump linked list\n");
    printfScreenWithAttribute(screen, LTCYAN, "ds  <address> <#lines>   - dump stack\n");
    printfScreenWithAttribute(screen, LTCYAN, "dds <address> <#lines>   - dump stack double word\n");
    printfScreenWithAttribute(screen, LTCYAN, "w   <address>            - display symbols on the stack\n");

    return TRUE;
}

// DL

LONG debuggerDumpLinkedList(SCREEN *screen, BYTE *cmd,
			    StackFrame *stackFrame, LONG Exception,
			    DEBUGGER_PARSER *parser)
{
     LONG valid;
     extern BYTE *lastLinkAddress;
     extern LONG repeatCommand;
     extern LONG displayLength;
     extern LONG lastDisplayLength;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     if (repeatCommand)
     {
	lastLinkAddress = dumpLinkedList(screen, lastLinkAddress, lastDisplayLength);
	return TRUE;
     }
     lastLinkAddress = (BYTE *) EvaluateExpression(stackFrame, &cmd, &valid);
     if (!valid)
	lastLinkAddress = (BYTE *) stackFrame->tESP;
     displayLength = EvaluateExpression(stackFrame, &cmd, &valid);
     if (!displayLength || displayLength > screen->nLines - 5)
	displayLength = screen->nLines - 5;
     lastLinkAddress = dumpLinkedList(screen, lastLinkAddress, displayLength);
     return TRUE;

}

// DW

LONG debuggerDumpWord(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser)
{
     LONG valid;
     extern BYTE *lastDumpAddress;
     extern LONG repeatCommand;
     extern LONG displayLength;
     extern LONG lastDisplayLength;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     if (repeatCommand)
     {
	lastDumpAddress = dumpWord(screen, lastDumpAddress, lastDisplayLength);
	return TRUE;
     }
     lastDumpAddress = (BYTE *) EvaluateExpression(stackFrame, &cmd, &valid);
     if (!valid)
	lastDumpAddress = (BYTE *) stackFrame->tESP;
     displayLength = EvaluateExpression(stackFrame, &cmd, &valid);
     if (!displayLength || displayLength > screen->nLines - 5)
	displayLength = screen->nLines - 5;
     lastDumpAddress = dumpWord(screen, lastDumpAddress, displayLength);
     return TRUE;
}

// DS

LONG debuggerDumpStack(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser)
{
     LONG valid;
     extern BYTE *lastDumpAddress;
     extern LONG repeatCommand;
     extern LONG displayLength;
     extern LONG lastDisplayLength;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     if (repeatCommand)
     {
	lastDumpAddress = dumpStack(screen, stackFrame, lastDumpAddress, lastDisplayLength);
	return TRUE;
     }
     lastDumpAddress = (BYTE *) EvaluateExpression(stackFrame, &cmd, &valid);
     if (!valid)
	lastDumpAddress = (BYTE *) stackFrame->tESP;
     displayLength = EvaluateExpression(stackFrame, &cmd, &valid);
     if (!displayLength || displayLength > screen->nLines - 5)
	displayLength = screen->nLines - 5;
     lastDumpAddress = dumpStack(screen, stackFrame, lastDumpAddress, displayLength);
     return TRUE;

}

// DDS

LONG debuggerDumpDoubleStack(SCREEN *screen, BYTE *cmd,
			     StackFrame *stackFrame, LONG Exception,
			     DEBUGGER_PARSER *parser)
{
     LONG valid;
     extern BYTE *lastDumpAddress;
     extern LONG repeatCommand;
     extern LONG displayLength;
     extern LONG lastDisplayLength;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     if (repeatCommand)
     {
	lastDumpAddress = dumpDoubleStack(screen, stackFrame, lastDumpAddress,
						    lastDisplayLength);
	return TRUE;
     }
     lastDumpAddress = (BYTE *) EvaluateExpression(stackFrame, &cmd, &valid);
     if (!valid)
	lastDumpAddress = (BYTE *) stackFrame->tESP;
     displayLength = EvaluateExpression(stackFrame, &cmd, &valid);
     if (!displayLength || displayLength > screen->nLines - 5)
	displayLength = screen->nLines - 5;
     lastDumpAddress = dumpDoubleStack(screen, stackFrame, lastDumpAddress,
						    displayLength);
     return TRUE;

}

// DD

LONG debuggerDumpDouble(SCREEN *screen, BYTE *cmd,
			StackFrame *stackFrame, LONG Exception,
			DEBUGGER_PARSER *parser)
{
     LONG valid;
     extern BYTE *lastDumpAddress;
     extern LONG repeatCommand;
     extern LONG displayLength;
     extern LONG lastDisplayLength;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     if (repeatCommand)
     {
	lastDumpAddress = dumpDouble(screen, lastDumpAddress, lastDisplayLength);
	return TRUE;
     }
     lastDumpAddress = (BYTE *) EvaluateExpression(stackFrame, &cmd, &valid);
     if (!valid)
	lastDumpAddress = (BYTE *) stackFrame->tESP;
     displayLength = EvaluateExpression(stackFrame, &cmd, &valid);
     if (!displayLength || displayLength > screen->nLines - 5)
	displayLength = screen->nLines - 5;
     lastDumpAddress = dumpDouble(screen, lastDumpAddress, displayLength);
     return TRUE;

}

// D

LONG debuggerDumpByte(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser)
{
     LONG valid;
     extern BYTE *lastDumpAddress;
     extern LONG repeatCommand;
     extern LONG displayLength;
     extern LONG lastDisplayLength;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     if (repeatCommand)
     {
	lastDumpAddress = dump(screen, lastDumpAddress, lastDisplayLength);
	return TRUE;
     }
     lastDumpAddress = (BYTE *) EvaluateExpression(stackFrame, &cmd, &valid);
     if (!valid)
	lastDumpAddress = (BYTE *) stackFrame->tESP;
     displayLength = EvaluateExpression(stackFrame, &cmd, &valid);
     if (!displayLength || displayLength > screen->nLines - 5)
	displayLength = screen->nLines - 5;
     lastDumpAddress = dump(screen, lastDumpAddress, displayLength);
     return TRUE;

}

LONG displayDisassembleHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "u   <address> <#lines>   - unassemble code (32-bit)\n");
    printfScreenWithAttribute(screen, LTCYAN, "uu  <address> <#lines>   - unassemble code (16-bit)\n");
    return TRUE;
}

// UU

LONG processDisassemble16(SCREEN *screen, BYTE *cmd,
			  StackFrame *stackFrame, LONG Exception,
			  DEBUGGER_PARSER *parser)
{
     LONG valid;
     extern LONG lastUnasmAddress;
     extern LONG repeatCommand;
     extern LONG displayLength;
     extern LONG lastDisplayLength;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     if (repeatCommand)
     {
	lastUnasmAddress = disassemble(screen, stackFrame, lastUnasmAddress,
						    lastDisplayLength, LTCYAN, 0);
	return TRUE;
     }
     lastUnasmAddress = EvaluateExpression(stackFrame, &cmd, &valid);
     if (!valid)
	lastUnasmAddress = stackFrame->tEIP;
     displayLength = EvaluateExpression(stackFrame, &cmd, &valid);
     if (!displayLength || displayLength > screen->nLines - 5)
		     displayLength = screen->nLines - 5;
     lastUnasmAddress = disassemble(screen, stackFrame, lastUnasmAddress,
						 displayLength, LTCYAN, 0);
     return TRUE;
}

// U

LONG processDisassemble32(SCREEN *screen, BYTE *cmd,
			  StackFrame *stackFrame, LONG Exception,
			  DEBUGGER_PARSER *parser)
{
     LONG valid;
     extern LONG lastUnasmAddress;
     extern LONG repeatCommand;
     extern LONG displayLength;
     extern LONG lastDisplayLength;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     if (repeatCommand)
     {
	lastUnasmAddress = disassemble(screen, stackFrame, lastUnasmAddress,
				       lastDisplayLength, LTCYAN, 1);
	return TRUE;
     }
     lastUnasmAddress = EvaluateExpression(stackFrame, &cmd, &valid);
     if (!valid)
	lastUnasmAddress = stackFrame->tEIP;
     displayLength = EvaluateExpression(stackFrame, &cmd, &valid);
     if (!displayLength || displayLength > screen->nLines - 5)
	displayLength = screen->nLines - 5;
     lastUnasmAddress = disassemble(screen, stackFrame, lastUnasmAddress,
						 displayLength, LTCYAN, 1);
     return TRUE;

}


