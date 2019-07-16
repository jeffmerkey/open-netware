/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*                           Reserved.
*
*
*   AUTHOR   :  Darren Major & Merrill Teemant
*   FILE     :  MALLOC.H
*   DESCRIP  :  Multi-Processing Memory Manager for MANOS v1.0
*   DATE     :  January 26, 1998
*
*
***************************************************************************/

/********************************** header files ************************************/

/********************************** #defines ****************************************/
// chapter types

#define MALLOC_PAGES	  	0x4C414D00
#define MALLOC_BYTES	  	0x42414D00
#define PMALLOC_PAGES	  	0x4C415000
#define PMALLOC_BYTES	  	0x42415000
#define STACK_PAGES		0x4C415300

#define	MALLOC_BYTE_INIT	0xEDDEE222
#define MALLOC_PAGE_INIT	0x33443234

/********************************** typedefs*****************************************/

/********************************** globals *****************************************/


/********************************** function declarations ***************************/

extern	uint32	owner_table[1024];
void    ureturn(void *ptr);

void	malloc_print_stats(void);

void	malloc_tables_init(void);


void    *malloc(uint32  size);
void    free(void *ptr);

void    *kmalloc(uint32  size);
void    kfree(void *ptr);

uint32 	pmalloc_init(void);
void    pfree(void *ptr);
void    *pmalloc(uint32  size);

uint32	stmalloc_init(void);
void    stfree(void *ptr);
void    *stmalloc(uint32  size, uint32 physical_flag);




