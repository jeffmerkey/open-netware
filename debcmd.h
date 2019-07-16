
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  DEBCMD.H
*   DESCRIP  :  Multi-Processing Debugger Command Library for MANOS v1.0
*   DATE     :  August 6, 1998
*
*
***************************************************************************/

#include "types.h"

extern LONG enterKeyACC(SCREEN *screen, LONG key, void *stackFrame,
		     ACCELERATOR *accel);
extern LONG activateRegisterDisplayACC(SCREEN *screen, LONG key, void *stackFrame,
		     ACCELERATOR *accel);

extern LONG displayDebuggerHelpHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayDebuggerHelp(SCREEN *screen, BYTE *commandLine,
			 StackFrame *stackFrame, LONG Exception,
			 DEBUGGER_PARSER *parser);

extern LONG ascTableHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayASCTable(SCREEN *screen, BYTE *cmd,
		     StackFrame *stackFrame, LONG Exception,
		     DEBUGGER_PARSER *parser);

extern LONG displayScreenStructHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG DisplayScreenStructure(SCREEN *screen, BYTE *cmd,
			    StackFrame *stackFrame, LONG Exception,
			    DEBUGGER_PARSER *parser);

extern LONG displayToggleHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ProcessTUToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG ProcessTDToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG ProcessTLToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG ProcessTGToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG ProcessTCToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG ProcessTNToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG ProcessTRToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG ProcessTSToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG ProcessTAToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG ProcessTToggle(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);

extern LONG displayDebuggerVersionHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG DisplayDebuggerVersion(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);

extern LONG displayKernelProcessHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayKernelProcess(SCREEN *screen, BYTE *cmd,
			  StackFrame *stackFrame, LONG Exception,
			  DEBUGGER_PARSER *parser);

extern LONG displayKernelQueueHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayKernelQueue(SCREEN *screen, BYTE *cmd,
			StackFrame *stackFrame, LONG Exception,
			DEBUGGER_PARSER *parser);

extern LONG displaySymbolsHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displaySymbols(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser);

extern LONG displayLoaderMapHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayLoaderMap(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser);

extern LONG displayModuleHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayModuleInfo(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser);

extern LONG displayProcessesHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayProcesses(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser);

extern LONG displayRegistersHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayControlRegisters(SCREEN *screen, BYTE *cmd,
			     StackFrame *stackFrame, LONG Exception,
			     DEBUGGER_PARSER *parser);
extern LONG displayAllRegisters(SCREEN *screen, BYTE *cmd,
			     StackFrame *stackFrame, LONG Exception,
			     DEBUGGER_PARSER *parser);
extern LONG displaySegmentRegisters(SCREEN *screen, BYTE *cmd,
			     StackFrame *stackFrame, LONG Exception,
			     DEBUGGER_PARSER *parser);
extern LONG displayNumericRegisters(SCREEN *screen, BYTE *cmd,
			     StackFrame *stackFrame, LONG Exception,
			     DEBUGGER_PARSER *parser);
extern LONG displayGeneralRegisters(SCREEN *screen, BYTE *cmd,
			     StackFrame *stackFrame, LONG Exception,
			     DEBUGGER_PARSER *parser);
extern LONG displayDefaultRegisters(SCREEN *screen, BYTE *cmd,
			     StackFrame *stackFrame, LONG Exception,
			     DEBUGGER_PARSER *parser);

extern LONG displayAPICHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayAPICInfo(SCREEN *screen, BYTE *cmd,
		     StackFrame *stackFrame, LONG Exception,
		     DEBUGGER_PARSER *parser);

extern LONG listProcessors(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser);
extern LONG listProcessorFrame(SCREEN *screen, BYTE *cmd,
			StackFrame *stackFrame, LONG Exception,
			DEBUGGER_PARSER *parser);

extern LONG displayMPSHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayMPS(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser);

extern LONG clearScreenHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG clearDebuggerScreen(SCREEN *screen, BYTE *cmd,
			 StackFrame *stackFrame, LONG Exception,
			 DEBUGGER_PARSER *parser);


extern LONG SearchMemoryHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG SearchMemory(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser);
extern LONG SearchMemoryB(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser);
extern LONG SearchMemoryW(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser);
extern LONG SearchMemoryD(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser);

extern LONG changeMemoryHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG changeWordValue(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser);
extern LONG changeDoubleValue(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser);
extern LONG changeByteValue(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser);
extern LONG changeDefaultValue(SCREEN *screen, BYTE *cmd,
			StackFrame *stackFrame, LONG Exception,
			DEBUGGER_PARSER *parser);


extern LONG displayCloseHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayCloseSymbols(SCREEN *screen, BYTE *cmd,
			 StackFrame *stackFrame, LONG Exception,
			 DEBUGGER_PARSER *parser);


extern LONG displayINTRHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayInterruptTable(SCREEN *screen, BYTE *cmd,
			   StackFrame *stackFrame, LONG Exception,
			   DEBUGGER_PARSER *parser);


extern LONG viewScreensHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayScreenList(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser);


extern LONG displayIOAPICHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayIOAPICInfo(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser);



extern LONG displayDumpHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG debuggerWalkStack(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser);
extern LONG debuggerDumpLinkedList(SCREEN *screen, BYTE *cmd,
			    StackFrame *stackFrame, LONG Exception,
			    DEBUGGER_PARSER *parser);
extern LONG debuggerDumpWord(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser);
extern LONG debuggerDumpStack(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser);
extern LONG debuggerDumpDoubleStack(SCREEN *screen, BYTE *cmd,
			     StackFrame *stackFrame, LONG Exception,
			     DEBUGGER_PARSER *parser);
extern LONG debuggerDumpDouble(SCREEN *screen, BYTE *cmd,
			StackFrame *stackFrame, LONG Exception,
			DEBUGGER_PARSER *parser);
extern LONG debuggerDumpByte(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser);


extern LONG displayDisassembleHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG processDisassemble16(SCREEN *screen, BYTE *cmd,
			  StackFrame *stackFrame, LONG Exception,
			  DEBUGGER_PARSER *parser);
extern LONG processDisassemble32(SCREEN *screen, BYTE *cmd,
			  StackFrame *stackFrame, LONG Exception,
			  DEBUGGER_PARSER *parser);
