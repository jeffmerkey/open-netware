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
*   FILE     :  MPT.C
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

/********************************** #defines ****************************************/
/********************************** structures **************************************/

#define	MPT_TEST_STAMP		0x5354504D
#define	MPT_TEST_UNSTAMP		0x55555555

/********************************** typedefs ****************************************/

/********************************** Global Variables ********************************/

extern	LONG	page_fault_post_alloc_counter;

uint32	*move_address;
uint32	mpt_physical_block_mutex = 0;
uint32	vm_special_mutex = 0;
uint32	vm_special_stack[4] = {-1, -1, -1, -1};
uint32	mpt_mutex_flag = 0;
//uint32  *master_page_table[128];
quad_entry	*master_page_table;
uint32  mpt_size = 0;
uint32	mpt_init_flag = 0;
uint32		mpt_rwlock_mah;
mah_node	*mpt_rwlock = 0;
uint32		mpt_temp_rwlock[2];
uint32		mpt_list_head_mah;
mah_node	*mpt_list_head;
uint32		mpt_system_list_head_mah;
mah_node	*mpt_system_list_head;
uint32		mpt_physical_block_list_head_mah;
mah_node	*mpt_physical_block_list_head;
uint32		mpt_system_page_table[64];
uint32		mpt_system_page_use_count = 0;
uint32		mpt_system_page_activity = 0;
uint32		mpt_system_page_max_counter = 10;
uint32		mpt_temp_list_head[2];
uint32		mpt_system_page_count = 0;
uint32	mpt_counter = 0;
uint32	mpt_expanding_flag = 0;
mah_node 	*mpt_activity_counter;
uint32	mpt_activity_counter_mah;
uint32	mpt_expansion_counter[4] = {0};
uint32	mpt_move_counter = {0};
uint32	mpt_total_counter = 0;
uint32	mpt_special_move_counter = 0;
uint32	mpt_physical_block_activity = 0;

uint32	logical_source_page_index;
uint32	*logical_source_page;
uint32	logical_destination_page_index;
uint32	*logical_destination_page;
uint32	page_wait_thread_table[256];
uint32	page_wait_address_table[256];
uint32	page_wait_head = 0;
uint32	page_wait_tail = 0;
uint32	page_wait_need_flag = 0;

uint32	mpt_total_pages = 0;
uint32  mpt_reserved_pages = 0;
uint32  mpt_physical_pages = 0;
uint32  mpt_logical_pages = 0;
uint32  mpt_special_pages = 0;
uint32  mpt_available_pages = 0;
uint32	mpt_system_minimum = 16;


typedef	struct	mpt_node_def
{
	uint32			stamp;
	struct	mpt_node_def	*next_link;
	struct	mpt_node_def	*last_link;
	struct	mpt_node_def	**list_head;
	uint32			*count_head;
	uint32			instance;
	uint32			index;
} mpt_node;
/********************************** function declarations ***************************/

uint32	mpt_physical_block_instance = 0;

void	mpt_lock(void)
{
	spin_lock(&mpt_physical_block_mutex);
	mpt_physical_block_instance ++;

}


void	mpt_unlock(void)
{
	spin_unlock(&mpt_physical_block_mutex);

}



/***************************************************************************
*
*   FUNCTION	 :  mpt_print_stats(void)
*
*	USAGE		 :	intialize the master page table
*
*	RETURN VALUE :	void
*
***************************************************************************/
uint32	boot_pte;
uint32	boot_pte_count;


/***************************************************************************
*
*   FUNCTION	 :  mpt_init(void)
*
*	USAGE		 :	intialize the master page table
*
*	RETURN VALUE :	void
*
***************************************************************************/
void	mpt_init(
	uint32	beginning_page_index,
	uint32	ending_page_index)
{
	uint32	i, j, skip_flag;
	uint32	*pte, *debug_pte;

	uint32	table_page_count;
	boot_pte = beginning_page_index;
	boot_pte_count = 8;
	table_page_count = ((ending_page_index + 0x1FF) >> 9) + 8;
	while (((table_page_count + beginning_page_index) & PHYSICAL_BLOCK_PAGE_UNMASK) != 0)
		table_page_count ++;

	pmah_alloc(&mpt_activity_counter_mah, &mpt_activity_counter);
	pmah_alloc(&mpt_rwlock_mah, &mpt_rwlock);
	pmah_alloc(&mpt_list_head_mah, &mpt_list_head);
	pmah_alloc(&mpt_system_list_head_mah, &mpt_system_list_head);
	pmah_alloc(&mpt_physical_block_list_head_mah, &mpt_physical_block_list_head);

	mpt_size = ending_page_index;
	mpt_total_pages = ending_page_index;
	mpt_reserved_pages = beginning_page_index + table_page_count;

//	generic_chapter_alloc((uint32 **) &master_page_table, ((ending_page_index * 8) + LOGICAL_PAGE_SIZE - 1) >> LOGICAL_PAGE_SHIFT, CHAPTER_SYSTEM_MEMORY | CHAPTER_PHYSICAL_MAH);
	mat_alloc(&system_book, (chapter_node *) RESERVED_CCB, (uint32 **) &master_page_table, 1);
	for (i=0; i<table_page_count-8; i++)
		MMU_page_map((uint32) &master_page_table[i * QUADS_PER_PAGE] >> LOGICAL_PAGE_SHIFT, beginning_page_index + i + 8, 0);

	if (ending_page_index > (PHYSICAL_EQUALS_LOGICAL_BOUNDARY >> PHYSICAL_PAGE_SHIFT))
	{
		for (i=beginning_page_index + table_page_count; i<PHYSICAL_EQUALS_LOGICAL_BOUNDARY >> PHYSICAL_PAGE_SHIFT; i+= PHYSICAL_BLOCK_PAGE_COUNT)
		{
			SetDataD((uint32 *) &master_page_table[i], MPT_PHYSICAL_BLOCK, PHYSICAL_BLOCK_PAGE_COUNT * 2);
			mpt_return_physical_block(i);
		}
		for (; i<ending_page_index; i++)
		{
			mpt_physical_pages ++;
			mpt_return_page(i);
		}
	}
	else
	{
		for (i=beginning_page_index + table_page_count; i<ending_page_index; i+= PHYSICAL_BLOCK_PAGE_COUNT)
		{
			SetDataD((uint32 *) &master_page_table[i], MPT_PHYSICAL_BLOCK, PHYSICAL_BLOCK_PAGE_COUNT * 2);
			mpt_return_physical_block(i);
		}

	}
	SetDataD((uint32 *) master_page_table, MPT_RESERVED_PAGE, ((beginning_page_index + table_page_count) * 8) / 4);
}



/***************************************************************************
*
*   FUNCTION	 :  mpt_wait_for_page(
*						table_entry     **entry,
*						uint32          p_index)
*
*	USAGE		 :	map the entries in the master page table
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/

void	mpt_wait_for_page(
	uint32	fault_address,
	uint32	thread_id)
{
	if (page_wait_thread_table[page_wait_head & 0xFF] != 0)
		rescheduleThread((PROCESS *) page_wait_thread_table[page_wait_head & 0xFF]);

	page_wait_thread_table[page_wait_head & 0xFF] = thread_id;
	page_wait_address_table[page_wait_head & 0xFF] = fault_address;
	page_wait_head ++;
	page_wait_need_flag = 1;
}

/***************************************************************************
*
*   FUNCTION	 :  mpt_map_entry(
*						table_entry     **entry,
*						uint32          p_index)
*
*	USAGE		 :	map the entries in the master page table
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/
void  mpt_map_entry(
	quad_entry	**entry,
	uint32          p_index)
{
	if (p_index < mpt_size)
	{
		*entry = &master_page_table[p_index];
		return;
	}
vmdbg	panic("invalide entry passed to mpt_map_entry error");
}

//
//	NEW MASTER PAGE TABLE APIS
//
uint32	mpt_alloc_backoff_counter;

LONG	mpt_alloc_page(
	LONG	*ppn,
	uint32		mpt_entry_low,
	uint32		mpt_entry_high)
{
	uint32	i;
	uint32	processor_id = 0;
	uint32	max_processor_id;
	uint32	max_count;
	uint32	p_index;
	uint32	flags = get_flags_no_cli();
	quad_entry	*p_entry;

//	alock_read_lock(mpt_rwlock, &processor_id);
	cli();
//	spin_lock(&mpt_physical_block_mutex);
	mpt_lock();
	mpt_activity_counter[processor_id * QUADS_PER_PAGE].low_entry ++;
	if (new_index_alloc(mpt_list_head, processor_id, &p_index, &p_entry, mpt_map_entry, 0) == 0)
	{
		p_entry->low_entry = mpt_entry_low;
		p_entry->high_entry = mpt_entry_high + get_processor_id();
//		alock_read_unlock(mpt_physical_block_mutex, processor_id);
//		spin_unlock(&mpt_physical_block_mutex);
		mpt_unlock();
		set_flags(flags);
		*ppn = p_index;
		return(0);
	}
//	alock_read_unlock(mpt_physical_block_mutex, processor_id);
//	spin_unlock(&mpt_physical_block_mutex);
	mpt_unlock();
	set_flags(flags);

/*	if ((mpt_list_head[0 * QUADS_PER_PAGE].high_entry +
	     mpt_list_head[1 * QUADS_PER_PAGE].high_entry +
	     mpt_list_head[2 * QUADS_PER_PAGE].high_entry +
	     mpt_list_head[3 * QUADS_PER_PAGE].high_entry) != 0)
	{
		max_count = 0;
		for (i=0; i<4; i++)
		{
			if (mpt_list_head[i * QUADS_PER_PAGE].high_entry > max_count)
			{
				max_count = mpt_list_head[i * QUADS_PER_PAGE].high_entry;
				max_processor_id = i;
			}
		}

		if (max_count > 0)
		{
			alock_write_lock(mpt_physical_block_mutex);

			while (alock_read_check(mpt_physical_block_mutex, max_processor_id) != 0)
				mpt_alloc_backoff_counter ++;

			if (new_index_alloc(mpt_list_head, max_processor_id, &p_index, &p_entry, mpt_map_entry, 0) == 0)
			{
				p_entry->low_entry = mpt_entry_low;
				p_entry->high_entry = mpt_entry_high + get_processor_id();
				*ppn = p_index;

				for (i=0; i<((max_count + 1) / 2); i++)
				{
					if (new_index_alloc(mpt_list_head, max_processor_id, &p_index, &p_entry, mpt_map_entry, 0) != 0)
						break;
					new_index_return(mpt_list_head, processor_id, p_index, mpt_map_entry, 0);
				}
				alock_write_unlock(mpt_physical_block_mutex);
				return(0);
			}
			alock_write_unlock(mpt_physical_block_mutex);
		}
	}
*/
	if (mpt_alloc_physical_block(&p_index) == 0)
	{
//		alock_read_lock(mpt_physical_block_mutex, &processor_id);
		cli();
//		spin_lock(&mpt_physical_block_mutex);
		mpt_lock();
		for (i=1; i<PHYSICAL_BLOCK_PAGE_COUNT; i++)
			new_index_return(mpt_list_head, processor_id, p_index + i, mpt_map_entry, 0);
		p_entry = &master_page_table[p_index];
		p_entry->low_entry = mpt_entry_low;
		p_entry->high_entry = mpt_entry_high + get_processor_id();
//		alock_read_unlock(mpt_physical_block_mutex, processor_id);
//		spin_unlock(&mpt_physical_block_mutex);
		mpt_unlock();
		set_flags(flags);
		*ppn = p_index;
		return(0);
	}
	return(-1);
}

LONG	mpt_return_page(
	LONG	ppn)
{
	uint32	processor_id = 0;
	uint32	flags = get_flags_no_cli();

//	alock_read_lock(mpt_physical_block_mutex, &processor_id);
	cli();
//	spin_lock(&mpt_physical_block_mutex);
	mpt_lock();
	mpt_activity_counter[processor_id * QUADS_PER_PAGE].high_entry ++;
	new_index_return(mpt_list_head, processor_id, ppn, mpt_map_entry, 0);
//	alock_read_unlock(mpt_physical_block_mutex, processor_id);
//	spin_unlock(&mpt_physical_block_mutex);
	mpt_unlock();
	set_flags(flags);
	return(0);
}

LONG	mpt_system_alloc_page(
	LONG	*ppn,
	uint32		mpt_entry_low,
	uint32		mpt_entry_high)
{
	uint32	i;
	uint32	p_index;
	uint32	flags = get_flags_no_cli();
	quad_entry	*p_entry;

	if (mpt_system_page_count < mpt_system_page_max_counter)
	{
		if (mpt_alloc_physical_block(&p_index) == 0)
		{
//		alock_read_lock(mpt_physical_block_mutex, &processor_id);
			cli();
//			spin_lock(&mpt_physical_block_mutex);
			mpt_lock();
			for (i=0; i<PHYSICAL_BLOCK_PAGE_COUNT; i++)
			{
				new_index_return(mpt_system_list_head, 0, p_index + i, mpt_map_entry, 0);
				master_page_table[p_index + i].low_entry |= MPT_SYSTEM_PAGE_BIT;
				mpt_system_page_count ++;
			}
//		alock_read_unlock(mpt_physical_block_mutex, processor_id);
//			spin_unlock(&mpt_physical_block_mutex);
			mpt_unlock();
			set_flags(flags);
		}
	}
	cli();
//	spin_lock(&mpt_physical_block_mutex);
	mpt_lock();
vmdbg	if (mpt_system_page_count != mpt_system_list_head->high_entry)
vmdbg		panic("list count check failure in mpt_system_alloc_page");
	if (new_index_alloc(mpt_system_list_head, 0, &p_index, &p_entry, mpt_map_entry, 0) == 0)
	{
		mpt_system_page_activity ++;
		mpt_system_page_count --;
		p_entry->low_entry = mpt_entry_low;
		p_entry->high_entry = mpt_entry_high + get_processor_id();
//		spin_unlock(&mpt_physical_block_mutex);
		mpt_unlock();
		*ppn = p_index;
		set_flags(flags);
		return(0);
	}

	if (boot_pte_count > 0)
	{
		boot_pte_count --;
		*ppn = boot_pte + boot_pte_count;
//		spin_unlock(&mpt_physical_block_mutex);
		mpt_unlock();
		return(0);
	}
//	spin_unlock(&mpt_physical_block_mutex);
	mpt_unlock();
	set_flags(flags);
vmdbg		panic("allocation in mpt_system_alloc_page");
	return(-1);
};


LONG	mpt_system_return_page(
	LONG	ppn)
{
	uint32	flags = get_flags_no_cli();

	cli();
//	spin_lock(&mpt_physical_block_mutex);
	mpt_lock();
	mpt_system_page_activity ++;
	mpt_system_page_count ++;
	new_index_return(mpt_system_list_head, 0, ppn, mpt_map_entry, 0);
	master_page_table[ppn].low_entry |= MPT_SYSTEM_PAGE_BIT;
//	spin_unlock(&mpt_physical_block_mutex);
	mpt_unlock();
	set_flags(flags);
	return(0);
}

LONG	mpt_alloc_physical_block(
	LONG	*pbn)
{
	uint32	*block_ptr;
	uint32	flags = get_flags_no_cli();

	cli();
//	spin_lock(&mpt_physical_block_mutex);
	mpt_lock();
	mpt_physical_block_activity ++;
	if (new_list_alloc(mpt_physical_block_list_head, 0, &block_ptr) == 0)
	{
//		spin_unlock(&mpt_physical_block_mutex);
		mpt_unlock();
		*pbn = (uint32) block_ptr >> PHYSICAL_PAGE_SHIFT;
		set_flags(flags);
		return(0);
	}
//	spin_unlock(&mpt_physical_block_mutex);
	mpt_unlock();
	set_flags(flags);
	return(-1);
}

LONG	mpt_return_physical_block(
	LONG	pbn)
{
	uint32	*block_ptr;
	uint32	flags = get_flags_no_cli();

	block_ptr = (uint32 *) (pbn << PHYSICAL_PAGE_SHIFT);
	cli();
//	spin_lock(&mpt_physical_block_mutex);
	mpt_lock();
	mpt_physical_block_activity ++;
	new_list_return(mpt_physical_block_list_head, 0, block_ptr);
//	spin_unlock(&mpt_physical_block_mutex);
	mpt_unlock();
	set_flags(flags);
	return(0);
}

uint32	mpt_consolidate_backoff_counter=0;

uint32	mpt_consolidate_physical_blocks(
	uint32		next_block_id)
{
	uint32	i, j, k;
	uint32	lock_state = -1;
	uint32	flags;
	uint32	ppn;
	uint32	new_ppn;
	uint32	stack_ppn;
	uint32	process_ppn;
	uint32	thread_id;
	uint32	free_pages;
	uint32	repeat_flag;
	uint32	abort_flag;
	uint32	lpn;
	uint32	sys_page_flag[PHYSICAL_BLOCK_PAGE_COUNT];
	quad_entry	*p_entry;
	chapter_node	*chapter;

	new_list_list_count(mpt_list_head, &free_pages);
	if (free_pages <= (PHYSICAL_BLOCK_PAGE_COUNT * 2))
		return(-1);

	MMU_page_get_info((uint32) &ppn >> LOGICAL_PAGE_SHIFT, &stack_ppn, &flags);
	stack_ppn = stack_ppn >> LOGICAL_PAGE_SHIFT;
	thread_id = (uint32) get_running_process();
	MMU_page_get_info(thread_id >> LOGICAL_PAGE_SHIFT, &process_ppn, &flags);
	process_ppn = process_ppn >> LOGICAL_PAGE_SHIFT;
	for (i=next_block_id; i<PHYSICAL_EQUALS_LOGICAL_BOUNDARY >> PHYSICAL_PAGE_SHIFT; i+= PHYSICAL_BLOCK_PAGE_COUNT)
	{
		if ((i != (stack_ppn & PHYSICAL_BLOCK_PAGE_MASK)) &&
			(i != (process_ppn & PHYSICAL_BLOCK_PAGE_MASK)))
		{
			abort_flag = 0;
			repeat_flag = 0;

			for (j=0; j<PHYSICAL_BLOCK_PAGE_COUNT;)
			{
				sys_page_flag[j] = 0;

				if ((master_page_table[i + j].low_entry & MPT_NONMOVABLE_PAGE_BIT) == 0)
				{


//					alock_write_lock(mpt_physical_block_mutex);
//					if (lock_state == 0)
//						BREAK();
					cli();
//					spin_lock(&mpt_physical_block_mutex);
					mpt_lock();
					lock_state = 0;
//					for (k=0; k<active_processor_count; k++)
//					{
//						while (alock_read_check(mpt_physical_block_mutex, k) != 0)
//							mpt_consolidate_backoff_counter ++;
//					}

					if ((master_page_table[i + j].low_entry & 0xF0000000) == 0)
					{
						if (new_index_alloc_specific(mpt_list_head, i + j, &p_entry, mpt_map_entry, 0) != 0)
								abort_flag = 1;

//						spin_unlock(&mpt_physical_block_mutex);
						mpt_unlock();
						lock_state = 0x23232323;
					}
					else
					{
						if ((master_page_table[i + j].low_entry & MPT_SYSTEM_PAGE_BIT) != 0)
						{
							sys_page_flag[j] = 1;
							if (new_index_alloc_specific(mpt_system_list_head, i + j, &p_entry, mpt_map_entry, 0) != 0)
							{
									abort_flag = 1;
								mpt_system_page_count --;
							}
//							spin_unlock(&mpt_physical_block_mutex);
							mpt_unlock();
							lock_state = 0x45454545;
						}
						else if ((master_page_table[i + j].low_entry & MPT_ASSIGNED_BIT) != 0)
						{
							chapter = (chapter_node *) ((master_page_table[i+j].low_entry & 0x0FFFFFFF) << 4);
//							if (chapter->chapter_stamp != CHAPTER_CONTROL_BLOCK_STAMP)
//								BREAK();
//							alock_write_unlock(mpt_physical_block_mutex);
//							spin_unlock(&mpt_physical_block_mutex);
							mpt_unlock();
							lock_state = 0x56565656;
//							alock_write_lock(chapter->rwlock);
							spin_lock((uint32 *) chapter->rwlock);
//							for (k=0; k<active_processor_count; k++)
//							{
//								while (alock_read_check(chapter->rwlock, k) != 0)
//									mpt_consolidate_backoff_counter ++;
//							}

							if (mpt_alloc_page(&new_ppn, MPT_MOVING_PAGE, MPT_MOVING_PAGE) == 0)
							{
								if (((master_page_table[i + j].low_entry & MPT_ASSIGNED_BIT) == 0) ||
									((master_page_table[i + j].low_entry << 4) != (uint32) chapter))
								{
//									BREAK();
									mpt_return_page(new_ppn);
									repeat_flag = 1;
								}
								else
								{
									mpt_move_counter ++;
									lpn = master_page_table[i + j].high_entry >> LOGICAL_PAGE_SHIFT;
									master_page_table[new_ppn].low_entry = master_page_table[i + j].low_entry;
									master_page_table[new_ppn].high_entry = master_page_table[i + j].high_entry;
									master_page_table[i + j].low_entry = 0x01111111;
//									master_page_table[i + j].high_entry = 0x01111111;
									MMU_page_get_info(lpn, &ppn, &flags);
									ppn = ppn >> LOGICAL_PAGE_SHIFT;
//									if (ppn != (i + j))
//										BREAK();
									MMU_page_set_info(lpn, ppn << LOGICAL_PAGE_SHIFT, flags | MMU_FLAGS_MOVING_BIT | MMU_FLAGS_NOT_PRESENT_BIT);
									MMU_page_map(((uint32) &move_address[j * LONGS_PER_PAGE]) >> LOGICAL_PAGE_SHIFT, new_ppn, 0);
									MMU_invalidate_page(lpn);
									CopyDataD((uint32 *) (ppn << LOGICAL_PAGE_SHIFT), &move_address[j * LONGS_PER_PAGE], LONGS_PER_PAGE);
									MMU_page_set_info(lpn, new_ppn << LOGICAL_PAGE_SHIFT, flags);
									MMU_page_unmap(((uint32) &move_address[j * LONGS_PER_PAGE]) >> LOGICAL_PAGE_SHIFT, 0, 0);
									MMU_invalidate_page(lpn);
								}
							}
							else
							{
								repeat_flag = 1;
							}
//							alock_write_unlock(chapter->rwlock);
							spin_unlock((uint32 *) chapter->rwlock);
						}
						else
						{
//							BREAK();
						}
					}

				}
				else
				{
					abort_flag = 1;
				}

				if (lock_state == 0)
				{
					repeat_flag = 1;
//					BREAK();
				}

				if (abort_flag != 0)
				{
					while (j > 0)
					{
						j--;
						if (sys_page_flag[j] == 0)
							mpt_return_page(i+j);
						else
							mpt_system_return_page(i+j);

					}
					break;
				}

				if (repeat_flag == 0)
					j++;
			}
			if (j == PHYSICAL_BLOCK_PAGE_COUNT)
			{
				SetDataD((uint32 *) &master_page_table[i], MPT_PHYSICAL_BLOCK, PHYSICAL_BLOCK_PAGE_COUNT * 2);
				mpt_return_physical_block(i);
				set_flags(flags);
				return(i + PHYSICAL_BLOCK_PAGE_COUNT);
			}
		}
	}
	set_flags(flags);
	return(-1);
}
