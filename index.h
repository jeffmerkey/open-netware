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
*   FILE     :  INDEX.H
*   DESCRIP  :  Multi-Processing Memory Manager for MANOS v1.0
*   DATE     :  January 26, 1998
*
*
***************************************************************************/


/********************************** header files *******************************/


/********************************** #defines *******************************/
#define	GENERIC_INDEX_ALLOCATED_BITS	0x0000000F
#define GENERIC_INDEX_MASK		0x000000F0

/********************************** typedefs *******************************/
typedef	struct	list_node_def
{
	struct	list_node_def	*next;
	struct	list_node_def	*last;
	uint32			*list_head;
	uint32			free_flag;
} list_node;

#define	NODE_FREE_STAMP		0x45455246
void	new_index_list_count(
	mah_node	*list_head,
	uint32		*list_count);

uint32	new_index_alloc(
	mah_node	*list_head,
	uint32		processor_id,
	uint32		*new_index,
	quad_entry	**new_entry,
	void		(*map_entry_routine)(),
	void		*map_entry_parameter);

uint32	new_index_alloc_specific(
	mah_node	*list_head,
	uint32		p_index,
	quad_entry	**new_entry,
	void		(*map_entry_routine)(),
	void		*map_entry_parameter);

uint32  new_index_return(
	mah_node	*list_head,
	uint32		processor_id,
	uint32		p_index,
	void		(*map_entry_routine)(),
	void		*map_entry_parameter);

void	new_list_list_count(
	mah_node	*list_head,
	uint32		*list_count);

uint32	new_list_alloc(
	mah_node	*list_head,
	uint32		processor_id,
	uint32		**n_ptr);

uint32	new_list_alloc_specific(
	uint32		*n_ptr);

void  new_list_return(
	mah_node	*list_head,
	uint32		processor_id,
	uint32		*n_ptr);

