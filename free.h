/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*                           Reserved.
*
*   AUTHOR   :  Darren Major & Merrill Teemant
*   FILE     :  FREE.H
*   DESCRIP  :  Multi-Processing Memory Manager for MANOS v1.0
*   DATE     :  January 26, 1998
*
*
***************************************************************************/

/********************************** header files ************************************/

/********************************** #defines ****************************************/

/********************************** typedefs*****************************************/
extern	uint32  malloc_return_counter[];


/********************************** function declarations ***************************/

void    free(void *ptr);
void    pfree(void *ptr);
void    stfree(void *ptr);
