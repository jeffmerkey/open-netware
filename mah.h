/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*                           Reserved.
*
*   AUTHOR   :  Darren Major & Merrill Teemant
*   FILE     :  MAH.H
*   DESCRIP  :  Multi-Processing Memory Manager for MANOS v1.0
*   DATE     :  January 26, 1998
*
*
***************************************************************************/

/********************************** header files ************************************/


/********************************** #defines ****************************************/
#define		MAH_INDEXES_PER_CHAPTER_SHIFT	17
#define		MAH_INDEXES_PER_CHAPTER_MASK	0x0001FFFF
#define		MAH_INDEXES_PER_CHAPTER			0x00020000
#define		MASTER_AFFINITY_HANDLE			0xDEEDDEED

#define		MAH_CHAPTER_TYPE		0x4D414500
/********************************** typedefs*****************************************/



typedef	struct	mah_byte_node_def
{
	struct	mah_byte_node_def	*next_link;
	struct	mah_byte_node_def	*last_link;
	struct	book_node_def	*book_link;
	struct	page_node_def	*page_link;

	uint32	    	mah_stamp;
	uint32	    	mah_type;
	uint32		mah_mutex;
	uint32		mah_extension_flag;

	uint32		mah_size;	// number of contiguous 4k pages
	uint32		mah_skip;	// used to create holes in memory by skipping logical addresses
	uint32		mah_flags;
	uint32		mah_initialization_value;

	uint32					mah_rwlock_mah;
	mah_node				*mah_rwlock;
	uint32					mah_list_head_mah;
	mah_node				*mah_list_head;

	uint32					mah_total_stats_mah;
	mah_node				*mah_total_stats_ptr;
	uint32					mah_temp_stats_mah;
	mah_node				*mah_temp_stats_ptr;

	uint32		indexes_per_block;
	uint32		blocks_per_chapter;
	uint32		block_size;



} mah_byte_node;



/********************************** function declarations ***************************/
void    mah_init(void);
void	mah_post_init(void);

void	mah_expand(void);

void mah_map_entry(
	quad_entry	**h_entry,
	uint32          h_index,
	mah_byte_node	*m_manager);

uint32  mah_return(
	uint32  h_index);

uint32  mah_alloc(
	uint32  *handle,
	mah_node	 **ptr);

uint32  pmah_alloc(
	uint32  *new_mah_index,
	mah_node	  **new_mah_ptr);
