/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*                           Reserved.
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
#define uint32  unsigned long
#define uint8   unsigned char
#define LONG    uint32

#define	BYTE_FREE_STAMP		0x46465242


#define FOUR_K_SHIFT                    12
#define FOUR_K_MASK                     0x00000FFF

#define MASTER_PAGE_SHIFT               17
#define MASTER_PAGE_MASK                0x0001FFFF

#define ONE_MEG_SHIFT					20
#define ONE_MEG_MASK					0x000FFFFF
#define ONE_MEG_PAGE                    0x00100000

#define TWO_MEG_SHIFT                   21
#define TWO_MEG_MASK                    0x001FFFFF
#define TWO_MEG_PAGE                    0x00200000

#define ENTRIES_PER_PAGE				(4096 / 8)
#define FREE_ENTRY                      0x11000000
#define ALLOCATED_ENTRY					0x22000000

#define RESERVED_CHAPTER				0xAAAAAAAA
#define AVAILABLE_CHAPTER				0xBBBBBBBB
#define IN_USE_CHAPTER					0xCCCCCCCC
#define NO_CHAPTER						0xDDDDDDDD

#define POOL_IDLE			0xABABABAB
#define POOL_BUSY			0xCDCDCDCD
#define BUSY_FLAG			0xDDDDDDDE

#define VM_NONpageable_PAGES            0x11111111
#define VM_pageable_PAGES               0x22222222

#define ERROR_RETURN(ret_value)         ret_value
#define SUCCESS_RETURN(ret_value)       0

#define	 BYTE_INITIALIZE_DATA_BIT	0x00000001
#define	BYTE_PHYSICAL_MAH		0x00000008
#define	BYTE_DONT_ALLOC_BIT		0x00000080

/********************************** typedefs *******************************/

typedef	struct	byte_link_def
{
	uint32	byte_stamp;
	uint32	processor_id;
	struct	byte_link_def	*next_link;
	struct	byte_link_def	*last_link;
} byte_link;

typedef	struct	byte_node_def
{
	struct	byte_node_def	*next_link;
	struct	byte_node_def	*last_link;
	struct	book_node_def	*book_link;
	struct	page_node_def	*page_link;

	uint32	    	byte_stamp;
	uint32	    	byte_type;
	uint32		byte_mutex;
	uint32		byte_extension_flag;

	uint32		byte_size;	// number of contiguous 4k pages
	uint32		byte_skip;	// used to create holes in memory by skipping logical addresses
	uint32		byte_flags;
	uint32		byte_initialization_value;

	uint32					byte_rwlock_mah;
	mah_node				*byte_rwlock;
	uint32					byte_list_head_mah;
	mah_node				*byte_list_head;

	uint32					byte_total_stats_mah;
	mah_node				*byte_total_stats_ptr;
	uint32					byte_temp_stats_mah;
	mah_node				*byte_temp_stats_ptr;

	uint32					byte_aside_list_head_mah;
	mah_node				*byte_aside_list_head;
	uint32					byte_index_moh;
	mah_node				*byte_index_ptr;

	uint32			byte_page_reduction_flag;
} byte_node;



/********************************** function declarations *******************************/
void	generic_byte_init(
	byte_node	**b_manager,
	page_node	*p_manager,
	uint32		byte_type,
	uint32		byte_flags,
	uint32		byte_size,
	uint32		byte_skip,
	uint32		byte_initializatio_value);

uint32    generic_byte_expand_pages(
	byte_node	*b_manager,
	uint32		blocking_flag);

//uint32    generic_byte_expand_indexs(
//	byte_node	*b_manager);

uint32    generic_byte_expand_chapters(
	byte_node	*b_manager,
	uint32		blocking_flag);

uint32  generic_byte_alloc(
	uint32		**byte_ptr,
	byte_node	*b_manager);

uint32  generic_byte_cross_processor_alloc(
	byte_node	*b_manager,
	uint32		processor_id,
	uint32		**byte_ptr);

uint32  generic_byte_return(
	uint32		*b_ptr,
	byte_node	*b_manager);




