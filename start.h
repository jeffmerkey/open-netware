
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
*   FILE     :  START.H
*   DESCRIP  :  Installable Startup Routine System for MANOS v1.0
*   DATE     :  July 28, 1998
*
*
***************************************************************************/

#include "types.h"

typedef struct _STARTUP_NODE
{
    struct _STARTUP_NODE *startupNext;
    struct _STARTUP_NODE *startupPrior;
    LONG (*startupRoutine)(struct _STARTUP_NODE *node,
			   DOS_TABLE *dos, PM_TABLE *pm);
    LONG Parm1;
    LONG Parm2;
    LONG Parm3;
    LONG Parm4;
} STARTUP_NODE;

extern LONG SystemStartupRoutine(DOS_TABLE *dos, PM_TABLE *pm);
extern LONG AddStartupRoutine(STARTUP_NODE *startup_node);
extern LONG RemoveStartupRoutine(STARTUP_NODE *startup_node);


