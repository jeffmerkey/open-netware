
/**************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*                           Reserved.
*
*   AUTHOR   :  Darren Major
*   FILE     :  MEM.C
*   DESCRIP  :  Multi-Processing Memory Manager for MANOS v1.0
*   DATE     :  January 5, 1998
*
*
***************************************************************************/

#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "string.h"
#include "kernel.h"
#include "keyboard.h"
#include "screen.h"
#include "types.h"
#include "emit.h"
#include "dos.h"
#include "tss.h"
#include "mps.h"
#include "os.h"
#include "globals.h"

#define uint32  unsigned long
#define uint8   unsigned char

#define LONG    uint32

uint32 kmalloc_init(
	book_node	*book);
uint32 malloc_init(
	book_node	*book);
page_node	physical_page_manager;
page_node	physical_chapter_manager;

extern	uint32	*move_address;
uint32	consolidate_delay = 7;
uint32	active_processor_count = 4;
extern	uint32	StartOfHighMemory;
extern	uint32	HighMemoryLength;
uint32	page_collection_routine(void);
uint32	page_fault_wake_up(void);
uint32	physical_block_consolidation_routine(void);
extern	uint32	*vmpage;
extern	uint32	*master_malloc_table;

void	compression_init(void);
void InitPagedMemory(void)
{
	uint32	i, j, *b_ptr1, *b_ptr2, *b_ptr3;
	chapter_node *chapter;
	if ((StartOfHighMemory + HighMemoryLength) & 0x000FFFFF)
	{
		printf("\n\n\n memory alignment problem detected !!!\n");
		printf(" total memory reported = %08x\n\n\n", StartOfHighMemory + HighMemoryLength);
	}
	vmpage = (uint32 *) StartOfHighMemory;
	StartOfHighMemory += 4096;
	master_malloc_table = (uint32 *) StartOfHighMemory;
	StartOfHighMemory += 4096;
	malloc_tables_init();

//	BREAK();
	active_processor_count = 1;

	mat_init();
	ccb_init();

	generic_page_init(&physical_chapter_manager,
		&system_book,
		HUGE_PAGES,
		PAGE_PHYSICAL_MAH,
		0,
		MAT_VIRTUAL_ADDRESS,
		1024, 	// page size
		1024,	// page skip
		0);


	mpt_init(StartOfHighMemory >> 12, ((StartOfHighMemory + HighMemoryLength) >> 12) & 0xFFFFFF00);


	generic_page_init(&physical_page_manager,
		&system_book,
		PHYSICAL_PAGES,
		PAGE_PHYSICAL_MAH | PAGE_INITIALIZE_DATA_BIT | PAGE_PHYSICAL_EQUALS_LOGICAL_BIT | CHAPTER_NOT_SPARSE_BIT | CHAPTER_PHYSICAL_MAH | PAGE_SUB_OWNER_BIT,
		0,
		MAT_PHYSICAL_ADDRESS,
		16, 	// page size  64k;
		16,	// page skip
		0);

	for (i=0; i<PHYSICAL_EQUALS_LOGICAL_BOUNDARY; i+= LOGICAL_CHAPTER_SIZE)
	{
		chapter_create(&chapter, &physical_page_manager, physical_page_manager.page_flags,PHYSICAL_CHAPTER_PAGE_SIZE, i);
	}

	mah_init();

//	moh_init();
	malloc_init(&system_book);
	kmalloc_init(&system_book);
	pmalloc_init();
/*
	for (i=1; i<64; i++)
	{
		printf(" small pmalloc size %d  \n\r", (i << 8));
		if ((b_ptr1 = pmalloc(i << 8)) == 0)
			BREAK();
		if ((b_ptr2 = pmalloc(i << 8)) == 0)
			BREAK();
		if ((b_ptr3 = pmalloc(i << 8)) == 0)
			BREAK();
		SetDataD(b_ptr1, 0x22222222, i << 6);
		SetDataD(b_ptr2, 0x22222222, i << 6);
		SetDataD(b_ptr3, 0x22222222, i << 6);
		pfree(b_ptr2);
		pfree(b_ptr3);
		pfree(b_ptr1);
	}

	for (i=32; i>0; i--)
	{
		if ((b_ptr1 = malloc(i << 20)) == 0)
			BREAK();
		printf(" huge malloc size %d  addr = %08x \n\r", i << 20, (uint32) b_ptr1);
		SetDataD(b_ptr1, 0x33333333, i << 18);
		free(b_ptr1);
	}


	for (i=32; i>0; i--)
	{
		if ((b_ptr1 = kmalloc(i << 20)) == 0)
			BREAK();
		printf(" huge kmalloc size %d  addr = %08x \n\r", i << 20, (uint32) b_ptr1);
		SetDataD(b_ptr1, 0x33333333, i << 18);
		kfree(b_ptr1);
	}


	for (i=1; i<64; i++)
	{
		printf(" small malloc size %d  \n\r");
		if ((b_ptr1 = malloc(i << 8)) == 0)
			BREAK();
		if ((b_ptr2 = malloc(i << 8)) == 0)
			BREAK();
		if ((b_ptr3 = malloc(i << 8)) == 0)
			BREAK();
		SetDataD(b_ptr1, 0x22222222, i << 6);
		SetDataD(b_ptr2, 0x22222222, i << 6);
		SetDataD(b_ptr3, 0x22222222, i << 6);
		free(b_ptr2);
		free(b_ptr3);
		free(b_ptr1);
	}

	for (i=1; i<30; i++)
	{
		printf(" medium malloc size %d  \n\r", i << 12);
		if ((b_ptr1 = malloc(i << 12)) == 0)
			BREAK();
		if ((b_ptr2 = malloc(i << 12)) == 0)
			BREAK();
		if ((b_ptr3 = malloc(i << 12)) == 0)
			BREAK();
		SetDataD(b_ptr1, 0x11111111, i << 10);
		SetDataD(b_ptr2, 0x11111111, i << 10);
		SetDataD(b_ptr3, 0x11111111, i << 10);
		free(b_ptr2);
		free(b_ptr3);
		free(b_ptr1);
	}

	for (i=1; i<64; i++)
	{
		if ((b_ptr1 = malloc(i << 16)) == 0)
			BREAK();
		printf(" large malloc size %d  addr = %08x \n\r", i << 16, (uint32) b_ptr1);
		SetDataD(b_ptr1, 0x33333333, i << 14);
		free(b_ptr1);
	}




	for (i=1; i<64; i++)
	{
		printf(" small kmalloc size %d  \n\r");
		if ((b_ptr1 = kmalloc(i << 8)) == 0)
			BREAK();
		if ((b_ptr2 = kmalloc(i << 8)) == 0)
			BREAK();
		if ((b_ptr3 = kmalloc(i << 8)) == 0)
			BREAK();
		SetDataD(b_ptr1, 0x22222222, i << 6);
		SetDataD(b_ptr2, 0x22222222, i << 6);
		SetDataD(b_ptr3, 0x22222222, i << 6);
		kfree(b_ptr2);
		kfree(b_ptr3);
		kfree(b_ptr1);
	}

	for (i=1; i<30; i++)
	{
		printf(" medium kmalloc size %d  \n\r", i << 12);
		if ((b_ptr1 = kmalloc(i << 12)) == 0)
			BREAK();
		if ((b_ptr2 = kmalloc(i << 12)) == 0)
			BREAK();
		if ((b_ptr3 = kmalloc(i << 12)) == 0)
			BREAK();
		SetDataD(b_ptr1, 0x11111111, i << 10);
		SetDataD(b_ptr2, 0x11111111, i << 10);
		SetDataD(b_ptr3, 0x11111111, i << 10);
		kfree(b_ptr2);
		kfree(b_ptr3);
		kfree(b_ptr1);
	}

	for (i=1; i<64; i++)
	{
		if ((b_ptr1 = kmalloc(i << 16)) == 0)
			BREAK();
		printf(" large kmalloc size %d  addr = %08x \n\r", i << 16, (uint32) b_ptr1);
		SetDataD(b_ptr1, 0x33333333, i << 14);
		kfree(b_ptr1);
	}


*/
	stmalloc_init();
	active_processor_count = 4;


	createThread((BYTE *) "page collection", page_collection_routine, 0x4000, 0, -1);
	createThread((BYTE *) "page fault wakeup", page_fault_wake_up, 0x4000, 0, -1);
//	createThread((BYTE *) "physical block consolidation", physical_block_consolidation_routine , 0x4000, 0, -1);

//	mat_alloc(&system_book, (chapter_node *) -3, &move_address, 1);
//	logical_destination_page = &logical_source_page[1024 * 16];
//	logical_source_page_index = (uint32) logical_source_page >> 12;
//	logical_destination_page_index = (uint32) logical_destination_page >> 12;

//	compression_init();
	return;
}

uint32	physical_block_consolidation_routine(void)
{
	uint32	next_block_id = 0;
	while (1)
	{
		if (consolidate_delay != 0)
		{
			if ((next_block_id = mpt_consolidate_physical_blocks(next_block_id)) == (uint32) -1)
				next_block_id = 0;
			delayThread(consolidate_delay);
		}
		else
			delayThread(18);

	}
//	return(0);
}

uint32	page_collection_routine(void)
{
	page_node	*page;

	cli();
	while (1)
	{
		spin_lock(&system_book.book_mutex);
		system_book.collecting_flag = 1;
		spin_unlock(&system_book.book_mutex);

		if ((page = system_book.page_list_head) != 0)
		{
			do
			{
				if ((page->page_flags & PAGE_PHYSICAL_EQUALS_LOGICAL_BIT) == 0)
					system_book.collection_counter += generic_page_free(page, 0);
				page = page->next_link;
			} while (page != system_book.page_list_head);
		}
		system_book.collecting_flag = 0;
	       delayThread(18);
	}
//	return(0);
}
