

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  IA32.C
*   DESCRIP  :  Multi-Processing IA32 specific Debugger code MANOS v1.0
*   DATE     :  December 28, 1997
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
#include "ia32.h"
#include "altdebug.h"

BYTE *IA32Flags[]={
   "CF", 0, "PF", 0, "AF",    0, "ZF", "SF", "TF", "IF", "DF", "OF",
   0,    0, "NT", 0, "RF", "VM", "AC", "VIF","VIP","ID",    0,    0,
   0,
};

BYTE *BreakDescription[]={
   "EXECUTE",  "WRITE",  "IOPORT",  "READ/WRITE",
};

BYTE *BreakLengthDescription[]={
   ": 1 BYTE",  ": 2 BYTE",  ": ??????",  ": 4 BYTE",
};

BYTE *ExceptionDescription[]={
   "Divide By Zero",                 //  0
   "Debugger Exception (INT1)",      //  1
   "Non-Maskable Interrupt",         //  2
   "Debugger Breakpoint (INT3)",     //  3
   "Overflow Exception",             //  4
   "Bounds Check",                   //  5
   "Invalid Opcode",                 //  6
   "No Coprocessor",                 //  7
   "Double Fault",                   //  8
   "Cops",                           //  9
   "Invalid Task State Segment",     //  10
   "Segment Not Present",            //  11
   "Stack Exception",                //  12
   "General Protection",             //  13
   "Page Fault",                     //  14
   "InvalidInterrupt",               //  15
   "Coprocessor Error",              //  16
   "AlignmentCheck",                 //  17
   "Machine Check",                  //  18
   "Enter Debugger Request",         //  19
   "Unvectored Exception",           //  20
   "Directed NMI Breakpoint",        //  21
   "Switch Focus Processor",         //  22
   "PANIC!!! "                       //  23
};

BYTE char32spc[] = { "xxxúxxxúxxxúxxxùxxxúxxxúxxxúxxx " };
BYTE flset[] = { "VMRF  NT    OFDNIETFMIZR  AC  PE  CY" };
BYTE floff[] = { "              UPID  PLNZ      PO  NC" };
BYTE fluse[] = { 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1 };
NUMERIC_FRAME npx[MAX_PROCESSORS];

LONG MTRR_BASE_REGS[] = {
  MTRR_PHYS_BASE_0, MTRR_PHYS_BASE_1, MTRR_PHYS_BASE_2, MTRR_PHYS_BASE_3,
  MTRR_PHYS_BASE_4, MTRR_PHYS_BASE_5, MTRR_PHYS_BASE_6, MTRR_PHYS_BASE_7
};

LONG MTRR_MASK_VALUES[] = {
  MTRR_PHYS_MASK_0, MTRR_PHYS_MASK_1, MTRR_PHYS_MASK_2, MTRR_PHYS_MASK_3,
  MTRR_PHYS_MASK_4, MTRR_PHYS_MASK_5, MTRR_PHYS_MASK_6, MTRR_PHYS_MASK_7
};

LONG FaultAddressHistory[POOLED_RESOURCES][MAX_PROCESSORS];
LONG FaultEIPHistory[POOLED_RESOURCES][MAX_PROCESSORS];
LONG FaultStackHistory[POOLED_RESOURCES][MAX_PROCESSORS];
LONG FaultNest[MAX_PROCESSORS];

extern LONG BreakReserved[4];
extern LONG BreakPoints[4];
extern LONG BreakType[4];
extern LONG BreakLength[4];
extern LONG BreakTemp[4];
extern LONG BreakGo[4];
extern LONG BreakProceed[4];
extern LONG BreakMask[MAX_PROCESSORS];
extern StackFrame *CurrentFrame[MAX_PROCESSORS];
extern LONG NestedInterrupts[MAX_PROCESSORS];
extern LONG registerDisplay;
extern SCREEN *oldScreen;
extern BYTE BreakCondition[4][100];
extern LONG ConditionalBreakpoint[4];
extern StackFrame lastStackFrame;
extern LONG lastCR0;
extern LONG lastCR2;
extern LONG lastCR4;
extern LONG CurrentDR7;
extern LONG CurrentDR6[MAX_PROCESSORS];
extern LONG ExitOSFlag;
extern rlock_t debug_mutex;

// F10

LONG SLDexitToDOSACC(SCREEN *screen, LONG key, void *p,
		     ACCELERATOR *accel)
{
     BYTE displayBuffer[100]={""};
     register StackFrame *stackFrame = p;
     register LONG ekey;
     extern LONG debuggerActive;

     if (screen) {};
     if (key) {};
     if (p) {};
     if (accel) {};

     sprintf(displayBuffer, "Exiting ... ");
     PutVidStringCleol(sourceScreen, displayBuffer,
			      sourceScreen->nLines - 2,
			      BRITEWHITE | BGCYAN);

     ExitOSFlag = 1;
     stackFrame->tSystemFlags &= ~SINGLE_STEP;
     stackFrame->tSystemFlags |= RESUME;
     debuggerActive--;
     return -1;

}

// F8

LONG SLDprocessProceedACC(SCREEN *screen, LONG key, void *p,
			  ACCELERATOR *accel)
{
     register StackFrame *stackFrame = p;
     BYTE displayBuffer[100]={""};
     register LONG i;
     extern BYTE lastCommand;
     extern LONG lastCR0;
     extern LONG lastCR2;
     extern LONG lastCR4;
     extern LONG debuggerActive;
     extern LONG SourceDebuggerActive;
     extern StackFrame lastStackFrame;
     extern SCREEN *oldScreen;
     extern LONG BreakReserved[4];
     extern LONG BreakPoints[4];
     extern LONG BreakType[4];
     extern LONG BreakLength[4];
     extern LONG BreakTemp[4];
     extern LONG BreakGo[4];
     extern LONG BreakProceed[4];
     extern LONG nextUnasmAddress;

     if (screen) {};
     if (key) {};
     if (p) {};
     if (accel) {};

     if (needs_proceed)
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      BreakReserved[i] = TRUE;
	      BreakPoints[i] = nextUnasmAddress;
	      BreakType[i] = BREAK_EXECUTE;
	      BreakLength[i] = ONE_BYTE_FIELD;
	      BreakTemp[i] = TRUE;
	      BreakProceed[i] = TRUE;
	      SetDebugRegisters();
	      lastCommand = 'P';
	      lastCR0 = ReadCR0();
	      lastCR2 = ReadCR2();
	      lastCR4 = ReadCR4();
	      CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
			      sizeof(StackFrame));

	      sprintf(displayBuffer, "Proceeding ... (ctrl-esc to halt execution)");
	      PutVidStringCleol(sourceScreen, displayBuffer,
				       sourceScreen->nLines - 2,
				       BRITEWHITE | BGCYAN);

	      stackFrame->tSystemFlags &= ~SINGLE_STEP;
	      stackFrame->tSystemFlags |= RESUME;
	      debuggerActive--;
	      return -1;
	   }
	}
     }
     lastCommand = 'P';
     lastCR0 = ReadCR0();
     lastCR2 = ReadCR2();
     lastCR4 = ReadCR4();
     CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
		     sizeof(StackFrame));

     sprintf(displayBuffer, "Proceeding ... (ctrl-esc to halt execution)");
     PutVidStringCleol(sourceScreen, displayBuffer,
			      sourceScreen->nLines - 2,
			      BRITEWHITE | BGCYAN);

     stackFrame->tSystemFlags |= (SINGLE_STEP | RESUME);
     debuggerActive--;
     return -1;

}

// F7

LONG SLDprocessTraceACC(SCREEN *screen, LONG key, void *p,
			ACCELERATOR *accel)
{
     register StackFrame *stackFrame = p;
     BYTE displayBuffer[100]={""};
     extern BYTE lastCommand;
     extern LONG lastCR0;
     extern LONG lastCR2;
     extern LONG lastCR4;
     extern LONG SourceDebuggerActive;
     extern LONG debuggerActive;
     extern StackFrame lastStackFrame;
     extern SCREEN *oldScreen;

     if (screen) {};
     if (key) {};
     if (p) {};
     if (accel) {};

     lastCommand = 'T';
     lastCR0 = ReadCR0();
     lastCR2 = ReadCR2();
     lastCR4 = ReadCR4();
     CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
		     sizeof(StackFrame));

     sprintf(displayBuffer, "Tracing ... (ctrl-esc to halt execution)");
     PutVidStringCleol(sourceScreen, displayBuffer,
			      sourceScreen->nLines - 2,
			      BRITEWHITE | BGCYAN);

     stackFrame->tSystemFlags |= (SINGLE_STEP | RESUME);
     debuggerActive--;
     return -1;

}

// F9

LONG SLDprocessGoACC(SCREEN *screen, LONG key, void *p,
		     ACCELERATOR *accel)
{
     register StackFrame *stackFrame = p;
     register LONG address;
     LONG valid;
     register LONG i;
     extern BYTE lastCommand;
     extern LONG lastCR0;
     extern LONG lastCR2;
     extern LONG lastCR4;
     extern LONG debuggerActive;
     extern StackFrame lastStackFrame;
     extern SCREEN *oldScreen;

     if (screen) {};
     if (key) {};
     if (p) {};
     if (accel) {};

     ClearTempBreakpoints();
     lastCommand = 'G';
     lastCR0 = ReadCR0();
     lastCR2 = ReadCR2();
     lastCR4 = ReadCR4();
     CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
			sizeof(StackFrame));

     stackFrame->tSystemFlags &= ~SINGLE_STEP;
     stackFrame->tSystemFlags |= RESUME;

     if (activeScreen == sourceScreen || activeScreen == debugScreen)
	SetVideoOwner(oldScreen);

     debuggerActive--;
     return -1;

}


// F10

LONG exitToDOSACC(SCREEN *screen, LONG key, void *p,
		  ACCELERATOR *accel)
{
     register StackFrame *stackFrame = p;
     register LONG ekey;
     extern LONG debuggerActive;

     if (screen) {};
     if (key) {};
     if (p) {};
     if (accel) {};

     printfScreen(screen, "\n");
     printfScreen(screen, "Exit to MS-DOS? <y/n> ");
     ekey = GetKey();
     printfScreen(screen, "%c\n", ekey);
     if (toupper(ekey & 0xFF) == 'Y')
     {
	printfScreen(screen, "Exiting ...\n");
	ExitOSFlag = 1;
	stackFrame->tSystemFlags &= ~SINGLE_STEP;
	stackFrame->tSystemFlags |= RESUME;
	debuggerActive--;
	return -1;
     }
     return TRUE;

}

// F8

LONG processProceedACC(SCREEN *screen, LONG key, void *p,
		       ACCELERATOR *accel)
{
     register StackFrame *stackFrame = p;
     BYTE displayBuffer[100]={""};
     register LONG i;
     extern BYTE lastCommand;
     extern LONG lastCR0;
     extern LONG lastCR2;
     extern LONG lastCR4;
     extern LONG debuggerActive;
     extern LONG SourceDebuggerActive;
     extern StackFrame lastStackFrame;
     extern SCREEN *oldScreen;
     extern LONG BreakReserved[4];
     extern LONG BreakPoints[4];
     extern LONG BreakType[4];
     extern LONG BreakLength[4];
     extern LONG BreakTemp[4];
     extern LONG BreakGo[4];
     extern LONG BreakProceed[4];
     extern LONG nextUnasmAddress;

     if (screen) {};
     if (key) {};
     if (p) {};
     if (accel) {};

     printfScreen(screen, "\n");
     if (needs_proceed)
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      BreakReserved[i] = TRUE;
	      BreakPoints[i] = nextUnasmAddress;
	      BreakType[i] = BREAK_EXECUTE;
	      BreakLength[i] = ONE_BYTE_FIELD;
	      BreakTemp[i] = TRUE;
	      BreakProceed[i] = TRUE;
	      SetDebugRegisters();
	      lastCommand = 'P';
	      lastCR0 = ReadCR0();
	      lastCR2 = ReadCR2();
	      lastCR4 = ReadCR4();
	      CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
			      sizeof(StackFrame));

	      sprintf(displayBuffer, "Proceeding ... (ctrl-esc to halt execution)");
	      PutVidStringCleol(sourceScreen, displayBuffer,
				       sourceScreen->nLines - 2,
				       BRITEWHITE | BGCYAN);

	      stackFrame->tSystemFlags &= ~SINGLE_STEP;
	      stackFrame->tSystemFlags |= RESUME;
	      SetVideoOwner(oldScreen);
	      debuggerActive--;
	      return -1;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "\nNo breakpoint available for Proceed, (single step) instead");
     }
     lastCommand = 'P';
     lastCR0 = ReadCR0();
     lastCR2 = ReadCR2();
     lastCR4 = ReadCR4();
     CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
		     sizeof(StackFrame));
     sprintf(displayBuffer, "Proceeding ... (ctrl-esc to halt execution)");
     PutVidStringCleol(sourceScreen, displayBuffer,
			      sourceScreen->nLines - 2,
			      BRITEWHITE | BGCYAN);

     stackFrame->tSystemFlags |= (SINGLE_STEP | RESUME);
     SetVideoOwner(oldScreen);
     debuggerActive--;
     return -1;

}

// F7

LONG processTraceACC(SCREEN *screen, LONG key, void *p,
		     ACCELERATOR *accel)
{
     register StackFrame *stackFrame = p;
     BYTE displayBuffer[100]={""};
     extern BYTE lastCommand;
     extern LONG lastCR0;
     extern LONG lastCR2;
     extern LONG lastCR4;
     extern LONG SourceDebuggerActive;
     extern LONG debuggerActive;
     extern StackFrame lastStackFrame;
     extern SCREEN *oldScreen;

     if (screen) {};
     if (key) {};
     if (p) {};
     if (accel) {};

     printfScreen(screen, "\n");
     lastCommand = 'T';
     lastCR0 = ReadCR0();
     lastCR2 = ReadCR2();
     lastCR4 = ReadCR4();
     CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
		     sizeof(StackFrame));
     sprintf(displayBuffer, "Tracing ... (ctrl-esc to halt execution)");
     PutVidStringCleol(sourceScreen, displayBuffer,
			      sourceScreen->nLines - 2,
			      BRITEWHITE | BGCYAN);

     stackFrame->tSystemFlags |= (SINGLE_STEP | RESUME);
     SetVideoOwner(oldScreen);
     debuggerActive--;
     return -1;

}

// F9

LONG processGoACC(SCREEN *screen, LONG key, void *p,
		  ACCELERATOR *accel)
{
     register StackFrame *stackFrame = p;
     register LONG address;
     LONG valid;
     register LONG i;
     extern BYTE lastCommand;
     extern LONG lastCR0;
     extern LONG lastCR2;
     extern LONG lastCR4;
     extern LONG debuggerActive;
     extern StackFrame lastStackFrame;
     extern SCREEN *oldScreen;

     if (screen) {};
     if (key) {};
     if (p) {};
     if (accel) {};

     printfScreen(screen, "\n");
     ClearTempBreakpoints();
     lastCommand = 'G';
     lastCR0 = ReadCR0();
     lastCR2 = ReadCR2();
     lastCR4 = ReadCR4();
     CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
			sizeof(StackFrame));

     stackFrame->tSystemFlags &= ~SINGLE_STEP;
     stackFrame->tSystemFlags |= RESUME;
     SetVideoOwner(oldScreen);
     debuggerActive--;
     return -1;

}


LONG executeCommandHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "t                        - trace\n");
    printfScreenWithAttribute(screen, LTCYAN, "p                        - proceed\n");
    printfScreenWithAttribute(screen, LTCYAN, "g or g <address>         - go\n");
    printfScreenWithAttribute(screen, LTCYAN, "\n");
    printfScreenWithAttribute(screen, LTCYAN, "PF7                      - trace\n");
    printfScreenWithAttribute(screen, LTCYAN, "PF8                      - proceed\n");
    printfScreenWithAttribute(screen, LTCYAN, "PF9                      - go\n");
    return TRUE;
}

// Q, X, QUIT, EXIT

LONG exitToDOS(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser)
{
     register LONG ekey;
     extern LONG debuggerActive;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     printfScreen(screen, "Exit to MS-DOS? <y/n> ");
     ekey = GetKey();
     printfScreen(screen, "%c\n", ekey);
     if (toupper(ekey & 0xFF) == 'Y')
     {
	printfScreen(screen, "Exiting ...\n");
	ExitOSFlag = 1;
	stackFrame->tSystemFlags &= ~SINGLE_STEP;
	stackFrame->tSystemFlags |= RESUME;
	debuggerActive--;
	return -1;
     }
     return TRUE;

}

// P

LONG processProceed(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser)
{
     BYTE displayBuffer[100]={""};
     register LONG i;
     extern BYTE lastCommand;
     extern LONG lastCR0;
     extern LONG lastCR2;
     extern LONG lastCR4;
     extern LONG debuggerActive;
     extern LONG SourceDebuggerActive;
     extern StackFrame lastStackFrame;
     extern SCREEN *oldScreen;
     extern LONG BreakReserved[4];
     extern LONG BreakPoints[4];
     extern LONG BreakType[4];
     extern LONG BreakLength[4];
     extern LONG BreakTemp[4];
     extern LONG BreakGo[4];
     extern LONG BreakProceed[4];
     extern LONG nextUnasmAddress;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     if (needs_proceed)
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      BreakReserved[i] = TRUE;
	      BreakPoints[i] = nextUnasmAddress;
	      BreakType[i] = BREAK_EXECUTE;
	      BreakLength[i] = ONE_BYTE_FIELD;
	      BreakTemp[i] = TRUE;
	      BreakProceed[i] = TRUE;
	      SetDebugRegisters();
	      lastCommand = 'P';
	      lastCR0 = ReadCR0();
	      lastCR2 = ReadCR2();
	      lastCR4 = ReadCR4();
	      CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
			      sizeof(StackFrame));

	      sprintf(displayBuffer, "Proceeding ... (ctrl-esc to halt execution)");
	      PutVidStringCleol(sourceScreen, displayBuffer,
				       sourceScreen->nLines - 2, BRITEWHITE | BGRED);

	      stackFrame->tSystemFlags &= ~SINGLE_STEP;
	      stackFrame->tSystemFlags |= RESUME;
	      SetVideoOwner(oldScreen);
	      debuggerActive--;
	      return -1;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "\nNo breakpoint available for Proceed, (single step) instead");
     }
     lastCommand = 'P';
     lastCR0 = ReadCR0();
     lastCR2 = ReadCR2();
     lastCR4 = ReadCR4();
     CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
		     sizeof(StackFrame));
     sprintf(displayBuffer, "Proceeding ... (ctrl-esc to halt execution)");
     PutVidStringCleol(sourceScreen, displayBuffer,
			      sourceScreen->nLines - 2, BRITEWHITE | BGRED);

     stackFrame->tSystemFlags |= (SINGLE_STEP | RESUME);
     SetVideoOwner(oldScreen);
     debuggerActive--;
     return -1;

}


// T

LONG processTrace(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser)
{
     BYTE displayBuffer[100]={""};
     extern BYTE lastCommand;
     extern LONG lastCR0;
     extern LONG lastCR2;
     extern LONG lastCR4;
     extern LONG SourceDebuggerActive;
     extern LONG debuggerActive;
     extern StackFrame lastStackFrame;
     extern SCREEN *oldScreen;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     lastCommand = 'T';
     lastCR0 = ReadCR0();
     lastCR2 = ReadCR2();
     lastCR4 = ReadCR4();
     CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
		     sizeof(StackFrame));
     sprintf(displayBuffer, "Tracing ... (ctrl-esc to halt execution)");
     PutVidStringCleol(sourceScreen, displayBuffer,
			      sourceScreen->nLines - 2, BRITEWHITE | BGRED);

     stackFrame->tSystemFlags |= (SINGLE_STEP | RESUME);
     SetVideoOwner(oldScreen);
     debuggerActive--;
     return -1;

}

// G

LONG processGo(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     register LONG address;
     LONG valid;
     register LONG i;
     extern BYTE lastCommand;
     extern LONG lastCR0;
     extern LONG lastCR2;
     extern LONG lastCR4;
     extern LONG debuggerActive;
     extern StackFrame lastStackFrame;
     extern SCREEN *oldScreen;
     extern LONG BreakReserved[4];
     extern LONG BreakPoints[4];
     extern LONG BreakType[4];
     extern LONG BreakLength[4];
     extern LONG BreakTemp[4];
     extern LONG BreakGo[4];
     extern LONG BreakProceed[4];

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
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      BreakReserved[i] = TRUE;
	      BreakPoints[i] = address;
	      BreakType[i] = BREAK_EXECUTE;
	      BreakLength[i] = ONE_BYTE_FIELD;
	      BreakTemp[i] = TRUE;
	      BreakGo[i] = TRUE;
	      SetDebugRegisters();
	      printfScreen(screen, "\n");
	      lastCommand = 'G';
	      lastCR0 = ReadCR0();
	      lastCR2 = ReadCR2();
	      lastCR4 = ReadCR4();
	      CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
			      sizeof(StackFrame));

	      stackFrame->tSystemFlags &= ~SINGLE_STEP;
	      stackFrame->tSystemFlags |= RESUME;
	      SetVideoOwner(oldScreen);
	      debuggerActive--;
	      return -1;
	   }
	}
     }
     else
     {
	ClearTempBreakpoints();
	printfScreen(screen, "\n");
	lastCommand = 'G';
	lastCR0 = ReadCR0();
	lastCR2 = ReadCR2();
	lastCR4 = ReadCR4();
	CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
			sizeof(StackFrame));

	stackFrame->tSystemFlags &= ~SINGLE_STEP;
	stackFrame->tSystemFlags |= RESUME;
	SetVideoOwner(oldScreen);
	debuggerActive--;
	return -1;
     }
     printfScreenWithAttribute(screen, BRITEWHITE, "no breakpoint available for GO\n");
     return TRUE;

}

LONG processorCommandHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "lp  [p#]                 - list processor status\n");
    printfScreenWithAttribute(screen, LTCYAN, "ls  [p#]                 - switch to a specific processor\n");
    printfScreenWithAttribute(screen, LTCYAN, "lb  [p#]                 - signal a specific processor\n");
    printfScreenWithAttribute(screen, LTCYAN, "ln  [p#]                 - nmi a specific processor\n");
    printfScreenWithAttribute(screen, LTCYAN, "lr  [p#]                 - display processor registers\n");
    return TRUE;
}

// LB

LONG breakProcessor(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser)
{
     register LONG pnum;
     LONG valid;
     extern BYTE lastCommand;
     extern LONG lastCR0;
     extern LONG lastCR2;
     extern LONG lastCR4;
     extern LONG debuggerActive;
     extern StackFrame lastStackFrame;
     extern SCREEN *oldScreen;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     pnum = EvaluateExpression(stackFrame, &cmd, &valid);
     if (valid && pnum < num_procs &&
	 pnum != get_processor_id() &&
	 processor_table[pnum].ProcessorState != PROCESSOR_INACTIVE)
     {
	processor_table[pnum].DebugSignal = TRUE;
	stackFrame->tSystemFlags &= ~SINGLE_STEP;
	stackFrame->tSystemFlags |= RESUME;
	printfScreen(screen, "\n");
	lastCommand = 'G';
	lastCR0 = ReadCR0();
	lastCR2 = ReadCR2();
	lastCR4 = ReadCR4();
	CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
		 sizeof(StackFrame));
	SetVideoOwner(oldScreen);
	debuggerActive--;
	return -1;
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid processor specified\n");
     return TRUE;


}

// LN

LONG breakNMIProcessors(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser)
{
     register LONG pnum;
     LONG valid;
     extern BYTE lastCommand;
     extern LONG lastCR0;
     extern LONG lastCR2;
     extern LONG lastCR4;
     extern LONG debuggerActive;
     extern StackFrame lastStackFrame;
     extern SCREEN *oldScreen;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     pnum = EvaluateExpression(stackFrame, &cmd, &valid);
     if (valid && pnum < num_procs &&
	 pnum != get_processor_id() &&
	 processor_table[pnum].ProcessorState != PROCESSOR_INACTIVE)
     {
	xcall_processor(pnum, XCALL_DIRECTED, 0, 0, 0);

	stackFrame->tSystemFlags &= ~SINGLE_STEP;
	stackFrame->tSystemFlags |= RESUME;
	printfScreen(screen, "\n");
	lastCommand = 'G';
	lastCR0 = ReadCR0();
	lastCR2 = ReadCR2();
	lastCR4 = ReadCR4();
	CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
			   sizeof(StackFrame));
	SetVideoOwner(oldScreen);
	debuggerActive--;
	return -1;
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid processor specified\n");
     return TRUE;


}




LONG displayEAXHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}
    return TRUE;
}

// EAX

LONG ChangeEAXRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;

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
	stackFrame->tEAX = value;
	printfScreenWithAttribute(screen, BRITEWHITE,
				 "EAX changed to 0x%08X\n", value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change register command\n");
     return TRUE;

}


LONG displayEBXHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// EBX

LONG ChangeEBXRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;

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
	stackFrame->tEBX = value;
	printfScreenWithAttribute(screen, BRITEWHITE,
				 "EBX changed to 0x%08X\n", value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change register command\n");
     return TRUE;

}

LONG displayECXHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// ECX

LONG ChangeECXRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;

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
	stackFrame->tECX = value;
	printfScreenWithAttribute(screen, BRITEWHITE,
				 "ECX changed to 0x%08X\n", value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change register command\n");
     return TRUE;

}


LONG displayEDXHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// EDX

LONG ChangeEDXRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;

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
	stackFrame->tEDX = value;
	printfScreenWithAttribute(screen, BRITEWHITE,
				 "EDX changed to 0x%08X\n", value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change register command\n");
     return TRUE;

}

LONG displayESIHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// ESI

LONG ChangeESIRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;

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
	stackFrame->tESI = value;
	printfScreenWithAttribute(screen, BRITEWHITE,
				 "ESI changed to 0x%08X\n", value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change register command\n");
     return TRUE;

}


LONG displayEDIHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// EDI

LONG ChangeEDIRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;

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
	stackFrame->tEDI = value;
	printfScreenWithAttribute(screen, BRITEWHITE,
				 "EDI changed to 0x%08X\n", value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change register command\n");
     return TRUE;

}


LONG displayEBPHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// EBP

LONG ChangeEBPRegister(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;

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
	stackFrame->tEBP = value;
	printfScreenWithAttribute(screen, BRITEWHITE,
				 "EBP changed to 0x%08X\n", value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change register command\n");
     return TRUE;

}


LONG displayESPHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// ESP

LONG ChangeESPRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;

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
	stackFrame->tESP = value;
	printfScreenWithAttribute(screen, BRITEWHITE,
				 "ESP changed to 0x%08X\n", value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change register command\n");
     return TRUE;

}


LONG displayEIPHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// EIP

LONG ChangeEIPRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;

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
	stackFrame->tEIP = value;
	printfScreenWithAttribute(screen, BRITEWHITE,
				 "EIP changed to 0x%08X\n", value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change register command\n");
     return TRUE;

}

LONG displayCSHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// CS

LONG ChangeCSRegister(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;
     register WORD oldW;

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
	oldW = stackFrame->tCS;
	stackFrame->tCS = (WORD) value;
	printfScreenWithAttribute(screen, BRITEWHITE, "CS: = [%04X] changed to CS: = [%04X]\n",
				oldW, (WORD) value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change segment register command\n");
     return TRUE;

}

LONG displayDSHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// DS

LONG ChangeDSRegister(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;
     register WORD oldW;

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
	oldW = stackFrame->tDS;
	stackFrame->tDS = (WORD) value;
	printfScreenWithAttribute(screen, BRITEWHITE, "DS: = [%04X] changed to DS: = [%04X]\n",
				oldW, (WORD) value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change segment register command\n");
     return TRUE;

}

LONG displayESHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// ES

LONG ChangeESRegister(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;
     register WORD oldW;

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
	oldW = stackFrame->tES;
	stackFrame->tES = (WORD) value;
	printfScreenWithAttribute(screen, BRITEWHITE, "ES: = [%04X] changed to ES: = [%04X]\n",
				oldW, (WORD) value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change segment register command\n");
     return TRUE;

}

LONG displayFSHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// FS

LONG ChangeFSRegister(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;
     register WORD oldW;

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
	oldW = stackFrame->tFS;
	stackFrame->tFS = (WORD) value;
	printfScreenWithAttribute(screen, BRITEWHITE, "FS: = [%04X] changed to FS: = [%04X]\n",
				oldW, (WORD) value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change segment register command\n");
     return TRUE;

}

LONG displayGSHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// GS

LONG ChangeGSRegister(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;
     register WORD oldW;

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
	oldW = stackFrame->tGS;
	stackFrame->tGS = (WORD) value;
	printfScreenWithAttribute(screen, BRITEWHITE, "GS: = [%04X] changed to GS: = [%04X]\n",
				oldW, (WORD) value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change segment register command\n");
     return TRUE;

}

LONG displaySSHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// SS

LONG ChangeSSRegister(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value;
     register WORD oldW;

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
	oldW = stackFrame->tSS;
	stackFrame->tSS = (WORD) value;
	printfScreenWithAttribute(screen, BRITEWHITE, "SS: = [%04X] changed to SS: = [%04X]\n",
				oldW, (WORD) value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change segment register command\n");

     return TRUE;

}

LONG displayRFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// RF

LONG ChangeRFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & RF_FLAG;
	(value) ? (stackFrame->tSystemFlags |= RF_FLAG) : (stackFrame->tSystemFlags &= ~RF_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag RF[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}

LONG displayTFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// TF

LONG ChangeTFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & TF_FLAG;
	(value) ? (stackFrame->tSystemFlags |= TF_FLAG) : (stackFrame->tSystemFlags &= ~TF_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag TF[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}

LONG displayZFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// ZF

LONG ChangeZFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & ZF_FLAG;
	(value) ? (stackFrame->tSystemFlags |= ZF_FLAG) : (stackFrame->tSystemFlags &= ~ZF_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag ZF[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}

LONG displaySFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// SF

LONG ChangeSFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & SF_FLAG;
	(value) ? (stackFrame->tSystemFlags |= SF_FLAG) : (stackFrame->tSystemFlags &= ~SF_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag SF[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}

LONG displayPFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// PF

LONG ChangePFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & PF_FLAG;
	(value) ? (stackFrame->tSystemFlags |= PF_FLAG) : (stackFrame->tSystemFlags &= ~PF_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag PF[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}

LONG displayCFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// CF

LONG ChangeCFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & CF_FLAG;
	(value) ? (stackFrame->tSystemFlags |= CF_FLAG) : (stackFrame->tSystemFlags &= ~CF_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag CF[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}

LONG displayOFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// OF

LONG ChangeOFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & OF_FLAG;
	(value) ? (stackFrame->tSystemFlags |= OF_FLAG) : (stackFrame->tSystemFlags &= ~OF_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag OF[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}


LONG displayIFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// IF

LONG ChangeIFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & IF_FLAG;
	(value) ? (stackFrame->tSystemFlags |= IF_FLAG) : (stackFrame->tSystemFlags &= ~IF_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag IF[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}

LONG displayIDHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// ID

LONG ChangeIDFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & ID_FLAGS;
	(value) ? (stackFrame->tSystemFlags |= ID_FLAGS) : (stackFrame->tSystemFlags &= ~ID_FLAGS);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag ID[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}

LONG displayDFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// DF

LONG ChangeDFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser)
{

     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & DF_FLAG;
	(value) ? (stackFrame->tSystemFlags |= DF_FLAG) : (stackFrame->tSystemFlags &= ~DF_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag DF[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}

LONG displayNTHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// NT

LONG ChangeNTFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & NT_FLAG;
	(value) ? (stackFrame->tSystemFlags |= NT_FLAG) : (stackFrame->tSystemFlags &= ~NT_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag NT[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}

LONG displayVMHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// VM

LONG ChangeVMFlag(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & VM_FLAG;
	(value) ? (stackFrame->tSystemFlags |= VM_FLAG) : (stackFrame->tSystemFlags &= ~VM_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag VM[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}


LONG displayVIFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// VIF

LONG ChangeVIFFlag(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & VIF_FLAG;
	(value) ? (stackFrame->tSystemFlags |= VIF_FLAG) : (stackFrame->tSystemFlags &= ~VIF_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag VIF[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}

LONG displayVIPHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// VIP

LONG ChangeVIPFlag(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & VIP_FLAG;
	(value) ? (stackFrame->tSystemFlags |= VIP_FLAG) : (stackFrame->tSystemFlags &= ~VIP_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag VIP[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}

LONG displayAFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// AF

LONG ChangeAFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & AF_FLAG;
	(value) ? (stackFrame->tSystemFlags |= AF_FLAG) : (stackFrame->tSystemFlags &= ~AF_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag AF[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}


LONG displayACHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    return TRUE;
}

// AC

LONG ChangeACFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG value, oldD;

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
	oldD = stackFrame->tSystemFlags & AC_FLAG;
	(value) ? (stackFrame->tSystemFlags |= AC_FLAG) : (stackFrame->tSystemFlags &= ~AC_FLAG);
	printfScreenWithAttribute(screen, BRITEWHITE, "EFlag AC[%08X] changed to (%d)\n",
				oldD, value);
     }
     else
	printfScreenWithAttribute(screen, BRITEWHITE, "invalid change flags command\n");
     return TRUE;

}

LONG displayMTRRHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "mtrr                     - display memory type range registers\n");
    return TRUE;
}

// MTRR

LONG DisplayMTRRRegisters(SCREEN *screen, BYTE *cmd,
			  StackFrame *stackFrame, LONG Exception,
			  DEBUGGER_PARSER *parser)
{

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     displayMTRRRegisters(screen);
     return TRUE;

}

LONG displayGDTHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, ".g or .g <address>       - display global descriptor table\n");
    return TRUE;
}

// .G

LONG displayGDT(SCREEN *screen, BYTE *cmd,
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
	DisplayGDT(screen, (BYTE *) address);
     else
	DisplayGDT(screen, (BYTE *) 0);
     return TRUE;
}

LONG displayIDTHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, ".i or .i <address>       - display interrupt descriptor table\n");
    return TRUE;
}

// .I

LONG displayIDT(SCREEN *screen, BYTE *cmd,
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
	DisplayIDT(screen, (BYTE *) address);
     else
	DisplayIDT(screen, (BYTE *) 0);
     return TRUE;
}

LONG evaluateExpressionHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    extern void displayExpressionHelp(SCREEN *screen);

    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    displayExpressionHelp(screen);
    return TRUE;
}

// .E

LONG evaluateExpression(SCREEN *screen, BYTE *cmd,
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

     EvaluateCommandExpression(screen, stackFrame, cmd);
     return TRUE;
}

LONG displayDOSTableHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, ".d                       - display dos table\n");
    return TRUE;
}

// .D

LONG displayDOSTable(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     DisplayDOSTable(screen, DosDataTable);
     return TRUE;
}

LONG portCommandHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "i   <port>               - input byte from port\n");
    printfScreenWithAttribute(screen, LTCYAN, "ib  <port>               - input byte from port\n");
    printfScreenWithAttribute(screen, LTCYAN, "iw  <port>               - input word from port\n");
    printfScreenWithAttribute(screen, LTCYAN, "id  <port>               - input double word from port\n");
    printfScreenWithAttribute(screen, LTCYAN, "o   <port> <val>         - output byte to port\n");
    printfScreenWithAttribute(screen, LTCYAN, "ob  <port> <val>         - output byte to port\n");
    printfScreenWithAttribute(screen, LTCYAN, "ow  <port> <val>         - output word to port\n");
    printfScreenWithAttribute(screen, LTCYAN, "od  <port> <val>         - output double word to port\n");
    return TRUE;
}

// IW

LONG inputWordPort(SCREEN *screen, BYTE *cmd,
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
     {
	printfScreenWithAttribute(screen, BRITEWHITE, "inportw (%04X) = %04X\n",
					  address, inw(address));
     }
     else
     {
	printfScreenWithAttribute(screen, LTCYAN, "bad port command\n");
     }
     return TRUE;

}

// ID

LONG inputDoublePort(SCREEN *screen, BYTE *cmd,
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
     {
	printfScreenWithAttribute(screen, BRITEWHITE, "inportd (%04X) = %08X\n",
					  address, ind(address));
     }
     else
     {
	printfScreenWithAttribute(screen, LTCYAN, "bad port command\n");
     }
     return TRUE;

}

// IB

LONG inputBytePort(SCREEN *screen, BYTE *cmd,
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
     {
	printfScreenWithAttribute(screen, BRITEWHITE, "inportb (%04X) = %02X\n",
					  address, inb(address));
     }
     else
     {
	printfScreenWithAttribute(screen, LTCYAN, "bad port command\n");
     }
     return TRUE;

}

// I

LONG inputPort(SCREEN *screen, BYTE *cmd,
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
     {
	printfScreenWithAttribute(screen, BRITEWHITE, "inportb (%04X) = %02X\n",
					  address, inb(address));
     }
     else
     {
	printfScreenWithAttribute(screen, LTCYAN, "bad port command\n");
     }
     return TRUE;

}

// OW

LONG outputWordPort(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG port, value;
     LONG valid;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     port = EvaluateExpression(stackFrame, &cmd, &valid);
     if (valid)
     {
	value = EvaluateExpression(stackFrame, &cmd, &valid);
	if (valid)
	{
	   printfScreenWithAttribute(screen, BRITEWHITE, "outportw (%04X) = %04X\n",
					    port, (WORD)value);
	   outw(port, value);
	   return TRUE;
	}
     }
     else
	printfScreenWithAttribute(screen, LTCYAN, "bad port command\n");

     return TRUE;

}

// OD

LONG outputDoublePort(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG port, value;
     LONG valid;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     port = EvaluateExpression(stackFrame, &cmd, &valid);
     if (valid)
     {
	value = EvaluateExpression(stackFrame, &cmd, &valid);
	if (valid)
	{
	   printfScreenWithAttribute(screen, BRITEWHITE, "outportd (%04X) = %08X\n",
					    port, (LONG)value);
	   outd(port, value);
	   return TRUE;
	}
     }
     else
	printfScreenWithAttribute(screen, LTCYAN, "bad port command\n");

     return TRUE;

}

// OB

LONG outputBytePort(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG port, value;
     LONG valid;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     port = EvaluateExpression(stackFrame, &cmd, &valid);
     if (valid)
     {
	value = EvaluateExpression(stackFrame, &cmd, &valid);
	if (valid)
	{
	   printfScreenWithAttribute(screen, BRITEWHITE, "outportb (%04X) = %02X\n",
					    port, (BYTE)value);
	   outb(port, value);
	   return TRUE;
	}
     }
     else
	printfScreenWithAttribute(screen, LTCYAN, "bad port command\n");

     return TRUE;

}

// O

LONG outputPort(SCREEN *screen, BYTE *cmd,
		StackFrame *stackFrame, LONG Exception,
		DEBUGGER_PARSER *parser)
{
     register LONG port, value;
     LONG valid;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     cmd = &cmd[parser->debugCommandNameLength];
     while (*cmd && *cmd == ' ') cmd++;

     port = EvaluateExpression(stackFrame, &cmd, &valid);
     if (valid)
     {
	value = EvaluateExpression(stackFrame, &cmd, &valid);
	if (valid)
	{
	   printfScreenWithAttribute(screen, BRITEWHITE, "outportb (%04X) = %02X\n",
					    port, (BYTE)value);
	   outb(port, value);
	   return TRUE;
	}
     }
     else
	printfScreenWithAttribute(screen, LTCYAN, "bad port command\n");

     return TRUE;

}

LONG breakpointCommandHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser)
{
    if (screen) {}
    if (commandLine) {}
    if (parser) {}

    printfScreenWithAttribute(screen, LTCYAN, "b                        - display all breakpoints\n");
    printfScreenWithAttribute(screen, LTCYAN, "b   <address>            - set execute breakpoint\n");
    printfScreenWithAttribute(screen, LTCYAN, "bc  [#] (1-4)            - clear breakpoint\n");
    printfScreenWithAttribute(screen, LTCYAN, "bca                      - clear all breakpoints\n");
    printfScreenWithAttribute(screen, LTCYAN, "br[#] <address>          - set read/write breakpoint #=1,2 or 4 byte len\n");
    printfScreenWithAttribute(screen, LTCYAN, "bw[#] <address>          - set write only breakpoint #=1,2 or 4 byte len\n");
    printfScreenWithAttribute(screen, LTCYAN, "bi[#] <address>          - set io address breakpoint #=1,2 or 4 byte len\n");
    printfScreenWithAttribute(screen, LTCYAN, "bm  [p#]                 - mask breaks for specific processor \n");
    printfScreenWithAttribute(screen, LTCYAN, "bst                      - display temporary (go/proceed) breakpoints\n");
    return TRUE;
}

// BCA

LONG breakpointClearAll(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG i;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     for (i=0; i < 4; i++)
     {
	BreakReserved[i] = 0;
	BreakPoints[i] = 0;
	BreakType[i] = 0;
	BreakLength[i] = 0;
	ConditionalBreakpoint[i] = 0;
     }
     SetDebugRegisters();
     printfScreenWithAttribute(screen, BRITEWHITE, "all breakpoints cleared\n");

     return TRUE;

}

// BC

LONG breakpointClear(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     LONG valid;
     register LONG i, address;
     register BYTE *symbolName;

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
	i = address;
	if (i < 4)
	{
	   symbolName = GetSymbolFromValue(BreakPoints[i]);
	   printfScreenWithAttribute(screen, BRITEWHITE,
			     "breakpoint %i at 0x%08X (%s %s) %s cleared\n",
			     i,
			     BreakPoints[i],
			     BreakDescription[(BreakType[i] & 3)],
			     BreakLengthDescription[(BreakLength[i] & 3)],
			     symbolName ? symbolName : "");
	   BreakReserved[i] = 0;
	   BreakPoints[i] = 0;
	   BreakType[i] = 0;
	   BreakLength[i] = 0;
	   ConditionalBreakpoint[i] = 0;
	   SetDebugRegisters();
	   return TRUE;
	}
	else
	   printfScreenWithAttribute(screen, BRITEWHITE, "breakpoint out of range\n");
	return TRUE;
     }
     printfScreenWithAttribute(screen, BRITEWHITE, "breakpoint not found\n");
     return TRUE;
}

// BM

LONG breakpointMask(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG address, pnum, i;
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
     {
	pnum = address;
	if (pnum < MAX_PROCESSORS)
	{
	   if (BreakMask[pnum])
	      BreakMask[pnum] = 0;
	   else
	      BreakMask[pnum] = 1;
	   printfScreenWithAttribute(screen, LTGREEN, "processor %i : %s\n", pnum,
				   BreakMask[pnum] ? "BREAKS_MASKED" : "BREAKS_UNMASKED");
	}
	else
	   printfScreenWithAttribute(screen, LTGREEN, "processor (%i) invalid\n", pnum);
     }
     else
     {
	for (i=0; i < MAX_PROCESSORS; i++)
	{
	   printfScreenWithAttribute(screen, LTGREEN, "processor %i : %s\n", i,
				BreakMask[i] ? "BREAKS_MASKED" : "BREAKS_UNMASKED");
	}
     }
     return TRUE;

}

// BW1

LONG breakpointWord1(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser)
{
     register LONG address, i, r;
     register BYTE *pB, *symbolName;
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
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      pB = cmd;
	      EvaluateExpression(stackFrame, &cmd, &valid);
	      if (valid)
	      {
		 ConditionalBreakpoint[i] = 1;
		 for (r=0; r < 80 && *pB; r++)
		    BreakCondition[i][r] = *pB++;
		 BreakCondition[i][r] = '\0';
	      }
	      BreakReserved[i] = 1;
	      BreakPoints[i] = address;
	      BreakType[i] = BREAK_WRITE;
	      BreakLength[i] = ONE_BYTE_FIELD;
	      symbolName = GetSymbolFromValue(address);
	      printfScreenWithAttribute(screen, BRITEWHITE,
				 "breakpoint %i set to 0x%08X (%s %s) %s\n",
				 i,
				 BreakPoints[i],
				 BreakDescription[(BreakType[i] & 3)],
				 BreakLengthDescription[(BreakLength[i] & 3)],
				 symbolName ? symbolName : "");
	      if (ConditionalBreakpoint[i])
	      {
				 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	      }
	      SetDebugRegisters();
	      return TRUE;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "no breakpoint available\n");
     }
     return TRUE;

}

// BW2

LONG breakpointWord2(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser)
{
     register LONG address, i, r;
     register BYTE *pB, *symbolName;
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
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      pB = cmd;
	      EvaluateExpression(stackFrame, &cmd, &valid);
	      if (valid)
	      {
		 ConditionalBreakpoint[i] = 1;
		 for (r=0; r < 80 && *pB; r++)
		    BreakCondition[i][r] = *pB++;
		 BreakCondition[i][r] = '\0';
	      }
	      BreakReserved[i] = 1;
	      BreakPoints[i] = address;
	      BreakType[i] = BREAK_WRITE;
	      BreakLength[i] = TWO_BYTE_FIELD;
	      symbolName = GetSymbolFromValue(address);
	      printfScreenWithAttribute(screen, BRITEWHITE,
				 "breakpoint %i set to 0x%08X (%s %s) %s\n",
				 i,
				 BreakPoints[i],
				 BreakDescription[(BreakType[i] & 3)],
				 BreakLengthDescription[(BreakLength[i] & 3)],
				 symbolName ? symbolName : "");
	      if (ConditionalBreakpoint[i])
	      {
				 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	      }
	      SetDebugRegisters();
	      return TRUE;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "no breakpoint available\n");
     }
     return TRUE;

}

// BW4

LONG breakpointWord4(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser)
{
     register LONG address, i, r;
     register BYTE *pB, *symbolName;
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
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      pB = cmd;
	      EvaluateExpression(stackFrame, &cmd, &valid);
	      if (valid)
	      {
		 ConditionalBreakpoint[i] = 1;
		 for (r=0; r < 80 && *pB; r++)
		    BreakCondition[i][r] = *pB++;
		 BreakCondition[i][r] = '\0';
	      }
	      BreakReserved[i] = 1;
	      BreakPoints[i] = address;
	      BreakType[i] = BREAK_WRITE;
	      BreakLength[i] = FOUR_BYTE_FIELD;
	      symbolName = GetSymbolFromValue(address);
	      printfScreenWithAttribute(screen, BRITEWHITE,
				 "breakpoint %i set to 0x%08X (%s %s) %s\n",
				 i,
				 BreakPoints[i],
				 BreakDescription[(BreakType[i] & 3)],
				 BreakLengthDescription[(BreakLength[i] & 3)],
				 symbolName ? symbolName : "");
	      if (ConditionalBreakpoint[i])
	      {
				 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	      }
	      SetDebugRegisters();
	      return TRUE;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "no breakpoint available\n");
     }
     return TRUE;

}

// BW

LONG breakpointWord(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser)
{
     register LONG address, i, r;
     register BYTE *pB, *symbolName;
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
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      pB = cmd;
	      EvaluateExpression(stackFrame, &cmd, &valid);
	      if (valid)
	      {
		 ConditionalBreakpoint[i] = 1;
		 for (r=0; r < 80 && *pB; r++)
		    BreakCondition[i][r] = *pB++;
		 BreakCondition[i][r] = '\0';
	      }
	      BreakReserved[i] = 1;
	      BreakPoints[i] = address;
	      BreakType[i] = BREAK_WRITE;
	      BreakLength[i] = ONE_BYTE_FIELD;
	      symbolName = GetSymbolFromValue(address);
	      printfScreenWithAttribute(screen, BRITEWHITE,
				 "breakpoint %i set to 0x%08X (%s %s) %s\n",
				 i,
				 BreakPoints[i],
				 BreakDescription[(BreakType[i] & 3)],
				 BreakLengthDescription[(BreakLength[i] & 3)],
				 symbolName ? symbolName : "");
	      if (ConditionalBreakpoint[i])
	      {
				 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	      }
	      SetDebugRegisters();
	      return TRUE;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "no breakpoint available\n");
     }
     return TRUE;

}


// BR1

LONG breakpointRead1(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG address, i, r;
     register BYTE *pB, *symbolName;
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
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      pB = cmd;
	      EvaluateExpression(stackFrame, &cmd, &valid);
	      if (valid)
	      {
		 ConditionalBreakpoint[i] = 1;
		 for (r=0; r < 80 && *pB; r++)
		    BreakCondition[i][r] = *pB++;
		 BreakCondition[i][r] = '\0';
	      }
	      BreakReserved[i] = 1;
	      BreakPoints[i] = address;
	      BreakType[i] = BREAK_READWRITE;
	      BreakLength[i] = ONE_BYTE_FIELD;
	      symbolName = GetSymbolFromValue(address);
	      printfScreenWithAttribute(screen, BRITEWHITE,
				 "breakpoint %i set to 0x%08X (%s %s) %s\n",
				 i,
				 BreakPoints[i],
				 BreakDescription[(BreakType[i] & 3)],
				 BreakLengthDescription[(BreakLength[i] & 3)],
				 symbolName ? symbolName : "");
	      if (ConditionalBreakpoint[i])
	      {
		 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	      }
	      SetDebugRegisters();
	      return TRUE;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "no breakpoint available\n");
     }
     return TRUE;

}


// BR2

LONG breakpointRead2(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG address, i, r;
     register BYTE *pB, *symbolName;
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
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      pB = cmd;
	      EvaluateExpression(stackFrame, &cmd, &valid);
	      if (valid)
	      {
		 ConditionalBreakpoint[i] = 1;
		 for (r=0; r < 80 && *pB; r++)
		    BreakCondition[i][r] = *pB++;
		 BreakCondition[i][r] = '\0';
	      }
	      BreakReserved[i] = 1;
	      BreakPoints[i] = address;
	      BreakType[i] = BREAK_READWRITE;
	      BreakLength[i] = TWO_BYTE_FIELD;
	      symbolName = GetSymbolFromValue(address);
	      printfScreenWithAttribute(screen, BRITEWHITE,
				 "breakpoint %i set to 0x%08X (%s %s) %s\n",
				 i,
				 BreakPoints[i],
				 BreakDescription[(BreakType[i] & 3)],
				 BreakLengthDescription[(BreakLength[i] & 3)],
				 symbolName ? symbolName : "");
	      if (ConditionalBreakpoint[i])
	      {
		 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	      }
	      SetDebugRegisters();
	      return TRUE;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "no breakpoint available\n");
     }
     return TRUE;

}


// BR4

LONG breakpointRead4(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG address, i, r;
     register BYTE *pB, *symbolName;
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
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      pB = cmd;
	      EvaluateExpression(stackFrame, &cmd, &valid);
	      if (valid)
	      {
		 ConditionalBreakpoint[i] = 1;
		 for (r=0; r < 80 && *pB; r++)
		    BreakCondition[i][r] = *pB++;
		 BreakCondition[i][r] = '\0';
	      }
	      BreakReserved[i] = 1;
	      BreakPoints[i] = address;
	      BreakType[i] = BREAK_READWRITE;
	      BreakLength[i] = FOUR_BYTE_FIELD;
	      symbolName = GetSymbolFromValue(address);
	      printfScreenWithAttribute(screen, BRITEWHITE,
				 "breakpoint %i set to 0x%08X (%s %s) %s\n",
				 i,
				 BreakPoints[i],
				 BreakDescription[(BreakType[i] & 3)],
				 BreakLengthDescription[(BreakLength[i] & 3)],
				 symbolName ? symbolName : "");
	      if (ConditionalBreakpoint[i])
	      {
		 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	      }
	      SetDebugRegisters();
	      return TRUE;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "no breakpoint available\n");
     }
     return TRUE;

}


// BR

LONG breakpointRead(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG address, i, r;
     register BYTE *pB, *symbolName;
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
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      pB = cmd;
	      EvaluateExpression(stackFrame, &cmd, &valid);
	      if (valid)
	      {
		 ConditionalBreakpoint[i] = 1;
		 for (r=0; r < 80 && *pB; r++)
		    BreakCondition[i][r] = *pB++;
		 BreakCondition[i][r] = '\0';
	      }
	      BreakReserved[i] = 1;
	      BreakPoints[i] = address;
	      BreakType[i] = BREAK_READWRITE;
	      BreakLength[i] = ONE_BYTE_FIELD;
	      symbolName = GetSymbolFromValue(address);
	      printfScreenWithAttribute(screen, BRITEWHITE,
				 "breakpoint %i set to 0x%08X (%s %s) %s\n",
				 i,
				 BreakPoints[i],
				 BreakDescription[(BreakType[i] & 3)],
				 BreakLengthDescription[(BreakLength[i] & 3)],
				 symbolName ? symbolName : "");
	      if (ConditionalBreakpoint[i])
	      {
		 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	      }
	      SetDebugRegisters();
	      return TRUE;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "no breakpoint available\n");
     }
     return TRUE;

}

// BI1

LONG breakpointIO1(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG address, i, r;
     register BYTE *pB, *symbolName;
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
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      pB = cmd;
	      EvaluateExpression(stackFrame, &cmd, &valid);
	      if (valid)
	      {
		 ConditionalBreakpoint[i] = 1;
		 for (r=0; r < 80 && *pB; r++)
		    BreakCondition[i][r] = *pB++;
		 BreakCondition[i][r] = '\0';
	      }
	      BreakReserved[i] = 1;
	      BreakPoints[i] = address;
	      BreakType[i] = BREAK_IOPORT;
	      BreakLength[i] = ONE_BYTE_FIELD;
	      symbolName = GetSymbolFromValue(address);
	      printfScreenWithAttribute(screen, BRITEWHITE,
				 "breakpoint %i set to 0x%08X (%s %s) %s\n",
				 i,
				 BreakPoints[i],
				 BreakDescription[(BreakType[i] & 3)],
				 BreakLengthDescription[(BreakLength[i] & 3)],
				 symbolName ? symbolName : "");
	      if (ConditionalBreakpoint[i])
	      {
		 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	      }
	      SetDebugRegisters();
	      return TRUE;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "no breakpoint available\n");
     }
     return TRUE;

}

// BI2

LONG breakpointIO2(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG address, i, r;
     register BYTE *pB, *symbolName;
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
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      pB = cmd;
	      EvaluateExpression(stackFrame, &cmd, &valid);
	      if (valid)
	      {
		 ConditionalBreakpoint[i] = 1;
		 for (r=0; r < 80 && *pB; r++)
		    BreakCondition[i][r] = *pB++;
		 BreakCondition[i][r] = '\0';
	      }
	      BreakReserved[i] = 1;
	      BreakPoints[i] = address;
	      BreakType[i] = BREAK_IOPORT;
	      BreakLength[i] = TWO_BYTE_FIELD;
	      symbolName = GetSymbolFromValue(address);
	      printfScreenWithAttribute(screen, BRITEWHITE,
				 "breakpoint %i set to 0x%08X (%s %s) %s\n",
				 i,
				 BreakPoints[i],
				 BreakDescription[(BreakType[i] & 3)],
				 BreakLengthDescription[(BreakLength[i] & 3)],
				 symbolName ? symbolName : "");
	      if (ConditionalBreakpoint[i])
	      {
		 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	      }
	      SetDebugRegisters();
	      return TRUE;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "no breakpoint available\n");
     }
     return TRUE;

}

// BI4

LONG breakpointIO4(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG address, i, r;
     register BYTE *pB, *symbolName;
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
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      pB = cmd;
	      EvaluateExpression(stackFrame, &cmd, &valid);
	      if (valid)
	      {
		 ConditionalBreakpoint[i] = 1;
		 for (r=0; r < 80 && *pB; r++)
		    BreakCondition[i][r] = *pB++;
		 BreakCondition[i][r] = '\0';
	      }
	      BreakReserved[i] = 1;
	      BreakPoints[i] = address;
	      BreakType[i] = BREAK_IOPORT;
	      BreakLength[i] = FOUR_BYTE_FIELD;
	      symbolName = GetSymbolFromValue(address);
	      printfScreenWithAttribute(screen, BRITEWHITE,
				 "breakpoint %i set to 0x%08X (%s %s) %s\n",
				 i,
				 BreakPoints[i],
				 BreakDescription[(BreakType[i] & 3)],
				 BreakLengthDescription[(BreakLength[i] & 3)],
				 symbolName ? symbolName : "");
	      if (ConditionalBreakpoint[i])
	      {
		 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	      }
	      SetDebugRegisters();
	      return TRUE;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "no breakpoint available\n");
     }
     return TRUE;

}

// BI

LONG breakpointIO(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG address, i, r;
     register BYTE *pB, *symbolName;
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
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      pB = cmd;
	      EvaluateExpression(stackFrame, &cmd, &valid);
	      if (valid)
	      {
		 ConditionalBreakpoint[i] = 1;
		 for (r=0; r < 80 && *pB; r++)
		    BreakCondition[i][r] = *pB++;
		 BreakCondition[i][r] = '\0';
	      }
	      BreakReserved[i] = 1;
	      BreakPoints[i] = address;
	      BreakType[i] = BREAK_IOPORT;
	      BreakLength[i] = ONE_BYTE_FIELD;
	      symbolName = GetSymbolFromValue(address);
	      printfScreenWithAttribute(screen, BRITEWHITE,
				 "breakpoint %i set to 0x%08X (%s %s) %s\n",
				 i,
				 BreakPoints[i],
				 BreakDescription[(BreakType[i] & 3)],
				 BreakLengthDescription[(BreakLength[i] & 3)],
				 symbolName ? symbolName : "");
	      if (ConditionalBreakpoint[i])
	      {
		 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	      }
	      SetDebugRegisters();
	      return TRUE;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "no breakpoint available\n");
     }
     return TRUE;

}

// B

LONG breakpointExecute(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser)
{
     register LONG address, i, r;
     register BYTE *pB, *symbolName;
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
     {
	for (i=0; i < 4; i++)
	{
	   if (BreakReserved[i])
	   {
	      symbolName = GetSymbolFromValue(BreakPoints[i]);
	      printfScreenWithAttribute(screen, BRITEWHITE,
				"Break %i is at 0x%08X (%s %s) %s\n",
				i,
				BreakPoints[i],
				BreakDescription[(BreakType[i] & 3)],
				BreakLengthDescription[(BreakLength[i] & 3)],
				symbolName ? symbolName : "");
	      if (ConditionalBreakpoint[i])
	      {
		 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	      }
	   }
	}
     }
     else
     {
	for (i=0; i < 4; i++)
	{
	   if (!BreakReserved[i])
	   {
	      pB = cmd;
	      EvaluateExpression(stackFrame, &cmd, &valid);
	      if (valid)
	      {
		 ConditionalBreakpoint[i] = 1;
		 for (r=0; r < 80 && *pB; r++)
		    BreakCondition[i][r] = *pB++;
		 BreakCondition[i][r] = '\0';
	      }
	      BreakReserved[i] = 1;
	      BreakPoints[i] = address;
	      BreakType[i] = BREAK_EXECUTE;
	      BreakLength[i] = ONE_BYTE_FIELD;
	      symbolName = GetSymbolFromValue(address);
	      printfScreenWithAttribute(screen, BRITEWHITE,
				 "breakpoint %i set to 0x%08X (%s %s) %s\n",
				 i,
				 BreakPoints[i],
				 BreakDescription[(BreakType[i] & 3)],
				 BreakLengthDescription[(BreakLength[i] & 3)],
				 symbolName ? symbolName : "");
	      if (ConditionalBreakpoint[i])
	      {
		 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	      }
	      SetDebugRegisters();
	      return TRUE;
	   }
	}
	printfScreenWithAttribute(screen, BRITEWHITE, "no breakpoint available\n");
     }
     return TRUE;

}

// BST

LONG breakpointShowTemp(SCREEN *screen, BYTE *cmd,
			StackFrame *stackFrame, LONG Exception,
			DEBUGGER_PARSER *parser)
{
     register LONG address, i, r;
     register BYTE *pB, *symbolName;
     LONG valid;

     if (screen) {};
     if (cmd) {};
     if (stackFrame) {};
     if (Exception) {};
     if (parser) {};

     for (i=0; i < 4; i++)
     {
	if (BreakReserved[i] && BreakTemp[i])
	{
	   symbolName = GetSymbolFromValue(BreakPoints[i]);
	   printfScreenWithAttribute(screen, BRITEWHITE,
				"Break %i is at 0x%08X (%s %s) %s [%s]\n",
				i,
				BreakPoints[i],
				BreakDescription[(BreakType[i] & 3)],
				BreakLengthDescription[(BreakLength[i] & 3)],
				symbolName ? symbolName : "",
				BreakGo[i] ? "GO" : BreakProceed[i] ? "PROCEED" : "");
	   if (ConditionalBreakpoint[i])
	   {
		 printfScreenWithAttribute(screen, BRITEWHITE,
				   "if (%s) is TRUE\n",
				   BreakCondition[i]);
	   }
	}
     }
     return TRUE;

}

LONG debugger_entry(LONG Exception, StackFrame *stackFrame, LONG FaultAddress, BYTE *panicMsg)
{

    register LONG retCode, i;
    register LONG processor = get_physical_processor();

    if (processor > MAX_PROCESSORS)
       return 0;

    set_processor_id(processor);

    //
    //  if we are in real mode, then don't activate the debugger
    //

    if (inRealModeFlag)
       return 0;

    (processor_table[processor].MaskTimer)();

    CurrentFrame[processor] = stackFrame;
    NestedInterrupts[processor]++;
    processor_table[processor].ProcessorState = PROCESSOR_LOCK;
    CurrentDR6[processor] = ReadDR6();

    if (DosDataTable->FPU_TYPE)
       save_npx(&npx[processor]);

    switch(Exception)
    {
       case 0:
	  EventNotify(EVENT_DIVIDE_ZERO, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 1:
	  EventNotify(EVENT_DEBUGGER_EXCEPTION, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 2:
	  EventNotify(EVENT_NMI, (LONG) stackFrame);
	  nmiSetup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 3:
	  EventNotify(EVENT_DEBUGGER_BREAK, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 4:
	  EventNotify(EVENT_OVERFLOW, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 5:
	  EventNotify(EVENT_BOUNDS_CHECK, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 6:
	  EventNotify(EVENT_INVL_OPCODE, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 7:
	  EventNotify(EVENT_NO_COPROCESSOR, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 8:
	  EventNotify(EVENT_DOUBLE_FAULT, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 9:
	  EventNotify(EVENT_COPS, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 10:
	  EventNotify(EVENT_TSS_CHECK, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 11:
	  EventNotify(EVENT_SEGMENT_FAULT, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 12:
	  EventNotify(EVENT_STACK_EXCEPTION, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 13:
	  EventNotify(EVENT_GENERAL_PROTECT, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 14:
	  EventNotify(EVENT_PAGE_FAULT, (LONG) stackFrame);

	  WriteDR7(CurrentDR7);
	  processor_table[processor].pageFaults++;
	  FaultAddressHistory[FaultNest[processor]][processor] = FaultAddress;
	  FaultEIPHistory[FaultNest[processor]][processor] = stackFrame->tEIP;
	  FaultStackHistory[FaultNest[processor]][processor] = stackFrame->tESP;
	  FaultNest[processor]++;
	  retCode = PageFaultHandler(FaultAddress,
				    (LONG) processorSet[processor].running_process,
				    processor,
				    FaultNest[processor],
				    (LONG)stackFrame);
	  cli();

	  WriteDR7(0);
	  switch (retCode)
	  {
	     case 0:
		FaultNest[processor]--;
		FaultAddressHistory[FaultNest[processor]][processor] = 0;
		FaultEIPHistory[FaultNest[processor]][processor] = 0;
		FaultStackHistory[FaultNest[processor]][processor] = 0;
		break;

	     default:
		debugger_setup(processor, Exception, stackFrame, panicMsg);
		FaultNest[processor]--;
		FaultAddressHistory[FaultNest[processor]][processor] = 0;
		FaultEIPHistory[FaultNest[processor]][processor] = 0;
		FaultStackHistory[FaultNest[processor]][processor] = 0;
		break;
	  }
	  break;

       case 16:
	  EventNotify(EVENT_COPROCESSOR_ERROR, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 17:
	  EventNotify(EVENT_ALIGNMENT_CHECK, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       case 18:
	  EventNotify(EVENT_MACHINE_CHECK, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;

       default:
	  EventNotify(EVENT_INVL_INT, (LONG) stackFrame);
	  debugger_setup(processor, Exception, stackFrame, panicMsg);
	  break;
    }

    LoadDebugRegisters();
    if (DosDataTable->FPU_TYPE)
       load_npx(&npx[processor]);

    processor_table[processor].ProcessorState = PROCESSOR_ACTIVE;
    NestedInterrupts[processor]--;
    CurrentFrame[processor] = 0;
    (processor_table[processor].UnmaskTimer)();

    if (ExitOSFlag)
    {
	if (!processor)
	   ExitOS();
	else
	   processor_exit(processor);
    }

    return 0;

}

LONG debugger_setup(LONG processor, LONG Exception, StackFrame *stackFrame,
		    BYTE *panicMsg)
{

      BYTE *cmd;
      LONG valid;

      EventNotify(EVENT_ENTER_DEBUGGER, processor);

      switch (Exception)
      {
	 case 1:
	    if (BreakMask[processor])
	    {
	       stackFrame->tSystemFlags &= ~SINGLE_STEP;
	       stackFrame->tSystemFlags |= RESUME;
	       break;
	    }
	    else
	    if (CurrentDR6[processor] & B0_BIT && CurrentDR7 & G0_BIT && ConditionalBreakpoint[0])
	    {
	       cmd = &BreakCondition[0][0];
	       if (!EvaluateExpression(stackFrame, &cmd, &valid))
	       {
		  stackFrame->tSystemFlags &= ~SINGLE_STEP;
		  stackFrame->tSystemFlags |= RESUME;
		  break;
	       }
	    }
	    else
	    if (CurrentDR6[processor] & B1_BIT && CurrentDR7 & G1_BIT && ConditionalBreakpoint[1])
	    {
	       cmd = &BreakCondition[1][0];
	       if (!EvaluateExpression(stackFrame, &cmd, &valid))
	       {
		  stackFrame->tSystemFlags &= ~SINGLE_STEP;
		  stackFrame->tSystemFlags |= RESUME;
		  break;
	       }
	    }
	    else
	    if (CurrentDR6[processor] & B2_BIT && CurrentDR7 & G2_BIT && ConditionalBreakpoint[2])
	    {
	       cmd = &BreakCondition[2][0];
	       if (!EvaluateExpression(stackFrame, &cmd, &valid))
	       {
		  stackFrame->tSystemFlags &= ~SINGLE_STEP;
		  stackFrame->tSystemFlags |= RESUME;
		  break;
	       }
	    }
	    else
	    if (CurrentDR6[processor] & B3_BIT && CurrentDR7 & G3_BIT && ConditionalBreakpoint[3])
	    {
	       cmd = &BreakCondition[3][0];
	       if (!EvaluateExpression(stackFrame, &cmd, &valid))
	       {
		  stackFrame->tSystemFlags &= ~SINGLE_STEP;
		  stackFrame->tSystemFlags |= RESUME;
		  break;
	       }
	    }
	    if (debug_lock(&debug_mutex, processor))
	    {
	       // disable PIC interrupts

	       outb(irq_mask[0], PIC1_DEBUG_MASK);
	       outb(irq_mask[1], PIC2_DEBUG_MASK);

	       StopProcessorsExclSelf(processor);
	       if (!AlternateDebuggerRoutine(debugScreen, stackFrame, Exception, processor))
		  debugger_command_entry(processor, Exception, stackFrame, panicMsg);
	       FreeProcessorsExclSelf(processor);

	       outb(irq_mask[0], mask_value[0]);
	       outb(irq_mask[1], mask_value[1]);

	       debug_unlock(&debug_mutex);
	    }
	    break;


	 case 3:
	    if (BreakMask[processor])
	    {
	       stackFrame->tSystemFlags &= ~SINGLE_STEP;
	       stackFrame->tSystemFlags |= RESUME;
	       break;
	    }
	    if (debug_lock(&debug_mutex, processor))
	    {
	       // disable PIC interrupts

	       outb(irq_mask[0], PIC1_DEBUG_MASK);
	       outb(irq_mask[1], PIC2_DEBUG_MASK);

	       StopProcessorsExclSelf(processor);
	       if (!AlternateDebuggerRoutine(debugScreen, stackFrame, Exception, processor))
		  debugger_command_entry(processor, Exception, stackFrame, panicMsg);
	       FreeProcessorsExclSelf(processor);

	       outb(irq_mask[0], mask_value[0]);
	       outb(irq_mask[1], mask_value[1]);

	       debug_unlock(&debug_mutex);
	    }
	    break;

	 default:
	    if (debug_lock(&debug_mutex, processor))
	    {
	       // disable PIC interrupts

	       outb(irq_mask[0], PIC1_DEBUG_MASK);
	       outb(irq_mask[1], PIC2_DEBUG_MASK);

	       StopProcessorsExclSelf(processor);
	       if (!AlternateDebuggerRoutine(debugScreen, stackFrame, Exception, processor))
		  debugger_command_entry(processor, Exception, stackFrame, panicMsg);
	       FreeProcessorsExclSelf(processor);

	       outb(irq_mask[0], mask_value[0]);
	       outb(irq_mask[1], mask_value[1]);

	       debug_unlock(&debug_mutex);
	    }
	    break;
      }

      EventNotify(EVENT_LEAVE_DEBUGGER, processor);

      return 0;

}


void InitializeDebuggerRegisters(void)
{

   CurrentDR7 = 0;
   CurrentDR7 = (GEXACT | LEXACT);   // set mode to GLOBAL EXACT
   WriteDR0(0);                      // clear out DR0-DR6
   WriteDR1(0);
   WriteDR2(0);
   WriteDR3(0);
   WriteDR6(0);
   WriteDR7(CurrentDR7);            // set DR7 register

}

void ClearDebuggerRegisters(void)
{
   WriteDR0(0);   // clear out all breakpoints and breakpoint
   WriteDR1(0);   // registers DR0-DR7
   WriteDR2(0);
   WriteDR3(0);
   WriteDR6(0);
   WriteDR7(0);
}


void displayMTRRRegisters(SCREEN *debugScreen)
{

    register int i;
    LONG base1, base2;
    LONG mask1, mask2;

    if (DosDataTable->CPU_TYPE == 6)
    {
       printfScreenWithAttribute(debugScreen, BRITEWHITE, "memory type range registers\n");
       for (i = 0; i < 8; i++)
       {
	  ReadMSR(MTRR_BASE_REGS[i], &base1, &base2);
	  ReadMSR(MTRR_MASK_VALUES[i], &mask1, &mask2);
	  printfScreenWithAttribute(debugScreen, BRITEWHITE,
			  "MTRR_BASE_%i  %08X:%08X   MTRR_MASK_%i  %08X:%08X\n",
			   i, base1, base2, i, mask1, mask2);
       }
    }
    else
       printfScreenWithAttribute(debugScreen, BRITEWHITE, "memory type range registers are Pentium Pro/II/Xeon only\n");
    return;

}


void DisplayGDT(SCREEN *debugScreen, BYTE *GDT_ADDRESS)
{

    register int i, r;
    LONG count;
    LONG gdt_pointer;
    WORD gdt_index;
    BYTE *p;
    BYTE GDTR[8];
    GDT *gdt;
    TSS *tss;

    SetPauseMode(debugScreen, debugScreen->nLines - 3);

    if (GDT_ADDRESS)
    {
       gdt_index = *(WORD *)(&GDT_ADDRESS[0]);
       gdt_pointer = *(LONG *)(&GDT_ADDRESS[2]);
    }
    else
    {
       ReadGDTR((LONG *)&GDTR[0]);
       gdt_index = *(WORD *)(&GDTR[0]);
       gdt_pointer = *(LONG *)(&GDTR[2]);
    }

    printfScreenWithAttribute(debugScreen, BRITEWHITE, "GDTR: %04X:%08X  Processor: %i\n", gdt_index,
			gdt_pointer, get_processor_id());

    count = 0;
    gdt_index = (gdt_index + 7) / 8;
    p = (BYTE *) gdt_pointer;
    for (i=0; i < gdt_index; i++)
    {
       printfScreenWithAttribute(debugScreen, BRITEWHITE, "%08X (%04i):", (LONG) count, i);
       gdt = (GDT *) &p[0];
       for (r=0; r < 8; r++)
       {
	  printfScreenWithAttribute(debugScreen, LTCYAN, " %02X", (BYTE) p[r]);
       }
       if ((gdt->GDTType & 0x92) == 0x92)
       {
	  printfScreenWithAttribute(debugScreen, LTCYAN, "  b:%08X lim:%08X t:%02X ot:%02X",
			   ((gdt->Base3 << 24) | (gdt->Base2 << 16) | (gdt->Base1)),
			   (((gdt->OtherType & 0xF) << 16) | (gdt->Limit)),
			   gdt->GDTType, gdt->OtherType);
       }
       else if ((gdt->GDTType & 0x89) == 0x89)
       {
	  tss = (TSS *) gdt;
	  printfScreenWithAttribute(debugScreen, LTCYAN, "  tss:%08X lim:%04X t:%02X ot:%02X",
			   ((tss->TSSBase3 << 24) | (tss->TSSBase2 << 16) | (tss->TSSBase1)),
			   tss->TSSLimit, tss->TSSType, tss->TSSOtherType);
       }
       if (printfScreen(debugScreen, "\n"))
	  break;

       p = (void *)((LONG) p + (LONG) 8);
       count += 8;
    }

    ClearPauseMode(debugScreen);

    return;

}

void DisplayIDT(SCREEN *debugScreen, BYTE *IDT_ADDRESS)
{

    register int i, r;
    LONG count;
    LONG idt_pointer;
    WORD idt_index;
    BYTE *p;
    BYTE IDTR[8];
    IDT *idt;
    TSS_GATE *tss_gate;

    SetPauseMode(debugScreen, debugScreen->nLines - 3);

    if (IDT_ADDRESS)
    {
       idt_index = *(WORD *)(&IDT_ADDRESS[0]);
       idt_pointer = *(LONG *)(&IDT_ADDRESS[2]);
    }
    else
    {
       ReadIDTR((LONG *)&IDTR[0]);
       idt_index = *(WORD *)(&IDTR[0]);
       idt_pointer = *(LONG *)(&IDTR[2]);
    }

    printfScreenWithAttribute(debugScreen, BRITEWHITE, "IDTR: %04X:%08X  Processor: %i\n", idt_index,
			idt_pointer, get_processor_id());

    count = 0;
    idt_index = (idt_index + 7) / 8;
    p = (BYTE *) idt_pointer;
    for (i=0; i < idt_index; i++)
    {
       printfScreenWithAttribute(debugScreen, BRITEWHITE, "%08X (%04i):", (LONG) count, i);
       idt = (IDT *) &p[0];
       for (r=0; r < 8; r++)
       {
	   printfScreenWithAttribute(debugScreen, LTCYAN, " %02X", (BYTE) p[r]);
       }
       if ((idt->IDTFlags & 0x8E) == 0x8E)
       {
	  printfScreenWithAttribute(debugScreen, LTCYAN, "  b:%08X s:%04X t:%02X ot:%02X",
			     ((idt->IDTHigh << 16) | (idt->IDTLow)),
			     idt->IDTSegment,
			     idt->IDTFlags, idt->IDTSkip);

       }
       else if ((idt->IDTFlags & 0x85) == 0x85)
       {
	  tss_gate = (TSS_GATE *) idt;
	  printfScreenWithAttribute(debugScreen, LTCYAN, "  task_gate: %04X t:%02X",
			     tss_gate->TSSSelector, tss_gate->TSSFlags);
       }
       if (printfScreen(debugScreen, "\n"))
	  break;

       p = (void *)((LONG) p + (LONG) 8);
       count += 8;
    }

    ClearPauseMode(debugScreen);

    return;

}

void SetDebugRegisters(void)
{
   register int i;

   for (i=0; i < 4; i++)
   {
      switch (i)
      {
	 case 0:
	    if (BreakReserved[i])
	    {
	       CurrentDR7 &= 0xFFF0FFFF;
	       CurrentDR7 |= G0_BIT;
	       CurrentDR7 |= ((BreakType[i] << ((i * 4) + 16)) |
			      (BreakLength[i] << ((i * 4) + 18)));
	    }
	    else
	    {
	       CurrentDR7 &= 0xFFF0FFFF;
	       CurrentDR7 &= ~G0_BIT;
	       CurrentDR7 &= ~L0_BIT;
	    }
	    WriteDR0(BreakPoints[i]);
	    break;

	 case 1:
	    if (BreakReserved[i])
	    {
	       CurrentDR7 &= 0xFF0FFFFF;
	       CurrentDR7 |= G1_BIT;
	       CurrentDR7 |= ((BreakType[i] << ((i * 4) + 16)) |
			      (BreakLength[i] << ((i * 4) + 18)));
	    }
	    else
	    {
	       CurrentDR7 &= 0xFF0FFFFF;
	       CurrentDR7 &= ~G1_BIT;
	       CurrentDR7 &= ~L1_BIT;
	    }
	    WriteDR1(BreakPoints[i]);
	    break;

	 case 2:
	    if (BreakReserved[i])
	    {
	       CurrentDR7 &= 0xF0FFFFFF;
	       CurrentDR7 |= G2_BIT;
	       CurrentDR7 |= ((BreakType[i] << ((i * 4) + 16)) |
			      (BreakLength[i] << ((i * 4) + 18)));
	    }
	    else
	    {
	       CurrentDR7 &= 0xF0FFFFFF;
	       CurrentDR7 &= ~G2_BIT;
	       CurrentDR7 &= ~L2_BIT;
	    }
	    WriteDR2(BreakPoints[i]);
	    break;

	 case 3:
	    if (BreakReserved[i])
	    {
	       CurrentDR7 &= 0x0FFFFFFF;
	       CurrentDR7 |= G3_BIT;
	       CurrentDR7 |= ((BreakType[i] << ((i * 4) + 16)) |
			      (BreakLength[i] << ((i * 4) + 18)));
	    }
	    else
	    {
	       CurrentDR7 &= 0x0FFFFFFF;
	       CurrentDR7 &= ~G3_BIT;
	       CurrentDR7 &= ~L3_BIT;
	    }
	    WriteDR3(BreakPoints[i]);
	    break;

      }
   }
   return;

}

void LoadDebugRegisters(void)
{

   register int i;

   for (i=0; i < 4; i++)
   {
      switch (i)
      {
	 case 0:
	    if (BreakReserved[i])
	       WriteDR0(BreakPoints[i]);
	    break;

	 case 1:
	    if (BreakReserved[i])
	       WriteDR1(BreakPoints[i]);
	    break;

	 case 2:
	    if (BreakReserved[i])
	       WriteDR2(BreakPoints[i]);
	    break;

	 case 3:
	    if (BreakReserved[i])
	       WriteDR3(BreakPoints[i]);
	    break;
      }
   }

}

void DisplayTSS(SCREEN *debugScreen, StackFrame *stackFrame)
{

    LONG flags, i, f = 0;

    printfScreenWithAttribute(debugScreen, BRITEWHITE, "Task State Segment at 0x%08X\n", stackFrame);
    printfScreenWithAttribute(debugScreen, LTCYAN, "LDT: %08X  CR3: %08X  IOMAP: %08X  BLINK: %08X\n",
	stackFrame->tLDT, stackFrame->tCR3, stackFrame->tIOMap, stackFrame->tReserved[0]);
    printfScreenWithAttribute(debugScreen, LTCYAN, "CS: %04X  DS: %04X  ES: %04X  FS: %04X  GS: %04X  SS: %04X\n",
       stackFrame->tCS, stackFrame->tDS, stackFrame->tES,
       stackFrame->tFS, stackFrame->tGS, stackFrame->tSS);
    printfScreenWithAttribute(debugScreen, LTCYAN, "EAX: %08X  EBX: %08X  ECX: %08X  EDX: %08X\n",
	stackFrame->tEAX, stackFrame->tEBX, stackFrame->tECX, stackFrame->tEDX);
    printfScreenWithAttribute(debugScreen, LTCYAN, "ESI: %08X  EDI: %08X  ESP: %08X  EBP: %08X\n",
	stackFrame->tESI, stackFrame->tEDI, stackFrame->tESP, stackFrame->tEBP);
    printfScreenWithAttribute(debugScreen, LTCYAN, "EIP: %08X  FLAGS: %08X ",
	stackFrame->tEIP, stackFrame->tSystemFlags);
    printfScreenWithAttribute(debugScreen, LTCYAN, " (");
    for (i=0; i < 22; i++)
    {
       if (IA32Flags[i])
       {
	  if ((stackFrame->tSystemFlags >> i) & 0x00000001)
	  {
	     if (f)
		printfScreen(debugScreen, " ");
	     f = 1;
	     printfScreenWithAttribute(debugScreen, LTCYAN, "%s", IA32Flags[i]);
	  }
       }
    }
    printfScreenWithAttribute(debugScreen, LTCYAN, ")\n");


}

void WindowRegisterFrame(LONG num, StackFrame *stackFrame)
{

      BYTE registerBuffer[100] = { "" };
      LONG *stack;
      LONG curCR2;

      sprintf(registerBuffer, " EAX: %08X ", stackFrame->tEAX);
      window_write_string(num, registerBuffer, 0, 0,
			  stackFrame->tEAX != lastStackFrame.tEAX
			  ? BRITEWHITE
			  : LTCYAN);

      sprintf(registerBuffer, " EBX: %08X ", stackFrame->tEBX);
      window_write_string(num, registerBuffer, 1, 0,
			  stackFrame->tEBX != lastStackFrame.tEBX
			  ? BRITEWHITE
			  : LTCYAN);

      sprintf(registerBuffer, " ECX: %08X ", stackFrame->tECX);
      window_write_string(num, registerBuffer, 2, 0,
			  stackFrame->tECX != lastStackFrame.tECX
			  ? BRITEWHITE
			  : LTCYAN);

      sprintf(registerBuffer, " EDX: %08X ", stackFrame->tEDX);
      window_write_string(num, registerBuffer, 3, 0,
			  stackFrame->tEDX != lastStackFrame.tEDX
			  ? BRITEWHITE
			  : LTCYAN);

      sprintf(registerBuffer, " ESI: %08X ", stackFrame->tESI);
      window_write_string(num, registerBuffer, 4, 0,
			  stackFrame->tESI != lastStackFrame.tESI
			  ? BRITEWHITE
			  : LTCYAN);

      sprintf(registerBuffer, " EDI: %08X ", stackFrame->tEDI);
      window_write_string(num, registerBuffer, 5, 0,
			  stackFrame->tEDI != lastStackFrame.tEDI
			  ? BRITEWHITE
			  : LTCYAN);

      sprintf(registerBuffer, " EBP: %08X ", stackFrame->tEBP);
      window_write_string(num, registerBuffer, 6, 0,
			  stackFrame->tEBP != lastStackFrame.tEBP
			  ? BRITEWHITE
			  : LTCYAN);

      sprintf(registerBuffer, " ESP: %08X ", stackFrame->tESP);
      window_write_string(num, registerBuffer, 7, 0,
			  stackFrame->tESP != lastStackFrame.tESP
			  ? BRITEWHITE
			  : LTCYAN);

      sprintf(registerBuffer, " EIP: %08X ", stackFrame->tEIP);
      window_write_string(num, registerBuffer, 8, 0,
			  stackFrame->tEIP != lastStackFrame.tEIP
			  ? BRITEWHITE
			  : LTCYAN);

      sprintf(registerBuffer, " FLG: %08X ", stackFrame->tSystemFlags);
      window_write_string(num, registerBuffer, 9, 0,
			  stackFrame->tSystemFlags != lastStackFrame.tSystemFlags
			  ? BRITEWHITE
			  : LTCYAN);

      sprintf(registerBuffer, " CR3: %08X ", stackFrame->tCR3);
      window_write_string(num, registerBuffer, 10, 0,
			  stackFrame->tCR3 != lastStackFrame.tCR3
			  ? BRITEWHITE
			  : LTCYAN);

      curCR2 = ReadCR2();
      sprintf(registerBuffer, " CR2: %08X ", curCR2);
      window_write_string(num, registerBuffer, 11, 0,
			  curCR2 != lastCR2
			  ? BRITEWHITE
			  : LTCYAN);

      stack = (LONG *) stackFrame->tESP;
      sprintf(registerBuffer, " +28: %08X ", stack[10]);
      window_write_string(num, registerBuffer, 12, 0, LTGREEN);

      sprintf(registerBuffer, " +24: %08X ", stack[9]);
      window_write_string(num, registerBuffer, 13, 0, LTGREEN);

      sprintf(registerBuffer, " +20: %08X ", stack[8]);
      window_write_string(num, registerBuffer, 14, 0, LTGREEN);

      sprintf(registerBuffer, " +1C: %08X ", stack[7]);
      window_write_string(num, registerBuffer, 15, 0, LTGREEN);

      sprintf(registerBuffer, " +18: %08X ", stack[6]);
      window_write_string(num, registerBuffer, 16, 0, LTGREEN);

      sprintf(registerBuffer, " +14: %08X ", stack[5]);
      window_write_string(num, registerBuffer, 17, 0, LTGREEN);

      sprintf(registerBuffer, " +10: %08X ", stack[4]);
      window_write_string(num, registerBuffer, 18, 0, LTGREEN);

      sprintf(registerBuffer, " +0C: %08X ", stack[3]);
      window_write_string(num, registerBuffer, 19, 0, LTGREEN);

      sprintf(registerBuffer, " +08: %08X ", stack[2]);
      window_write_string(num, registerBuffer, 20, 0, LTGREEN);

      sprintf(registerBuffer, " +04: %08X ", stack[1]);
      window_write_string(num, registerBuffer, 21, 0, LTGREEN);

      sprintf(registerBuffer, " +00: %08X ", stack[0]);
      window_write_string(num, registerBuffer, 22, 0, LTGREEN);


}

void DisplayGeneralRegisters(SCREEN *debugScreen, StackFrame *stackFrame)
{

    LONG flags, i, f = 0;

    printfScreenWithAttribute(debugScreen,
	  stackFrame->tEAX != lastStackFrame.tEAX
	  ? LTCYAN
	  : CYAN,
	  "EAX: %08X ", stackFrame->tEAX);

    printfScreenWithAttribute(debugScreen,
	  stackFrame->tEBX != lastStackFrame.tEBX
	  ? LTCYAN
	  : CYAN,
	  "EBX: %08X ", stackFrame->tEBX);

    printfScreenWithAttribute(debugScreen,
	  stackFrame->tECX != lastStackFrame.tECX
	  ? LTCYAN
	  : CYAN,
	  "ECX: %08X ", stackFrame->tECX);

    printfScreenWithAttribute(debugScreen,
	  stackFrame->tEDX != lastStackFrame.tEDX
	  ? LTCYAN
	  : CYAN,
	  "EDX: %08X\n", stackFrame->tEDX);

    printfScreenWithAttribute(debugScreen,
	  stackFrame->tESI != lastStackFrame.tESI
	  ? LTCYAN
	  : CYAN,
	  "ESI: %08X ", stackFrame->tESI);

    printfScreenWithAttribute(debugScreen,
	  stackFrame->tEDI != lastStackFrame.tEDI
	  ? LTCYAN
	  : CYAN,
	  "EDI: %08X ", stackFrame->tEDI);

    printfScreenWithAttribute(debugScreen,
	  stackFrame->tESP != lastStackFrame.tESP
	  ? LTCYAN
	  : CYAN,
	  "ESP: %08X ", stackFrame->tESP);

    printfScreenWithAttribute(debugScreen,
	  stackFrame->tEBP != lastStackFrame.tEBP
	  ? LTCYAN
	  : CYAN,
	  "EBP: %08X\n", stackFrame->tEBP);

    printfScreenWithAttribute(debugScreen,
	  stackFrame->tEIP != lastStackFrame.tEIP
	  ? LTCYAN
	  : CYAN,
	  "EIP: %08X ", stackFrame->tEIP);

    printfScreenWithAttribute(debugScreen,
	  stackFrame->tSystemFlags != lastStackFrame.tSystemFlags
	  ? LTCYAN
	  : CYAN,
	  "FLAGS: %08X ", stackFrame->tSystemFlags);

    printfScreenWithAttribute(debugScreen, LTRED, " (");
    for (i=0; i < 22; i++)
    {
       if (IA32Flags[i])
       {
	  if ((stackFrame->tSystemFlags >> i) & 0x00000001)
	  {
	     if (f)
		printfScreen(debugScreen, " ");
	     f = 1;
	     printfScreenWithAttribute(debugScreen, LTRED, "%s", IA32Flags[i]);
	  }
       }
    }
    printfScreenWithAttribute(debugScreen, LTRED, ")\n");

}

void DisplaySegmentRegisters(SCREEN *debugScreen, StackFrame *stackFrame)
{

    printfScreenWithAttribute(debugScreen,
			    stackFrame->tCS != lastStackFrame.tCS
			    ? LTCYAN
			    : CYAN,
			    "CS: %04X ",
			    stackFrame->tCS);

    printfScreenWithAttribute(debugScreen,
			    stackFrame->tDS != lastStackFrame.tDS
			    ? LTCYAN
			    : CYAN,
			    "DS: %04X ",
			    stackFrame->tDS);

    printfScreenWithAttribute(debugScreen,
			    stackFrame->tES != lastStackFrame.tES
			    ? LTCYAN
			    : CYAN,
			    "ES: %04X ",
			    stackFrame->tES);

    printfScreenWithAttribute(debugScreen,
			    stackFrame->tFS != lastStackFrame.tFS
			    ? LTCYAN
			    : CYAN,
			    "FS: %04X ",
			    stackFrame->tFS);

    printfScreenWithAttribute(debugScreen,
			    stackFrame->tGS != lastStackFrame.tGS
			    ? LTCYAN
			    : CYAN,
			    "GS: %04X ",
			    stackFrame->tGS);

    printfScreenWithAttribute(debugScreen,
			    stackFrame->tSS != lastStackFrame.tSS
			    ? LTCYAN
			    : CYAN,
			    "SS: %04X\n",
			    stackFrame->tSS);

}

void DisplayControlRegisters(SCREEN *debugScreen, LONG processor, StackFrame *stackFrame)
{

    BYTE GDTR[8], IDTR[8];

    if (stackFrame) {};

    printfScreenWithAttribute(debugScreen,
			    LTCYAN,
			    "CR0: %08X ",
			    ReadCR0());

    printfScreenWithAttribute(debugScreen,
			    LTCYAN,
			    "CR2: %08X ",
			    ReadCR2());

    printfScreenWithAttribute(debugScreen,
			    LTCYAN,
			    "CR3: %08X ",
			    ReadCR3());

    printfScreenWithAttribute(debugScreen,
			    LTCYAN,
			    "CR4: %08X\n",
			    ReadCR4());

    printfScreenWithAttribute(debugScreen,
			    LTCYAN,
			    "DR0: %08X ",
			    ReadDR0());

    printfScreenWithAttribute(debugScreen,
			    LTCYAN,
			    "DR1: %08X ",
			    ReadDR1());

    printfScreenWithAttribute(debugScreen,
			    LTCYAN,
			    "DR2: %08X ",
			    ReadDR2());

    printfScreenWithAttribute(debugScreen,
			    LTCYAN,
			    "DR3: %08X\n",
			    ReadDR3());

    printfScreenWithAttribute(debugScreen,
			    LTCYAN,
			    "DR6: %08X ",
			    ReadDR6());

    printfScreenWithAttribute(debugScreen,
			    LTCYAN,
			    "DR7: %08X ",
			    ReadDR7());

    printfScreenWithAttribute(debugScreen,
			    LTCYAN,
			    "VR6: %08X ",
			    CurrentDR6[processor]);

    printfScreenWithAttribute(debugScreen,
			    LTCYAN,
			    "VR7: %08X\n",
			    CurrentDR7);

    ReadGDTR((LONG *)&GDTR[0]);
    ReadIDTR((LONG *)&IDTR[0]);
    printfScreenWithAttribute(debugScreen, LTCYAN, "GDTR: %04X:%08X IDTR: %04X:%08X  LDTR: %04X  TR: %04X\n",
			*(WORD *)&GDTR[0], *(LONG *)&GDTR[2],
			*(WORD *)&IDTR[0], *(LONG *)&IDTR[2],
			ReadLDTR(), ReadTR());

}

double ldexp(double v, int e)
{

   double two = 2.0;

   if (e < 0)
   {
      e = -e; /* This just might overflow on two-complement machines.  */
      if (e < 0) return 0.0;
      while (e > 0)
      {
	 if (e & 1) v /= two;
	 two *= two;
	 e >>= 1;
      }
   }
   else if (e > 0)
   {
      while (e > 0)
      {
	 if (e & 1) v *= two;
	 two *= two;
	 e >>= 1;
      }
   }
   return v;

}

void DisplayNPXRegisters(SCREEN *debugScreen, LONG processor)
{

     register int i;
     double d;
     int tag;
     int tos;

     tos = (npx[processor].status >> 11) & 7;
     if (tos) {};

     printfScreenWithAttribute(debugScreen, LTCYAN, "Control: 0x%04X  Status: 0x%04X  Tag: 0x%04X  Processor: %i\n",
	       npx[processor].control & 0xFFFF, npx[processor].status & 0xFFFF, npx[processor].tag & 0xFFFF,
	       processor);

     for (i = 0; i < 8; i++)
     {
	tos = (npx[processor].status >> 11) & 7;
	printfScreenWithAttribute(debugScreen, LTCYAN, "st(%d)  ", i);

	if (npx[processor].reg[i].sign)
	    printfScreenWithAttribute(debugScreen, LTCYAN, "-");
	else
	    printfScreenWithAttribute(debugScreen, LTCYAN, "+");

	printfScreenWithAttribute(debugScreen, LTCYAN, " %04X %04X %04X %04X e %04X    ",
		 npx[processor].reg[i].sig3,
		 npx[processor].reg[i].sig2,
		 npx[processor].reg[i].sig1,
		 npx[processor].reg[i].sig0,
		 npx[processor].reg[i].exponent);

	 if (tos) {};
	 tag = (npx[processor].tag >> (((i + tos) % 8) * 2)) & 3;
	 switch (tag)
	 {

	    case 0:
	       printfScreenWithAttribute(debugScreen, LTCYAN, "Valid");

	       if (((int) npx[processor].reg[i].exponent - 16382 < 1000) &&
		  ((int) npx[processor].reg[i].exponent - 16382 > -1000))
	       {
		  d = npx[processor].reg[i].sig3 / 65536.0 + npx[processor].reg[i].sig2 / 65536.0 / 65536.0
		   + npx[processor].reg[i].sig1 / 65536.0 / 65536.0 / 65536.0;
		  d = ldexp(d, (int) npx[processor].reg[i].exponent - 16382);
		  if (npx[processor].reg[i].sign)
		     d = -d;
		  printfScreenWithAttribute(debugScreen, LTCYAN, "  %.16g", d);
	       }
	       else
		  printfScreenWithAttribute(debugScreen, LTCYAN, "  (too big to display)");

	       printfScreen(debugScreen, "\n");
	       break;

	    case 1:
	       printfScreenWithAttribute(debugScreen, LTCYAN, "Zero\n");
	       break;

	    case 2:
	       printfScreenWithAttribute(debugScreen, LTCYAN, "Special\n");
	       break;

	    case 3:
	       printfScreenWithAttribute(debugScreen, LTCYAN, "Empty\n");
	       break;
	}
     }

}

void ConsoleDisplayBreakReason(SCREEN *debugScreen, StackFrame *stackFrame,
			LONG Exception, BYTE *panicMsg, LONG processor,
			LONG lastCommand)
{
       if ((CurrentDR6[processor] & B0_BIT) && (CurrentDR7 & G0_BIT) && Exception == 1)
       {
	  if (BreakGo[0])
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - GO breakpoint (0)\n",
				 stackFrame->tEIP);
	  else if (BreakProceed[0])
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - Proceed breakpoint (0)\n",
				 stackFrame->tEIP);
	  else if (ConditionalBreakpoint[0])
	  {
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - breakpoint 0 (%s)\n",
				 stackFrame->tEIP, BreakDescription[(BreakType[0] & 3)]);
	     printfScreenWithAttribute(debugScreen, BRITEWHITE, "expr: %s was TRUE\n",
				 BreakCondition[0]);
	  }
	  else
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - breakpoint 0 (%s)\n",
				 stackFrame->tEIP, BreakDescription[(BreakType[0] & 3)]);
       }
       else if ((CurrentDR6[processor] & B1_BIT) && (CurrentDR7 & G1_BIT) && Exception == 1)
       {
	  if (BreakGo[1])
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - GO breakpoint (1)\n",
				 stackFrame->tEIP);
	  else if (BreakProceed[1])
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - Proceed breakpoint (1)\n",
				 stackFrame->tEIP);
	  else if (ConditionalBreakpoint[1])
	  {
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - breakpoint 1 (%s)\n",
				 stackFrame->tEIP, BreakDescription[(BreakType[1] & 3)]);
	     printfScreenWithAttribute(debugScreen, BRITEWHITE, "expr: %s was TRUE\n",
				 BreakCondition[1]);
	  }
	  else
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - breakpoint 1 (%s)\n",
				 stackFrame->tEIP, BreakDescription[(BreakType[1] & 3)]);
       }
       else if ((CurrentDR6[processor] & B2_BIT) && (CurrentDR7 & G2_BIT) && Exception == 1)
       {
	  if (BreakGo[2])
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - GO breakpoint (2)\n",
				 stackFrame->tEIP);
	  else if (BreakProceed[2])
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - Proceed breakpoint (2)\n",
				 stackFrame->tEIP);
	  else if (ConditionalBreakpoint[2])
	  {
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - breakpoint 2 (%s)\n",
				 stackFrame->tEIP, BreakDescription[(BreakType[2] & 3)]);
	     printfScreenWithAttribute(debugScreen, BRITEWHITE, "expr: %s was TRUE\n",
				 BreakCondition[2]);
	  }
	  else
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - breakpoint 2 (%s)\n",
				 stackFrame->tEIP, BreakDescription[(BreakType[2] & 3)]);
       }
       else if ((CurrentDR6[processor] & B3_BIT) && (CurrentDR7 & G3_BIT) && Exception == 1)
       {
	  if (BreakGo[3])
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - GO breakpoint (3)\n",
				 stackFrame->tEIP);
	  else if (BreakProceed[3])
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - Proceed breakpoint (3)\n",
				 stackFrame->tEIP);
	  else if (ConditionalBreakpoint[3])
	  {
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - breakpoint 3 (%s)\n",
				 stackFrame->tEIP, BreakDescription[(BreakType[3] & 3)]);
	     printfScreenWithAttribute(debugScreen, BRITEWHITE, "expr: %s was TRUE\n",
				 BreakCondition[3]);
	  }
	  else
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - breakpoint 3 (%s)\n",
				 stackFrame->tEIP, BreakDescription[(BreakType[3] & 3)]);
       }
       else
       {
	  // if the last command was a Proceed that was converted into a
	  // single step command, report proceed single step
	  if (lastCommand == 'P' && Exception == 1)
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - Proceed (single step)\n", stackFrame->tEIP);
	  else if (lastCommand == 'T' && Exception == 1)
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - Trace (single step)\n", stackFrame->tEIP);
	  else if (lastCommand == F8 && Exception == 1)
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - Proceed (single step)\n", stackFrame->tEIP);
	  else if (lastCommand == F7 && Exception == 1)
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - Trace (single step)\n", stackFrame->tEIP);
	  else
	     printfScreenWithAttribute(debugScreen, LTGREEN, "Break at 0x%08X due to - %s\n", stackFrame->tEIP, ExceptionDescription[Exception]);
       }
       if (Exception == 23 && panicMsg)
	  printfScreenWithAttribute(debugScreen, LTGREEN, " %s\n", panicMsg);

}

void SourceDisplayBreakReason(SCREEN *sourceScreen, StackFrame *stackFrame,
			       LONG Exception, BYTE *panicMsg, LONG processor,
			       LONG lastCommand)
{
    extern void DisplaySLDFunctionLine(SCREEN *screen);
    BYTE displayBuffer[256] = {""};
    BYTE workBuffer[100] = {""};
    register LONG i;

    if ((CurrentDR6[processor] & B0_BIT) && (CurrentDR7 & G0_BIT) && Exception == 1)
    {
	  if (BreakGo[0])
	     sprintf(displayBuffer, "Break at 0x%08X due to - GO breakpoint (0)",
	     stackFrame->tEIP);
	  else if (BreakProceed[0])
	     sprintf(displayBuffer, "Break at 0x%08X due to - Proceed breakpoint (0)",
	     stackFrame->tEIP);
	  else if (ConditionalBreakpoint[0])
	     sprintf(displayBuffer, "Break at 0x%08X due to - breakpoint 0 (%s)",
	     stackFrame->tEIP, BreakDescription[(BreakType[0] & 3)]);
	  else
	     sprintf(displayBuffer, "Break at 0x%08X due to - breakpoint 0 (%s)",
	     stackFrame->tEIP, BreakDescription[(BreakType[0] & 3)]);
    }
    else if ((CurrentDR6[processor] & B1_BIT) && (CurrentDR7 & G1_BIT) && Exception == 1)
    {
	  if (BreakGo[1])
	     sprintf(displayBuffer, "Break at 0x%08X due to - GO breakpoint (1)",
	     stackFrame->tEIP);
	  else if (BreakProceed[1])
	     sprintf(displayBuffer, "Break at 0x%08X due to - Proceed breakpoint (1)",
	     stackFrame->tEIP);
	  else if (ConditionalBreakpoint[1])
	     sprintf(displayBuffer, "Break at 0x%08X due to - breakpoint 1 (%s)",
	     stackFrame->tEIP, BreakDescription[(BreakType[1] & 3)]);
	  else
	     sprintf(displayBuffer, "Break at 0x%08X due to - breakpoint 1 (%s)",
	     stackFrame->tEIP, BreakDescription[(BreakType[1] & 3)]);
    }
    else if ((CurrentDR6[processor] & B2_BIT) && (CurrentDR7 & G2_BIT) && Exception == 1)
    {
	  if (BreakGo[2])
	     sprintf(displayBuffer, "Break at 0x%08X due to - GO breakpoint (2)",
	     stackFrame->tEIP);
	  else if (BreakProceed[2])
	     sprintf(displayBuffer, "Break at 0x%08X due to - Proceed breakpoint (2)",
	     stackFrame->tEIP);
	  else if (ConditionalBreakpoint[2])
	     sprintf(displayBuffer, "Break at 0x%08X due to - breakpoint 2 (%s)",
	     stackFrame->tEIP, BreakDescription[(BreakType[2] & 3)]);
	  else
	     sprintf(displayBuffer, "Break at 0x%08X due to - breakpoint 2 (%s)",
	     stackFrame->tEIP, BreakDescription[(BreakType[2] & 3)]);
    }
    else if ((CurrentDR6[processor] & B3_BIT) && (CurrentDR7 & G3_BIT) && Exception == 1)
    {
	  if (BreakGo[3])
	     sprintf(displayBuffer, "Break at 0x%08X due to - GO breakpoint (3)",
	     stackFrame->tEIP);
	  else if (BreakProceed[3])
	     sprintf(displayBuffer, "Break at 0x%08X due to - Proceed breakpoint (3)",
	     stackFrame->tEIP);
	  else if (ConditionalBreakpoint[3])
	     sprintf(displayBuffer, "Break at 0x%08X due to - breakpoint 3 (%s)",
	     stackFrame->tEIP, BreakDescription[(BreakType[3] & 3)]);
	  else
	     sprintf(displayBuffer, "Break at 0x%08X due to - breakpoint 3 (%s)",
	     stackFrame->tEIP, BreakDescription[(BreakType[3] & 3)]);
    }
    else
    {
	  if (lastCommand == 'P' && Exception == 1)
	     sprintf(displayBuffer, "Break at 0x%08X due to - Proceed (single step)", stackFrame->tEIP);
	  else if (lastCommand == 'T' && Exception == 1)
	     sprintf(displayBuffer, "Break at 0x%08X due to - Trace (single step)", stackFrame->tEIP);
	  else if (lastCommand == F8 && Exception == 1)
	     sprintf(displayBuffer, "Break at 0x%08X due to - Proceed (single step)", stackFrame->tEIP);
	  else if (lastCommand == F7 && Exception == 1)
	     sprintf(displayBuffer, "Break at 0x%08X due to - Trace (single step)", stackFrame->tEIP);
	  else
	     sprintf(displayBuffer, "Break at 0x%08X due to - %s", stackFrame->tEIP, ExceptionDescription[Exception]);
    }
    if (Exception == 23 && panicMsg)
       sprintf(displayBuffer, " 0x%08X %s", panicMsg);

    for (i=0; i <  sourceScreen->nCols - strlen(displayBuffer) && *panicMsg; i++)
       workBuffer[i] = panicMsg[i];
    workBuffer[i] = '\0';

    PutVidStringCleol(sourceScreen, displayBuffer,
		      sourceScreen->nLines - 2, BLACK | BGCYAN);

    DisplaySLDFunctionLine(sourceScreen);
    return;

}

