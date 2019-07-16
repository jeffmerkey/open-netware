/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*                           Reserved.
*
*   AUTHOR   :  Darren Major & Merrill Teemant
*   FILE     :  CHAPTER.H
*   DESCRIP  :  Multi-Processing Memory Manager for MANOS v1.0
*   DATE     :  January 26, 1998
*
*
***************************************************************************/

/********************************** header files ************************************/

/********************************** #defines ****************************************/
#define PHYSICAL_PAGES		0x47415000
#define HUGE_PAGES		0x47415048
#define	CCB_PAGES		0x50424343
#define	CCB_BYTES		0x42424343

#define	VM256_BYTES		0x564D3200
#define VM64_BYTES		0x564D3600

#define	CHAPTER_MANAGER_STAMP		0x52474d43
#define	CHAPTER_CONTROL_BLOCK_STAMP	0x53424343
#define	DONT_INITIALIZE_VALUE	0x37523129

#define	VMNP_FLAG_BIT		0x00000002
#define	PTE_FLAG_BIT		0x00000001
#define CCB_FLAG_BIT		0x00000004

#define	RESERVED_CCB		0x42434352
#define ALLOCATED_STAMP		0x41424344
#define UNMAP_STAMP		0x45464748

#define	CHAPTER_PHYSICAL_EQUALS_LOGICAL_BIT	0x00010000
#define CHAPTER_ALLOC_IMMEDIATE_BIT		0x00020000
#define	CHAPTER_INITIALIZE_DATA_BIT		0x00040000
#define CHAPTER_PHYSICAL_MAH			0x00080000
#define CHAPTER_SYSTEM_MEMORY			0x00100000
#define CHAPTER_NOT_SPARSE_BIT			0x00200000
#define CHAPTER_EXTENSION_GROUP_BIT		0x00400000
#define CHAPTER_EXTENSION_BIT			0x00800000
#define CHAPTER_REDUCTION_BIT			0x01000000
#define	CHAPTER_SPECIAL_BIT			0x02000000

#define C_ENTRY_MASK			0xF0000000
#define C_ENTRY_UNMASK                  0x0FFFFFFF

#define	C_ENTRY_LIST_ENTRY		0x00000000
#define	C_ENTRY_LIST_EXTENSION		0x10000000
#define	C_ENTRY_OUT_OF_BOUNDS		0x20000000
#define	C_ENTRY_SUB_OWNER_ID		0x30000000
#define	C_ENTRY_ALLOCATED		0x40000000
#define	C_ENTRY_MAPPED			0x50000000
#define	C_ENTRY_LIST_TEMPORARY		0x60000000


/********************************** typedefs*****************************************/
typedef struct  chapter_node_def
{
	struct	chapter_node_def	*book_next_link;
	struct	chapter_node_def	*book_last_link;
	struct	chapter_node_def	*page_next_link;
	struct	chapter_node_def	*page_last_link;

	struct	book_node_def		*book_link;
	struct	page_node_def		*page_link;
	uint32		chapter_flags;
	uint32		chapter_initialization_value;


	uint32		chapter_stamp;
	uint32		chapter_index;
	uint32		chapter_type;
	uint32		chapter_logical_index;

	uint32          *base_address;
	uint32		base_mask;
	uint32		*pte;
	uint32		chapter_count;

	mah_node	*rwlock;
	uint32		rwlock_mah;
	mah_node	*index_list_head;
	uint32		index_list_head_mah;

	uint32		special_unmap_list_head_mah;
	mah_node	*special_unmap_list_head;


	uint32		special_index_list_head_mah;
	mah_node	*special_index_list_head;

	uint32		page_count;
	uint32		filler[32 - 25];
	quad_entry	*page_index_table[32];
} chapter_node;



/********************************** function declarations ***************************/


void  chapter_map_entry(
	quad_entry	**entry,
	uint32          p_index,
	chapter_node	*chapter);

uint32  generic_chapter_return(
	chapter_node   *ccb);

uint32  generic_chapter_alloc(
	uint32			**page_ptr,
	uint32			page_count,
	uint32			chapter_flags);

uint32    chapter_index_expand(
	struct page_node_def	*p_manager);

uint32    chapter_expand(
	struct page_node_def	*p_manager);

uint32    special_chapter_expand(
	struct page_node_def	*p_manager);

uint32	chapter_create(
	chapter_node	**new_chapter,
	struct page_node_def	*p_manager,
	uint32		chapter_flags,
	uint32		page_count,
	uint32		logical_base);

void	chapter_destroy(
	chapter_node	*new_chapter);

void	chapter_unmap_queued_pages(
	struct	page_node_def	*p_manager);

uint32	chapter_queue_unmap_page(
	uint32		*logical_address,
	struct	page_node_def	*p_manager);

uint32	chapter_index_alloc(
	uint32		**logical_address,
	struct	page_node_def	*p_manager,
	uint32		owner_id);

uint32	chapter_index_return(
	uint32		*logical_address,
	struct page_node_def	*p_manager);

void	chapter_set_owner_id(
	chapter_node	*chapter,
	uint32		*logical_address,
	uint32		owner_id);

uint32	chapter_get_owner_id(
	chapter_node	*chapter,
	uint32		*logical_address,
	uint32		*owner_id);

uint32	chapter_map_page(
	chapter_node	*chapter,
	uint32		*logical_address);

uint32	chapter_validate_page(
	chapter_node	*chapter,
	uint32		*logical_address);


void	ccb_init(void);

uint32	vm256_alloc(
	uint32	**new_chapter);

uint32  vm256_return(
	uint32  *page_ptr);

uint32	vm256_special_alloc(
	uint32	**n_ptr);

void  vm256_special_return(
	uint32  *n_ptr);

uint32	vm64_alloc(
	uint32	**new_chapter);

uint32  vm64_return(
	uint32  *page_ptr);

