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
*   FILE     :  CHAPTER.C
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

#define	CCB_ELEMENTS	0x45424343
/********************************** structures **************************************/

/********************************** typedefs ****************************************/
extern	page_node	physical_chapter_manager;

byte_node	*vm256_manager;
page_node	vmpage_manager;
byte_node	*vm64_manager;
uint32		vm256_special_list_head[4];

uint32		*vmpage;

/********************************** Global Variables ********************************/





/***************************************************************************
*
*   FUNCTION	 :  generic_page_map_entry(
*						table_entry     **entry,
*						uint32          p_index,
*						chapter_manager	*c_manager)
*
*	USAGE		 :	map a page in the chapter manager
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/

void  chapter_map_entry(
	quad_entry	**entry,
	uint32          p_index,
	chapter_node	*chapter)
{
	if (chapter->page_index_table[(p_index & 0x3FF) >> 5] != 0)
	{
		*entry = &chapter->page_index_table[(p_index & 0x3FF) >> 5][p_index & 0x1F];
		return;
	}
vmdbg	panic("invalide entry passed to chapter_map_entry");
}

/***************************************************************************
*
*   FUNCTION	 :  generic_chapter_return(
*						chapter_control_block   *ccb)
*
*	USAGE		 :	return a page in the chapter manager
*
*	RETURN VALUE :	0 - SUCCESS
*				   -2 - ERROR
*
***************************************************************************/
uint32  generic_chapter_return(
	chapter_node   *ccb)
{
	chapter_destroy(ccb);
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
uint32  generic_chapter_alloc(
	uint32			**page_ptr,
	uint32			page_count,
	uint32			chapter_flags)
{
	register uint32 retCode;
	uint32	phy_pte;
	uint32	i, j;
	uint32	*pte;
	uint32	p_index;
	quad_entry	*p_entry;
	quad_entry	*l_entry;
	uint32	logical_page;
	uint32	ccode;
	chapter_node   *chapter;
	uint32	chapter_count;
	uint32	temp_page_count;
	uint32	flags = get_flags_no_cli();

	if (chapter_create(&chapter, &physical_chapter_manager, chapter_flags, page_count, -1) == 0)
	{
		if ((chapter_flags & CHAPTER_ALLOC_IMMEDIATE_BIT) != 0)
		{

			chapter_count = (page_count + PHYSICAL_CHAPTER_PAGE_SIZE - 1) >> PHYSICAL_CHAPTER_PAGE_SHIFT;
			logical_page = (uint32) chapter->base_address;

			for (i=0; i<chapter_count; i++)
			{
				new_mat_lookup_ccb(&chapter, (uint32 *) logical_page);
				temp_page_count = ((i+1) < chapter_count) ? PHYSICAL_CHAPTER_PAGE_SIZE : page_count - (i * PHYSICAL_CHAPTER_PAGE_SIZE);
//				alock_write_lock(chapter->rwlock);
				cli();
				spin_lock((uint32 *) chapter->rwlock);
				for (j=0; j<temp_page_count; j++, logical_page += LOGICAL_PAGE_SIZE)
				{
					chapter_map_entry(&l_entry, logical_page >> LOGICAL_PAGE_SHIFT, chapter);
					l_entry->low_entry = 0;
					l_entry->high_entry = ALLOCATED_STAMP;

					if ((chapter_flags & CHAPTER_SYSTEM_MEMORY) == 0)
						ccode = mpt_alloc_page(&p_index, ((uint32) chapter >> 4) | MPT_ASSIGNED_BIT, logical_page);
					else
						ccode = mpt_system_alloc_page(&p_index, ((uint32) chapter >> 4) | MPT_NONMOVABLE_PAGE_BIT, logical_page);
					if (ccode != 0)
					{
						panic("error returned from mpt_alloc_page or mpt_system_alloc_page");
					}
					MMU_page_map(logical_page >> 12, p_index, 0);
				}
//				alock_write_unlock(chapter->rwlock);
				spin_unlock((uint32 *) chapter->rwlock);
				set_flags(flags);
			}
		}
		*page_ptr = chapter->base_address;
		set_flags(flags);
		return(0);
	}
	set_flags(flags);
	return(-1);
}

/***************************************************************************
*
*   FUNCTION	 :  generic_page_expand(
*						chapter_manager			*c_manager)
*
*	USAGE		 :	get a new page in the chapter manager, get a new chapter
*					if necessary
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/

/*
uint32  chapter_index_expand(
	page_node	*p_manager)
{
	uint32          i, j, k, l;
	uint32		base;
	uint32		table_count;
	uint32		ccode;
	uint32		*page_ptr;
        uint32		flags = get_flags_no_cli();

	quad_entry	*table;
	chapter_node	*chapter;
	book_node	*book;

	if ((chapter = p_manager->chapter_list_head) != 0)
	{
		do
		{
			if (chapter->base_mask != (LONG) -1)
			{
				ccode = (chapter->chapter_flags & CHAPTER_SPECIAL_BIT) ? vm256_special_alloc((uint32 **) &table) : vm256_alloc((uint32 **) &table);
				if (ccode != 0)
				{
//					p_manager->page_extension_flag = 0;
					return(-1);
				}
//				alock_write_lock(chapter->rwlock);
				cli();
				spin_lock((uint32 *) chapter->rwlock);

				for (i=0; i<32; i++)
				{
					if ((chapter->base_mask & (1 << i)) == 0)
						break;
				}
				if (i < 32)
				{
					chapter->page_count = (i+1) * 32;
					chapter->base_mask  |= 1 << i;
					chapter->page_index_table[i] = table;
					base = (chapter->chapter_index << 10) + (i * 32);
					SetDataD((uint32 *) table, C_ENTRY_OUT_OF_BOUNDS, 64);
					for (j=0; j<=32-p_manager->page_size; j+=p_manager->page_skip)
					{
						p_manager->total_page_count ++;
						for (k=1; k<p_manager->page_size; k++)
						{
							table[j + k].low_entry = C_ENTRY_LIST_EXTENSION;
							table[j + k].high_entry = 0;
						}
						table[j].low_entry = C_ENTRY_LIST_ENTRY;
						table[j].high_entry = 0;
						if ((i + j) != 0)
							new_index_return(chapter->index_list_head, 0, base + j, chapter_map_entry, chapter);
						else
						{
							table[j].low_entry = -1;
							table[j].high_entry = -1;
						}
					}
//					alock_write_unlock(chapter->rwlock);
					spin_unlock((uint32 *) chapter->rwlock);
					set_flags(flags);
//					p_manager->page_extension_flag = 0;
					return(0);
				}
				else
				{
					if (chapter->chapter_flags & CHAPTER_SPECIAL_BIT)
						vm256_special_return((uint32 *) table);
					else
						vm256_return((uint32 *) table);
				}
//				alock_write_unlock(chapter->rwlock);
				spin_unlock((uint32 *) chapter->rwlock);
				set_flags(flags);
			}
			chapter = chapter->page_next_link;
		} while (chapter != p_manager->chapter_list_head);
	}
//	p_manager->page_extension_flag = 0;
	return(-1);

}
*/
uint32    chapter_expand(
	page_node	*p_manager)
{
	chapter_node	*chapter;
	while (1)
	{
		if (p_manager->page_extension_flag == 0)
		{
			spin_lock(&p_manager->page_mutex);
			if (p_manager->page_extension_flag == 0)
				break;
			spin_unlock(&p_manager->page_mutex);
		}
		thread_switch();
	}
	p_manager->page_extension_flag = (uint32) get_running_process();
	spin_unlock(&p_manager->page_mutex);

	if (chapter_create(&chapter, p_manager, p_manager->page_flags, PHYSICAL_CHAPTER_PAGE_SIZE, -1) == 0)
	{
		p_manager->page_extension_flag = 0;
		return(0);
	}
	p_manager->page_extension_flag = 0;
	return(-1);
}

uint32    special_chapter_expand(
	page_node	*p_manager)
{
	chapter_node	*chapter;
	uint32		p_index;
	uint32		*logical_page;
	uint32		i, j, *table;
	uint32		base;

	while (1)
	{
		if (p_manager->page_extension_flag == 0)
		{
			spin_lock(&p_manager->page_mutex);
			if (p_manager->page_extension_flag == 0)
				break;
			spin_unlock(&p_manager->page_mutex);
		}
		thread_switch();
	}
	p_manager->page_extension_flag = (uint32) get_running_process();
	spin_unlock(&p_manager->page_mutex);

	mat_alloc(p_manager->book_link, (chapter_node *) RESERVED_CCB, &logical_page, 1);
	if (mpt_system_alloc_page(&p_index, MPT_CCB_PAGE, MPT_CCB_PAGE) == 0)
	{
		MMU_page_map((uint32) logical_page >> 12, p_index, 0);

		for (i=0; i<LOGICAL_PAGE_SIZE; i+= 256)
			vm256_special_return(&logical_page[i/4]);

		if (chapter_create(&chapter, p_manager, p_manager->page_flags, PHYSICAL_CHAPTER_PAGE_SIZE, (uint32) logical_page) == 0)
		{
			p_manager->page_extension_flag = 0;
			return(0);
		}
		p_manager->page_extension_flag = 0;
		return(-1);
	}
	p_manager->page_extension_flag = 0;
	return(-1);
}


/***************************************************************************
*
*   FUNCTION	 :  generic_control_block_init(
*	page_map_entry(
*						table_entry     **entry,
*						uint32          p_index,
*						chapter_manager	*c_manager)
*
*	USAGE		 :	map a page in the chapter manager
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/

uint32	chapter_create(
	chapter_node	**new_chapter,
	page_node	*p_manager,
	uint32		chapter_flags,
	uint32		page_count,
	uint32		logical_base)
{
	uint32	i, j, ccode;
	uint32	chapter_count;
	uint32	*base_address;
	book_node	*book;
	chapter_node	*head;
	chapter_node	*chapter = 0;

	*new_chapter = 0;
	chapter_count = (page_count + PHYSICAL_CHAPTER_PAGE_SIZE - 1) >> PHYSICAL_CHAPTER_PAGE_SHIFT;
	if (logical_base == (uint32) -1)
		mat_alloc(p_manager->book_link, (chapter_node *) RESERVED_CCB, &base_address, chapter_count);
	else
		base_address = (uint32 *) logical_base;

	for (i=0; i<chapter_count; i++)
	{
		ccode = (chapter_flags & CHAPTER_SPECIAL_BIT) ? vm256_special_alloc((uint32 **) &chapter) : vm256_alloc((uint32 **) &chapter);
		if (ccode != 0)
		{
			while (i > 1)
			{
				BREAK();
			}
			*new_chapter = 0;
			return(-1);

		}
		chapter->base_address = &base_address[(i << LOGICAL_CHAPTER_SHIFT)/4];
		chapter->chapter_index = (uint32) chapter->base_address >> LOGICAL_CHAPTER_SHIFT;
		book = p_manager->book_link;
		chapter->page_link = p_manager;
		chapter->book_link = book;
		chapter->chapter_flags = chapter_flags;
		chapter->page_count = ((i+1) < chapter_count) ? 1024 : ((page_count-1) & 0x3FF) + 1;
		chapter->base_mask = 0;
		chapter->chapter_type = p_manager->page_type;
		chapter->chapter_stamp = CHAPTER_CONTROL_BLOCK_STAMP;
		chapter->pte = 0;

		if ((chapter_flags & CHAPTER_PHYSICAL_MAH) == 0)
		{
			mah_alloc(&chapter->rwlock_mah, &chapter->rwlock);
			mah_alloc(&chapter->index_list_head_mah, &chapter->index_list_head);
			mah_alloc(&chapter->special_unmap_list_head_mah, &chapter->special_unmap_list_head);
			mah_alloc(&chapter->special_index_list_head_mah, &chapter->special_index_list_head);
		}
		else
		{
			pmah_alloc(&chapter->rwlock_mah, &chapter->rwlock);
			pmah_alloc(&chapter->index_list_head_mah, &chapter->index_list_head);
			pmah_alloc(&chapter->special_unmap_list_head_mah, &chapter->special_unmap_list_head);
			pmah_alloc(&chapter->special_index_list_head_mah, &chapter->special_index_list_head);
		}

		cli();
		spin_lock(&book->book_mutex);
		book->chapter_hash_size ++;
		if ((head = book->chapter_hash_table[chapter->chapter_index & 0xFF]) == 0)
		{
			chapter->book_next_link = chapter;
			chapter->book_last_link = chapter;
			book->chapter_hash_table[chapter->chapter_index & 0xFF] = chapter;
		}
		else
		{
			chapter->book_last_link = head->book_last_link;
			chapter->book_next_link = head;
			head->book_last_link->book_next_link = chapter;
			head->book_last_link = chapter;
		}
		if ((head = p_manager->chapter_list_head) == 0)
		{
			chapter->page_next_link = chapter;
			chapter->page_last_link = chapter;
			p_manager->chapter_list_head = chapter;
		}
		else
		{
			chapter->page_last_link = head->page_last_link;
			chapter->page_next_link = head;
			head->page_last_link->page_next_link = chapter;
			head->page_last_link = chapter;
		}
		chapter->chapter_logical_index = p_manager->chapter_logical_index ++;
		spin_unlock(&book->book_mutex);
		sti();

		mat_set_ccb(book, chapter, chapter->base_address);
		if (chapter_count > 1)
		{
			chapter->chapter_flags |= CHAPTER_EXTENSION_GROUP_BIT;
			if ((i+1) < chapter_count)
				chapter->chapter_flags |= CHAPTER_EXTENSION_BIT;
		}
		if (*new_chapter == 0)
			*new_chapter = chapter;
		if ((chapter->chapter_flags & CHAPTER_NOT_SPARSE_BIT) != 0)
		{
			for (j=0; j<32; j++)
			{
				if ((j * 32) < chapter->page_count)
				{
					ccode = (chapter_flags & CHAPTER_SPECIAL_BIT) ? vm256_special_alloc((uint32 **) &chapter->page_index_table[j]) : vm256_alloc((uint32 **) &chapter->page_index_table[j]);
					if (ccode != 0)
						break;
					chapter->base_mask |= 1 << j;
					SetDataD((uint32 *) chapter->page_index_table[j], (chapter->chapter_flags & PAGE_PHYSICAL_EQUALS_LOGICAL_BIT) ? C_ENTRY_MAPPED : C_ENTRY_ALLOCATED, 256 / 4);
				}
			}
		}
	}
	return(0);
}


/***************************************************************************
*
*   FUNCTION	 :  generic_control_block_uninit(
*	page_map_entry(
*						table_entry     **entry,
*						uint32          p_index,
*						chapter_manager	*c_manager)
*
*	USAGE		 :	map a page in the chapter manager
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/

void	chapter_destroy(
	chapter_node	*new_chapter)
{
	uint32		j, k;
	uint32		t_index;
	uint32		done_flag = 1;
	uint32		chapter_count;
	uint32		*base_address;
        uint32		flags = get_flags_no_cli();
        quad_entry	*l_entry;
	book_node	*book;
	page_node	*p_manager;
	chapter_node	*chapter;

	p_manager = new_chapter->page_link;
	book = new_chapter->book_link;

	new_chapter->pte = 0;
//	if ((p_manager->page_flags & PAGE_PHYSICAL_EQUALS_LOGICAL_BIT) == 0)
//		as_chapter_out((uint32) chapter->base_address >> 12);


	base_address = new_chapter->base_address;
	while (done_flag != 0)
	{
		if (new_mat_lookup_ccb(&chapter, base_address) == 0)
		{
			cli();
//			alock_write_lock(chapter->rwlock);
			cli();
			spin_lock((uint32 *) chapter->rwlock);
			spin_lock(&book->book_mutex);

			chapter->book_next_link->book_last_link = chapter->book_last_link;
			chapter->book_last_link->book_next_link = chapter->book_next_link;
			if (book->chapter_hash_table[chapter->chapter_index & 0xFF] == chapter)
			{
				if (chapter->book_next_link == chapter)
						book->chapter_hash_table[chapter->chapter_index & 0xFF] = 0;
				else
					book->chapter_hash_table[chapter->chapter_index & 0xFF] = chapter->book_next_link;
			}
			chapter->page_next_link->page_last_link = chapter->page_last_link;
			chapter->page_last_link->page_next_link = chapter->page_next_link;
			if (p_manager->chapter_list_head == chapter)
			{
				if (chapter->page_next_link == chapter)
					p_manager->chapter_list_head = 0;
				else
					p_manager->chapter_list_head = chapter->page_next_link;
			}
			spin_unlock(&book->book_mutex);
			sti();

			if ((chapter->chapter_flags & CHAPTER_PHYSICAL_MAH) == 0)
			{
				mah_return(chapter->rwlock_mah);
				mah_return(chapter->index_list_head_mah);
				mah_return(chapter->special_unmap_list_head_mah);
				mah_return(chapter->special_index_list_head_mah);
			}
			for (j=0; j<32; j++)
			{
				if ((l_entry = chapter->page_index_table[j]) != 0)
				{
					for (k=0; k<32; k++, l_entry ++)
					{
						if ((l_entry->low_entry & C_ENTRY_MASK) == C_ENTRY_MAPPED)
						{
							MMU_page_unmap((uint32) &chapter->base_address[((j * 32) + k) * LONGS_PER_PAGE] >> LOGICAL_PAGE_SHIFT, &t_index, 0);
							if ((t_index >> 12) != 0)
								mpt_return_page(t_index >> 12);
						}
					}
					vm256_return((uint32 *) chapter->page_index_table[j]);
				}
			}
//			alock_write_unlock(chapter->rwlock);
//			spin_unlock((uint32 *) chapter->rwlock);   already returned
			set_flags(flags);
			vm256_return((uint32 *) chapter);
			mat_return(book, chapter, base_address, 1);
		}
		if ((chapter->chapter_flags & CHAPTER_EXTENSION_BIT) == 0)
			done_flag = 0;
		base_address += (LOGICAL_CHAPTER_SIZE/4);
	}

	cli();
	if (VMFlushTLB() != 0)
		panic("FlushTLB returned an error ");
	sti();
}

uint32	funny_retry_counter = 0;
uint32	chapter_index_alloc_backoff_counter = 0;
uint32	chapter_index_alloc(
	uint32		**logical_address,
	page_node	*p_manager,
	uint32		owner_id)
{
	uint32		i, j, k;
	uint32		l_index;
	uint32		t_index;
	uint32		retry_flag = 1;
	uint32		processor_id = 0;
	uint32		max_processor_id;
	uint32		max_count;
	uint32		ccode;
	uint32		base;
	uint32		flags = get_flags_no_cli();
	quad_entry	*table;
	quad_entry	*l_entry;
	chapter_node	*chapter;

	if ((chapter = p_manager->chapter_list_head) == 0)
	{
		set_flags(flags);
		return(PAGE_LOGICAL_ALLOCATION_ERROR);
	}

	while (retry_flag != 0)
	{
		retry_flag = 0;
vmdbg		if (((uint32) chapter & 0x000000FF) != 0)
vmdbg			panic("integrity check in chapter_index_alloc");
		do
		{
//			alock_read_lock(chapter->rwlock, &processor_id);
			cli();
			spin_lock((uint32 *) chapter->rwlock);
			if (new_index_alloc(chapter->index_list_head, processor_id, &l_index, &l_entry, chapter_map_entry, chapter) == 0)
			{
				if ((p_manager->page_flags & PAGE_ALLOC_IMMEDIATE_BIT) != 0)
				{
					for (i=0; i<p_manager->page_size; i++, l_entry ++)
					{
						if ((p_manager->page_flags & PAGE_SYSTEM_MEMORY) != 0)
							ccode = mpt_system_alloc_page(&t_index, ((uint32) chapter >> 4) | MPT_NONMOVABLE_PAGE_BIT, (l_index + i) << LOGICAL_PAGE_SHIFT);
						else
							ccode = mpt_alloc_page(&t_index, ((uint32) chapter >> 4) | MPT_ASSIGNED_BIT, (l_index + i) << LOGICAL_PAGE_SHIFT);
						if (ccode != 0)
						{
							while (i > 0)
							{
								i--;
								l_entry--;
								l_entry->low_entry = C_ENTRY_LIST_EXTENSION;
								l_entry->high_entry = 0;
								MMU_page_unmap(l_index + i, &t_index, 0);
								if ((t_index >> 12) != 0)
									mpt_return_page(t_index >> 12);
							}
							l_entry->low_entry = C_ENTRY_LIST_ENTRY;
							l_entry->high_entry = 0;
							new_index_return(chapter->index_list_head, processor_id, l_index, chapter_map_entry, chapter);
//							alock_read_unlock(chapter->rwlock, processor_id);
							spin_unlock((uint32 *) chapter->rwlock);
							set_flags(flags);
							return(PAGE_PHYSICAL_ALLOCATION_ERROR);
						}
						l_entry->low_entry = t_index + C_ENTRY_MAPPED;
						l_entry->high_entry = 0;
						MMU_page_map(l_index + i, t_index, 0);
					}
					if ((p_manager->page_flags & PAGE_SUB_OWNER_BIT) != 0)
					{
						l_entry->low_entry = (owner_id >> 4) + C_ENTRY_SUB_OWNER_ID;
						l_entry->high_entry = 0;
					}
//					alock_read_unlock(chapter->rwlock, processor_id);
					spin_unlock((uint32 *) chapter->rwlock);
					set_flags(flags);
					if ((p_manager->page_flags & PAGE_INITIALIZE_DATA_BIT) != 0)
						SetDataD((uint32 *) (l_index << LOGICAL_PAGE_SHIFT), p_manager->page_initialization_value, (p_manager->page_size * 4096)/4);
					*logical_address = (uint32 *) (l_index << LOGICAL_PAGE_SHIFT);
					return(0);
				}
				for (i=0; i<p_manager->page_size; i++, l_entry ++)
				{
					l_entry->low_entry = C_ENTRY_ALLOCATED;
					l_entry->high_entry = 0;
				}
				if ((p_manager->page_flags & PAGE_SUB_OWNER_BIT) != 0)
				{
					l_entry->low_entry = (owner_id >> 4) + C_ENTRY_SUB_OWNER_ID;
					l_entry->high_entry = 0;
				}

//				alock_read_unlock(chapter->rwlock, processor_id);
				spin_unlock((uint32 *) chapter->rwlock);
				set_flags(flags);
				*logical_address = (uint32 *) (l_index << LOGICAL_PAGE_SHIFT);
				return(0);
			}
//			alock_read_unlock(chapter->rwlock, processor_id);
			spin_unlock((uint32 *) chapter->rwlock);
			set_flags(flags);
/*
//			if ((chapter->index_list_head[0 * QUADS_PER_PAGE].high_entry +
//			     chapter->index_list_head[1 * QUADS_PER_PAGE].high_entry +
//			     chapter->index_list_head[2 * QUADS_PER_PAGE].high_entry +
//			     chapter->index_list_head[3 * QUADS_PER_PAGE].high_entry) != 0)
//			{
				max_count = 0;
				for (i=0; i<4; i++)
				{
					if (chapter->index_list_head[i * QUADS_PER_PAGE].high_entry > max_count)
					{
						max_count = chapter->index_list_head[i * QUADS_PER_PAGE].high_entry;
						max_processor_id = i;
					}
				}
				if (max_count > 0)
				{
					alock_write_lock(chapter->rwlock);
					while (alock_read_check(chapter->rwlock, max_processor_id) != 0)
						chapter_index_alloc_backoff_counter ++;

					if (new_index_alloc(chapter->index_list_head, max_processor_id, &l_index, &l_entry, chapter_map_entry, chapter) == 0)
					{
						if ((p_manager->page_flags & PAGE_ALLOC_IMMEDIATE_BIT) != 0)
						{
							for (i=0; i<p_manager->page_size; i++, l_entry ++)
							{
								if ((p_manager->page_flags & PAGE_SYSTEM_MEMORY) != 0)
									ccode = mpt_system_alloc_page(&t_index, ((uint32) chapter >> 4) | MPT_NONMOVABLE_PAGE_BIT, (l_index + i) << LOGICAL_PAGE_SHIFT);
								else
									ccode = mpt_alloc_page(&t_index, ((uint32) chapter >> 4) | MPT_ASSIGNED_BIT, (l_index + i) << LOGICAL_PAGE_SHIFT);
								if (ccode != 0)
								{
									while (i > 0)
									{
										i--;
										l_entry --;
										l_entry->low_entry = C_ENTRY_LIST_EXTENSION;
										l_entry->high_entry = 0;
										MMU_page_unmap(l_index + i, &t_index, 0);
										if ((t_index >> 12) != 0)
											mpt_return_page(t_index >> 12);
									}
									l_entry->low_entry = C_ENTRY_LIST_ENTRY;
									l_entry->high_entry = 0;
									new_index_return(chapter->index_list_head, processor_id, l_index, chapter_map_entry, chapter);
									alock_write_unlock(chapter->rwlock);
									return(PAGE_PHYSICAL_ALLOCATION_ERROR);
								}
								l_entry->low_entry = t_index + C_ENTRY_MAPPED;
								l_entry->high_entry = 0;
								MMU_page_map(l_index + i, t_index, 0);
							}
							if ((p_manager->page_flags & PAGE_INITIALIZE_DATA_BIT) != 0)
								SetDataD((uint32 *) (l_index << LOGICAL_PAGE_SHIFT), p_manager->page_initialization_value, (p_manager->page_size * 4096)/4);
							*logical_address = (uint32 *) (l_index << LOGICAL_PAGE_SHIFT);
						}
						else
						{
							for (i=0; i<p_manager->page_size; i++, l_entry ++)
							{
								l_entry->low_entry = C_ENTRY_ALLOCATED;
								l_entry->high_entry = 0;
							}
						}
						if ((p_manager->page_flags & PAGE_SUB_OWNER_BIT) != 0)
						{
							l_entry->low_entry = (owner_id >> 4) + C_ENTRY_SUB_OWNER_ID;
							l_entry->high_entry = 0;
						}

						for (i=0; i<((max_count + 1) / 2); i++)
						{
							if (new_index_alloc(chapter->index_list_head, max_processor_id, &l_index, &l_entry, chapter_map_entry, chapter) != 0)
								break;
							new_index_return(chapter->index_list_head, processor_id, l_index, chapter_map_entry, chapter);
						}
						alock_write_unlock(chapter->rwlock);
						return(0);
					}
					alock_write_unlock(chapter->rwlock);
				}

  //			}
			else
			{
				chapter = chapter->page_next_link;
			}
		} while (chapter != p_manager->chapter_list_head);
		do
		{
			for (i=0; i<32; i++)
			{
				if ((l_entry = chapter->page_index_table[i]) != 0)
				{
					for (j=0; j<32; j++)
					{
						if ((l_entry[j].low_entry & C_ENTRY_MASK) == C_ENTRY_LIST_ENTRY)
						{
							funny_retry = 1;
							funny_retry_counter ++;
							break;
						}
					}
					if (j != 32)
						break;
				}
			}
			if (i != 32)
				break;
*/
			chapter = chapter->page_next_link;
		} while (chapter != p_manager->chapter_list_head);
		do
		{
			if (chapter->base_mask != (LONG) -1)
			{
				retry_flag = 1;
				ccode = (chapter->chapter_flags & CHAPTER_SPECIAL_BIT) ? vm256_special_alloc((uint32 **) &table) : vm256_alloc((uint32 **) &table);
				if (ccode != 0)
				{
vmdbg				panic("error returned from vm256_alloc");
					set_flags(flags);
					return(-1);
				}
//				alock_write_lock(chapter->rwlock);
				cli();
				spin_lock((uint32 *) chapter->rwlock);

				for (i=0; i<32; i++)
				{
					if ((chapter->base_mask & (1 << i)) == 0)
						break;
				}
				if (i < 32)
				{
					chapter->page_count = (i+1) * 32;
					chapter->base_mask  |= 1 << i;
					chapter->page_index_table[i] = table;
					base = (chapter->chapter_index << 10) + (i * 32);
					SetDataD((uint32 *) table, C_ENTRY_OUT_OF_BOUNDS, 64);
					for (j=0; j<=32-p_manager->page_size; j+=p_manager->page_skip)
					{
						p_manager->total_page_count ++;
						for (k=1; k<p_manager->page_size; k++)
						{
							table[j + k].low_entry = C_ENTRY_LIST_EXTENSION;
							table[j + k].high_entry = 0;
						}
						table[j].low_entry = C_ENTRY_LIST_ENTRY;
						table[j].high_entry = 0;
						if ((i + j) != 0)
							new_index_return(chapter->index_list_head, 0, base + j, chapter_map_entry, chapter);
						else
						{
							table[j].low_entry = -1;
							table[j].high_entry = -1;
						}
					}
				}
				else
				{
					if (chapter->chapter_flags & CHAPTER_SPECIAL_BIT)
						vm256_special_return((uint32 *) table);
					else
						vm256_return((uint32 *) table);
				}
//				alock_write_unlock(chapter->rwlock);
				spin_unlock((uint32 *) chapter->rwlock);
			}
			chapter = chapter->page_next_link;
		} while (chapter != p_manager->chapter_list_head);
	}
	set_flags(flags);
	return(PAGE_LOGICAL_ALLOCATION_ERROR);
}

uint32	chapter_unmap_temp_counter_1 = 0;
uint32	chapter_unmap_temp_counter_2 = 0;
uint32	chapter_unmap_backoff_counter = 0;
void	chapter_unmap_queued_pages(
	page_node	*p_manager)
{
	uint32		i, j, k;
	uint32		c_index;
	uint32		l_index;
	uint32		list_count;
	uint32		*logical_address;
	uint32		*base_address;
        uint32		flags = get_flags_no_cli();
	quad_entry	*l_entry;
	chapter_node	*chapter;


	if ((chapter = p_manager->chapter_list_head) != 0)
	{
		do
		{
			new_list_list_count(chapter->special_unmap_list_head, &list_count);
			if (list_count > 0)
			{
//				alock_write_lock(chapter->rwlock);
				cli();
				spin_lock((uint32 *) chapter->rwlock);
				for (i=0; i<active_processor_count; i++)
				{
//					while (alock_read_check(chapter->rwlock, i) != 0)
//						chapter_unmap_backoff_counter ++;

					while (new_list_alloc(chapter->special_unmap_list_head, i, &logical_address) == 0)
					{
						logical_address -= 32;
						c_index = ((uint32) logical_address >> LOGICAL_PAGE_SHIFT) & 0x3FF;
vmdbg						if (chapter->page_index_table[c_index >> 5] == 0)
vmdbg							panic("integrity check in chaper_unmap_queued_pages 1");
						l_entry = &chapter->page_index_table[c_index >> 5][c_index & 0x1F];
						for (k=0; k<p_manager->page_size; k++, l_entry++)
						{
							if ((l_entry->low_entry & C_ENTRY_MASK) == C_ENTRY_MAPPED)
							{
								MMU_page_unmap((uint32)&logical_address[k * 1024] >> 12, &c_index, 0);
vmdbg								if ((c_index >> 12) == 0)
vmdbg									panic("integrity check in chaper_unmap_queued_pages 2");
								mpt_return_page(c_index >> 12);

							}
							if (k==0)
							{
								l_entry->low_entry = C_ENTRY_LIST_TEMPORARY;
								l_entry->high_entry = chapter->special_index_list_head->low_entry;
								chapter->special_index_list_head->low_entry = (uint32) logical_address >> LOGICAL_PAGE_SHIFT;
							}
							else
							{
								l_entry->low_entry = C_ENTRY_LIST_EXTENSION;
								l_entry->high_entry = 0;
							}
						}
						if (p_manager->page_flags & PAGE_SUB_OWNER_BIT)
						{
							l_entry->low_entry = C_ENTRY_OUT_OF_BOUNDS;
							l_entry->high_entry = 0;
						}
					}
				}
				cli();
				if (VMFlushTLB() != 0)
					panic("FlushTLB failure");
				sti();
				if (chapter->special_index_list_head->low_entry != 0)
				{
					while ((l_index = chapter->special_index_list_head->low_entry) != 0)
//			while (new_index_alloc(chapter->special_index_list_head, 0, &p_index, &p_entry, chapter_map_entry, chapter) == 0)
					{
						chapter_map_entry(&l_entry, l_index, chapter);
						chapter->special_index_list_head->low_entry = l_entry->high_entry;
						l_entry->low_entry = C_ENTRY_LIST_ENTRY;
						l_entry->high_entry = 0;
						chapter_unmap_temp_counter_2 ++;
						new_index_return(chapter->index_list_head, 0, l_index, chapter_map_entry, chapter);
					}
				}

//				alock_write_unlock(chapter->rwlock);
//				alock_write_lock(chapter->rwlock);
//				alock_write_unlock(chapter->rwlock);
				spin_unlock((uint32 *) chapter->rwlock);
				set_flags(flags);
			}
			chapter = chapter->page_next_link;
		} while (chapter != p_manager->chapter_list_head);
	}
}

uint32	chapter_queue_unmap_page(
	uint32		*logical_address,
	page_node	*p_manager)
{
	uint32		processor_id = 0;
        uint32		flags = get_flags_no_cli();
	chapter_node	*chapter;

	if (new_mat_lookup_ccb(&chapter, logical_address) == 0)
	{
vmdbg		if (chapter->page_link != p_manager)
vmdbg			panic("integrity check in chapter_queue_unmap_page");

		SetDataD(logical_address, 0x17853548, 64);
//		alock_read_lock(chapter->rwlock, &processor_id);
		cli();
		spin_lock((uint32 *) chapter->rwlock);
		new_list_return(chapter->special_unmap_list_head, processor_id, (uint32 *) &logical_address[32]);
//		alock_read_unlock(chapter->rwlock, processor_id);
		spin_unlock((uint32 *) chapter->rwlock);
		set_flags(flags);
		return(0);
	}
vmdbg	panic("invalid address passed to chapter_queue_unmap_page");
	set_flags(flags);
	return(-1);
}


uint32	chapter_index_return(
	uint32		*logical_address,
	page_node	*p_manager)
{
	uint32		k;
	uint32		c_index;
	uint32		processor_id = 0;
	uint32		flags = get_flags_no_cli();
	quad_entry	*l_entry;
	chapter_node	*chapter;

	if (new_mat_lookup_ccb(&chapter, logical_address) == 0)
	{
		c_index = ((uint32) logical_address >> LOGICAL_PAGE_SHIFT) & 0x3FF;
//		alock_read_lock(chapter->rwlock, &processor_id);
		cli();
		spin_lock((uint32 *) chapter->rwlock);
		if (chapter->page_index_table[c_index >> 5] != 0)
		{
			l_entry = &chapter->page_index_table[c_index >> 5][c_index & 0x1F];
			for (k=0; k<p_manager->page_size; k++, l_entry++)
			{
				if ((l_entry->low_entry & C_ENTRY_MASK) == C_ENTRY_MAPPED)
				{
					MMU_page_unmap((uint32)&logical_address[k * 1024] >> 12, &c_index, 0);
vmdbg					if ((c_index >> 12) == 0)
vmdbg						panic("integrity check in chapter_index_return");
					mpt_return_page(c_index >> 12);

				}
				l_entry->low_entry = (k==0) ? C_ENTRY_LIST_ENTRY : C_ENTRY_LIST_EXTENSION;
				l_entry->high_entry = 0;
			}
			if (p_manager->page_flags & PAGE_SUB_OWNER_BIT)
			{
				l_entry->low_entry = C_ENTRY_OUT_OF_BOUNDS;
				l_entry->high_entry = 0;
			}
			cli();
			if (VMFlushTLB() != 0)
				panic("TLB flush error");
			sti();
			new_index_return(chapter->index_list_head, processor_id, (uint32) logical_address >> LOGICAL_PAGE_SHIFT, chapter_map_entry, chapter);
//			alock_read_unlock(chapter->rwlock, processor_id);
			spin_unlock((uint32 *) chapter->rwlock);
			set_flags(flags);
			return(0);
		}
//		alock_read_unlock(chapter->rwlock, processor_id);
		spin_unlock((uint32 *) chapter->rwlock);
		set_flags(flags);
	}
vmdbg	panic("invalid address passed to chapter_return_index");
	set_flags(flags);
	return(-1);
}

void	chapter_set_owner_id(
	chapter_node	*chapter,
	uint32		*logical_address,
	uint32		owner_id)
{
	uint32	c_index;
	uint32	i;
//	uint32	processor_id = 0;
	uint32	flags = get_flags_no_cli();
	quad_entry	*l_entry;

//	alock_read_lock(chapter->rwlock, &processor_id);
	cli();
	spin_lock((uint32 *) chapter->rwlock);
	if ((chapter->page_link->page_flags & PAGE_PHYSICAL_EQUALS_LOGICAL_BIT) != 0)
	{
		c_index = (((uint32) logical_address >> LOGICAL_PAGE_SHIFT) & 0x3FF);
		l_entry = &chapter->page_index_table[c_index >> 5][c_index & 0x1F];
		for (i=0; i<chapter->page_link->page_size; i++, l_entry++)
			l_entry->high_entry = (owner_id >> 4) + C_ENTRY_SUB_OWNER_ID;
	}
	else
	{
		c_index = (((uint32) logical_address >> LOGICAL_PAGE_SHIFT) & 0x3FF) + chapter->page_link->page_size;
		l_entry = &chapter->page_index_table[c_index >> 5][c_index & 0x1F];
vmdbg		if ((l_entry->low_entry & C_ENTRY_MASK) != C_ENTRY_SUB_OWNER_ID)
vmdbg			panic("integrity check in chapter_set_owner_id");
		l_entry->low_entry = (owner_id >> 4) + C_ENTRY_SUB_OWNER_ID;
//	alock_read_unlock(chapter->rwlock, processor_id);
	}
	spin_unlock((uint32 *) chapter->rwlock);
	set_flags(flags);
}

LONG	chapter_get_owner_id(
	chapter_node	*chapter,
	uint32		*logical_address,
	uint32		*owner_id)
{
	uint32	index;
	uint32	c_index;
//	uint32	processor_id = 0;
	uint32	flags = get_flags_no_cli();
	quad_entry	*l_entry;

//	alock_read_lock(chapter->rwlock, &processor_id);
	cli();
	spin_lock((uint32 *) chapter->rwlock);

	if ((chapter->page_link->page_flags & PAGE_PHYSICAL_EQUALS_LOGICAL_BIT) != 0)
	{
		c_index = (((uint32) logical_address >> LOGICAL_PAGE_SHIFT) & 0x3FF);
		l_entry = &chapter->page_index_table[c_index >> 5][c_index & 0x1F];
		*owner_id = l_entry->high_entry << 4;
		spin_unlock((uint32 *) chapter->rwlock);
		set_flags(flags);
		return(0);
	}
	c_index = (((uint32) logical_address >> LOGICAL_PAGE_SHIFT) & 0x3FF);
	for (index = c_index & 0x1F; index < 32; index ++)
	{
		l_entry = &chapter->page_index_table[c_index >> 5][index];
		if ((l_entry->low_entry & C_ENTRY_MASK) == C_ENTRY_SUB_OWNER_ID)
		{
			*owner_id = l_entry->low_entry << 4;
//			alock_read_unlock(chapter->rwlock, processor_id);
			spin_unlock((uint32 *) chapter->rwlock);
			set_flags(flags);
			return(0);
		}
	}
//	alock_read_unlock(chapter->rwlock, processor_id);
	spin_unlock((uint32 *) chapter->rwlock);
	set_flags(flags);
	return(-1);
}

uint32	chapter_map_page(
	chapter_node	*chapter,
	uint32		*logical_address)
{
	uint32	i;
	uint32	c_index;
	uint32	ccode;
	uint32	page_count;
	uint32	t_index;
//	uint32	processor_id = 0;
	uint32	flags = get_flags_no_cli();
	quad_entry	*l_entry;

//	alock_read_lock(chapter->rwlock, &processor_id);
	cli();
	spin_lock((uint32 *) chapter->rwlock);

	c_index = ((uint32) logical_address >> LOGICAL_PAGE_SHIFT) & 0x3FF;
	page_count = chapter->page_count - c_index;
	if (page_count > 32)
		page_count = 32;
	if (chapter->page_index_table[c_index >> 5] == 0)
	{
		if ((chapter->chapter_flags & CHAPTER_NOT_SPARSE_BIT) != 0)
		{
			set_flags(flags);
			return(-1);
		}
		vm256_alloc((uint32 **) &l_entry);
		chapter->page_index_table[c_index >> 5] = l_entry;
		for (i=0; i<page_count; i++, l_entry++)
		{
			l_entry->low_entry = C_ENTRY_ALLOCATED;
			l_entry->high_entry = 0;
		}
		for (; i<32; i++, l_entry++)
		{
			l_entry->low_entry = C_ENTRY_OUT_OF_BOUNDS;
			l_entry->high_entry = 0;

		}
	}
	l_entry = &chapter->page_index_table[c_index >> 5][c_index & 0x1F];
	if ((l_entry->low_entry & C_ENTRY_MASK) != C_ENTRY_ALLOCATED)
	{
		set_flags(flags);
		return(-2);
	}
	if ((chapter->page_link->page_flags & PAGE_SYSTEM_MEMORY) != 0)
		ccode = mpt_system_alloc_page(&t_index, ((uint32) chapter >> 4) | MPT_NONMOVABLE_PAGE_BIT, (uint32) logical_address);
	else
		ccode = mpt_alloc_page(&t_index, ((uint32) chapter >> 4) | MPT_ASSIGNED_BIT, (uint32)logical_address);
	if (ccode != 0)
	{
//		alock_read_unlock(chapter->rwlock, processor_id);
		spin_unlock((uint32 *) chapter->rwlock);
		set_flags(flags);
//		mpt_return_page(t_index);
		return(-1);
	}
	l_entry->low_entry = C_ENTRY_MAPPED + t_index;
	l_entry->high_entry = 0;
	MMU_raw_page_map((uint32) logical_address >> LOGICAL_PAGE_SHIFT, t_index, 0);
//	alock_read_unlock(chapter->rwlock, processor_id);
	spin_unlock((uint32 *) chapter->rwlock);
	set_flags(flags);
	if ((chapter->page_link->page_flags | PAGE_INITIALIZE_DATA_BIT) != 0)
		SetDataD(logical_address, chapter->page_link->page_initialization_value, LOGICAL_PAGE_SIZE/4);
	return(0);
}

uint32	chapter_validate_page(
	chapter_node	*chapter,
	uint32		*logical_address)
{
	uint32	c_index;
	uint32	ccode;
//	uint32	processor_id = 0;
	uint32	flags;
	quad_entry	*l_entry;

//	alock_read_lock(chapter->rwlock, &processor_id);
//	flags = get_flags_no_cli();
//	cli();
//	spin_lock((uint32 *) chapter->rwlock);

	c_index = ((uint32) logical_address >> LOGICAL_PAGE_SHIFT) & 0x3FF;
	if (chapter->page_index_table[c_index >> 5] == 0)
	{
		return(-1);
	}
	l_entry = &chapter->page_index_table[c_index >> 5][c_index & 0x1F];
	if ((l_entry->low_entry & C_ENTRY_MASK) == C_ENTRY_ALLOCATED)
	{
		return(1);
	}
	if ((l_entry->low_entry & C_ENTRY_MASK) == C_ENTRY_MAPPED)
	{
		return(0);
	}
//	spin_unlock((uint32 *) chapter->rwlock);
//	set_flags(flags);
	return(-1);
}


void	ccb_init(void)
{
	generic_page_init(&vmpage_manager,
		&system_book,
		CCB_PAGES,
		PAGE_PHYSICAL_MAH | PAGE_ALLOC_IMMEDIATE_BIT | PAGE_SYSTEM_MEMORY | CHAPTER_PHYSICAL_MAH | CHAPTER_SPECIAL_BIT,
		0,
		MAT_PHYSICAL_ADDRESS,
		4, 		// page size
		4,	// page skip
		0);
	vm256_manager = (byte_node *) &vmpage[0];
	generic_byte_init(&vm256_manager,
		&vmpage_manager,
		VM256_BYTES,
		BYTE_PHYSICAL_MAH | BYTE_INITIALIZE_DATA_BIT | BYTE_DONT_ALLOC_BIT,
		256,		// byte size
		256,		// byte skip
		0);	// byte initialization value
	vm64_manager = (byte_node *) &vmpage[0x100];
	generic_byte_init(&vm64_manager,
		&vmpage_manager,
		VM64_BYTES,
		BYTE_PHYSICAL_MAH | BYTE_DONT_ALLOC_BIT,
		64,		// byte size
		64,		// byte skip
		0);	// byte initialization value
}




uint32	vm256_alloc(
	uint32	**new_chapter)
{
	uint32  *page_ptr;

//	chapter_index_expand(&vmpage_manager);
	while (1)
	{
		if (generic_byte_alloc(&page_ptr, vm256_manager) == 0)
		{
			*new_chapter = page_ptr;
			SetDataD(page_ptr, 0, 256 / 4);
			return(0);
		}
		else
		{
			if (generic_byte_expand_pages(vm256_manager, 0) != 0)
			{
				if (special_chapter_expand(vm256_manager->page_link) != 0)
				{
					return(-1);
				}
//				while (generic_byte_expand_indexs(vm256_manager) == 0);
			}
		}
	}
}


uint32  vm256_return(
	uint32  *page_ptr)
{
	generic_byte_return((uint32 *) page_ptr, vm256_manager);
	return(0);
}

uint32	vm256_special_alloc(
	uint32	**n_ptr)
{
	uint32  *page_ptr;
	if (generic_byte_alloc(&page_ptr, vm256_manager) == 0)
	{
		*n_ptr = page_ptr;
		return(0);
	}
	if (new_list_alloc((mah_node *) &vm256_special_list_head[0], 0, &page_ptr) == 0)
	{
		*n_ptr = page_ptr;
		SetDataD(page_ptr, 0, 256 / 4);
		return(0);
	}
	return(-1);
}

void  vm256_special_return(
	uint32  *n_ptr)
{

	new_list_return((mah_node *) &vm256_special_list_head[0], 0, n_ptr);
}

uint32	vm64_alloc(
	uint32	**new_chapter)
{
	uint32  *page_ptr;

	while (1)
	{
		if (generic_byte_alloc(&page_ptr, vm64_manager) == 0)
		{
			*new_chapter = page_ptr;
			return(0);
		}
		else
		{
			if (generic_byte_expand_pages(vm64_manager, 0) != 0)
			{
				if (special_chapter_expand(vm64_manager->page_link) != 0)
				{
					return(-1);
				}
			}
		}
	}
}


uint32  vm64_return(
	uint32  *page_ptr)
{
	generic_byte_return((uint32 *) page_ptr, vm64_manager);
	return(0);
}

