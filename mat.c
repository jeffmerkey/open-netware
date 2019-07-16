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
*   AUTHOR   :  Darren Major & Merrill Teamont
*   FILE     :  MAT.C
*   DESCRIP  :  Multi-Processing Memory Manager for MANOS v1.0
*   DATE     :  January 26, 1998
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
#include "kernel.h"
#include "globals.h"

extern	uint32	logical_pte_table_size;

/********************************** #defines ****************************************/

/********************************** structures **************************************/

/********************************** typedefs ****************************************/

/********************************** Global Variables ********************************/

uint32	master_zone_table[ZONES_PER_ADDRESS_SPACE] =
	{KERNEL_SPACE, 0, 0, 0,
	LIBRARY_SPACE, 0, 0, 0,
	APPLICATION_SPACE, 0, 0, 0,
	0, 0, 0, RESERVED_SPACE};



book_node	system_book;
uint32		*system_mat_table;


void	LAS_get_current(
	book_node	**book)
{
	*book = &system_book;
}

void	mat_init()
{
	uint32	i;
	uint32	chapter_count;

	SetDataD((uint32 *) &system_book, 0, sizeof(system_book)/4);
	SetDataD(system_mat_table, 0, LOGICAL_PAGE_SIZE / 4);
	system_book.address_space_type = KERNEL_SPACE;
	system_book.mat_table = system_mat_table;
	chapter_count = PHYSICAL_EQUALS_LOGICAL_BOUNDARY >> LOGICAL_CHAPTER_SHIFT;
	chapter_count += logical_pte_table_size >> LOGICAL_CHAPTER_SHIFT;
	for (i=0; i<ZONES_PER_ADDRESS_SPACE; i++)
		system_book.mat_zones[i] = -1;

	for (i=0; i<PHYSICAL_EQUALS_LOGICAL_BOUNDARY >> LOGICAL_CHAPTER_SHIFT; i++)
	{
		system_mat_table[i] = RESERVED_CCB;
		system_book.mat_zones[0] = chapter_count;
	}
	for (; i<chapter_count; i++)
	{
		system_mat_table[i] = -1;
		system_book.mat_zones[0] = chapter_count;
	}
}

uint32	mat_alloc(
	book_node	*book,
	chapter_node	*ccb,
	uint32		**new_address,
	uint32		chapter_count)
{
	uint32	i, j, k;
	spin_lock(&book->book_mutex);
	while (1)
	{
		for (i=0; i<ZONES_PER_ADDRESS_SPACE; i++)
		{
			if (book->mat_zones[i] != (uint32) -1)
			{
				for (j=0; j<(CHAPTERS_PER_ZONE - (chapter_count - 1)); j++)
				{
					for (k=0; k<chapter_count; k++)
					{
						if (book->mat_table[(i << CHAPTERS_PER_ZONE_SHIFT) + j + k] != 0)
						{
							while (k > 0)
							{
								k --;
								book->mat_table[(i * CHAPTERS_PER_ZONE) + j + k]  = 0;
							}
							break;
						}
						book->mat_table[(i << CHAPTERS_PER_ZONE_SHIFT) + j + k] = (uint32) ccb;
					}
					if (k == chapter_count)
					{
						book->mat_zones[i] += k;
						spin_unlock(&book->book_mutex);
						*new_address = (uint32 *) (((i << CHAPTERS_PER_ZONE_SHIFT) + j) << LOGICAL_CHAPTER_SHIFT);
						return(0);
					}
				}
			}
		}

		for (i=0; i<ZONES_PER_ADDRESS_SPACE; i++)
		{
			if (book->address_space_type == master_zone_table[i])
			{
				if (book->mat_zones[i] == (uint32) -1)
				{
					book->mat_zones[i] = 0;
					break;
				}
			}
		}
		if (i == ZONES_PER_ADDRESS_SPACE)
		{
			for (i=0; i<ZONES_PER_ADDRESS_SPACE; i++)
			{
				if (master_zone_table[i] == 0)
				{
					master_zone_table[i] = book->address_space_type;
					book->mat_zones[i] = 0;
					break;
				}
			}
			if (i == ZONES_PER_ADDRESS_SPACE)
			{
				spin_unlock(&book->book_mutex);
				*new_address = 0;
vmdbg				panic("system is out of logical addresses");
				return(-1);
			}
		}
	}
}


uint32	mat_return(
	book_node	*book,
	chapter_node	*ccb,
	uint32		*address,
	uint32		chapter_count)
{
	uint32	chapter_index;
	uint32	i;

	spin_lock(&book->book_mutex);

	chapter_index = (uint32) address >> LOGICAL_CHAPTER_SHIFT;
	for (i=0; i<chapter_count; i++)
	{
		if (book->mat_table[chapter_index + i] != (uint32) ccb)
		{
			while (i > 1)
			{
				i--;
				book->mat_table[chapter_index + i] = (uint32) ccb;
			}
			spin_unlock(&book->book_mutex);
			return(-1);
		}
		book->mat_table[chapter_index + i] = 0;
	}
	spin_unlock(&book->book_mutex);
	return(0);
}

uint32	new_mat_lookup_ccb(
	chapter_node	**new_chapter,
	uint32		*address)
{
	uint32	chapter_index;
	chapter_node	*chapter;
	book_node	*book;

	LAS_get_current(&book);
	chapter_index = (uint32) address >> LOGICAL_CHAPTER_SHIFT;
	if ((chapter = (chapter_node *) book->mat_table[chapter_index]) != 0)
	{
		if (book->mat_table[chapter_index] != RESERVED_CCB)
		{
			*new_chapter = chapter;
			return(0);
		}
	}
	return(MAT_INVALID_CHAPTER);
}

uint32	mat_validate_ccb(
	chapter_node	**new_chapter,
	uint32		*address)
{
	uint32	chapter_index;
	chapter_node	*chapter;
	book_node	*book;

	LAS_get_current(&book);
	chapter_index = (uint32) address >> LOGICAL_CHAPTER_SHIFT;
	if ((chapter = (chapter_node *) book->mat_table[chapter_index]) != 0)
	{
		if (book->mat_table[chapter_index] != RESERVED_CCB)
		{
			*new_chapter = chapter;
			return(0);
		}
		return(MAT_RESERVED_CHAPTER);
	}
	return(MAT_INVALID_CHAPTER);
}

uint32	mat_set_ccb(
	book_node	*book,
	chapter_node	*new_ccb,
	uint32		*address)
{
	uint32	chapter_index;

	chapter_index = (uint32) address >> LOGICAL_CHAPTER_SHIFT;
	if (book->mat_table[chapter_index] == RESERVED_CCB)
	{
		book->mat_table[chapter_index] = (uint32) new_ccb;
		return(0);
	}
	return(-1);
}
