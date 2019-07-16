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
*   FILE     :  MAT.H
*   DESCRIP  :  Multi-Processing Memory Manager for MANOS v1.0
*   DATE     :  Febuary 17, 1998
*
*
***************************************************************************/

/********************************** header files ************************************/

/********************************** #defines ****************************************/
#define	MAT_ADDRESS_INUSE	0xCCDDCC00
#define	MAT_ADDRESS_FREE	0xDDFFDD00

#define	MAT_VIRTUAL_ADDRESS		0
#define MAT_PHYSICAL_ADDRESS		16

#define	MAT_INVALID_INDEX		-3
#define	MAT_INVALID_CHAPTER		-4
#define	MAT_RESERVED_CHAPTER	-5
/********************************** typedefs*****************************************/

void	LAS_get_current(
	book_node	**new_book);

void	mat_init(void);

uint32	mat_alloc(
	book_node	*book,
	chapter_node	*ccb,
	uint32		**address,
	uint32		chapter_count);

uint32	mat_return(
	book_node	*book,
	chapter_node	*ccb,
	uint32		*address,
	uint32		chapter_count);

uint32	new_mat_lookup_ccb(
	chapter_node	**new_ccb,
	uint32		*address);

uint32	mat_validate_ccb(
	chapter_node	**new_chapter,
	uint32		*address);

uint32	mat_set_ccb(
	book_node	*book,
	chapter_node	*new_ccb,
	uint32		*address);


