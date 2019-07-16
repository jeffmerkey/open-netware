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
*   FILE     :  PAGE.H
*   DESCRIP  :  Multi-Processing Memory Manager for MANOS v1.0
*   DATE     :  January 26, 1998
*
*
***************************************************************************/


/********************************** header files *******************************/


/********************************** #defines *******************************/

#define	PAGE_ALLOC_LATER		0
#define	PAGE_ALLOC_TOP			1
#define PAGE_ALLOC_BOTTOM		2
#define PAGE_ALLOC_ALL			3


#define	PAGE_PHYSICAL_EQUALS_LOGICAL_BIT	0x00000001
#define PAGE_ALLOC_IMMEDIATE_BIT		0x00000002
#define	PAGE_INITIALIZE_DATA_BIT		0x00000004
#define PAGE_PHYSICAL_MAH			0x00000008
#define PAGE_SYSTEM_MEMORY			0x00000010
#define	PAGE_SUB_OWNER_BIT			0x00000020

#define	PAGE_PHYSICAL_ALLOCATION_ERROR		0x44444444
#define PAGE_LOGICAL_ALLOCATION_ERROR		0x55555555

/********************************** typedefs *******************************/


typedef	struct	page_node_def
{
	struct	page_node_def	*next_link;
	struct	page_node_def	*last_link;
	struct	book_node_def	*book_link;
	struct	byte_node_def	*byte_link;

	uint32		page_stamp;
	uint32		page_type;
	uint32		page_mutex;
	uint32		page_extension_flag;

	uint32		page_size;	// number of contiguous 4k pages
	uint32		page_skip;	// used to create holes in memory by skipping logical addresses
	uint32		page_flags;
	uint32		page_initialization_value;

	uint32		page_total_stats_mah;
	mah_node	*page_total_stats_ptr;
	uint32		page_temp_stats_mah;
	mah_node	*page_temp_stats_ptr;

	uint32		page_rwlock_mah;
	mah_node	*page_rwlock;
	uint32		unmap_rwlock_mah;
	mah_node	*unmap_rwlock;

	uint32		page_list_head_mah;
	mah_node	*page_list_head;


	uint32		mpt_page_type;
	uint32		mat_page_type;

	uint32		total_page_count;
	uint32		pages_per_chapter;
	uint32		chapter_logical_index;
	chapter_node	*chapter_list_head;
} page_node;



extern	uint32	LARGE_PAGE_SIZE;
extern uint32	LARGE_PAGE_MASK;

void	generic_page_init(
	page_node	*p_manager,
	book_node	*book,
	uint32	page_type,
	uint32	page_flags,
	uint32	mpt_page_type,
	uint32	mat_page_type,
	uint32	page_size,
	uint32	page_skip,
	uint32	page_initialization_value);

uint32  generic_page_free(
	page_node	*p_manager,
	uint32		all_flag);

uint32  generic_page_return(
	uint32		p_index,
	page_node	*p_manager);


uint32  generic_page_alloc(
	uint32		**page_ptr,
	page_node	*p_manager,
	uint32		owner_id);
