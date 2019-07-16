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
*   FILE     :  MOH.C
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

/********************************** structures **************************************/

/********************************** typedefs ****************************************/

/********************************** Global Variables ********************************/
page_node	mah_page_manager;
mah_byte_node	mah_manager;

/********************************** function declarations ***************************/

uint32	mah_temp_page_list_head[4];
uint32	mah_temp_page_index_head[4];
uint32	mah_temp_byte_list_head[4];
uint32	mah_temp_page_total_stats[4];
uint32	mah_temp_byte_total_stats[4];

uint32	mah_alloc_counter[4];
uint32  mah_return_counter[4];
uint32	mah_expansion_counter[4];
uint32	mah_temp_index = 0;
uint32	mah_temp[64];
uint32	mah_init_flag = 1;


uint32	mah_write_wait_counter = 0;
uint32	mah_read_wait_counter = 0;
uint32	mah_read_check_counter = 0;
uint32	pmah_index = 0;
mah_node	pmah_buffer[0x8000 / 8];

/********************************** function declarations ***************************/

void	mah_print_stats(void)
{
	uint32	i;
	uint32	total_mah_alloc_counter = 0;
	uint32	total_mah_return_counter = 0;
	uint32	total_mah_expansion_counter = 0;

	printf("\n\n mah page statistics \n\n");
	for ( i = 0; i < 4; ++i)
	{
		total_mah_alloc_counter += mah_alloc_counter[i];
		mah_alloc_counter[i] = 0;
		total_mah_return_counter += mah_return_counter[i];
		mah_return_counter[i] = 0;
		total_mah_expansion_counter += mah_expansion_counter[i];
		mah_expansion_counter[i] = 0;
	}
	printf("mah alloc counter = %d\n",total_mah_alloc_counter);
	printf("mah return counter = %d\n",total_mah_return_counter);
	printf("mah expansion counter = %d\n", total_mah_expansion_counter);
}

/***************************************************************************
*
*   FUNCTION	 :  void    mah_init(void)
*
*	USAGE		 :	initialize the master affinity handle table.
*
*	RETURN VALUE :	void
*
***************************************************************************/
void    mah_init(void)
{
	mah_byte_node	*head;
	uint32		*temp;
/*
	generic_page_pre_init(&mah_page_manager,
			&system_book,
			MAH_CHAPTER_TYPE,
			PAGE_PHYSICAL_EQUALS_LOGICAL_BIT | PAGE_ALLOC_IMMEDIATE_BIT | PAGE_INITIALIZE_DATA_BIT,
			MPT_NON_PAGEABLE_PAGE,
			MAT_PHYSICAL_ADDRESS,
			6,			// physical = logical
			8,			// alloc immediate_flag
			0);
*/

	generic_page_init(&mah_page_manager,
		&system_book,
		MAH_CHAPTER_TYPE,
		PAGE_INITIALIZE_DATA_BIT | PAGE_PHYSICAL_MAH | PAGE_ALLOC_IMMEDIATE_BIT | CHAPTER_PHYSICAL_MAH,
		0,
		MAT_VIRTUAL_ADDRESS,
		6,	// page size
		8,	// page skip
		0);

	mah_manager.mah_stamp = 0x4848414D;
	mah_manager.mah_type = mah_manager.mah_stamp;
	mah_manager.mah_mutex = 0;
	mah_manager.mah_extension_flag = 0;

	mah_manager.mah_size = 8;
	mah_manager.mah_skip = 8;	// used to create holes in memory by skipping logical addresses
	mah_manager.mah_flags = 0;
	mah_manager.mah_initialization_value = 0;

	mah_manager.indexes_per_block = 4096 / 8;		// one page / 8 bytes
	mah_manager.block_size = 8;				// 8 pages
	mah_manager.blocks_per_chapter = 1024 / mah_manager.block_size;

	pmah_alloc(&mah_manager.mah_rwlock_mah, &mah_manager.mah_rwlock);
	pmah_alloc(&mah_manager.mah_list_head_mah, &mah_manager.mah_list_head);
	pmah_alloc(&mah_manager.mah_total_stats_mah, &mah_manager.mah_total_stats_ptr);
	pmah_alloc(&mah_manager.mah_temp_stats_mah, &mah_manager.mah_temp_stats_ptr);

	mah_manager.book_link = mah_page_manager.book_link;
	mah_manager.page_link = &mah_page_manager;

	cli();
	spin_lock(&mah_manager.book_link->book_mutex);
	if ((head = (mah_byte_node *) mah_page_manager.byte_link) == 0)
	{
		mah_manager.last_link = &mah_manager;
		mah_manager.next_link = &mah_manager;
		mah_page_manager.byte_link = (byte_node *) &mah_manager;
	}
	else
	{
		mah_manager.last_link = head->last_link;
		mah_manager.next_link = head;
		head->last_link->next_link = &mah_manager;
		head->last_link = &mah_manager;
	}
	spin_unlock(&mah_page_manager.book_link->book_mutex);
	sti();
}


/***************************************************************************
*
*   FUNCTION	 :  mah_map_entry(
*						table_entry     **h_entry,
*						uint32          h_index)
*
*	USAGE		 :	map the master affinity handle table.
*
*	RETURN VALUE :	0 - SUCCESS
*					-1 - FAILURE
*
***************************************************************************/

void mah_map_entry(
	quad_entry	**h_entry,
	uint32          h_index,
	mah_byte_node	*m_manager)
{
	uint32	b_index;
	uint32	c_index;
	uint32	m_index;
	quad_entry	*block;
	chapter_node	*chapter;
	page_node	*p_manager;

	b_index = h_index / m_manager->indexes_per_block;
	c_index = b_index / m_manager->blocks_per_chapter;
	m_index = h_index % m_manager->indexes_per_block;
	b_index = b_index % m_manager->blocks_per_chapter;
	p_manager = m_manager->page_link;
	if ((chapter = p_manager->chapter_list_head) != 0)
	{
		do
		{
			if (chapter->chapter_logical_index == c_index)
			{
				block = (quad_entry *) &chapter->base_address[(b_index * m_manager->block_size * 4096) / 4];
				*h_entry = &block[m_index];
				return;
			}
			chapter = chapter->page_next_link;
		} while (chapter != p_manager->chapter_list_head);
	}
vmdbg	panic("invalid mah id passed to mah_map_entry");
	return;
}



/***************************************************************************
*
*   FUNCTION	 :  mah_return(
*						uint32  mah_index)
*
*	USAGE		 :	returns entries to the mah table
*
*	RETURN VALUE :	0 - SUCCESS
*
***************************************************************************/
uint32  mah_return(
	uint32  ret_mah_index)
{
	uint32	processor_id = 0;
	uint32	mah_index;
	uint32	flags = get_flags_no_cli();

//	alock_read_lock(mah_manager.mah_rwlock, &processor_id);
	cli();
	spin_lock((uint32 *) mah_manager.mah_rwlock);
	new_index_return(mah_manager.mah_list_head, processor_id, ret_mah_index, mah_map_entry, &mah_manager);
	mah_return_counter[processor_id]++;
//	alock_read_unlock(mah_manager.mah_rwlock, processor_id);
	spin_unlock((uint32 *) mah_manager.mah_rwlock);
	set_flags(flags);

	return(0);
}




/***************************************************************************
*
*   FUNCTION	 :  mah_expand(void)
*
*	USAGE		 :	allocates pages in the physical address space
*
*	RETURN VALUE :	VOID
*
***************************************************************************/

void    mah_expand(void)
{
	uint32	i;
	uint32	*page_ptr;
	uint32	mah_base;
	chapter_node	*chapter;

	while (1)
	{
		if (generic_page_alloc(&page_ptr, &mah_page_manager, 0) == 0)
		{
//			generic_page_index(page_ptr, &mah_base, &mah_page_manager);
			new_mat_lookup_ccb(&chapter, page_ptr);
			mah_base = (chapter->chapter_logical_index << LOGICAL_CHAPTER_SHIFT) >> LOGICAL_PAGE_SHIFT;
			mah_base += ((uint32) page_ptr >> LOGICAL_PAGE_SHIFT) & PHYSICAL_CHAPTER_PAGE_UNMASK;
			mah_base /= mah_manager.block_size;
			mah_base *= mah_manager.indexes_per_block;
			for (i=0; i<mah_manager.indexes_per_block; i++, page_ptr += 2)
			{
				page_ptr[0] = 0;
				page_ptr[1] = 0;
				mah_return(i+mah_base);
			}
			mah_expansion_counter[get_processor_id()] ++;
			return;
		}
//		if (chapter_index_expand(&mah_page_manager) != 0)
//		{
			if (chapter_expand(&mah_page_manager) != 0)
			{
				return;
			}
//		}
	}
}

/***************************************************************************
*
*   FUNCTION	 :  mah_alloc(
*						uint32  *mah_index,
*						uint32  **mah_ptr)

*
*	USAGE		 :	allocates available space for each processor
*
*	RETURN VALUE :	0 - SUCCESS
*
***************************************************************************/
uint32	mah_alloc_backoff_counter = 0;

uint32  mah_alloc(
	uint32  *new_mah_index,
	mah_node  **new_mah_ptr)
{
	quad_entry	*mah_ptr;
	uint32	processor_id = 0;
	uint32	max_processor_id;
	uint32	p_index;
	uint32	max_count;
	uint32	i;
	uint32	temp_index;
	uint32	flags = get_flags_no_cli();

	while (1)
	{
//		alock_read_lock(mah_manager.mah_rwlock, &processor_id);
		cli();
		spin_lock((uint32 *) mah_manager.mah_rwlock);

		if (new_index_alloc(mah_manager.mah_list_head, processor_id, new_mah_index, &mah_ptr, mah_map_entry, &mah_manager) == 0)
		{
			mah_alloc_counter[processor_id]++;
//			alock_read_unlock(mah_manager.mah_rwlock, processor_id);
			spin_unlock((uint32 *) mah_manager.mah_rwlock);
			set_flags(flags);
			mah_ptr->low_entry = 0;
			mah_ptr->high_entry = 0;
			*new_mah_ptr = (mah_node *) mah_ptr;
			return(0);
		}
//		alock_read_unlock(mah_manager.mah_rwlock, processor_id);
		spin_unlock((uint32 *) mah_manager.mah_rwlock);
		set_flags(flags);
/*
		if ((mah_manager.mah_list_head[0 * QUADS_PER_PAGE].high_entry +
		     mah_manager.mah_list_head[1 * QUADS_PER_PAGE].high_entry +
		     mah_manager.mah_list_head[2 * QUADS_PER_PAGE].high_entry +
		     mah_manager.mah_list_head[3 * QUADS_PER_PAGE].high_entry) != 0)
		{
			max_count = 0;
			for (i=0; i<4; i++)
			{
				if (mah_manager.mah_list_head[i * QUADS_PER_PAGE].high_entry > max_count)
				{
					max_count = mah_manager.mah_list_head[i * QUADS_PER_PAGE].high_entry;
					max_processor_id = i;
				}
			}

			if (max_count > 0)
			{
				alock_write_lock(mah_manager.mah_rwlock);

				while (alock_read_check(mah_manager.mah_rwlock, max_processor_id) != 0)
					mah_alloc_backoff_counter ++;
				if (new_index_alloc(mah_manager.mah_list_head, max_processor_id, new_mah_index, &mah_ptr, mah_map_entry, &mah_manager) == 0)
				{
					mah_alloc_counter[processor_id]++;
					mah_ptr->low_entry = 0;
					mah_ptr->high_entry = 0;
					*new_mah_ptr = (mah_node *) mah_ptr;
					for (i=0; i<((max_count + 1) / 2); i++)
					{
						if (new_index_alloc(mah_manager.mah_list_head, max_processor_id, &temp_index, &mah_ptr, mah_map_entry, &mah_manager) == 0)
							break;
						new_index_return(mah_manager.mah_list_head, processor_id, temp_index, mah_map_entry, &mah_manager);
					}
					alock_write_unlock(mah_manager.mah_rwlock);
					return(0);
				}
				alock_write_unlock(mah_manager.mah_rwlock);
			}
		}
		else
*/
		{
			mah_expand();
		}
	}
}

uint32  pmah_alloc(
	uint32  *new_mah_index,
	mah_node  **new_mah_ptr)
{
	*new_mah_ptr = (mah_node *) &pmah_buffer[pmah_index];
	*new_mah_index = pmah_index;
	pmah_index ++;
	return(0);
}
