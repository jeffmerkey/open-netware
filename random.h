/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*                           Reserved.
*
*   This program is an unpublished work of TRG, Inc. and contains
*   trade secrets and other proprietary information.  Unauthorized
*   use, copying, disclosure, or distribution of this file without the
*   consent of TRG, Inc. can subject the offender to severe criminal
*   and/or civil penalities.
*
*
*   AUTHOR   :  Darren Major
*   FILE     :  RANDOM.H
*   DESCRIP  :  Multi-Processing Memory Manager for MANOS v1.0
*   DATE     :  March 4, 1998
*
*
***************************************************************************/

/********************************** header files ************************************/

/********************************** #defines ****************************************/

/********************************** typedefs*****************************************/

/********************************** globals *****************************************/

/********************************** function declarations ***************************/

uint32	i_rand(
	uint32	*seed);

uint32	rand(
	uint32	modulo);

