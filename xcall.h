

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
*   FILE     :  XCALL.H
*   DESCRIP  :  Cross Processor IPI Defines for MANOS v1.0
*   DATE     :  January 8, 1998
*
*
***************************************************************************/

#include "types.h"

#define   XCALL_EMPTY        0
#define   XCALL_DEBUG        1
#define   XCALL_FLUSH_TLB    2
#define   XCALL_INVL_PAGE    3
#define   XCALL_SHUTDOWN     4
#define   XCALL_SERVICE      5
#define   XCALL_DIRECTED     6
#define   XCALL_RELOAD_DEBUG 7
#define   XCALL_RETRY_COUNT  0xFFFFFF


