/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*                           Reserved.
*
*   AUTHOR   :  Darren Major & Merrill Teemant
*   FILE     :  GLOBALS.H
*   DESCRIP  :  Multi-Processing Memory Manager for MANOS v1.0
*   DATE     :  January 26, 1998
*
*
***************************************************************************/


/********************************** header files ************************************/

/********************************** #defines ****************************************/
#define	uint32	unsigned long
#define uint8 	unsigned char

#define	BYTES_PER_PAGE		0x1000
#define LONGS_PER_PAGE		0x0400
#define QUADS_PER_PAGE		0x0200
#define DQUADS_PER_PAGE		0x0100

#define	get_flags_no_cli	get_flags

#define	TT_ALIVE			1
#define	TT_DEAD				2
#define	TT_DYING			3

#define FOUR_K_SHIFT                    12
#define FOUR_K_MASK                     0x00000FFF

#define MASTER_PAGE_SHIFT               17
#define MASTER_PAGE_MASK                0x0001FFFF

#define ONE_MEG_SHIFT					20
#define ONE_MEG_MASK					0x000FFFFF
#define ONE_MEG_PAGE                    0x00100000

#define TWO_MEG_SHIFT                   21
#define TWO_MEG_MASK                    0x001FFFFF
#define TWO_MEG_PAGE                    0x00200000

#define ENTRIES_PER_PAGE				(4096 / 8)
#define FREE_ENTRY                      0x11000000
#define ALLOCATED_ENTRY					0x22000000

#define RESERVED_CHAPTER				0xAAAAAAAA
#define AVAILABLE_CHAPTER				0xBBBBBBBB
#define IN_USE_CHAPTER					0xCCCCCCCC
#define NO_CHAPTER						0xDDDDDDDD

#define POOL_IDLE						0xABABABAB
#define POOL_BUSY						0xCDCDCDCD
#define BUSY_FLAG						0xDDDDDDDE

#define VM_NONpageable_PAGES            0x11111111
#define VM_pageable_PAGES               0x22222222

#define ERROR_RETURN(ret_value)         ret_value
#define SUCCESS_RETURN(ret_value)       0

//#define	vmdbg	//
#define vmdbg

//
//	icheck macro turns on integrity check code
//
//#define	icheck	//
#define	icheck

//
// pcheck macro turns on checks for code path testing
//
//define	pcheck	//
#define	pcheck

/********************************** typedefs*****************************************/
typedef struct  table_entry_def
{
	uint32  next_index;
	uint32  last_index;
} table_entry;

typedef	struct	quad_entry_def
{
	uint32	low_entry;
	uint32	high_entry;
} quad_entry;

typedef	struct	mah_node_def
{
	uint32	low_entry;
	uint32	high_entry;
} mah_node;

/********************************** function declarations ***************************/


#include "index.h"
#include "mmu.h"
#include "book.h"
#include "chapter.h"
#include "page.h"
#include "byte.h"
#include "mpt.h"
#include "mah.h"
#include "mat.h"
#include "malloc.h"


extern void FlushTLB(void);
//extern LONG LoadPDE(LONG pde);
extern void EnablePaging(void);
extern void DisablePaging(void);
extern uint32	active_processor_count;
