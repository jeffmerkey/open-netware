
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
*   FILE     :  ACCEL.H
*   DESCRIP  :  Keystroke Accelerator for MANOS v1.0
*   DATE     :  August 11, 1998
*
*
***************************************************************************/

#include "types.h"

typedef struct _ACCELERATOR
{
    struct _ACCELERATOR *accelNext;
    struct _ACCELERATOR *accelPrior;
    LONG (*accelRoutine)(SCREEN *screen, LONG key, void *p, struct _ACCELERATOR *parser);
    LONG (*accelRoutineHelp)(SCREEN *screen, LONG key, struct _ACCELERATOR *parser);
    LONG accelFlags;
    LONG key;
    LONG supervisorCommand;
    BYTE *shortHelp;
    void *moduleContext;
} ACCELERATOR;


extern LONG AccelRoutine(SCREEN *screen, LONG key, void *p);
extern LONG AccelHelpRoutine(SCREEN *screen, LONG key);
extern LONG AddAccelRoutine(ACCELERATOR *newAccel);
extern LONG RemoveAccelRoutine(ACCELERATOR *newAccel);
