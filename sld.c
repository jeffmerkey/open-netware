
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
*   FILE     :  SLD.C
*   DESCRIP  :  Multi-Processing Source Level Debugger for MANOS v1.0
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
#include "sld.h"

extern SOURCE_LINE_INFO executeLineInfo;
extern SOURCE_LINE_INFO logicalLineInfo;
extern LONG debugInfoPresent;
extern MODULE_HANDLE *executeModuleHandle;
extern LONG Indent;
extern MODULE_HANDLE *logicalModuleHandle;
extern BYTE **logicalSourceFile;
extern LONG logicalFramePosition;
extern LONG logicalLineAddress;
extern LONG logicalModuleIndex;
extern BYTE lastCommand;
extern LONG nextUnasmAddress;
extern LONG BreakReserved[4];
extern LONG BreakPoints[4];
extern LONG BreakType[4];
extern LONG BreakLength[4];
extern LONG BreakTemp[4];
extern LONG BreakGo[4];
extern LONG BreakProceed[4];
extern LONG BreakMask[MAX_PROCESSORS];
extern BYTE BreakCondition[4][100];
extern LONG ConditionalBreakpoint[4];
extern StackFrame lastStackFrame;
extern LONG lastCR0;
extern LONG lastCR2;
extern LONG lastCR4;
extern LONG SourceDebuggerActive;
extern LONG debuggerActive;
extern LONG repeatCommand;
extern LONG totalLines;
extern LONG currentException;
extern BYTE *currentPanicMsg;

void DisplaySLDFunctionLine(SCREEN *screen)
{
    register LONG col = 0;

    PutVidString(screen, " F1", col, screen->nLines - 1, RED | BGWHITE);
    col += strlen(" F1");
    PutVidString(screen, "-Lex", col, screen->nLines - 1, BLACK | BGWHITE);
    col += strlen("-Lex");
    PutVidString(screen, " F2", col, screen->nLines - 1, RED | BGWHITE);
    col += strlen(" F2");
    PutVidString(screen, "-Bkpt", col, screen->nLines - 1, BLACK | BGWHITE);
    col += strlen("-Bkpt");
    PutVidString(screen, " F3", col, screen->nLines - 1, RED | BGWHITE);
    col += strlen(" F3");
    PutVidString(screen, "-Srch", col, screen->nLines - 1, BLACK | BGWHITE);
    col += strlen("-Srch");
    PutVidString(screen, " F4", col, screen->nLines - 1, RED | BGWHITE);
    col += strlen(" F4");
    PutVidString(screen, "-Crnt", col, screen->nLines - 1, BLACK | BGWHITE);
    col += strlen("-Crnt");
    PutVidString(screen, " F5", col, screen->nLines - 1, RED | BGWHITE);
    col += strlen(" F5");
    PutVidString(screen, "-Scrn", col, screen->nLines - 1, BLACK | BGWHITE);
    col += strlen("-Scrn");
    PutVidString(screen, " F6", col, screen->nLines - 1, RED | BGWHITE);
    col += strlen(" F6");
    PutVidString(screen, "-Cons", col, screen->nLines - 1, BLACK | BGWHITE);
    col += strlen("-Cons");
    PutVidString(screen, " F7", col, screen->nLines - 1, RED | BGWHITE);
    col += strlen(" F7");
    PutVidString(screen, "-Trace", col, screen->nLines - 1, BLACK | BGWHITE);
    col += strlen("-Trace");
    PutVidString(screen, " F8", col, screen->nLines - 1, RED | BGWHITE);
    col += strlen(" F8");
    PutVidString(screen, "-Step", col, screen->nLines - 1, BLACK | BGWHITE);
    col += strlen("-Step");
    PutVidString(screen, " F9", col, screen->nLines - 1, RED | BGWHITE);
    col += strlen(" F9");
    PutVidString(screen, "-Go", col, screen->nLines - 1, BLACK | BGWHITE);
    col += strlen("-Go");
    PutVidString(screen, " F10", col, screen->nLines - 1, RED | BGWHITE);
    col += strlen(" F10");
    PutVidString(screen, "-Q    ", col, screen->nLines - 1, BLACK | BGWHITE);

}

void UpdateSLDFrameInfo(SOURCE_LINE_INFO *info)
{
    if (totalLines && debugInfoPresent)
    {
       if (info->ModuleIndex != logicalModuleIndex)
       {
	  logicalModuleIndex = info->ModuleIndex;
	  logicalSourceFile = (BYTE **)info->LineTable;
       }

       if (info->LineNumber > logicalFramePosition + sourceScreen->nLines - 5)
	  logicalFramePosition = info->LineNumber;

       if (info->LineNumber < logicalFramePosition)
	  logicalFramePosition = info->LineNumber;
    }
    return;
}

void DisplaySLDWindowEmpty(StackFrame *stackFrame)
{
    BYTE displayBuffer[100]={""};
    register LONG i, processor = get_processor_id();

    for (i=0; i < sourceScreen->nLines - 4; i++)
       PutVidStringCleol(sourceScreen, "", i, YELLOW | BGBLUE);

    PutVidStringCleol(sourceScreen, "No Source Debug Information Present for This Module",
			 0, BRITEWHITE | BGBLUE);

    (logicalLineAddress != (LONG) -1)
    ? sprintf(displayBuffer, "Ident: %3d Addr: 0x%08X", Indent, logicalLineAddress)
    : sprintf(displayBuffer, " Ident: %d  Addr: ??? ", Indent);

    PutVidStringTransparent(sourceScreen, displayBuffer, sourceScreen->nCols - 28,
		    sourceScreen->nLines - 5, BRITEWHITE);
    PutVidCharCleol(sourceScreen, 0xC4, sourceScreen->nLines - 4,
		    BRITEWHITE | BGBLUE);

    sprintf(displayBuffer, "Proc: %d  Line: ???  Source: ???  Module: %s ",
	    processor,
	    (logicalModuleHandle) ? logicalModuleHandle->ModuleShortName : "");

    PutVidStringCleol(sourceScreen, displayBuffer, sourceScreen->nLines - 3,
	    BRITEWHITE | BGBLUE);

    SourceDisplayBreakReason(sourceScreen, stackFrame, currentException,
			     currentPanicMsg, processor, lastCommand);
    return;
}

void DisplaySLDWindow(StackFrame *stackFrame)
{

    BYTE formatBuffer[256]={""};
    BYTE displayBuffer[100]={""};
    register LONG i, r, j, lineAddress, len = sizeof(formatBuffer);
    register BYTE *iLine, *s, *v;
    register LONG processor = get_processor_id(), attribute;
    register LONG currentLineNumber;

    for (i=0; i < sourceScreen->nLines - 4; i++)
    {
       PutVidStringCleol(sourceScreen, "", i, YELLOW | BGBLUE);

       if (totalLines && debugInfoPresent)
       {
	  currentLineNumber = logicalFramePosition + i;
	  if (currentLineNumber > totalLines - 1)
	     continue;

	  lineAddress = GetValueFromLine(logicalLineInfo.SourceIndex,
				      logicalLineInfo.FileIndex,
				      logicalLineInfo.Segment,
				      currentLineNumber,
				      logicalModuleHandle, 0);

	  // format tabs and strip \t controls from data stream
	  // this will allow us to accurately indent the line
	  s = (BYTE *) logicalSourceFile[logicalFramePosition + i];
	  v = (BYTE *) formatBuffer;
	  for (r = 0; r < len; r++)
	  {
	     switch (*s)
	     {
		case '\t':
		   s++;
		   *v++ = ' ';
		   for (j=0; j < sourceScreen->TabSize - 1; j++)
		   {
		      if (r++ > len)
			 break;
		      *v++ = ' ';
		   }
		   break;

		default:
		   if (!*s)
		      *v++ = ' ';
		   else
		      *v++ = *s++;
		   break;
	     }
	  }
	  formatBuffer[len - 1] = '\0';

	  iLine = (BYTE *) formatBuffer;
	  for (r=0; *iLine && r < Indent; r++)
	     iLine++;

	  attribute = YELLOW | BGBLUE; // background

	  if (ValidBreakpoint(lineAddress))
	  {
	     if (logicalLineInfo.LineNumber == currentLineNumber)
	     {
		logicalLineAddress = lineAddress;
		attribute = RED | BGWHITE;
	     }
	     else
	     if (executeLineInfo.LineNumber == currentLineNumber &&
		 executeLineInfo.ModuleIndex == logicalModuleIndex &&
		 executeLineInfo.LineTable == (LONG *)logicalSourceFile)
		attribute = BLACK | BGGREEN;
	     else
		attribute = BRITEWHITE | BGRED;
	  }
	  else
	  if (logicalLineInfo.LineNumber == currentLineNumber)
	  {
	     logicalLineAddress = lineAddress;
	     attribute = BLUE | BGWHITE;
	  }
	  else
	  if (executeLineInfo.LineNumber == currentLineNumber &&
	      executeLineInfo.ModuleIndex == logicalModuleIndex &&
	      executeLineInfo.LineTable == (LONG *)logicalSourceFile)
	     attribute = BRITEWHITE | BGGREEN;

	  PutVidStringCleol(sourceScreen, iLine, i, attribute);
       }
    }

    (logicalLineAddress != (LONG) -1)
    ? sprintf(displayBuffer, "Ident: %3d Addr: 0x%08X", Indent, logicalLineAddress)
    : sprintf(displayBuffer, "Ident: %3d Addr: ??? ", Indent);

    PutVidStringTransparent(sourceScreen, displayBuffer, sourceScreen->nCols - 28,
		    sourceScreen->nLines - 5, BRITEWHITE);

    PutVidCharCleol(sourceScreen, 0xC4, sourceScreen->nLines - 4,
		    BRITEWHITE | BGBLUE);

    sprintf(displayBuffer, "Proc: %d  Line: %d  Source: %s  Module: %s",
		     processor,
		     logicalLineInfo.LineNumber,
		     logicalLineInfo.ModuleName,
		     (logicalModuleHandle)
		     ? logicalModuleHandle->ModuleShortName : "");

    PutVidStringCleol(sourceScreen, displayBuffer, sourceScreen->nLines - 3,
		      BRITEWHITE | BGBLUE);

    SourceDisplayBreakReason(sourceScreen, stackFrame, currentException,
			     currentPanicMsg, processor, lastCommand);

}

LONG SLDUpdate(StackFrame *stackFrame, LONG Exception, BYTE *panicMsg)
{
    LONG exact;
    register LONG i;

    if (panicMsg) {};

    logicalLineAddress = stackFrame->tEIP;

    GetLineInfoFromValue(logicalLineAddress,
			 &executeLineInfo,
			 &executeModuleHandle,
			 &totalLines,
			 &debugInfoPresent,
			 &exact);

    GetLineInfoFromValue(logicalLineAddress,
			 &logicalLineInfo,
			 &logicalModuleHandle,
			 &totalLines,
			 &debugInfoPresent,
			 &exact);

    //   if no debug line info is present in the module, then
    //   we return 0.

    if (!debugInfoPresent)
    {
       DisplaySLDWindowEmpty(stackFrame);
       return 0;
    }

    //   if we did not find a match for current address and line info
    //   then we single step until we find the next line entry

    switch (Exception)
    {
       case 1:
       case 3:
	  if (SourceDebuggerActive && debugInfoPresent && !exact)
	  {
	     ClearTempBreakpoints();
	     if (lastCommand == 'P')
	     {
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
			 stackFrame->tSystemFlags &= ~SINGLE_STEP;
			 stackFrame->tSystemFlags |= RESUME;
			 lastCommand = 'P';
			 lastCR0 = ReadCR0();
			 lastCR2 = ReadCR2();
			 lastCR4 = ReadCR4();
			 CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
			      sizeof(StackFrame));
			 debuggerActive--;
			 return -1;
		      }
		   }
		   printfScreenWithAttribute(debugScreen, BRITEWHITE, "\nNo breakpoint available for Proceed, (single step) instead");
		}
		lastCommand = 'P';
		lastCR0 = ReadCR0();
		lastCR2 = ReadCR2();
		lastCR4 = ReadCR4();
		CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
		     sizeof(StackFrame));
		stackFrame->tSystemFlags |= (SINGLE_STEP | RESUME);
		debuggerActive--;
		return -1;
	     }
	     else
	     {
		lastCommand = 'T';
		lastCR0 = ReadCR0();
		lastCR2 = ReadCR2();
		lastCR4 = ReadCR4();
		CopyData((LONG *)stackFrame, (LONG *)&lastStackFrame,
		     sizeof(StackFrame));
		stackFrame->tSystemFlags |= (SINGLE_STEP | RESUME);
		debuggerActive--;
		return -1;
	     }
	  }
	  break;

       default:
	  break;
    }
    UpdateSLDFrameInfo(&logicalLineInfo);
    DisplaySLDWindow(stackFrame);
    return 0;

}

// SLD Accelerators

LONG SLDInteractive(SCREEN *screen, LONG key, void *stackFrame,
		    ACCELERATOR *accel)
{
    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    if (!SourceDebuggerActive)
    {
       SourceDebuggerActive = TRUE;
       SetVideoOwner(sourceScreen);
    }
    else
    {
       SourceDebuggerActive = 0;
       SetVideoOwner(debugScreen);
    }

    UpdateSLDFrameInfo(&logicalLineInfo);
    DisplaySLDWindow(stackFrame);

    printfScreen(debugScreen, "\r");
    return TRUE;
}

LONG SLDSetBreak(SCREEN *screen, LONG key, void *stackFrame,
		ACCELERATOR *accel)
{
    register LONG i;
    register LONG breakCleared = 0;

    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    if (SourceDebuggerActive)
    {
       if (logicalLineAddress == (LONG) -1)
	  return TRUE;

       for (i=0; i < 4; i++)
       {
	  if (BreakPoints[i] == logicalLineAddress)
	  {
	     breakCleared = TRUE;
	     BreakReserved[i] = BreakPoints[i] = BreakType[i] = 0;
	     BreakLength[i] = ConditionalBreakpoint[i] = 0;
	     SetDebugRegisters();
	     break;
	  }
       }
       if (!breakCleared)
       {
	  for (i=0; i < 4; i++)
	  {
	     if (!BreakReserved[i])
	     {
		BreakReserved[i] = TRUE;
		BreakPoints[i] = logicalLineAddress;
		BreakType[i] = BREAK_EXECUTE;
		BreakLength[i] = ONE_BYTE_FIELD;
		SetDebugRegisters();
		break;
	     }
	  }
       }
       DisplaySLDWindow(stackFrame);
    }
    return TRUE;
}

LONG SLDScreens(SCREEN *screen, LONG key, void *stackFrame,
		ACCELERATOR *accel)
{
    register SCREEN *displayScreen;

    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    if (SourceDebuggerActive)
    {
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
    }
    return TRUE;
}

LONG SLDTab(SCREEN *screen, LONG key, void *stackFrame,
	    ACCELERATOR *accel)
{
    LONG valid;
    BYTE sourceInput[100] = { "" };
    BYTE *scmd = &sourceInput[0];
    register LONG newTabSize;

    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    if (SourceDebuggerActive)
    {
       sprintf(sourceInput, "Enter TAB Size <2-8> (%d): ", sourceScreen->TabSize);

       activate_window(2);
       window_write_string(2, sourceInput, 1, 1, BRITEWHITE);
       sourceInput[0] = '\0';

       disableAccelerators(sourceScreen);
       ScreenInputFromKeyboard(sourceScreen, &sourceInput[0], 0, 60, BRITEWHITE);
       enableAccelerators(sourceScreen);

       newTabSize = EvaluateExpression(stackFrame, &scmd, &valid);

       if (valid && newTabSize <= 8 && newTabSize >= 2)
	  sourceScreen->TabSize = newTabSize;
       close_window(2);
       SetXY(sourceScreen, 100, 100);
       DisplaySLDWindow(stackFrame);
    }
    return TRUE;
}

LONG SLDLeftArrow(SCREEN *screen, LONG key, void *stackFrame,
		  ACCELERATOR *accel)
{
    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    if (Indent)
       Indent--;

    DisplaySLDWindow(stackFrame);

    return TRUE;
}

LONG SLDRightArrow(SCREEN *screen, LONG key, void *stackFrame,
		   ACCELERATOR *accel)
{
    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    Indent++;
    DisplaySLDWindow(stackFrame);

    return TRUE;
}

LONG SLDHome(SCREEN *screen, LONG key, void *stackFrame,
	     ACCELERATOR *accel)
{
    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    Indent = 0;
    DisplaySLDWindow(stackFrame);
    return TRUE;
}

LONG SLDEnd(SCREEN *screen, LONG key, void *stackFrame,
	    ACCELERATOR *accel)
{
    BYTE formatBuffer[256]={""};
    register BYTE *iLine, *s, *v;
    register LONG i, r, j, len = sizeof(formatBuffer), count;

    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    s = (BYTE *) logicalSourceFile[logicalLineInfo.LineNumber];
    v = (BYTE *) formatBuffer;
    for (r = 0; r < len; r++)
    {
       if (*s == '\t')
       {
	  s++;
	  *v++ = ' ';
	  for (j=0; j < sourceScreen->TabSize - 1; j++)
	  {
	     if (r++ > len)
		break;
	     *v++ = ' ';
	  }
       }
       else if (!*s)
       {
	  *v = '\0';
	  break;
       }
       else
	  *v++ = *s++;
    }

    iLine = (BYTE *) formatBuffer;
    Indent = count = 0;
    while (*iLine++)
       if (count++ > screen->nCols - 2)
	  Indent++;

    DisplaySLDWindow(stackFrame);
    return TRUE;

}

LONG SLDCurrent(SCREEN *screen, LONG key, void *stackFrame,
		ACCELERATOR *accel)
{
    register StackFrame *executeStackFrame = stackFrame;

    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    logicalLineAddress = executeStackFrame->tEIP;

    GetLineInfoFromValue(logicalLineAddress,
			 &executeLineInfo,
			 &executeModuleHandle,
			 &totalLines,
			 &debugInfoPresent,
			 0);

    GetLineInfoFromValue(logicalLineAddress,
			 &logicalLineInfo,
			 &logicalModuleHandle,
			 &totalLines,
			 &debugInfoPresent,
			 0);

    if (!debugInfoPresent)
    {
       DisplaySLDWindowEmpty(stackFrame);
       return TRUE;
    }
    UpdateSLDFrameInfo(&logicalLineInfo);
    DisplaySLDWindow(stackFrame);

    return TRUE;
}

LONG SLDSearch(SCREEN *screen, LONG key, void *stackFrame,
	       ACCELERATOR *accel)
{
    BYTE sourceInput[100] = { "" };
    BYTE *scmd = &sourceInput[0];
    register LONG address;
    LONG valid;

    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    if (SourceDebuggerActive)
    {
       activate_window(1);
       window_write_string(1, "Enter Value: ", 1, 1, BRITEWHITE);
       disableAccelerators(sourceScreen);
       ScreenInputFromKeyboard(sourceScreen, &sourceInput[0], 0, 60, BRITEWHITE);
       enableAccelerators(sourceScreen);
       close_window(1);
       SetXY(sourceScreen, 100, 100);

       address = EvaluateExpression(stackFrame, &scmd,  &valid);
       if (valid)
       {
	  logicalLineAddress = address;

	  GetLineInfoFromValue(logicalLineAddress,
			       &logicalLineInfo,
			       &logicalModuleHandle,
			       &totalLines,
			       &debugInfoPresent,
			       0);

	  UpdateSLDFrameInfo(&logicalLineInfo);
	  DisplaySLDWindow(stackFrame);
       }
    }
    return TRUE;

}

LONG SLDScrollUp(SCREEN *screen, LONG key, void *stackFrame,
		 ACCELERATOR *accel)
{
    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    if (SourceDebuggerActive)
    {
       if (totalLines && logicalLineInfo.LineNumber > 1)
	  logicalLineInfo.LineNumber--;
       logicalLineAddress = GetValueFromLine(logicalLineInfo.SourceIndex,
					     logicalLineInfo.FileIndex,
					     logicalLineInfo.Segment,
					     logicalLineInfo.LineNumber,
					     logicalModuleHandle, 0);

       UpdateSLDFrameInfo(&logicalLineInfo);
       DisplaySLDWindow(stackFrame);
    }
    return TRUE;

}

LONG SLDScrollDown(SCREEN *screen, LONG key, void *stackFrame,
		   ACCELERATOR *accel)
{
    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    if (SourceDebuggerActive)
    {
       if (totalLines && logicalLineInfo.LineNumber < (totalLines - 1))
	  logicalLineInfo.LineNumber++;

       // if line exceeds frame marker, then advance frame marker by one

       if (logicalLineInfo.LineNumber > logicalFramePosition + sourceScreen->nLines - 5)
	  logicalFramePosition++;

       logicalLineAddress = GetValueFromLine(logicalLineInfo.SourceIndex,
					     logicalLineInfo.FileIndex,
					     logicalLineInfo.Segment,
					     logicalLineInfo.LineNumber,
					     logicalModuleHandle, 0);

       DisplaySLDWindow(stackFrame);
    }
    return TRUE;

}

LONG SLDPageUp(SCREEN *screen, LONG key, void *stackFrame,
	       ACCELERATOR *accel)
{
    register LONG i;

    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    if (SourceDebuggerActive)
    {
       for (i=0; i < sourceScreen->nLines - 5; i++)
       {
	  if (totalLines && logicalLineInfo.LineNumber > 1)
	     logicalLineInfo.LineNumber--;
       }
       logicalLineAddress = GetValueFromLine(logicalLineInfo.SourceIndex,
					     logicalLineInfo.FileIndex,
					     logicalLineInfo.Segment,
					     logicalLineInfo.LineNumber,
					     logicalModuleHandle, 0);

       UpdateSLDFrameInfo(&logicalLineInfo);
       DisplaySLDWindow(stackFrame);
    }
    return TRUE;

}

LONG SLDPageDown(SCREEN *screen, LONG key, void *stackFrame,
		 ACCELERATOR *accel)
{
    register LONG i;

    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    if (SourceDebuggerActive)
    {
       for (i=0; i < sourceScreen->nLines - 5; i++)
       {
	  if (totalLines && logicalLineInfo.LineNumber < (totalLines - 1))
	     logicalLineInfo.LineNumber++;

	  // if line exceeds frame marker, then advance frame marker by one

	  if (logicalLineInfo.LineNumber > logicalFramePosition + sourceScreen->nLines - 5)
	     logicalFramePosition++;
       }
       logicalLineAddress = GetValueFromLine(logicalLineInfo.SourceIndex,
					     logicalLineInfo.FileIndex,
					     logicalLineInfo.Segment,
					     logicalLineInfo.LineNumber,
					     logicalModuleHandle, 0);

       DisplaySLDWindow(stackFrame);
    }
    return TRUE;

}

LONG SLDCtrlPageUp(SCREEN *screen, LONG key, void *stackFrame,
		   ACCELERATOR *accel)
{
    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    if (SourceDebuggerActive)
    {
       if (totalLines)
	  logicalLineInfo.LineNumber = 1;

       logicalLineAddress = GetValueFromLine(logicalLineInfo.SourceIndex,
					     logicalLineInfo.FileIndex,
					     logicalLineInfo.Segment,
					     logicalLineInfo.LineNumber,
					     logicalModuleHandle, 0);

       UpdateSLDFrameInfo(&logicalLineInfo);
       DisplaySLDWindow(stackFrame);
    }
    return TRUE;

}

LONG SLDCtrlPageDown(SCREEN *screen, LONG key, void *stackFrame,
		     ACCELERATOR *accel)
{
    register LONG i;
    if (screen) {};
    if (key) {};
    if (stackFrame) {};
    if (accel) {};

    if (SourceDebuggerActive)
    {
       if (totalLines)
       {
	  logicalLineInfo.LineNumber = totalLines - 1;
	  for (i=0; i < sourceScreen->nLines - 6; i++)
	  {
	     if (logicalLineInfo.LineNumber > 1)
		logicalLineInfo.LineNumber--;
	  }
       }
       logicalLineAddress = GetValueFromLine(logicalLineInfo.SourceIndex,
					     logicalLineInfo.FileIndex,
					     logicalLineInfo.Segment,
					     logicalLineInfo.LineNumber,
					     logicalModuleHandle, 0);
       UpdateSLDFrameInfo(&logicalLineInfo);


       if (totalLines)
	  logicalLineInfo.LineNumber = totalLines - 1;
       logicalLineAddress = GetValueFromLine(logicalLineInfo.SourceIndex,
					     logicalLineInfo.FileIndex,
					     logicalLineInfo.Segment,
					     logicalLineInfo.LineNumber,
					     logicalModuleHandle, 0);
       UpdateSLDFrameInfo(&logicalLineInfo);
       DisplaySLDWindow(stackFrame);
    }
    return TRUE;

}




