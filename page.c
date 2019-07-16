/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*                           Reserved.
*
*   AUTHOR   :  Darren Major & Merrill Teemant
*   FILE     :  PAGE.C
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

#define	PAGE_FREE_STAMP	0x50475246

/********************************** structures **************************************/

/********************************** typedefs ****************************************/

/********************************** Global Variables ********************************/
extern  uint32  *PageDirectory;
extern	uint32	StartupPDE;

uint32  unaligned_page_table[4096];
uint32  *null_pde;
uint32  *null_pte;
uint32  *base_pde;

uint32  global_pde;

uint32	mpt_alloc_specific_fail_counter = 0;
uint32	page_read_check_counter;

uint32	LARGE_PAGE_MASK = 0x03;
uint32	LARGE_PAGE_SIZE	= 4;		// four meg

/********************************** function declarations ***************************/


void	generic_page_init(
	page_node	*p_manager,
	book_node	*book,
	uint32	page_type,
	uint32	page_flags,
	uint32	mpt_page_type,
	uint32	mat_page_type,
	uint32	page_size,
	uint32	page_skip,
	uint32	page_initialization_value)
{
	uint32	j;
	page_node	*head;

	p_manager->page_stamp = 50414745;
	p_manager->page_type = page_type;
	p_manager->page_mutex = 0;
	p_manager->page_extension_flag = 0;

	p_manager->page_skip = (page_skip < page_size) ? page_size : page_skip;
	p_manager->page_size = page_size;
	if (p_manager->page_skip > 32)
		p_manager->page_skip = 32;
	p_manager->page_flags = page_flags;
	p_manager->page_initialization_value = page_initialization_value;

	p_manager->total_page_count = 0;
	p_manager->pages_per_chapter = 1024;
	p_manager->chapter_logical_index = 0;
	p_manager->mpt_page_type = mpt_page_type;
	p_manager->mat_page_type = mat_page_type;
	if ((page_flags & PAGE_PHYSICAL_MAH) == 0)
	{
		mah_alloc(&p_manager->page_rwlock_mah, &p_manager->page_rwlock);
		mah_alloc(&p_manager->page_list_head_mah, &p_manager->page_list_head);
		mah_alloc(&p_manager->page_total_stats_mah, &p_manager->page_total_stats_ptr);
		mah_alloc(&p_manager->page_temp_stats_mah, &p_manager->page_temp_stats_ptr);
		mah_alloc(&p_manager->unmap_rwlock_mah, &p_manager->unmap_rwlock);
	}
	else
	{
		pmah_alloc(&p_manager->page_rwlock_mah, &p_manager->page_rwlock);
		pmah_alloc(&p_manager->page_list_head_mah, &p_manager->page_list_head);
		pmah_alloc(&p_manager->page_total_stats_mah, &p_manager->page_total_stats_ptr);
		pmah_alloc(&p_manager->page_temp_stats_mah, &p_manager->page_temp_stats_ptr);
		pmah_alloc(&p_manager->unmap_rwlock_mah, &p_manager->unmap_rwlock);
	}

	p_manager->book_link = book;
	p_manager->byte_link = 0;
	cli();
	spin_lock(&book->book_mutex);
	if ((head = book->page_list_head) == 0)
	{
		p_manager->next_link = p_manager;
		p_manager->last_link = p_manager;
		book->page_list_head = p_manager;
	}
	else
	{
		p_manager->last_link = head->last_link;
		p_manager->next_link = head;
		head->last_link->next_link = p_manager;
		head->last_link = p_manager;
	}

	spin_unlock(&book->book_mutex);
	sti();

}


/***************************************************************************
*
*   FUNCTION	 :  generic_page_free(
*						chapter_manager		*c_manager)
*
*	USAGE		 :	return a page in the chapter manager
*
*	RETURN VALUE :	0 - SUCCESS
*				   -2 - ERROR
*
***************************************************************************/
uint32  generic_page_free(
	page_node	*p_manager,
	uint32		all_flag)
{
	register uint32 retCode;
//	uint32		processor_id = 0;
	uint32		pages_returned_count = 0;
	uint32		*page_ptr;
	uint32		p_index;
	uint32		mah_index, mah_count;
	uint32		i, j, k;
	uint32		flags = get_flags_no_cli();

//	alock_write_lock(p_manager->page_rwlock);
	cli();
	spin_lock((uint32 *) p_manager->page_rwlock);
	for (i=0; i<active_processor_count; i++)
	{
//		while (alock_read_check(p_manager->page_rwlock, i) != 0)
//			page_read_check_counter ++;

		mah_index = i * QUADS_PER_PAGE;
		mah_count = p_manager->page_list_head[mah_index].high_entry;
		if (all_flag == 0)
			mah_count = (mah_count + 1) / 2;
		for (j=0; j<mah_count; j++)
		{
			if (new_list_alloc(p_manager->page_list_head, i, &page_ptr) == 0)
			{
//				SetDataD(page_ptr, 0xFFFEEEDD, LOGICAL_PAGE_SIZE / 4);
				if ((p_manager->page_flags & PAGE_PHYSICAL_EQUALS_LOGICAL_BIT) != 0)
				{
					for (k=0; k<p_manager->page_size; k++)
					{
						mpt_return_page((uint32)&page_ptr[k * 1024] >> 12);
//						mpt_fault_return((uint32)&page_ptr[k * 1024] >> 12);
						pages_returned_count ++;
					}
				}
				else
				{
                                	
					chapter_queue_unmap_page(page_ptr, p_manager);
//					for (k=0; k<p_manager->page_size; k++)
//					{
//						as_page_out((uint32)&page_ptr[k * 1024] >> 12, &p_index, 0);
//						MMU_page_unmap((uint32)&page_ptr[k * 1024] >> 12, &p_index, 0);
//						if ((p_index >> 12) != 0)
//							mpt_return_page(p_index >> 12);
//							mpt_fault_return(p_index >> 12);
//						mpt_return_logical((uint32)&page_ptr[k * 1024] >> 12);
//						pages_returned_count ++;
//					}
					pages_returned_count ++;
				}
//				generic_page_index(page_ptr, &p_index, p_manager);
//				chapter_index_special_return(page_ptr, p_manager);
			}
		}
	}
//	alock_write_unlock(p_manager->page_rwlock);
	spin_unlock((uint32 *) p_manager->page_rwlock);
	set_flags(flags);

	if (pages_returned_count > 0)
	{
		chapter_unmap_queued_pages(p_manager);
	}
	return(pages_returned_count);
}



/***************************************************************************
*
*   FUNCTION	 :  generic_page_return(
*						uint32			p_index,
*						chapter_manager		*c_manager)
*
*	USAGE		 :	return a page in the chapter manager
*
*	RETURN VALUE :	0 - SUCCESS
*				   -2 - ERROR
*
***************************************************************************/

uint32	generic_pages_return_count = 0;
uint32  generic_page_return(
	uint32			p_index,
	page_node		*p_manager)
{
	uint32	processor_id = 0;
	uint32	flags = get_flags_no_cli();
	uint32	*page_ptr;

	if ((p_manager->page_flags & PAGE_PHYSICAL_EQUALS_LOGICAL_BIT) == 0)
	{
		chapter_queue_unmap_page((uint32 *) (p_index << LOGICAL_PAGE_SHIFT), p_manager);

		if (generic_pages_return_count++ > 16)
		{
			generic_pages_return_count = 0;
			chapter_unmap_queued_pages(p_manager);
		}
		set_flags(flags);
		return(0);
	}
//	alock_read_lock(p_manager->page_rwlock, &processor_id);
	cli();
	spin_lock((uint32 *) p_manager->page_rwlock);
	page_ptr = (uint32 *) (p_index << LOGICAL_PAGE_SHIFT);
vmdbg	if (page_ptr[4] == PAGE_FREE_STAMP)
vmdbg		panic("integrity check in generic_page_return");
	new_list_return(p_manager->page_list_head, processor_id, page_ptr);
	page_ptr[4] = PAGE_FREE_STAMP;
//	alock_read_unlock(p_manager->page_rwlock, processor_id);
	spin_unlock((uint32 *) p_manager->page_rwlock);
	set_flags(flags);
	return(0);
}


/***************************************************************************
*
*   FUNCTION	 :  generic_page_alloc(
*						uint32			**page_ptr,
*						chapter_manager	*c_manager)
*
*	USAGE		 :	alloc a page in the chapter manager
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/

uint32	generic_page_backoff_counter = 0;

uint32  generic_page_alloc(
	uint32		**page_ptr,
	page_node	*p_manager,
	uint32		owner_id)
{
	uint32	max_count;
        uint32	max_processor_id;
	uint32	processor_id = 0;
	uint32	l_index;
	uint32	t_index;
	uint32	logical_page;
	uint32	i;
	uint32	base;
	uint32	*l_ptr;
	uint32	ccode;
	uint32	flags = get_flags_no_cli();
	quad_entry	*l_entry;
	quad_entry	*t_entry;
	chapter_node	*chapter;


	while (1)
	{
//		alock_read_lock(p_manager->page_rwlock, &processor_id);
		cli();
		spin_lock((uint32 *) p_manager->page_rwlock);

		if (new_list_alloc(p_manager->page_list_head, processor_id, &l_ptr) == 0)
		{
vmdbg			if (l_ptr[4] != PAGE_FREE_STAMP)
vmdbg				panic("integrity check in generic_page_alloc");
			l_ptr[4] = 0;
			if ((p_manager->page_flags & PAGE_SUB_OWNER_BIT) != 0)
				chapter_set_owner_id(chapter, l_ptr, owner_id);
			*page_ptr = l_ptr;
			spin_unlock((uint32 *) p_manager->page_rwlock);
			set_flags(flags);
			if ((p_manager->page_flags & PAGE_INITIALIZE_DATA_BIT) != 0)
				SetDataD(l_ptr, p_manager->page_initialization_value, (p_manager->page_size << LOGICAL_PAGE_SHIFT)/4);
			return(0);
		}
//		alock_read_unlock(p_manager->page_rwlock, processor_id);
		spin_unlock((uint32 *) p_manager->page_rwlock);
		set_flags(flags);
/*
		if ((p_manager->page_list_head[0 * QUADS_PER_PAGE].high_entry +
		     p_manager->page_list_head[1 * QUADS_PER_PAGE].high_entry +
		     p_manager->page_list_head[2 * QUADS_PER_PAGE].high_entry +
		     p_manager->page_list_head[3 * QUADS_PER_PAGE].high_entry) != 0)
		{
			max_count = 0;
			for (i=0; i<4; i++)
			{
				if (p_manager->page_list_head[i * QUADS_PER_PAGE].high_entry > max_count)
				{
					max_count = p_manager->page_list_head[i * QUADS_PER_PAGE].high_entry;
					max_processor_id = i;
				}
			}

			if (max_count > 0)
			{
				alock_write_lock(p_manager->page_rwlock);

				while (alock_read_check(p_manager->page_rwlock, max_processor_id) != 0)
					generic_page_backoff_counter ++;

				if (new_list_alloc(p_manager->page_list_head, max_processor_id, &l_ptr) == 0)
				{
					if (l_ptr[4] != PAGE_FREE_STAMP)
						BREAK();
					l_ptr[4] = 0;

					if ((p_manager->page_flags & PAGE_SUB_OWNER_BIT) != 0)
						chapter_set_owner_id(chapter, l_ptr, owner_id);
					*page_ptr = l_ptr;
					for (i=0; i<((max_count + 1) / 2); i++)
					{
						if (new_list_alloc(p_manager->page_list_head, max_processor_id, &l_ptr) != 0)
							break;
						new_list_return(p_manager->page_list_head, processor_id, l_ptr);
					}
					alock_write_unlock(p_manager->page_rwlock);
					if ((p_manager->page_flags & PAGE_INITIALIZE_DATA_BIT) != 0)
						SetDataD((*page_ptr), p_manager->page_initialization_value, (p_manager->page_size << LOGICAL_PAGE_SHIFT)/4);
					return(0);
				}
				alock_write_unlock(p_manager->page_rwlock);
			}
		}
		else
*/
		if ((p_manager->page_flags & PAGE_PHYSICAL_EQUALS_LOGICAL_BIT) != 0)
		{
			if (mpt_alloc_physical_block(&base) != 0)
			{
				return(PAGE_LOGICAL_ALLOCATION_ERROR);
			}
			l_ptr = (uint32 *) (base << LOGICAL_PAGE_SHIFT);
			for (i=p_manager->page_skip; i<=(PHYSICAL_BLOCK_PAGE_COUNT-p_manager->page_size); i+=p_manager->page_skip)
				generic_page_return(base + i, p_manager);
			if ((p_manager->page_flags & PAGE_SUB_OWNER_BIT) != 0)
			{
				if (new_mat_lookup_ccb(&chapter, l_ptr) == 0)
				{
					chapter_set_owner_id(chapter, l_ptr, owner_id);
				}
			}
			*page_ptr = l_ptr;
			return(0);
		}
		else
		{
			return (chapter_index_alloc(page_ptr, p_manager, owner_id));
		}
	}
}
