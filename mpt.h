/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*                           Reserved.
*
*   AUTHOR   :  Darren Major & Merrill Teemant
*   FILE     :  MPT.H
*   DESCRIP  :  Multi-Processing Memory Manager for MANOS v1.0
*   DATE     :  January 26, 1998
*
*
***************************************************************************/

/********************************** header files ************************************/

/********************************** #defines ****************************************/


#define	MPT_ASSIGNED_BIT			0x80000000
#define	MPT_SYSTEM_PAGE_BIT			0x40000000
#define MPT_NONMOVABLE_PAGE_BIT		0x20000000

#define	MPT_RESERVED_PAGE		0x00525048 | MPT_NONMOVABLE_PAGE_BIT
//#define MPT_PAGEABLE_PAGE		0x50504147
//#define MPT_MOVEABLE_PAGE		0x4D504147
//#define MPT_NON_PAGEABLE_PAGE		0x4E504147
#define MPT_PTE_PAGE			0x00455550 | MPT_NONMOVABLE_PAGE_BIT
#define MPT_CCB_PAGE			0x00424343 | MPT_NONMOVABLE_PAGE_BIT
#define MPT_PHYSICAL_BLOCK		0x00594850 | MPT_NONMOVABLE_PAGE_BIT
#define MPT_MOVING_PAGE			0x0042594A | MPT_NONMOVABLE_PAGE_BIT



#define	MPT_OUT_OF_MEMORY		-2
#define	MPT_BUSY			-1


/********************************** typedefs*****************************************/

/********************************** global declarations ***************************/

//extern	uint32  *master_page_table[128];
extern	quad_entry  *master_page_table;
extern	uint32	mpt_total_counter;

extern	uint32	mpt_total_pages;
extern	uint32  mpt_reserved_pages;
extern	uint32  mpt_physical_pages;
extern	uint32  mpt_logical_pages;
extern	uint32  mpt_special_pages;
extern	uint32  mpt_available_pages;

/********************************** function declarations ***************************/
void	mpt_init(
	uint32	beginning_page_index,
	uint32	ending_page_index);

void    mpt_post_init(void);

void	mpt_wait_for_page(
	uint32	fault_address,
	uint32	thread_id);

void  mpt_map_entry(
	quad_entry   **entry,
	uint32          p_index);

//
//	NEW MASTER PAGE TABLE APIS
//

LONG	mpt_alloc_page(
	LONG	*ppn,
	uint32		mpt_entry_low,
	uint32		mpt_entry_high);

LONG	mpt_return_page(
	LONG	ppn);

LONG	mpt_system_alloc_page(
	LONG	*ppn,
	uint32		mpt_entry_low,
	uint32		mpt_entry_high);

LONG	mpt_system_return_page(
	LONG	ppn);

LONG	mpt_alloc_physical_block(
	LONG	*pbn);

LONG	mpt_return_physical_block(
	LONG	pbn);

uint32	mpt_consolidate_physical_blocks(
	uint32	next_block);
