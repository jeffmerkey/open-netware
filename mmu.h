extern	LONG OSPAETable;
extern	LONG OSPageTable;
extern	LONG OSPageTableSize;
extern	LONG OSPageTableEnd;
extern	LONG OSPDE;
extern	LONG DebuggerPAETable;
extern	LONG DebuggerPageTable;
extern	LONG DebuggerPageTableSize;
extern	LONG DebuggerPageTableEnd;
extern	LONG DebuggerPDE;
extern	LONG DefaultPageSize;

#define	PAGING_BEING_MOVED_BITS   	0x00000400


#define	OWNER_MASK				0x0FFFFFFF

#define PRESENT_BIT                             0x00000001
#define READ_WRITE_PAGE_BIT                     0x00000002
#define USER_PAGE_BIT                           0x00000004
#define PAGE_WRITE_THROUGH_BIT                  0x00000008
#define PAGE_CACHING_DISABLE_BIT                0x00000010
#define PAGE_ACCESSED_BIT                       0x00000020
#define PAGE_DIRTY_BIT                          0x00000040
#define PAGE_SIZE_BIT                           0x00000080
#define GLOBAL_PAGE_BIT                         0x00000100


#define	MMU_FLAGS_NOT_PRESENT_BIT		0x00000001
#define	MMU_FLAGS_READ_ONLY_BIT			0x00000002
#define	MMU_FLAGS_PRIVILEDGE_BIT		0x00000004
#define	MMU_FLAGS_DIRTY_BIT			0x00000008
#define	MMU_FLAGS_ACCESSED_BIT			0x00000010
#define	MMU_FLAGS_MOVING_BIT			0x00000020

#define	EMPTY_PAGE_STAMP			0x50544d45


#define		KERNEL_SPACE		0x4E52454B
#define		LIBRARY_SPACE		0x5242494C
#define		APPLICATION_SPACE       0x4C505041
#define		RESERVED_SPACE		0x56534552

#define		ADDRESS_TABLE_SIZE	16
#define		CHAPTER_TABLE_SIZE	64
#define		CHAPTER_TABLE_MASK	0xFFFFFFC0
#define		CHAPTER_TABLE_UNMASK	0x0000003F
#define		MASTER_PAGE_TABLE_SIZE	128

#define		PHYSICAL_BLOCK_SIZE 		0x00010000
#define		PHYSICAL_BLOCK_PAGE_COUNT	16
#define		PHYSICAL_BLOCK_PAGE_MASK	0xFFFFFFF0
#define		PHYSICAL_BLOCK_PAGE_UNMASK	0x0000000F

#define	     	PHYSICAL_PAGE_SIZE 		0x00001000
#define		PHYSICAL_PAGE_SHIFT 		12
#define		PHYSICAL_PAGE_MASK 		0xFFFFF000
#define		PHYSICAL_PAGE_UNMASK		0x00000FFF

#define		LOGICAL_PAGE_SIZE 		0x00001000
#define		LOGICAL_PAGE_SHIFT 		12
#define		LOGICAL_PAGE_MASK 		0xFFFFF000
#define		LOGICAL_PAGE_UNMASK 		0x00000FFF

#define		PHYSICAL_CHAPTER_PAGE_SIZE	1024
#define		PHYSICAL_CHAPTER_PAGE_SHIFT	10
#define		PHYSICAL_CHAPTER_PAGE_MASK	0xFFFFFC00
#define		PHYSICAL_CHAPTER_PAGE_UNMASK	0x000003FF

#define		PHYSICAL_CHAPTER_SIZE 		0x00400000
#define		PHYSICAL_CHAPTER_SHIFT 		22
#define		PHYSICAL_CHAPTER_MASK 		0xFFC00000
#define		PHYSICAL_CHAPTER_UNMASK 	0x003FFFFF
#define		PHYSICAL_CHAPTERS_PER_LOGICAL 	1

#define		LOGICAL_CHAPTER_SIZE		0x00400000
#define		LOGICAL_CHAPTER_SHIFT 		22
#define		LOGICAL_CHAPTER_MASK		0xFFC00000
#define		LOGICAL_CHAPTER_UNMASK 		0x003FFFFF

#define		PHYSICAL_EQUALS_LOGICAL_BOUNDARY 0x04000000
#define		ZONE_SIZE 			0x10000000
#define		ZONES_PER_ADDRESS_SPACE 	16
#define		CHAPTERS_PER_ZONE		64
#define		CHAPTERS_PER_ZONE_SHIFT		6


#define		EMERGENCY_PAGE_COUNT		16

typedef	struct	PAS_node_def		// physical address space node def
{
	LONG		*pte;
	LONG		*pde;
} PAS_node;


typedef	struct	LAS_node_def		// logical address space node def
{
	LONG		las_type;      // kernel, library, application
	PAS_node	*pas_node;
	LONG		pas_handle;
	LONG		*mat_table;
	struct physical_chapter_node_def	*chapter_table[CHAPTER_TABLE_SIZE];
} LAS_node;

typedef	struct	physical_chapter_node_def
{

	LONG	mapping_table[32];	// 32, 32 (8byte) entries;
} physical_chapter_node;

//
//	MMU SUPPORT APIS
//

LONG	MMU_validate_page(
	LONG	logical_page);

LONG	MMU_raw_page_map(
	LONG	lpn,
	LONG	ppn,
	LONG	flags);

LONG	MMU_page_map(
	LONG	lpn,
	LONG	ppn,
	LONG	flags);

LONG	MMU_page_unmap(
	LONG	lpn,
	LONG	*ppn,
	LONG	*flags);

LONG	MMU_page_get_info(
	LONG	lpn,
	LONG	*ppn,
	LONG	*flags);

LONG	MMU_page_set_info(
	LONG	lpn,
	LONG	ppn,
	LONG	flags);

LONG	MMU_invalidate_page(
	LONG	lpn);

LONG	MMU_TLB_flush(void);

LONG	MMU_TLB_shoot_down(void);
