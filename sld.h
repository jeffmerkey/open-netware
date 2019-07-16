

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
*   FILE     :  SLD.H
*   DESCRIP  :  Multi-Processing Source Level Debugger for MANOS v1.0
*   DATE     :  August 6, 1998
*
*
***************************************************************************/

#include "types.h"

extern void UpdateSLDLineInfo(SOURCE_LINE_INFO *info);
extern void DisplaySLDWindowEmpty(StackFrame *stackFrame);
extern void DisplaySLDWindow(StackFrame *stackFrame);
extern LONG SLDUpdate(StackFrame *stackFrame, LONG Exception, BYTE *panicMsg);
extern LONG SLDInteractive(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDScrollUp(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDScrollDown(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDPageUp(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDPageDown(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDCtrlPageUp(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDCtrlPageDown(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDSearch(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDScreens(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDCurrent(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDSetBreak(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDLeftArrow(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDRightArrow(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDTab(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDHome(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
extern LONG SLDEnd(SCREEN *screen, LONG key, void *stackFrame, ACCELERATOR *accel);
