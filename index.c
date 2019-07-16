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
*   FILE     :  INDEX.C
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

/********************************** function declarations ***************************/

/***************************************************************************
*
*   FUNCTION	 :  new_index_alloc(
*				       uint32	*list_head;
*					uint32	*new_index
*					void	(*map_entry_routine)(),
*					void	*map_entry_parameter)
*
*	USAGE		 :	map a page in the chapter manager
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/

void	new_index_list_count(
	mah_node	*list_head,
	uint32		*list_count)
{
	uint32	i;
	*list_count = 0;

	for (i=0; i<active_processor_count; i++)
	{
		*list_count += list_head[i * QUADS_PER_PAGE].high_entry;
	}
}

/***************************************************************************
*
*   FUNCTION	 :  new_index_alloc(
*				       uint32	*list_head;
*					uint32	*new_index
*					void	(*map_entry_routine)(),
*					void	*map_entry_parameter)
*
*	USAGE		 :	map a page in the chapter manager
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/

uint32	new_index_alloc(
	mah_node	*list_head,
	uint32		processor_id,
	uint32		*new_index,
	quad_entry	**new_entry,
	void		(*map_entry_routine)(quad_entry **entry, uint32 index, void *parameter),
	void		*map_entry_parameter)
{
	uint32	p_index;
	uint32	n_index;
	uint32	l_index;
	quad_entry	*p_entry;
	quad_entry	*n_entry;
	quad_entry	*l_entry;

	if ((p_index = list_head[processor_id * QUADS_PER_PAGE].low_entry) != 0)
	{
		map_entry_routine(&p_entry, p_index, map_entry_parameter);
//		if ((p_entry->low_entry & 0xF0000000) != 0)
//			BREAK();
vmdbg		if (((p_entry->high_entry & 0xF0000000) >> 28) != processor_id)
vmdbg			panic("integrity check error in new_index_alloc 1");
		n_index = p_entry->low_entry & 0x0FFFFFFF;
		if (n_index != p_index)
		{
			l_index = p_entry->high_entry & 0x0FFFFFFF;
			map_entry_routine(&l_entry, l_index, map_entry_parameter);
//			if ((l_entry->low_entry & 0xF0000000) != 0)
//				BREAK();
vmdbg			if (((l_entry->high_entry & 0xF0000000) >> 28) != processor_id)
vmdbg				panic("integrity check error in new_index_alloc 2");

			map_entry_routine(&n_entry, n_index, map_entry_parameter);
//			if ((n_entry->low_entry & 0xF0000000) != 0)
//				BREAK();
vmdbg			if (((n_entry->high_entry & 0xF0000000) >> 28) != processor_id)
vmdbg				panic("integrity check error in new_index_alloc 3");

			n_entry->high_entry = l_index + (processor_id << 28);
			l_entry->low_entry = (l_entry->low_entry & 0xF0000000) + n_index;
			list_head[processor_id * QUADS_PER_PAGE].low_entry = n_index;
			list_head[processor_id * QUADS_PER_PAGE].high_entry --;
		}
		else
		{
			list_head[processor_id * QUADS_PER_PAGE].low_entry = 0;
			list_head[processor_id * QUADS_PER_PAGE].high_entry = 0;
		}
		if (new_entry != 0)
			*new_entry = p_entry;
		*new_index = p_index;
		return(0);
	}
	return(-1);
}

/***************************************************************************
*
*   FUNCTION	 :  new_index_alloc_specific(
*				       uint32	*list_head;
*					uint32	new_index
*					void	(*map_entry_routine)(),
*					void	*map_entry_parameter)
*
*	USAGE		 :	map a page in the chapter manager
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/

uint32	new_index_alloc_specific(
	mah_node	*list_head,
	uint32		p_index,
	quad_entry	**new_entry,
	void		(*map_entry_routine)(quad_entry **entry, uint32 index, void *parameter),
	void		*map_entry_parameter)
{
	uint32	n_index;
	uint32	l_index;
	quad_entry	*p_entry;
	quad_entry	*n_entry;
	quad_entry	*l_entry;
	uint32	processor_id;

	map_entry_routine(&p_entry, p_index, map_entry_parameter);

//	if ((p_entry->low_entry & 0xF0000000) != 0)
//		BREAK();

	processor_id = (p_entry->high_entry & 0xF0000000) >> 28;
	n_index = p_entry->low_entry & 0x0FFFFFFF;
	if (n_index != p_index)
	{
		l_index = p_entry->high_entry & 0x0FFFFFFF;
		map_entry_routine(&l_entry, l_index, map_entry_parameter);
//		if ((l_entry->low_entry & 0xF0000000) != 0)
//			BREAK();
vmdbg		if (((l_entry->high_entry & 0xF0000000) >> 28) != processor_id)
vmdbg			panic("integrity check in new_index_alloc_specific 1");

		map_entry_routine(&n_entry, n_index, map_entry_parameter);
//		if ((n_entry->low_entry & 0xF0000000) != 0)
//			BREAK();
vmdbg		if (((n_entry->high_entry & 0xF0000000) >> 28) != processor_id)
vmdbg			panic("integrity check in new_index_alloc_specific 2");

		n_entry->high_entry = l_index + (processor_id << 28);
		l_entry->low_entry = (l_entry->low_entry & 0xF0000000) + n_index;
		list_head[processor_id * QUADS_PER_PAGE].low_entry = n_index;
		list_head[processor_id * QUADS_PER_PAGE].high_entry --;
	}
	else
	{
		list_head[processor_id * QUADS_PER_PAGE].low_entry = 0;
		list_head[processor_id * QUADS_PER_PAGE].high_entry = 0;
	}
	*new_entry = p_entry;
	return(0);
}


/***************************************************************************
*
*   FUNCTION	 :  new_index_return(
*				       uint32	*list_head;
*					uint32	*new_index
*					void	(*map_entry_routine)(),
*					void	*map_entry_parameter)
*
*	USAGE		 :	return a page in the chapter manager
*
*	RETURN VALUE :	0 - SUCCESS
*				   -2 - ERROR
*
***************************************************************************/
uint32  new_index_return(
	mah_node	*list_head,
	uint32		processor_id,
	uint32		p_index,
	void		(*map_entry_routine)(quad_entry **entry, uint32 index, void *parameter),
	void		*map_entry_parameter)
{
	uint32	n_index;
	uint32	l_index;
	quad_entry	*p_entry;
	quad_entry	*l_entry;
	quad_entry	*n_entry;

	map_entry_routine(&p_entry, p_index, map_entry_parameter);

	if ((n_index = list_head[processor_id * QUADS_PER_PAGE].low_entry ) == 0)
	{
		p_entry->low_entry = p_index;
		p_entry->high_entry = p_index + (processor_id << 28);
		list_head[processor_id * QUADS_PER_PAGE].low_entry = p_index;
	}
	else
	{
		map_entry_routine(&n_entry, n_index, map_entry_parameter);
//		if ((n_entry->low_entry & 0xF0000000) != 0)
//			BREAK();
vmdbg		if (((n_entry->high_entry & 0xF0000000) >> 28) != processor_id)
vmdbg			panic("integrity check in new_index_return 1");

		l_index = n_entry->high_entry & 0x0FFFFFFF;
		map_entry_routine(&l_entry, l_index, map_entry_parameter);
//		if ((l_entry->low_entry & 0xF0000000) != 0)
//			BREAK();
vmdbg		if (((l_entry->high_entry & 0xF0000000) >> 28) != processor_id)
vmdbg			panic("integrity check in new_index_return 2");
		p_entry->low_entry = n_index;
		p_entry->high_entry = l_index + (processor_id << 28);
		n_entry->high_entry = p_index + (processor_id << 28);
		l_entry->low_entry = (l_entry->low_entry & 0xF0000000) + p_index;
	}
	list_head[processor_id * QUADS_PER_PAGE].high_entry ++;
	return(0);
}
/***************************************************************************
*
*   FUNCTION	 :  new_list_alloc(
*				       uint32	*list_head
*					uint32	*new_index)
*
*	USAGE		 :	map a page in the chapter manager
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/


void	new_list_list_count(
	mah_node	*list_head,
	uint32		*list_count)
{
	uint32	i;
	*list_count = 0;

	for (i=0; i<active_processor_count; i++)
	{
		*list_count += list_head[i * QUADS_PER_PAGE].high_entry;
	}
}

/***************************************************************************
*
*   FUNCTION	 :  new_list_alloc(
*				       uint32	*list_head
*					uint32	*new_index)
*
*	USAGE		 :	map a page in the chapter manager
*
*	RETURN VALUE :	0 - SUCCESS
*				   -1 - FAILURE
*
***************************************************************************/

uint32	new_list_alloc(
	mah_node	*list_head,
	uint32		processor_id,
	uint32		**n_ptr)
{
	list_node	*node;

	if ((node = (list_node *) list_head[processor_id * QUADS_PER_PAGE].low_entry) != 0)
	{
		if (node->next == node)
		{
			list_head[processor_id * QUADS_PER_PAGE].low_entry = 0;
		}
		else
		{
			list_head[processor_id * QUADS_PER_PAGE].low_entry = (uint32) node->next;
			node->next->last = node->last;
			node->last->next = node->next;
		}
vmdbg		if (node->free_flag != NODE_FREE_STAMP)
vmdbg			panic("integrity check in new_list_alloc 1");
vmdbg		if (node->list_head != &list_head[processor_id * QUADS_PER_PAGE].low_entry)
vmdbg			panic("integrity check in new_list_alloc 2");
		node->free_flag = 0;
		node->list_head = 0;
		list_head[processor_id * QUADS_PER_PAGE].high_entry --;
		*n_ptr = (uint32 *) node;
		return(0);
	}
	return(-1);
}


uint32	new_list_alloc_specific(
	uint32		*n_ptr)
{
	list_node	*node;
	quad_entry 	*list_head;

	node = (list_node *) n_ptr;
vmdbg	if (node->free_flag != NODE_FREE_STAMP)
vmdbg		panic("integrity check in new_list_alloc_specific 1");

	list_head = (quad_entry *) node->list_head;
	if (list_head[0].low_entry == (uint32) node)
	{
		list_head[0].low_entry = (node->next == node) ? 0 : (uint32) node->next;
	}
	node->next->last = node->last;
	node->last->next = node->next;
	list_head[0].high_entry --;
	node->free_flag = 0;
	node->list_head = 0;
	return(-1);
}

/***************************************************************************
*
*   FUNCTION	 :  new_list_return(
*				       uint32	*list_head;
*					uint32	*new_index
*					void	(*map_entry_routine)(),
*					void	*map_entry_parameter)
*
*	USAGE		 :	return a page in the chapter manager
*
*	RETURN VALUE :	0 - SUCCESS
*				   -2 - ERROR
*
***************************************************************************/
void  new_list_return(
	mah_node	*list_head,
	uint32		processor_id,
	uint32		*n_ptr)
{
	list_node	*node;
	list_node	*head;

	node = (list_node *) n_ptr;
	node->free_flag = NODE_FREE_STAMP;
	node->list_head = &list_head[processor_id * QUADS_PER_PAGE].low_entry;
	if ((head = (list_node *) list_head[processor_id * QUADS_PER_PAGE].low_entry) == 0)
	{
		node->next = node;
		node->last = node;
		list_head[processor_id * QUADS_PER_PAGE].low_entry = (uint32) node;
	}
	else
	{
		node->last = head->last;
		node->next = head;
		head->last->next = node;
		head->last = node;
	}
	list_head[processor_id * QUADS_PER_PAGE].high_entry ++;
}
