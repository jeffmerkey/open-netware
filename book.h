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
*   FILE     :  BOOK.H
*   DESCRIP  :  Multi-Processing Memory Manager for MANOS v1.0
*   DATE     :  January 26, 1998
*
*
***************************************************************************/

typedef	struct	book_node_def
{
	struct	page_node_def		*page_list_head;
	struct	chapter_node_def	*chapter_list_head;
	uint32	filler[2];

	uint32	book_mutex;
	uint32	address_space_type;
	uint32	chapter_hash_size;
	uint32	collecting_flag;
	uint32	collection_counter;
	uint32	mat_zones[ZONES_PER_ADDRESS_SPACE];
	uint32	*mat_table;		// pointer to 4K address mapping table
	struct	chapter_node_def	*chapter_hash_table[256];
	uint32	*malloc_table;
	uint32	*kmalloc_table;

} book_node;

extern	book_node	system_book;