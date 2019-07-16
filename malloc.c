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
*   AUTHOR   :  Darren Major & Merrill Teemant
*   FILE     :  MALLOC.C
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

/********************************** #defines ****************************************/
#define FOUR_K 4096

/********************************** structures **************************************/

/********************************** typedefs ****************************************/

/********************************** Global Variables ********************************/
page_node	malloc_page_manager[32];
byte_node	*malloc_byte_manager[32];
page_node	kmalloc_page_manager[32];
byte_node	*kmalloc_byte_manager[32];
extern	page_node	physical_page_manager;
byte_node	*pmalloc_byte_manager[32];
page_node	stack_chapter_manager;

uint32	byte_size_mapping_table[512];
uint32	k_size_mapping_table[512];
uint32	pmalloc_size_mapping_table[257];

uint32	malloc_expansion_counter[4] = {0,0,0,0};
uint32  malloc_total_counter[4] = {0,0,0,0};
uint32	malloc_counter_mah;
uint32	*malloc_counter;

uint32	kmalloc_error_counter[4] = {0,0,0,0};
uint32	kmalloc_expansion_counter[4] = {0,0,0,0};
uint32  kmalloc_total_counter[4] = {0,0,0,0};
uint32	kmalloc_counter_mah;
uint32	*kmalloc_counter;

uint32	pmalloc_error_counter[4] = {0,0,0,0};
uint32	pmalloc_expansion_counter[4] = {0,0,0,0};
uint32  pmalloc_total_counter[4] = {0,0,0,0};
uint32	pmalloc_counter_mah;
uint32	*pmalloc_counter;

uint32	total_malloc_counter = 0;
uint32	total_malloc_alloc_counter = 0;
uint32  total_malloc_return_counter = 0;
uint32	total_malloc_expansion_counter = 0;
uint32  total_malloc_total_counter = 0;

uint32	total_kpmalloc_counter = 0;
uint32	total_kmalloc_alloc_counter = 0;
uint32  total_kmalloc_return_counter = 0;
uint32	total_kmalloc_expansion_counter = 0;
uint32	total_kmalloc_error_counter = 0;
uint32  total_kmalloc_total_counter = 0;

uint32	total_pmalloc_counter = 0;
uint32	total_pmalloc_alloc_counter = 0;
uint32  total_pmalloc_return_counter = 0;
uint32	total_pmalloc_error_counter = 0;
uint32	total_pmalloc_expansion_counter = 0;
uint32  total_pmalloc_total_counter = 0;

extern	uint32	page_fault_success;
extern	uint32	page_fault_failure;
extern	uint32	page_fault_move_counter;
extern	uint32	page_fault_alloc_error;
extern	uint32	page_fault_post_alloc_counter;
uint32	*master_malloc_table;
/********************************** function declarations ***************************/

/***************************************************************************
*
*   FUNCTION	 :  malloc_print_stats()
*
*   USAGE	 :   malloc a byte size or a page, etc
*
*   RETURN VALUE :   void * // address to memory
*
***************************************************************************/
void	malloc_print_stats(void)
{
	uint32	i;

	total_malloc_alloc_counter = 0;
	total_malloc_return_counter = 0;
	total_malloc_expansion_counter = 0;
	total_pmalloc_alloc_counter = 0;
	total_pmalloc_return_counter = 0;
	total_pmalloc_expansion_counter = 0;
	total_kmalloc_alloc_counter = 0;
	total_kmalloc_return_counter = 0;
	total_kmalloc_expansion_counter = 0;

	printf("\n\n malloc page statistics \n\n");
	for ( i = 0; i < 4; ++i)
	{
		total_malloc_alloc_counter += malloc_counter[i * 2 * ENTRIES_PER_PAGE];
		malloc_counter[i * 2 * ENTRIES_PER_PAGE] = 0;
		total_malloc_return_counter += malloc_counter[(i * 2 * ENTRIES_PER_PAGE) + 1];
		malloc_counter[(i * 2 * ENTRIES_PER_PAGE) + 1] = 0;
		total_malloc_expansion_counter += malloc_expansion_counter[i];
		malloc_expansion_counter[i] = 0;

		total_pmalloc_alloc_counter += pmalloc_counter[i * 2 * ENTRIES_PER_PAGE];
		pmalloc_counter[i * 2 * ENTRIES_PER_PAGE] = 0;
		total_pmalloc_return_counter += pmalloc_counter[(i * 2 * ENTRIES_PER_PAGE) + 1];
		pmalloc_counter[(i * 2 * ENTRIES_PER_PAGE) + 1] = 0;
		total_malloc_expansion_counter += pmalloc_expansion_counter[i];
		pmalloc_expansion_counter[i] = 0;

		total_kmalloc_alloc_counter += kmalloc_counter[i * 2 * ENTRIES_PER_PAGE];
		kmalloc_counter[i * 2 * ENTRIES_PER_PAGE] = 0;
		total_kmalloc_return_counter += kmalloc_counter[(i * 2 * ENTRIES_PER_PAGE) + 1];
		kmalloc_counter[(i * 2 * ENTRIES_PER_PAGE) + 1] = 0;
		total_kmalloc_expansion_counter += kmalloc_expansion_counter[i];
		kmalloc_expansion_counter[i] = 0;
		total_kmalloc_error_counter += kmalloc_error_counter[i];
		kmalloc_error_counter[i] = 0;
	}

	printf("         malloc alloc counter = %d\n",total_malloc_alloc_counter);
	printf("        malloc return counter = %d\n",total_malloc_return_counter);
	printf("     malloc expansion counter = %d\n\n", total_malloc_expansion_counter);

	printf("        pmalloc alloc counter = %d\n",total_pmalloc_alloc_counter);
	printf("       pmalloc return counter = %d\n",total_pmalloc_return_counter);
	printf("    pmalloc expansion counter = %d\n", total_pmalloc_expansion_counter);
	printf("        pmalloc error counter = %d\n\n", total_pmalloc_error_counter);

	printf("        kmalloc alloc counter = %d\n",total_kmalloc_alloc_counter);
	printf("       kmalloc return counter = %d\n",total_kmalloc_return_counter);
	printf("    kmalloc expansion counter = %d\n", total_kmalloc_expansion_counter);
	printf("        kmalloc error counter = %d\n\n", total_kmalloc_error_counter);
	printf("           page_fault_success = %d\n", page_fault_success);
	printf("           page_fault_failure = %d\n", page_fault_failure);
	printf("             page_fault_moves = %d\n", page_fault_move_counter);
	printf("      page_fault_alloc_errors = %d\n", page_fault_alloc_error);
	printf("page_fault_post_alloc_counter = %d\n", page_fault_post_alloc_counter);

}




void   ureturn(void *page_ptr)
{
	uint32		owner_id;
	quad_entry	*l_entry;
	book_node	*book;
	chapter_node	*chapter;

	if (new_mat_lookup_ccb(&chapter, page_ptr) == 0)
	{
		if ((chapter->page_link->page_flags & PAGE_SUB_OWNER_BIT) != 0)
		{
			if (chapter_get_owner_id(chapter, page_ptr, &owner_id) != 0)
			{
vmdbg			panic("invalid address passed to ureturn");
				return;
			}

			if (owner_id != 0)
			{
				generic_byte_return((uint32 *) page_ptr, (byte_node *) owner_id);
				return;
			}
		}
		if (chapter->chapter_type == HUGE_PAGES)
			generic_chapter_return(chapter);
		else
			generic_page_return((uint32) page_ptr >> LOGICAL_PAGE_SHIFT, chapter->page_link);
		return;
	}
vmdbg	panic("invalid address passed to ureturn");
}


void	malloc_tables_init(void)
{
	uint32	i;

	for (i=0; i<15; i++)
	{
		if (((i+1) & 0x0003) == 0)
			master_malloc_table[i] = 16 + (i >> 2);
		else
			master_malloc_table[i] = 0 + (i >> 0);
	}

	for (; i<63; i++)
	{
		if (((i+1) & 0x000F) == 0)
			master_malloc_table[i] = 32 + (i >> 4);
		else
			master_malloc_table[i] = 16 + (i >> 2);
	}
	for (; i<255; i++)
	{
		if (((i+1) & 0x03F) == 0)
			master_malloc_table[i] = 48 + (i >> 6);
		else
			master_malloc_table[i] = 32 + (i >> 4);
	}
	for (; i<512; i++)
	{
		master_malloc_table[i] = 48 + (i >> 6);
	}
}

/***************************************************************************
*
*   FUNCTION	 :  init_malloc(void)
*
*	USAGE		 :	set up the tables and initialize chapter managers for
*					bytes and pages.
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/
uint32 malloc_init(
	book_node	*book)
{
	uint32	i;
	uint32	index;
	page_node	*p_manager;
	byte_node	*b_manager;

	mah_alloc(&malloc_counter_mah, (mah_node **) &malloc_counter);

	malloc_tables_init();

	vm256_alloc(&book->malloc_table);

	for (i=0; i<8; i++)
	{
		vm256_alloc((uint32 **) &p_manager);
		book->malloc_table[48 + i] = (uint32) p_manager;
		generic_page_init(p_manager,
				book,
				MALLOC_PAGES + i + 48,
				0,   //PAGE_INITIALIZE_DATA_BIT,			// pte_pagable_flag
				0,
				MAT_VIRTUAL_ADDRESS,
				4 * (i + 1),
				4 * (i + 1) + 4,
				0);
	}
	for (i=0; i<16; i++)
	{
		vm256_alloc((uint32 **) &p_manager);
		book->malloc_table[32 + i] = (uint32) p_manager;
		generic_page_init(p_manager,
				book,
				MALLOC_PAGES + i + 32,
				0,   //PAGE_INITIALIZE_DATA_BIT,			// pte_pagable_flag
				0,
				MAT_VIRTUAL_ADDRESS,
				(i + 1),
				((i + 1) + 4) & 0xFFFFFFFC,
				0);
	}
	p_manager = (page_node *) book->malloc_table[35];
	p_manager->page_flags |= PAGE_SUB_OWNER_BIT;
	for (i=0; i<16; i++)
	{
		generic_byte_init((byte_node **) &book->malloc_table[16 + i],
			p_manager,
			MALLOC_BYTES + i + 16,
			0, //BYTE_INITIALIZE_DATA_BIT,
			(i+1) * 1024,		// byte size
			(i+1) * 1024,		// byte skip
			MALLOC_BYTE_INIT);	// byte initialization value

	}
	for (i=0; i<16; i++)
	{
		generic_byte_init((byte_node **) &book->malloc_table[0 + i],
			p_manager,
			MALLOC_BYTES + i,
			0, //BYTE_INITIALIZE_DATA_BIT,
			(i+1) * 256,		// byte size
			(i+1) * 256,		// byte skip
			MALLOC_BYTE_INIT);	// byte initialization value
	}

	for (i=0; i<511; i++)
	{
		if ((index = master_malloc_table[i]) >= 32)
		{
			p_manager = (page_node *) book->malloc_table[index];
			if (((i+1) << 8) > (p_manager->page_size << 12))
			{
vmdbg			panic("failure in malloc_init");
			}
		}
		else
		{
			b_manager = (byte_node *) book->malloc_table[index];
			if (((i+1) << 8) > b_manager->byte_size)
			{
vmdbg			panic("failure in malloc_init");
			}
		}
	}
	return (0);
}

/***************************************************************************
*
*   FUNCTION	 :  	malloc(uint32 size)
*
*   USAGE	 :	malloc a byte size or a page, etc
*
*   RETURN VALUE :	void * // address to memory
*
***************************************************************************/
void * malloc(uint32 size)
{
	uint32		index;
	uint32  	*page_ptr;
	book_node	*book;

	malloc_counter[get_processor_id() * ENTRIES_PER_PAGE * 2]++;
	LAS_get_current(&book);

	if (size == 0)
	{
vmdbg		panic("malloc was passed a size of zero");
		return(0);
	}
	if (size < 0x00020000)
	{
		if ((index = master_malloc_table[(size-1) >> 8]) >= 32)
		{
			while (1)
			{
				if (generic_page_alloc(&page_ptr, (page_node *) book->malloc_table[index], 0) == 0)
				{
					return(page_ptr);
				}
				else
				{
					malloc_expansion_counter[get_processor_id()]++;
//					if (chapter_index_expand((page_node *) book->malloc_table[index]) != 0)
//					{
						if (chapter_expand((page_node *) book->malloc_table[index]) != 0)
						{
							return(0);
						}
//					}
				}
			}
		}
		while (1)
		{
			if (generic_byte_alloc(&page_ptr, (byte_node *) book->malloc_table[index]) == 0)
			{
				return(page_ptr);
			}
			else
			{
				if (generic_byte_expand_chapters((byte_node *) book->malloc_table[index], 0) != 0)
					return(0);
			}
		}
	}
	if (generic_chapter_alloc(&page_ptr, (size + 0xFFF) >> 12, 0) == 0)
	{
		return(page_ptr);
	}
	return(0);
}



/***************************************************************************
*
*   FUNCTION	 :  free(void *page_ptr)
*
*	USAGE		 :	free a page
*
*	RETURN VALUE :	void
*
***************************************************************************/
void   free(void *page_ptr)
{
	malloc_counter[(get_processor_id() * ENTRIES_PER_PAGE * 2) + 1]++;
	ureturn(page_ptr);
}


/***************************************************************************
*
*   FUNCTION	 :  init_kmalloc(void)
*
*	USAGE		 :	set up the tables and initialize chapter managers for
*					bytes and pages.
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/
uint32 kmalloc_init(
	book_node	*book)
{
	uint32	i;
	uint32	index;
	page_node	*p_manager;
	byte_node	*b_manager;

	mah_alloc(&kmalloc_counter_mah, (mah_node **) &kmalloc_counter);

	malloc_tables_init();

	vm256_alloc(&book->kmalloc_table);

	for (i=0; i<8; i++)
	{
		vm256_alloc((uint32 **) &p_manager);
		book->kmalloc_table[48 + i] = (uint32) p_manager;
		generic_page_init(p_manager,
				book,
				MALLOC_PAGES + i + 48,
				PAGE_ALLOC_IMMEDIATE_BIT,   //PAGE_INITIALIZE_DATA_BIT,			// pte_pagable_flag
				0,
				MAT_VIRTUAL_ADDRESS,
				4 * (i + 1),
				4 * (i + 1) + 4,
				0);
	}
	for (i=0; i<16; i++)
	{
		vm256_alloc((uint32 **) &p_manager);
		book->kmalloc_table[32 + i] = (uint32) p_manager;
		generic_page_init(p_manager,
				book,
				MALLOC_PAGES + i + 32,
				PAGE_ALLOC_IMMEDIATE_BIT,   //PAGE_INITIALIZE_DATA_BIT,			// pte_pagable_flag
				0,
				MAT_VIRTUAL_ADDRESS,
				(i + 1),
				((i + 1) + 4) & 0xFFFFFFFC,
				0);
	}
	p_manager = (page_node *) book->kmalloc_table[35];
	p_manager->page_flags |= PAGE_SUB_OWNER_BIT;
	for (i=0; i<16; i++)
	{
		generic_byte_init((byte_node **) &book->kmalloc_table[16 + i],
			p_manager,
			MALLOC_BYTES + i + 16,
			0, //BYTE_INITIALIZE_DATA_BIT,
			(i+1) * 1024,		// byte size
			(i+1) * 1024,		// byte skip
			MALLOC_BYTE_INIT);	// byte initialization value

	}
	for (i=0; i<16; i++)
	{
		generic_byte_init((byte_node **) &book->kmalloc_table[0 + i],
			p_manager,
			MALLOC_BYTES + i,
			0, //BYTE_INITIALIZE_DATA_BIT,
			(i+1) * 256,		// byte size
			(i+1) * 256,		// byte skip
			MALLOC_BYTE_INIT);	// byte initialization value
	}

	for (i=0; i<511; i++)
	{
		if ((index = master_malloc_table[i]) >= 32)
		{
			p_manager = (page_node *) book->kmalloc_table[index];
			if (((i+1) << 8) > (p_manager->page_size << 12))
			{
vmdbg			panic("failure in malloc_init");
			}
		}
		else
		{
			b_manager = (byte_node *) book->kmalloc_table[index];
			if (((i+1) << 8) > b_manager->byte_size)
			{
vmdbg			panic("failure in malloc_init");
			}
		}
	}
	return (0);
}

/***************************************************************************
*
*   FUNCTION	 :  	kmalloc(uint32 size)
*
*   USAGE	 :	kmalloc a byte size or a page, etc
*
*   RETURN VALUE :	void * // address to memory
*
***************************************************************************/
void * kmalloc(uint32 size)
{
	uint32		index;
	uint32  	*page_ptr;
	book_node	*book;

	kmalloc_counter[get_processor_id() * ENTRIES_PER_PAGE * 2]++;
	LAS_get_current(&book);

	if (size == 0)
	{
vmdbg		panic("kmalloc was passed a size of zero");
		return(0);
	}
	if (size < 0x00020000)
	{
		if ((index = master_malloc_table[(size-1) >> 8]) >= 32)
		{
			while (1)
			{
				if (generic_page_alloc(&page_ptr, (page_node *) book->kmalloc_table[index], 0) == 0)
				{
					return(page_ptr);
				}
				else
				{
					malloc_expansion_counter[get_processor_id()]++;
//					if (chapter_index_expand((page_node *) book->kmalloc_table[index]) != 0)
//					{
						if (chapter_expand((page_node *) book->kmalloc_table[index]) != 0)
						{
							return(0);
						}
//					}
				}
			}
		}
		while (1)
		{
			if (generic_byte_alloc(&page_ptr, (byte_node *) book->kmalloc_table[index]) == 0)
			{
				return(page_ptr);
			}
			else
			{
				if (generic_byte_expand_chapters((byte_node *) book->kmalloc_table[index], 0) != 0)
					return(0);
			}
		}
	}
	if (generic_chapter_alloc(&page_ptr, (size + 0xFFF) >> 12, 0) == 0)
	{
		return(page_ptr);
	}
	return(0);
}



/***************************************************************************
*
*   FUNCTION	 :  free(void *page_ptr)
*
*	USAGE		 :	free a page
*
*	RETURN VALUE :	void
*
***************************************************************************/
void   kfree(void *page_ptr)
{
	kmalloc_counter[(get_processor_id() * ENTRIES_PER_PAGE * 2) + 1]++;
	ureturn(page_ptr);
}

/***************************************************************************
*
*   FUNCTION	 :  init_malloc(void)
*
*	USAGE		 :	set up the tables and initialize chapter managers for
*					bytes and pages.
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/


uint32 pmalloc_init()
{
	int i, j;
	page_node	*page;
	byte_node	*byte;
	uint32	byte_size = 256;

	mah_alloc(&pmalloc_counter_mah, (mah_node **) &pmalloc_counter);
/*
	generic_page_init(&physical_page_manager,
		&system_book,
		PMALLOC_PAGES,
		PAGE_INITIALIZE_DATA_BIT | PAGE_PHYSICAL_EQUALS_LOGICAL_BIT,
		MPT_NON_PAGEABLE_PAGE,
		MAT_PHYSICAL_ADDRESS,
		16, 		// page size
		16,	// page skip
		0);
*/

	for (i=0; i<8; i++)
	{
		generic_byte_init(&pmalloc_byte_manager[i],
			&physical_page_manager,
			PMALLOC_BYTES + i,
			0, // BYTE_INITIALIZE_DATA_BIT,
			byte_size,		// byte size
			byte_size,		// byte skip
			MALLOC_BYTE_INIT);	// byte initialization value
		byte_size = byte_size * 2;
	}

	for (i=0; i<=128; i++)
	{
		for (j=0; j<8; j++)
		{
			if (pmalloc_byte_manager[j]->byte_size >= (i << 8))
			{
				pmalloc_size_mapping_table[i] = j;
				break;
			}
		}
	}

	return (0);
}

/***************************************************************************
*
*   FUNCTION	 :  	pmalloc(uint32 size)
*
*   USAGE	 :	malloc a byte size or a page, etc
*
*   RETURN VALUE :	void * // address to memory
*
***************************************************************************/

void * pmalloc(uint32 size)
{
	byte_node	*byte;
	page_node	*page;
	uint32  *page_ptr;
	uint32	ccode;

//	BREAK();
//	pmalloc_alloc_counter[get_processor_id()]++;

	if (size == 0)
	{
vmdbg		panic("pmalloc was passed a size of zero");
		return(0);
	}

	pmalloc_counter[get_processor_id() * ENTRIES_PER_PAGE * 2]++;
	if (size <= 0x8000)
	{
		byte = pmalloc_byte_manager[pmalloc_size_mapping_table[(size+0xFF)>>8]];
		while (1)
		{
			if (generic_byte_alloc(&page_ptr, byte) == 0)
			{
				return(page_ptr);
			}
			else
			{
				if (generic_byte_expand_chapters(byte, 1) != 0)
				{
					pmalloc_error_counter[get_processor_id()] ++;
					return(0);
				}
				pmalloc_expansion_counter[get_processor_id()]++;
			}
		}
	}
	else if (size <= 0x00010000)
	{
		page = &physical_page_manager;
		while (1)
		{
			if ((ccode = generic_page_alloc(&page_ptr, page, 0)) == 0)
			{
				return(page_ptr);
			}
			else
			{
				if (ccode != PAGE_LOGICAL_ALLOCATION_ERROR)
					return(0);
//				if (chapter_index_expand(page) != 0)
//				{
					if (chapter_expand(page) != 0)
					{
						pmalloc_error_counter[get_processor_id()] ++;
						return(0);
					}
//				}
				pmalloc_expansion_counter[get_processor_id()]++;
			}
		}
	}
	return(0);
}


/***************************************************************************
*
*   FUNCTION	 :  pfree(void *page_ptr)
*
*	USAGE		 :	free a page
*
*	RETURN VALUE :	void
*
***************************************************************************/

void   pfree(void *page_ptr)
{
	pmalloc_counter[(get_processor_id() * ENTRIES_PER_PAGE * 2) + 1]++;
	ureturn(page_ptr);
}


/***************************************************************************
*
*   FUNCTION	 :  	stmalloc(uint32 size)
*
*   USAGE	 :	malloc a byte size or a page, etc
*
*   RETURN VALUE :	void * // address to memory
*
***************************************************************************/


uint32 stmalloc_init()
{
	generic_page_init(&stack_chapter_manager,
		&system_book,
		STACK_PAGES,
		PAGE_ALLOC_IMMEDIATE_BIT | PAGE_SYSTEM_MEMORY , //PAGE_INITIALIZE_DATA_BIT,			// pte_pagable_flag
		0,
		MAT_VIRTUAL_ADDRESS,
		15,
		16,
		0);
	return (0);
}

/***************************************************************************
*
*   FUNCTION	 :  	stalloc(uint32 size)
*
*   USAGE	 :	malloc a byte size or a page, etc
*
*   RETURN VALUE :	void * // address to memory
*
***************************************************************************/

void *stmalloc(
	uint32 size,
	uint32 physical_flag)
{
	uint32	ccode;
	uint32  *page_ptr;
	page_node	*page;

	page = &stack_chapter_manager;
	while (size)
	{
		if ((ccode = generic_page_alloc(&page_ptr, page, 0)) == 0)
		{
			return(page_ptr);
		}
		else
		{
			if (ccode != PAGE_LOGICAL_ALLOCATION_ERROR)
				return(0);
//			if (chapter_index_expand(page) != 0)
//			{
				if (chapter_expand(page) != 0)
				{
					pmalloc_error_counter[get_processor_id()] ++;
					return(0);
				}
//			}
			physical_flag = get_processor_id();
			pmalloc_expansion_counter[physical_flag]++;
		}
	}
	return(0);
}


/***************************************************************************
*
*   FUNCTION	 :  stfree(void *page_ptr)
*
*	USAGE		 :	free a page
*
*	RETURN VALUE :	void
*
***************************************************************************/

void   stfree(void *page_ptr)
{
	ureturn(page_ptr);
}
