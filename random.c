/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*                           Reserved.
*
*   AUTHOR   :  Darren Major
*   FILE     :  RANDOM.C
*   DESCRIP  :  Multi-Processing Memory Manager for MANOS v1.0
*   DATE     :  March 4, 1998
*
*
***************************************************************************/

/********************************** header files ************************************/
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "emit.h"
#include "os.h"
#include "types.h"
#include "globals.h"

/********************************** #defines ****************************************/
/********************************** structures **************************************/

/********************************** typedefs ****************************************/

/********************************** Global Variables ********************************/

/********************************** function declarations ***************************/

uint32	system_seed;


/***************************************************************************
*
*   FUNCTION	 : rand(uin32 mod)
*
*   USAGE	 :
*
*   RETURN VALUE :   void * // address to memory
*
***************************************************************************/

uint32	i_rand(
	uint32	*seed)
{
	*seed = (1664525 * (*seed)) + 1013904223;
	return(*seed);
}

uint32	rand(
	uint32	modulo)
{
	return(i_rand(&system_seed) % modulo);
}

