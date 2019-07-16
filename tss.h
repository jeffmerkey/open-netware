

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
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
