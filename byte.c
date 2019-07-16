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
*   FILE     :  BYTE.C
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
#include "kernel.h"
#include "types.h"
#include "globals.h"
#define	vmdbg

/********************************** #defines ****************************************/

/********************************** structures **************************************/

/********************************** typedefs ****************************************/

/********************************** Global Variables ********************************/


/********************************** function declarations ***************************/
uint32	byte_check_counter  = 0;



void	generic_byte_init(
	byte_node	**new_byte,
	page_node	*page,
	uint32		byte_type,
	uint32		byte_flags,
	uint32		byte_size,
	uint32		byte_skip,
	uint32		byte_initialization_value)
{
	byte_node	*head;
	byte_node	*byte;

	if (byte_flags & BYTE_DONT_ALLOC_BIT)
	{
		byte = *new_byte;
	}
	else
	{
		vm256_alloc((uint32 **) &byte);
	}
        
	byte->byte_stamp = 42595445;
	byte->byte_type = byte_type;
	byte->byte_mutex = 0;
	byte->byte_extension_flag = 0;

	byte->byte_size = byte_size;
	byte->byte_skip = byte_skip;	// used to create holes in memory by skipping logical addresses
	byte->byte_flags = byte_flags;
	byte->byte_initialization_value = byte_initialization_value;

	if ((byte_flags & BYTE_PHYSICAL_MAH) == 0)
	{
		mah_alloc(&byte->byte_rwlock_mah, &byte->byte_rwlock);
		mah_alloc(&byte->byte_list_head_mah, &byte->byte_list_head);
		mah_alloc(&byte->byte_total_stats_mah, &byte->byte_total_stats_ptr);
		mah_alloc(&byte->byte_temp_stats_mah, &byte->byte_temp_stats_ptr);
		mah_alloc(&byte->byte_aside_list_head_mah, &byte->byte_aside_list_head);
	}
	else
	{
		pmah_alloc(&byte->byte_rwlock_mah, &byte->byte_rwlock);
		pmah_alloc(&byte->byte_list_head_mah, &byte->byte_list_head);
		pmah_alloc(&byte->byte_total_stats_mah, &byte->byte_total_stats_ptr);
		pmah_alloc(&byte->byte_temp_stats_mah, &byte->byte_temp_stats_ptr);
		pmah_alloc(&byte->byte_aside_list_head_mah, &byte->byte_aside_list_head);
	}

	byte->book_link = page->book_link;
	byte->page_link = page;

	cli();
	spin_lock(&page->book_link->book_mutex);
	if ((head = page->byte_link) == 0)
	{
		byte->last_link = byte;
		byte->next_link = byte;
		page->byte_link = byte;
	}
	else
	{
		byte->last_link = head->last_link;
		byte->next_link = head;
		head->last_link->next_link = byte;
		head->last_link = byte;
	}
	spin_unlock(&page->book_link->book_mutex);
	sti();
	*new_byte = byte;


}

/***************************************************************************
*
*   FUNCTION	 :  generic_byte_return(
*						uint32			*b_ptr,
*						chapter_manager	*c_manager)
*
*	USAGE		 :	return a byte block to the processor
*
*	RETURN VALUE :	0 - SUCCESS
*				   -2 - ERROR
*
***************************************************************************/
uint32  generic_byte_return(
	uint32		*b_ptr,
	byte_node	*b_manager)
{
	uint32		processor_id = 0;
	uint32		flags = get_flags_no_cli();

//	alock_read_lock(b_manager->byte_rwlock, &processor_id);
	cli();
	spin_lock((uint32 *) b_manager->byte_rwlock);

	new_list_return(b_manager->byte_list_head, processor_id, b_ptr);
vmdbg	if (b_ptr[4] == BYTE_FREE_STAMP)
vmdbg		panic("integrity check error in generic_byte_return");
vmdbg	b_ptr[4] = BYTE_FREE_STAMP;
//	alock_read_unlock(b_manager->byte_rwlock, processor_id);
	spin_unlock((uint32 *) b_manager->byte_rwlock);
        set_flags(flags);
	return(0);
}
/***************************************************************************
*
*   FUNCTION	 :  generic_byte_alloc(
*						uint32			**page_ptr,
*						chapter_manager	*c_manager)
*
*	USAGE		 :	alloc a specific block of bytes in a page
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/
uint32  generic_byte_alloc(
	uint32		**byte_ptr,
	byte_node	*b_manager)
{
	uint32	processor_id = 0;
	uint32	max_processor_id;
	uint32	max_count;
	uint32	p_index;
	uint32	*b_ptr;
	uint32	i;
	uint32	flags = get_flags_no_cli();
	byte_link	*b_node;
	byte_link	*head;
	*byte_ptr = 0;
//	while (1)
//	{
//		alock_read_lock(b_manager->byte_rwlock, &processor_id);
		cli();
		spin_lock((uint32 *) b_manager->byte_rwlock);
		if (new_list_alloc(b_manager->byte_list_head, processor_id, &b_ptr) == 0)
		{
//			alock_read_unlock(b_manager->byte_rwlock, processor_id);
			spin_unlock((uint32 *) b_manager->byte_rwlock);
			set_flags(flags);
			b_ptr[4] = 0;
			*byte_ptr = b_ptr;
			return(0);
		}
//		alock_read_unlock(b_manager->byte_rwlock, processor_id);
		spin_unlock((uint32 *) b_manager->byte_rwlock);
		set_flags(flags);
		return(-1);
/*
		if ((b_manager->byte_list_head[0 * QUADS_PER_PAGE].high_entry +
		     b_manager->byte_list_head[0 * QUADS_PER_PAGE].high_entry +
		     b_manager->byte_list_head[0 * QUADS_PER_PAGE].high_entry +
		     b_manager->byte_list_head[0 * QUADS_PER_PAGE].high_entry) == 0)
		     return(-1);

		max_count = 0;
		for (i=0; i<active_processor_count; i++)
		{
			if (b_manager->byte_list_head[i * QUADS_PER_PAGE].high_entry > max_count)
			{
				max_processor_id = i;
				max_count = b_manager->byte_list_head[i * QUADS_PER_PAGE].high_entry;
			}
		}
		if (max_count > 0)
		{
			alock_write_lock(b_manager->byte_rwlock);

			while (alock_read_check(b_manager->byte_rwlock, max_processor_id) != 0)
				byte_check_counter ++;

			if (new_list_alloc(b_manager->byte_list_head, max_processor_id, &b_ptr) == 0)
			{
				*byte_ptr = b_ptr;
				for (i=0; i<((max_count + 1) / 2); i++)
				{
					if (new_list_alloc(b_manager->byte_list_head, max_processor_id, &b_ptr) != 0)
						break;
					new_list_return(b_manager->byte_list_head, processor_id, b_ptr);
				}
				alock_write_unlock(b_manager->byte_rwlock);
				return(0);
			}
			alock_write_unlock(b_manager->byte_rwlock);
		}
*/
//	}
}

/***************************************************************************
*
*   FUNCTION	 :  generic_byte_expand(
*						chapter_manager			*c_manager)
*
*	USAGE		 :	get a new page in the chapter manager, get a new chapter
*					if necessary
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/
uint32    generic_byte_expand_pages(
	byte_node	*b_manager,
	uint32		blocking_flag)
{
	uint32          i;
	uint8		*page_ptr;
	while (1)
	{
		if (b_manager->byte_extension_flag == 0)
		{
			spin_lock(&b_manager->byte_mutex);
			if (b_manager->byte_extension_flag == 0)
				break;
			spin_unlock(&b_manager->byte_mutex);
		}
		if (blocking_flag != 0)
			return(-1);
		thread_switch();
	}

	b_manager->byte_extension_flag = (uint32) get_running_process();
	spin_unlock(&b_manager->byte_mutex);

//	while (1)
//	{
		if (generic_page_alloc((uint32 **) &page_ptr, b_manager->page_link, (uint32) b_manager) != 0)
		{
			b_manager->byte_extension_flag = 0;
			return(-1);
		}

//		break;
//		if ((ccode != PAGE_LOGICAL_ALLOCATION_ERROR) && (blocking_flag != 0))
//		{
//			b_manager->byte_extension_flag = 0;
//			return(-1);
//		}
//		if (chapter_index_expand(b_manager->page_link) != 0)
//		{
//			b_manager->byte_extension_flag = 0;
//			return(-1);
//		}
//	}


	/* for how much memory loop through and do a generic byte return.*/

	for (i=0; i<(b_manager->page_link->page_size * 4096)/b_manager->byte_size; i ++)
	{
		generic_byte_return((uint32 *) &page_ptr[i*b_manager->byte_size] ,b_manager);
	}
	b_manager->byte_extension_flag = 0;
	return(0);
}

/***************************************************************************
*
*   FUNCTION	 :  generic_byte_expand(
*						chapter_manager			*c_manager)
*
*	USAGE		 :	get a new page in the chapter manager, get a new chapter
*					if necessary
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/
/*uint32    generic_byte_expand_indexs(
	byte_node	*b_manager)
{
	uint32          i;
	uint8		*page_ptr;
	while (1)
	{
		if (b_manager->byte_extension_flag == 0)
		{
			spin_lock(&b_manager->byte_mutex);
			if (b_manager->byte_extension_flag == 0)
				break;
			spin_unlock(&b_manager->byte_mutex);
		}
		thread_switch();
	}

	b_manager->byte_extension_flag = (uint32) get_running_process();
	spin_unlock(&b_manager->byte_mutex);

	if (chapter_index_expand(b_manager->page_link) != 0)
	{
		b_manager->byte_extension_flag = 0;
		return(-1);
	}
	b_manager->byte_extension_flag = 0;
	return(0);
}
*/
uint32    generic_byte_expand_chapters(
	byte_node	*b_manager,
	uint32		blocking_flag)
{
	uint32          i;
	uint8		*page_ptr;

	while (1)
	{
		if (b_manager->byte_extension_flag == 0)
		{
			spin_lock(&b_manager->byte_mutex);
			if (b_manager->byte_extension_flag == 0)
				break;
			spin_unlock(&b_manager->byte_mutex);
		}
		if (blocking_flag != 0)
			return(-1);
		thread_switch();
	}

	b_manager->byte_extension_flag = (uint32) get_running_process();
	spin_unlock(&b_manager->byte_mutex);

	while (1)
	{
		if (generic_page_alloc((uint32 **) &page_ptr, b_manager->page_link, (uint32) b_manager) == 0)
			break;
//		if ((ccode != PAGE_LOGICAL_ALLOCATION_ERROR) && (blocking_flag != 0))
//			return(-1);
//		if (chapter_index_expand(b_manager->page_link) != 0)
//		{
			if (chapter_expand(b_manager->page_link) != 0)
			{
				b_manager->byte_extension_flag = 0;
				return(-1);
			}
//		}
	}


	/* for how much memory loop through and do a generic byte return.*/

	for (i=0; i<= (b_manager->page_link->page_size * 4096)-b_manager->byte_size; i += b_manager->byte_size)
	{
		generic_byte_return((uint32 *) &page_ptr[i] ,b_manager);
	}
	b_manager->byte_extension_flag = 0;
	return(0);
}
