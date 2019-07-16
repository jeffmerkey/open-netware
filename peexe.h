
#define TRUE 1
#define FALSE 0


/*
 * Old MZ header for DOS programs.
 * We check only the magic and the e_lfanew offset to the new executable
 * header.
 */
typedef struct
{
	WORD	e_magic;	/* MZ Header signature */
	WORD	e_cblp;		/* Bytes on last page of file */
	WORD	e_cp;		/* Pages in file */
	WORD	e_crlc;		/* Relocations */
	WORD	e_cparhdr;	/* Size of header in paragraphs */
	WORD	e_minalloc;	/* Minimum extra paragraphs needed */
	WORD	e_maxalloc;	/* Maximum extra paragraphs needed */
	WORD	e_ss;		/* Initial (relative) SS value */
	WORD	e_sp;		/* Initial SP value */
	WORD	e_csum;		/* Checksum */
	WORD	e_ip;		/* Initial IP value */
	WORD	e_cs;		/* Initial (relative) CS value */
	WORD	e_lfarlc;	/* File address of relocation table */
	WORD	e_ovno;		/* Overlay number */
	WORD	e_res[4];	/* Reserved words */
	WORD	e_oemid;	/* OEM identifier (for e_oeminfo) */
	WORD	e_oeminfo;	/* OEM information; e_oemid specific */
	WORD	e_res2[10];	/* Reserved words */
	WORD	e_lfanew;	/* Offset to extended header */
} IMAGE_DOS_HEADER,*PIMAGE_DOS_HEADER;

#define	IMAGE_DOS_SIGNATURE	0x5A4D		/* MZ */
#define	IMAGE_OS2_SIGNATURE	0x454E		/* NE */
#define	IMAGE_OS2_SIGNATURE_LE	0x454C		/* LE */
#define	IMAGE_VXD_SIGNATURE	0x454C		/* LE */
#define	IMAGE_NT_SIGNATURE	0x00004550	/* PE00 */

/*
 * This is the Windows executable (NE) header.
 * the name IMAGE_OS2_HEADER is misleading, but in the SDK this way.
 */
typedef struct 
{
    WORD  ne_magic;             /* 00 NE signature 'NE' */
    BYTE  linker_version;	/* 02 Linker version number */
    BYTE  linker_revision;	/* 03 Linker revision number */
    WORD  entry_tab_offset;	/* 04 Offset to entry table relative to NE */
    WORD  entry_tab_length;	/* 06 Length of entry table in bytes */
    DWORD reserved1;		/* 08 Reserved by Microsoft */
    WORD  format_flags;         /* 0c Flags about segments in this file */
    WORD  auto_data_seg;	/* 0e Automatic data segment number */
    WORD  local_heap_length;	/* 10 Initial size of local heap */
    WORD  stack_length;         /* 12 Initial size of stack */
    WORD  ip;			/* 14 Initial IP */
    WORD  cs;			/* 16 Initial CS */
    WORD  sp;			/* 18 Initial SP */
    WORD  ss;			/* 1a Initial SS */
    WORD  n_segment_tab;	/* 1c # of entries in segment table */
    WORD  n_mod_ref_tab;	/* 1e # of entries in module reference tab. */
    WORD  nrname_tab_length; 	/* 20 Length of nonresident-name table     */
    WORD  segment_tab_offset;	/* 22 Offset to segment table */
    WORD  resource_tab_offset;  /* 24 Offset to resource table */
    WORD  rname_tab_offset;	/* 26 Offset to resident-name table */
    WORD  moduleref_tab_offset; /* 28 Offset to module reference table */
    WORD  iname_tab_offset;	/* 2a Offset to imported name table */
    DWORD nrname_tab_offset;	/* 2c Offset to nonresident-name table */
    WORD  n_mov_entry_points;	/* 30 # of movable entry points */
    WORD  align_shift_count;	/* 32 Logical sector alignment shift count */
    WORD  n_resource_seg;	/* 34 # of resource segments */
    BYTE  operating_system;	/* 36 Flags indicating target OS */
    BYTE  additional_flags;	/* 37 Additional information flags */
    WORD  fastload_offset;	/* 38 Offset to fast load area */
    WORD  fastload_length;	/* 3a Length of fast load area */
    WORD  reserved2;		/* 3c Reserved by Microsoft */
    WORD  expect_version;	/* 3e Expected Windows version number */
} IMAGE_OS2_HEADER,*PIMAGE_OS2_HEADER;

/*
 * NE Header FORMAT FLAGS
 */
#define NE_FFLAGS_SINGLEDATA	0x0001
#define NE_FFLAGS_MULTIPLEDATA	0x0002
#define NE_FFLAGS_WIN32         0x0010
#define NE_FFLAGS_BUILTIN       0x0020  /* Wine built-in module */
#define NE_FFLAGS_SELFLOAD	0x0800
#define NE_FFLAGS_LINKERROR	0x2000
#define NE_FFLAGS_CALLWEP       0x4000
#define NE_FFLAGS_LIBMODULE	0x8000

/*
 * NE Header OPERATING SYSTEM
 */
#define NE_OSFLAGS_UNKNOWN	0x01
#define NE_OSFLAGS_WINDOWS	0x04

/*
 * NE Header ADDITIONAL FLAGS
 */
#define NE_AFLAGS_WIN2_PROTMODE	0x02
#define NE_AFLAGS_WIN2_PROFONTS	0x04
#define NE_AFLAGS_FASTLOAD	0x08

/*
 * Segment table entry
 */
struct ne_segment_table_entry_s
{
    WORD seg_data_offset;	/* Sector offset of segment data	*/
    WORD seg_data_length;	/* Length of segment data		*/
    WORD seg_flags;		/* Flags associated with this segment	*/
    WORD min_alloc;		/* Minimum allocation size for this	*/
};

/*
 * Segment Flags
 */
#define NE_SEGFLAGS_DATA	0x0001
#define NE_SEGFLAGS_ALLOCATED	0x0002
#define NE_SEGFLAGS_LOADED	0x0004
#define NE_SEGFLAGS_ITERATED	0x0008
#define NE_SEGFLAGS_MOVEABLE	0x0010
#define NE_SEGFLAGS_SHAREABLE	0x0020
#define NE_SEGFLAGS_PRELOAD	0x0040
#define NE_SEGFLAGS_EXECUTEONLY	0x0080
#define NE_SEGFLAGS_READONLY	0x0080
#define NE_SEGFLAGS_RELOC_DATA	0x0100
#define NE_SEGFLAGS_DISCARDABLE	0x1000

/*
 * Relocation table entry
 */
struct relocation_entry_s
{
    BYTE address_type;	/* Relocation address type		*/
    BYTE relocation_type;	/* Relocation type			*/
    WORD offset;		/* Offset in segment to fixup		*/
    WORD target1;		/* Target specification			*/
    WORD target2;		/* Target specification			*/
};

/*
 * Relocation address types
 */
#define NE_RADDR_LOWBYTE	0
#define NE_RADDR_SELECTOR	2
#define NE_RADDR_POINTER32	3
#define NE_RADDR_OFFSET16	5
#define NE_RADDR_POINTER48	11
#define NE_RADDR_OFFSET32	13

/*
 * Relocation types
 */
#define NE_RELTYPE_INTERNAL	0
#define NE_RELTYPE_ORDINAL	1
#define NE_RELTYPE_NAME		2
#define NE_RELTYPE_OSFIXUP	3
#define NE_RELFLAG_ADDITIVE	4

/*
 * DOS PSP
 */
struct dos_psp_s
{
    unsigned short pspInt20;
    unsigned short pspNextParagraph;
    unsigned char  pspReserved1;
    unsigned char  pspDispatcher[5];
    unsigned short pspTerminateVector[2];
    unsigned short pspControlCVector[2];
    unsigned short pspCritErrorVector[2];
    unsigned short pspReserved2[11];
    unsigned short pspEnvironment;
    unsigned short pspReserved3[23];
    unsigned char  pspFCB_1[16];
    unsigned char  pspFCB_2[16];
    unsigned char  pspReserved4[4];
    unsigned char  pspCommandTailCount;
    unsigned char  pspCommandTail[128];
};

/*
 * Entry table structures.
 */
struct entry_tab_header_s
{
    unsigned char n_entries;
    unsigned char seg_number;
};

struct entry_tab_movable_s
{
    unsigned char flags;
    unsigned char int3f[2];
    unsigned char seg_number;
    unsigned short offset;
};

struct entry_tab_fixed_s
{
    unsigned char flags;
    unsigned char offset[2];
};

/*
 * Resource table structures.
 */
struct resource_nameinfo_s
{
    unsigned short offset;
    unsigned short length;
    unsigned short flags;
    unsigned short id;
    unsigned int handle;
    unsigned short usage;
};

struct resource_typeinfo_s
{
    unsigned short type_id;	/* Type identifier */
    unsigned short count;	/* Number of resources of this type */
    unsigned long  reserved;
    /*
     * Name info array.
     */
};

#define NE_RSCTYPE_ACCELERATOR		0x8009
#define NE_RSCTYPE_BITMAP		0x8002
#define NE_RSCTYPE_CURSOR		0x8001
#define NE_RSCTYPE_DIALOG		0x8005
#define NE_RSCTYPE_FONT			0x8008
#define NE_RSCTYPE_FONTDIR		0x8007
#define NE_RSCTYPE_GROUP_CURSOR		0x800c
#define NE_RSCTYPE_GROUP_ICON		0x800e
#define NE_RSCTYPE_ICON			0x8003
#define NE_RSCTYPE_MENU			0x8004
#define NE_RSCTYPE_RCDATA		0x800a
#define NE_RSCTYPE_STRING		0x8006

int  load_typeinfo  (int, struct resource_typeinfo_s *);
int  load_nameinfo  (int, struct resource_nameinfo_s *);

typedef struct _IMAGE_FILE_HEADER {
	WORD	Machine;			/* 00 */
	WORD	NumberOfSections;		/* 02 */
	DWORD	TimeDateStamp;			/* 04 */
	DWORD	PointerToSymbolTable;		/* 08 */
	DWORD	NumberOfSymbols;		/* 0c */
	WORD	SizeOfOptionalHeader;		/* 10 */
	WORD	Characteristics;		/* 12 */
} IMAGE_FILE_HEADER,*PIMAGE_FILE_HEADER;

#define	IMAGE_SIZEOF_FILE_HEADER	20

#define	IMAGE_FILE_MACHINE_UNKNOWN	0
#define	IMAGE_FILE_MACHINE_I860		0x14d
#define	IMAGE_FILE_MACHINE_I386		0x14c
#define	IMAGE_FILE_MACHINE_R3000	0x162
#define	IMAGE_FILE_MACHINE_R4000	0x166
#define	IMAGE_FILE_MACHINE_R10000	0x168
#define	IMAGE_FILE_MACHINE_ALPHA	0x184
#define	IMAGE_FILE_MACHINE_POWERPC	0x1F0  

typedef struct _IMAGE_DATA_DIRECTORY
{
	DWORD	VirtualAddress;
	DWORD	Size;
} IMAGE_DATA_DIRECTORY,*PIMAGE_DATA_DIRECTORY;

#define	IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

/* Optional coff header - used by NT to provide additional information. */
typedef struct _IMAGE_OPTIONAL_HEADER
{
	/*
	 * Standard fields.
	 */

	WORD	Magic;
	BYTE	MajorLinkerVersion;
	BYTE	MinorLinkerVersion;
	DWORD	SizeOfCode;
	DWORD	SizeOfInitializedData;
	DWORD	SizeOfUninitializedData;
	DWORD	AddressOfEntryPoint;
	DWORD	BaseOfCode;
	DWORD	BaseOfData;

	/*
	 * NT additional fields.
	 */

	DWORD	ImageBase;
	DWORD	SectionAlignment;
	DWORD	FileAlignment;
	WORD	MajorOperatingSystemVersion;
	WORD	MinorOperatingSystemVersion;
	WORD	MajorImageVersion;
	WORD	MinorImageVersion;
	WORD	MajorSubsystemVersion;
	WORD	MinorSubsystemVersion;
	DWORD	Reserved1;
	DWORD	SizeOfImage;
	DWORD	SizeOfHeaders;
	DWORD	CheckSum;
	WORD	Subsystem;
	WORD	DllCharacteristics;
	DWORD	SizeOfStackReserve;
	DWORD	SizeOfStackCommit;
	DWORD	SizeOfHeapReserve;
	DWORD	SizeOfHeapCommit;
	DWORD	LoaderFlags;
	DWORD	NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER,*PIMAGE_OPTIONAL_HEADER;

/* These are indexes into the DataDirectory array */
#define IMAGE_FILE_EXPORT_DIRECTORY		0
#define IMAGE_FILE_IMPORT_DIRECTORY		1
#define IMAGE_FILE_RESOURCE_DIRECTORY		2
#define IMAGE_FILE_EXCEPTION_DIRECTORY		3
#define IMAGE_FILE_SECURITY_DIRECTORY		4
#define IMAGE_FILE_BASE_RELOCATION_TABLE	5
#define IMAGE_FILE_DEBUG_DIRECTORY		6
#define IMAGE_FILE_DESCRIPTION_STRING		7
#define IMAGE_FILE_MACHINE_VALUE		8  /* Mips */
#define IMAGE_FILE_THREAD_LOCAL_STORAGE		9
#define IMAGE_FILE_CALLBACK_DIRECTORY		10

/* Directory Entries, indices into the DataDirectory array */

#define	IMAGE_DIRECTORY_ENTRY_EXPORT		0
#define	IMAGE_DIRECTORY_ENTRY_IMPORT		1
#define	IMAGE_DIRECTORY_ENTRY_RESOURCE		2
#define	IMAGE_DIRECTORY_ENTRY_EXCEPTION		3
#define	IMAGE_DIRECTORY_ENTRY_SECURITY		4
#define	IMAGE_DIRECTORY_ENTRY_BASERELOC		5
#define	IMAGE_DIRECTORY_ENTRY_DEBUG		6
#define	IMAGE_DIRECTORY_ENTRY_COPYRIGHT		7
#define	IMAGE_DIRECTORY_ENTRY_GLOBALPTR		8   /* (MIPS GP) */
#define	IMAGE_DIRECTORY_ENTRY_TLS		9
#define	IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG	10
#define	IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT	11
#define	IMAGE_DIRECTORY_ENTRY_IAT		12  /* Import Address Table */

/* Subsystem Values */

#define	IMAGE_SUBSYSTEM_UNKNOWN		0
#define	IMAGE_SUBSYSTEM_NATIVE		1
#define	IMAGE_SUBSYSTEM_WINDOWS_GUI	2	/* Windows GUI subsystem */
#define	IMAGE_SUBSYSTEM_WINDOWS_CUI	3	/* Windows character subsystem*/
#define	IMAGE_SUBSYSTEM_OS2_CUI		5
#define	IMAGE_SUBSYSTEM_POSIX_CUI	7

typedef struct _IMAGE_NT_HEADERS {
	DWORD			Signature;
	IMAGE_FILE_HEADER	FileHeader;
	IMAGE_OPTIONAL_HEADER	OptionalHeader;
} IMAGE_NT_HEADERS,*PIMAGE_NT_HEADERS;

/* Section header format */

#define	IMAGE_SIZEOF_SHORT_NAME	8

typedef struct _IMAGE_SECTION_HEADER {
	BYTE	Name[IMAGE_SIZEOF_SHORT_NAME];
	union {
		DWORD	PhysicalAddress;
		DWORD	VirtualSize;
	} Misc;
	DWORD	VirtualAddress;
	DWORD	SizeOfRawData;
	DWORD	PointerToRawData;
	DWORD	PointerToRelocations;
	DWORD	PointerToLinenumbers;
	WORD	NumberOfRelocations;
	WORD	NumberOfLinenumbers;
	DWORD	Characteristics;
} IMAGE_SECTION_HEADER,*PIMAGE_SECTION_HEADER;

#define	IMAGE_SIZEOF_SECTION_HEADER 40

/* These defines are for the Characteristics bitfield. */
/* #define IMAGE_SCN_TYPE_REG			0x00000000 - Reserved */
/* #define IMAGE_SCN_TYPE_DSECT			0x00000001 - Reserved */
/* #define IMAGE_SCN_TYPE_NOLOAD		0x00000002 - Reserved */
/* #define IMAGE_SCN_TYPE_GROUP			0x00000004 - Reserved */
/* #define IMAGE_SCN_TYPE_NO_PAD		0x00000008 - Reserved */
/* #define IMAGE_SCN_TYPE_COPY			0x00000010 - Reserved */

#define IMAGE_SCN_CNT_CODE			0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA		0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA	0x00000080

#define	IMAGE_SCN_LNK_OTHER			0x00000100 
#define	IMAGE_SCN_LNK_INFO			0x00000200  
#define	IMAGE_SCN_LNK_OVERLAY			0x00000400 
#define	IMAGE_SCN_LNK_REMOVE			0x00000800
#define	IMAGE_SCN_LNK_COMDAT			0x00001000

/* 						0x00002000 - Reserved */
/* #define IMAGE_SCN_MEM_PROTECTED 		0x00004000 - Obsolete */
#define	IMAGE_SCN_MEM_FARDATA			0x00008000

/* #define IMAGE_SCN_MEM_SYSHEAP		0x00010000 - Obsolete */
#define	IMAGE_SCN_MEM_PURGEABLE			0x00020000
#define	IMAGE_SCN_MEM_16BIT			0x00020000
#define	IMAGE_SCN_MEM_LOCKED			0x00040000
#define	IMAGE_SCN_MEM_PRELOAD			0x00080000

#define	IMAGE_SCN_ALIGN_1BYTES			0x00100000
#define	IMAGE_SCN_ALIGN_2BYTES			0x00200000
#define	IMAGE_SCN_ALIGN_4BYTES			0x00300000
#define	IMAGE_SCN_ALIGN_8BYTES			0x00400000
#define	IMAGE_SCN_ALIGN_16BYTES			0x00500000  /* Default */
#define IMAGE_SCN_ALIGN_32BYTES			0x00600000
#define IMAGE_SCN_ALIGN_64BYTES			0x00700000
/* 						0x00800000 - Unused */

#define IMAGE_SCN_LNK_NRELOC_OVFL		0x01000000


#define IMAGE_SCN_MEM_DISCARDABLE		0x02000000
#define IMAGE_SCN_MEM_NOT_CACHED		0x04000000
#define IMAGE_SCN_MEM_NOT_PAGED			0x08000000
#define IMAGE_SCN_MEM_SHARED			0x10000000
#define IMAGE_SCN_MEM_EXECUTE			0x20000000
#define IMAGE_SCN_MEM_READ			0x40000000
#define IMAGE_SCN_MEM_WRITE			0x80000000

/* Import name entry */
typedef struct _IMAGE_IMPORT_BY_NAME {
	WORD	Hint;
	BYTE	Name[1];
} IMAGE_IMPORT_BY_NAME,*PIMAGE_IMPORT_BY_NAME;

/* Import thunk */
typedef struct _IMAGE_THUNK_DATA {
    union
    {
       LPBYTE	ForwarderString;
       LPDWORD	Function;
       DWORD	Ordinal;
       PIMAGE_IMPORT_BY_NAME	AddressOfData;
    } u1;
} IMAGE_THUNK_DATA,*PIMAGE_THUNK_DATA;

/* Import module directory */

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
	union
	{
	   DWORD Characteristics; /* 0 for terminating null import descriptor  */
	   PIMAGE_THUNK_DATA OriginalFirstThunk;	/* RVA to original unbound IAT */
	} u;
	DWORD	TimeDateStamp;	/* 0 if not bound,
				 * -1 if bound, and real date\time stamp
				 *    in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT
				 * (new BIND)
				 * otherwise date/time stamp of DLL bound to
				 * (Old BIND)
				 */
	DWORD	ForwarderChain;	/* -1 if no forwarders */
	DWORD	Name;
	/* RVA to IAT (if bound this IAT has actual addresses) */
	PIMAGE_THUNK_DATA FirstThunk;	
} IMAGE_IMPORT_DESCRIPTOR,*PIMAGE_IMPORT_DESCRIPTOR;

#define	IMAGE_ORDINAL_FLAG		0x80000000
#define	IMAGE_SNAP_BY_ORDINAL(Ordinal)	((Ordinal & IMAGE_ORDINAL_FLAG) != 0)
#define	IMAGE_ORDINAL(Ordinal)		(Ordinal & 0xffff)

/* Export module directory */

typedef struct _IMAGE_EXPORT_DIRECTORY {
	DWORD	Characteristics;
	DWORD	TimeDateStamp;
	WORD	MajorVersion;
	WORD	MinorVersion;
	DWORD	Name;
	DWORD	Base;
	DWORD	NumberOfFunctions;
	DWORD	NumberOfNames;
	LPDWORD	*AddressOfFunctions;
	LPDWORD	*AddressOfNames;
	LPWORD	*AddressOfNameOrdinals;
/*  u_char ModuleName[1]; */
} IMAGE_EXPORT_DIRECTORY,*PIMAGE_EXPORT_DIRECTORY;


/*
 * Resource directory stuff
 */
typedef struct _IMAGE_RESOURCE_DIRECTORY {
	DWORD	Characteristics;
	DWORD	TimeDateStamp;
	WORD	MajorVersion;
	WORD	MinorVersion;
	WORD	NumberOfNamedEntries;
	WORD	NumberOfIdEntries;
	/*  IMAGE_RESOURCE_DIRECTORY_ENTRY DirectoryEntries[]; */
} IMAGE_RESOURCE_DIRECTORY,*PIMAGE_RESOURCE_DIRECTORY;

#define	IMAGE_RESOURCE_NAME_IS_STRING		0x80000000
#define	IMAGE_RESOURCE_DATA_IS_DIRECTORY	0x80000000

typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY {
	union {
		struct {
//		   DWORD NameOffset:31;
//		   DWORD NameIsString:1;
// fix jmerkey!
		   DWORD NameOffset;
		} s;
		DWORD   Name;
		WORD    Id;
	} u1;
	union {
		DWORD   OffsetToData;
		struct {
//		   DWORD   OffsetToDirectory:31;
//		   DWORD   DataIsDirectory:1;
//  fix jmerkey!
//
		   DWORD  OffsetToDirectory;
		} s;
	} u2;
} IMAGE_RESOURCE_DIRECTORY_ENTRY,*PIMAGE_RESOURCE_DIRECTORY_ENTRY;

typedef struct tagImportDirectory {
   DWORD RVAFunctionNameList;
   DWORD UseLess1;
   DWORD UseLess2;
   DWORD RVAModuleName;
   DWORD RVAFunctionAddressList;
} IMAGE_IMPORT_MODULE_DIRECTORY, *PIMAGE_IMPORT_MODULE_DIRECTORY;

typedef struct _IMAGE_RESOURCE_DIRECTORY_STRING {
	WORD	Length;
	CHAR	NameString[1];
} IMAGE_RESOURCE_DIRECTORY_STRING,*PIMAGE_RESOURCE_DIRECTORY_STRING;

typedef struct _IMAGE_RESOURCE_DIR_STRING_U {
	WORD	Length;
	WCHAR	NameString[1];
} IMAGE_RESOURCE_DIR_STRING_U,*PIMAGE_RESOURCE_DIR_STRING_U;

typedef struct _IMAGE_RESOURCE_DATA_ENTRY {
	DWORD	OffsetToData;
	DWORD	Size;
	DWORD	CodePage;
	DWORD	Reserved;
} IMAGE_RESOURCE_DATA_ENTRY,*PIMAGE_RESOURCE_DATA_ENTRY;

typedef struct _IMAGE_BASE_RELOCATION
{
	DWORD	VirtualAddress;
	DWORD	SizeOfBlock;
	WORD	TypeOffset[1];
} IMAGE_BASE_RELOCATION,*PIMAGE_BASE_RELOCATION;

typedef struct _IMAGE_LOAD_CONFIG_DIRECTORY {
	DWORD	Characteristics;
	DWORD	TimeDateStamp;
	WORD	MajorVersion;
	WORD	MinorVersion;
	DWORD	GlobalFlagsClear;
	DWORD	GlobalFlagsSet;
	DWORD	CriticalSectionDefaultTimeout;
	DWORD	DeCommitFreeBlockThreshold;
	DWORD	DeCommitTotalFreeThreshold;
	LPVOID	LockPrefixTable;
	DWORD	MaximumAllocationSize;
	DWORD	VirtualMemoryThreshold;
	DWORD	ProcessHeapFlags;
	DWORD	Reserved[4];
} IMAGE_LOAD_CONFIG_DIRECTORY,*PIMAGE_LOAD_CONFIG_DIRECTORY;

typedef VOID (*PIMAGE_TLS_CALLBACK)(
	LPVOID DllHandle,DWORD Reason,LPVOID Reserved
);

typedef struct _IMAGE_TLS_DIRECTORY {
	DWORD	StartAddressOfRawData;
	DWORD	EndAddressOfRawData;
	LPDWORD	AddressOfIndex;
	PIMAGE_TLS_CALLBACK *AddressOfCallBacks;
	DWORD	SizeOfZeroFill;
	DWORD	Characteristics;
} IMAGE_TLS_DIRECTORY,*PIMAGE_TLS_DIRECTORY;

/*
 * The IMAGE_DEBUG_DIRECTORY data directory points to an array of
 * these structures.
 */
typedef struct _IMAGE_DEBUG_DIRECTORY {
	DWORD	Characteristics;
	DWORD	TimeDateStamp;
	WORD	MajorVersion;
	WORD	MinorVersion;
	DWORD	Type;
	DWORD	SizeOfData;
	DWORD	AddressOfRawData;
	DWORD	PointerToRawData;
} IMAGE_DEBUG_DIRECTORY,*PIMAGE_DEBUG_DIRECTORY;

/*
 * The type field above can take these (plus a few other
 * irrelevant) values.
 */
#define IMAGE_DEBUG_TYPE_UNKNOWN	0
#define IMAGE_DEBUG_TYPE_COFF		1
#define IMAGE_DEBUG_TYPE_CODEVIEW	2
#define IMAGE_DEBUG_TYPE_FPO		3
#define IMAGE_DEBUG_TYPE_MISC		4
#define IMAGE_DEBUG_TYPE_EXCEPTION	5
#define IMAGE_DEBUG_TYPE_FIXUP		6
#define IMAGE_DEBUG_TYPE_OMAP_TO_SRC	7
#define IMAGE_DEBUG_TYPE_OMAP_FROM_SRC	8


#define IMAGE_REL_BASED_ABSOLUTE 		0
#define IMAGE_REL_BASED_HIGH			1
#define IMAGE_REL_BASED_LOW			2
#define IMAGE_REL_BASED_HIGHLOW			3
#define IMAGE_REL_BASED_HIGHADJ			4
#define IMAGE_REL_BASED_MIPS_JMPADDR		5

/*
 * This is the structure that appears at the very start of a .DBG file.
 */
typedef struct _IMAGE_SEPARATE_DEBUG_HEADER {
	WORD	Signature;
	WORD	Flags;
	WORD	Machine;
	WORD	Characteristics;
	DWORD	TimeDateStamp;
	DWORD	CheckSum;
	DWORD	ImageBase;
	DWORD	SizeOfImage;
	DWORD	NumberOfSections;
	DWORD	ExportedNamesSize;
	DWORD	DebugDirectorySize;
	DWORD	Reserved[3 ];
} IMAGE_SEPARATE_DEBUG_HEADER,*PIMAGE_SEPARATE_DEBUG_HEADER;

#define IMAGE_SEPARATE_DEBUG_SIGNATURE 0x4944

#define IMAGE_LIBRARY_PROCESS_INIT	1
#define IMAGE_LIBRARY_PROCESS_TERM	8
#define IMAGE_LIBRARY_THREAD_INIT	4
#define IMAGE_LIBRARY_THREAD_TERM	2

#define IMAGE_LOADER_FLAGS_BREAK_ON_LOAD	1
#define IMAGE_LOADER_FLAGS_DEBUG_ON_LOAD	2

#define IMAGE_SYM_UNDEFINED			(short) 0
#define IMAGE_SYM_ABSOLUTE			(short) -1
#define IMAGE_SYM_DEBUG				(short) -2

//
// Type (derived) values.
//

#define IMAGE_SYM_DTYPE_NULL                0       // no derived type.
#define IMAGE_SYM_DTYPE_POINTER             1       // pointer.
#define IMAGE_SYM_DTYPE_FUNCTION            2       // function.
#define IMAGE_SYM_DTYPE_ARRAY               3       // array.

//
// Storage classes.
//

#define IMAGE_SYM_CLASS_END_OF_FUNCTION     (BYTE )-1
#define IMAGE_SYM_CLASS_NULL                0x0000
#define IMAGE_SYM_CLASS_AUTOMATIC           0x0001
#define IMAGE_SYM_CLASS_EXTERNAL            0x0002
#define IMAGE_SYM_CLASS_STATIC              0x0003
#define IMAGE_SYM_CLASS_REGISTER            0x0004
#define IMAGE_SYM_CLASS_EXTERNAL_DEF        0x0005
#define IMAGE_SYM_CLASS_LABEL               0x0006
#define IMAGE_SYM_CLASS_UNDEFINED_LABEL     0x0007
#define IMAGE_SYM_CLASS_MEMBER_OF_STRUCT    0x0008
#define IMAGE_SYM_CLASS_ARGUMENT            0x0009
#define IMAGE_SYM_CLASS_STRUCT_TAG          0x000A
#define IMAGE_SYM_CLASS_MEMBER_OF_UNION     0x000B
#define IMAGE_SYM_CLASS_UNION_TAG           0x000C
#define IMAGE_SYM_CLASS_TYPE_DEFINITION     0x000D
#define IMAGE_SYM_CLASS_UNDEFINED_STATIC    0x000E
#define IMAGE_SYM_CLASS_ENUM_TAG            0x000F
#define IMAGE_SYM_CLASS_MEMBER_OF_ENUM      0x0010
#define IMAGE_SYM_CLASS_REGISTER_PARAM      0x0011
#define IMAGE_SYM_CLASS_BIT_FIELD           0x0012

#define IMAGE_SYM_CLASS_FAR_EXTERNAL        0x0044  //

#define IMAGE_SYM_CLASS_BLOCK               0x0064
#define IMAGE_SYM_CLASS_FUNCTION            0x0065
#define IMAGE_SYM_CLASS_END_OF_STRUCT       0x0066
#define IMAGE_SYM_CLASS_FILE                0x0067
// new
#define IMAGE_SYM_CLASS_SECTION             0x0068
#define IMAGE_SYM_CLASS_WEAK_EXTERNAL       0x0069


//
// Symbol format.
//

typedef struct _IMAGE_SYMBOL {
    union {
        BYTE    ShortName[8];
        struct {
            DWORD   Short;     // if 0, use LongName
            DWORD   Long;      // offset into string table
        } Name;
        PBYTE   LongName[2];
    } N;
    DWORD   Value;
    SHORT   SectionNumber;
    WORD    Type;
    BYTE    StorageClass;
    BYTE    NumberOfAuxSymbols;
} IMAGE_SYMBOL;
typedef IMAGE_SYMBOL *PIMAGE_SYMBOL;

#define IMAGE_SIZEOF_SYMBOL                  18

typedef union _IMAGE_AUX_SYMBOL {
    struct {
        DWORD    TagIndex;                // struct, union, or enum tag index
        union {
            struct {
                WORD    Linenumber;       // declaration line number
                WORD    Size;             // size of struct, union, or enum
            } LnSz;
           DWORD    TotalSize;
        } Misc;
        union {
            struct {                      // if ISFCN, tag, or .bb
                DWORD    PointerToLinenumber;
                DWORD    PointerToNextFunction;
            } Function;
            struct {                      // if ISARY, up to 4 dimen.
		WORD     Dimension[4];
            } Array;
        } FcnAry;
        WORD    TvIndex;                  // tv index
    } Sym;
    struct {
        BYTE    Name[IMAGE_SIZEOF_SYMBOL];
    } File;
    struct {
        DWORD   Length;                   // section length
        WORD    NumberOfRelocations;      // number of relocation entries
        WORD    NumberOfLinenumbers;      // number of line numbers
        DWORD   CheckSum;                 // checksum for communal
        SHORT   Number;                   // section number to associate with
        BYTE    Selection;                // communal selection type
    } Section;
} IMAGE_AUX_SYMBOL;
typedef IMAGE_AUX_SYMBOL *PIMAGE_AUX_SYMBOL;

//
// Line number format.
//

typedef struct _IMAGE_LINENUMBER {
    union {
        DWORD   SymbolTableIndex;          // Symbol table index of function name if Linenumber is 0.
        DWORD   VirtualAddress;            // Virtual address of line number.
    } Type;
    WORD    Linenumber;                    // Line number.
} IMAGE_LINENUMBER;
typedef IMAGE_LINENUMBER *PIMAGE_LINENUMBER;

#define IMAGE_SIZEOF_LINENUMBER              6

#define IMAGE_FILE_RELOCS_STRIPPED           0x0001  // Relocation info stripped from file.
#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002  // File is executable  (i.e. no unresolved externel references).
#define IMAGE_FILE_LINE_NUMS_STRIPPED        0x0004  // Line nunbers stripped from file.
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED       0x0008  // Local symbols stripped from file.
#define IMAGE_FILE_BYTES_REVERSED_LO         0x0080  // Bytes of machine word are reversed.
#define IMAGE_FILE_32BIT_MACHINE             0x0100  // 32 bit word machine.
#define IMAGE_FILE_DEBUG_STRIPPED            0x0200  // Debugging info stripped from file in .DBG file
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   0x0400  // If Image is on removable media, copy and run from the swap file.
#define IMAGE_FILE_NET_RUN_FROM_SWAP         0x0800  // If Image is on Net, copy and run from the swap file.
#define IMAGE_FILE_SYSTEM                    0x1000  // System File.
#define IMAGE_FILE_DLL                       0x2000  // File is a DLL.
#define IMAGE_FILE_UP_SYSTEM_ONLY            0x4000  // File should only be run on a UP machine
#define IMAGE_FILE_BYTES_REVERSED_HI         0x8000  // Bytes of machine word are reversed.

#define IMAGE_FILE_MACHINE_UNKNOWN           0
#define IMAGE_FILE_MACHINE_I386              0x14c   // Intel 386.
#define IMAGE_FILE_MACHINE_R3000             0x162   // MIPS little-endian, 0x160 big-endian
#define IMAGE_FILE_MACHINE_R4000             0x166   // MIPS little-endian
#define IMAGE_FILE_MACHINE_R10000            0x168   // MIPS little-endian
#define IMAGE_FILE_MACHINE_ALPHA             0x184   // Alpha_AXP
#define IMAGE_FILE_MACHINE_POWERPC           0x1F0   // IBM PowerPC Little-Endian

#define IMAGE_FILE_MINIMAL_OBJECT	0x10
#define IMAGE_FILE_UPDATE_OBJECT	0x20
#define IMAGE_FILE_PATCH		0x400

typedef struct _IMAGE_COFF_SYMBOLS_HEADER {
    DWORD   NumberOfSymbols;
    DWORD   LvaToFirstSymbol;
    DWORD   NumberOfLinenumbers;
    DWORD   LvaToFirstLinenumber;
    DWORD   RvaToFirstByteOfCode;
    DWORD   RvaToLastByteOfCode;
    DWORD   RvaToFirstByteOfData;
    DWORD   RvaToLastByteOfData;
} IMAGE_COFF_SYMBOLS_HEADER, *PIMAGE_COFF_SYMBOLS_HEADER;

//
// Relocation format.
//

typedef struct _IMAGE_RELOCATION {
    union {
        DWORD   VirtualAddress;
        DWORD   RelocCount;             
    } u;
    DWORD   SymbolTableIndex;
    WORD    Type;
} IMAGE_RELOCATION;
typedef IMAGE_RELOCATION *PIMAGE_RELOCATION;

#define IMAGE_SIZEOF_RELOCATION         10

//
// I386 relocation types.
//

#define IMAGE_REL_I386_ABSOLUTE         0x0000  // Reference is absolute, no relocation is necessary
#define IMAGE_REL_I386_DIR16            0x0001  // Direct 16-bit reference to the symbols virtual address
#define IMAGE_REL_I386_REL16            0x0002  // PC-relative 16-bit reference to the symbols virtual address
#define IMAGE_REL_I386_DIR32            0x0006  // Direct 32-bit reference to the symbols virtual address
#define IMAGE_REL_I386_DIR32NB          0x0007  // Direct 32-bit reference to the symbols virtual address, base not included
#define IMAGE_REL_I386_SEG12            0x0009  // Direct 16-bit reference to the segment-selector bits of a 32-bit virtual address
#define IMAGE_REL_I386_SECTION          0x000A
#define IMAGE_REL_I386_SECREL           0x000B
#define IMAGE_REL_I386_REL32            0x0014  // PC-relative 32-bit reference to the symbols virtual address

#define CP_ACP		0
#define FILE_MAP_READ   0

typedef struct _BASE_RELOCATION
{
   DWORD VirtualAddress;
   DWORD SizeOfBlock;
} BASE_RELOCATION, *PBASE_RELOCATION;


typedef PIMAGE_COFF_SYMBOLS_HEADER PIMAGE_DEBUG_INFO;

extern void DumpHeader(SCREEN *screen, PIMAGE_FILE_HEADER pImageFileHeader);
extern void DumpOptionalHeader(SCREEN *screen, PIMAGE_OPTIONAL_HEADER optionalHeader);
extern void DumpSectionTable(SCREEN *screen, PIMAGE_SECTION_HEADER section, unsigned cSections, BOOL IsEXE);
extern void GetSectionName(WORD section, PSTR buffer, unsigned cbBuffer);
extern PSTR GetSZStorageClass(BYTE storageClass);
extern void DumpAuxSymbols(SCREEN *screen, PIMAGE_SYMBOL pSymbolTable);
extern BOOL LookupSymbolName(DWORD index, PSTR buffer, UINT length);
extern void DumpSymbolTable(SCREEN *screen, PIMAGE_SYMBOL pSymbolTable, unsigned cSymbols);
extern LPVOID GetSectionPtr(PSTR name, PIMAGE_NT_HEADERS pNTHeader, DWORD imageBase);
extern PIMAGE_SECTION_HEADER GetSectionHeader(PSTR name, PIMAGE_NT_HEADERS pNTHeader);
extern void DumpRawSectionData(SCREEN *screen, PIMAGE_SECTION_HEADER section, PVOID base, unsigned cSections);
extern void DumpLineNumbers(SCREEN *screen, PIMAGE_LINENUMBER pln, DWORD count);
extern void HexDump(PBYTE ptr, DWORD length);
extern LONG WideCharToMultiByte(void);
extern BOOL IsBadReadPtr(void *lp, UINT cb);
extern void DumpExeFile(SCREEN *screen, PIMAGE_DOS_HEADER dosHeader);
extern void DumpObjFile(SCREEN *screen, PIMAGE_FILE_HEADER pImageFileHeader);


