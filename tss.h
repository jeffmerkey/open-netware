

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
*   FILE     :  TSS.H
*   DESCRIP  :  Task State Segment Defines for MANOS v1.0
*   DATE     :  November 2, 1997
*
*
***************************************************************************/

#include "types.h"

typedef struct _StackFrame {
   LONG tReserved[7];
   LONG *tCR3;
   LONG tEIP;
   LONG tSystemFlags;
   LONG tEAX;
   LONG tECX;
   LONG tEDX;
   LONG tEBX;
   LONG tESP;
   LONG tEBP;
   LONG tESI;
   LONG tEDI;
   LONG tES;
   LONG tCS;
   LONG tSS;
   LONG tDS;
   LONG tFS;
   LONG tGS;
   LONG tLDT;
   LONG tIOMap;
} StackFrame;
