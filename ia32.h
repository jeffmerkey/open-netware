
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  IA32.H
*   DESCRIP  :  Multi-Processing IA32 specific Debugger code MANOS v1.0
*   DATE     :  December 28, 1997
*
*
***************************************************************************/

#include "types.h"

extern LONG SLDexitToDOSACC(SCREEN *screen, LONG key, void *stackFrame,
		  ACCELERATOR *accel);
extern LONG SLDprocessProceedACC(SCREEN *screen, LONG key, void *stackFrame,
		  ACCELERATOR *accel);
extern LONG SLDprocessTraceACC(SCREEN *screen, LONG key, void *stackFrame,
		  ACCELERATOR *accel);
extern LONG SLDprocessGoACC(SCREEN *screen, LONG key, void *stackFrame,
		  ACCELERATOR *accel);

extern LONG exitToDOSACC(SCREEN *screen, LONG key, void *stackFrame,
		  ACCELERATOR *accel);
extern LONG processProceedACC(SCREEN *screen, LONG key, void *stackFrame,
		  ACCELERATOR *accel);
extern LONG processTraceACC(SCREEN *screen, LONG key, void *stackFrame,
		  ACCELERATOR *accel);
extern LONG processGoACC(SCREEN *screen, LONG key, void *stackFrame,
		  ACCELERATOR *accel);

extern LONG executeCommandHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG exitToDOS(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser);
extern LONG processProceed(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser);
extern LONG processTrace(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser);
extern LONG processGo(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser);

extern LONG processorCommandHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG breakProcessor(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser);
extern LONG breakNMIProcessors(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser);

extern LONG displayEAXHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeEAXRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser);

extern LONG displayEBXHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeEBXRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser);

extern LONG displayECXHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeECXRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser);

extern LONG displayEDXHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeEDXRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser);

extern LONG displayESIHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeESIRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser);

extern LONG displayEDIHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeEDIRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser);

extern LONG displayEBPHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeEBPRegister(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser);

extern LONG displayESPHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeESPRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser);

extern LONG displayEIPHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeEIPRegister(SCREEN *screen, BYTE *cmd,
		       StackFrame *stackFrame, LONG Exception,
		       DEBUGGER_PARSER *parser);

extern LONG displayCSHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeCSRegister(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser);

extern LONG displayDSHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeDSRegister(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser);

extern LONG displayESHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeESRegister(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser);

extern LONG displayFSHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeFSRegister(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser);

extern LONG displayGSHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeGSRegister(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser);

extern LONG displaySSHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeSSRegister(SCREEN *screen, BYTE *cmd,
		      StackFrame *stackFrame, LONG Exception,
		      DEBUGGER_PARSER *parser);

extern LONG displayRFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeRFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser);

extern LONG displayTFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeTFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser);

extern LONG displayZFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeZFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser);

extern LONG displaySFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeSFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser);

extern LONG displayPFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangePFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser);

extern LONG displayCFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeCFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser);

extern LONG displayOFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeOFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser);

extern LONG displayIFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeIFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser);

extern LONG displayIDHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeIDFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser);

extern LONG displayDFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeDFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser);

extern LONG displayNTHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeNTFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser);

extern LONG displayVMHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeVMFlag(SCREEN *screen, BYTE *cmd,
	       StackFrame *stackFrame, LONG Exception,
	       DEBUGGER_PARSER *parser);

extern LONG displayVIFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeVIFFlag(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);

extern LONG displayVIPHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeVIPFlag(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);

extern LONG displayAFHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeAFFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser);

extern LONG displayACHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG ChangeACFlag(SCREEN *screen, BYTE *cmd,
		  StackFrame *stackFrame, LONG Exception,
		  DEBUGGER_PARSER *parser);

extern LONG displayMTRRHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG DisplayMTRRRegisters(SCREEN *screen, BYTE *cmd,
			  StackFrame *stackFrame, LONG Exception,
			  DEBUGGER_PARSER *parser);

extern LONG displayGDTHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayGDT(SCREEN *screen, BYTE *cmd,
		StackFrame *stackFrame, LONG Exception,
		DEBUGGER_PARSER *parser);

extern LONG displayIDTHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayIDT(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);

extern LONG evaluateExpressionHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG evaluateExpression(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);

extern LONG displayDOSTableHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG displayDOSTable(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);

extern LONG portCommandHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG inputWordPort(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG inputDoublePort(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG inputBytePort(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG inputPort(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG outputWordPort(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG outputDoublePort(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG outputBytePort(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG outputPort(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);


extern LONG breakpointCommandHelp(SCREEN *screen, BYTE *commandLine, DEBUGGER_PARSER *parser);
extern LONG breakpointClearAll(SCREEN *screen, BYTE *cmd,
			StackFrame *stackFrame, LONG Exception,
			DEBUGGER_PARSER *parser);
extern LONG breakpointClear(SCREEN *screen, BYTE *cmd,
		     StackFrame *stackFrame, LONG Exception,
		     DEBUGGER_PARSER *parser);
extern LONG breakpointMask(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser);
extern LONG breakpointWord1(SCREEN *screen, BYTE *cmd,
		     StackFrame *stackFrame, LONG Exception,
		     DEBUGGER_PARSER *parser);
extern LONG breakpointWord2(SCREEN *screen, BYTE *cmd,
		     StackFrame *stackFrame, LONG Exception,
		     DEBUGGER_PARSER *parser);
extern LONG breakpointWord4(SCREEN *screen, BYTE *cmd,
		     StackFrame *stackFrame, LONG Exception,
		     DEBUGGER_PARSER *parser);
extern LONG breakpointWord(SCREEN *screen, BYTE *cmd,
		    StackFrame *stackFrame, LONG Exception,
		    DEBUGGER_PARSER *parser);
extern LONG breakpointRead1(SCREEN *screen, BYTE *cmd,
		     StackFrame *stackFrame, LONG Exception,
		     DEBUGGER_PARSER *parser);
extern LONG breakpointRead2(SCREEN *screen, BYTE *cmd,
		     StackFrame *stackFrame, LONG Exception,
		     DEBUGGER_PARSER *parser);
extern LONG breakpointRead4(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG breakpointRead(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG breakpointIO1(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG breakpointIO2(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG breakpointIO4(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG breakpointIO(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG breakpointExecute(SCREEN *screen, BYTE *cmd,
		   StackFrame *stackFrame, LONG Exception,
		   DEBUGGER_PARSER *parser);
extern LONG breakpointShowTemp(SCREEN *screen, BYTE *cmd,
			StackFrame *stackFrame, LONG Exception,
			DEBUGGER_PARSER *parser);

