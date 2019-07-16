
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  COMMON.C
*   DESCRIP  :  Portable Executable Decode Library for MANOS v1.0
*   DATE     :  November 23, 1997
*
*
***************************************************************************/


#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "string.h"
#include "kernel.h"
#include "keyboard.h"
#include "screen.h"
#include "types.h"
#include "emit.h"
#include "dos.h"
#include "tss.h"
#include "os.h"
#include "mps.h"
#include "hal.h"
#include "timer.h"
#include "peexe.h"
#include "extrnvar.h"

PIMAGE_SYMBOL PCOFFSymbolTable = 0; // RVA to COFF symbol table (if present)
DWORD COFFSymbolCount = 0;	    // Number of symbols in COFF symbol table

typedef struct
{
	WORD	flag;
	LPSTR	name;
} WORD_FLAG_DESCRIPTIONS;

typedef struct
{
	DWORD	flag;
	LPSTR	name;
} DWORD_FLAG_DESCRIPTIONS;

#define IMAGE_FILE_DLL			0x2000

// Bitfield values and names for the IMAGE_FILE_HEADER flags
WORD_FLAG_DESCRIPTIONS ImageFileHeaderCharacteristics[] = 
{
{ IMAGE_FILE_RELOCS_STRIPPED, "RELOCS_STRIPPED" },
{ IMAGE_FILE_EXECUTABLE_IMAGE, "EXECUTABLE_IMAGE" },
{ IMAGE_FILE_LINE_NUMS_STRIPPED, "LINE_NUMS_STRIPPED" },
{ IMAGE_FILE_LOCAL_SYMS_STRIPPED, "LOCAL_SYMS_STRIPPED" },
{ IMAGE_FILE_MINIMAL_OBJECT, "MINIMAL_OBJECT" },
{ IMAGE_FILE_UPDATE_OBJECT, "UPDATE_OBJECT" },
//{ IMAGE_FILE_16BIT_MACHINE, "16BIT_MACHINE" },
{ IMAGE_FILE_BYTES_REVERSED_LO, "BYTES_REVERSED_LO" },
{ IMAGE_FILE_32BIT_MACHINE, "32BIT_MACHINE" },
{ IMAGE_FILE_PATCH, "PATCH" },
{ IMAGE_FILE_SYSTEM, "SYSTEM" },
{ IMAGE_FILE_DLL, "DLL" },
{ IMAGE_FILE_BYTES_REVERSED_HI, "BYTES_REVERSED_HI" }
};

#define NUMBER_IMAGE_HEADER_FLAGS (sizeof(ImageFileHeaderCharacteristics) / sizeof(WORD_FLAG_DESCRIPTIONS))

//
// Dump the IMAGE_FILE_HEADER for a PE file or an OBJ
//
void DumpHeader(SCREEN *screen, PIMAGE_FILE_HEADER pImageFileHeader)
{

	UINT i;
	char *szMachine;
	
	printfScreen(screen, "File Header\n");

	switch( pImageFileHeader->Machine )
	{
		case IMAGE_FILE_MACHINE_I386:	szMachine = "i386"; break;
		case IMAGE_FILE_MACHINE_I860:	szMachine = "i860"; break;
		case IMAGE_FILE_MACHINE_R3000:	szMachine = "R3000"; break;
		case IMAGE_FILE_MACHINE_R4000:	szMachine = "R4000"; break;
		case IMAGE_FILE_MACHINE_ALPHA:	szMachine = "alpha"; break;
		default:    szMachine = "unknown"; break;
	}

	printfScreen(screen, "  %s  %04X (%s)\n", "Machine              :",
				pImageFileHeader->Machine, szMachine);
	printfScreen(screen, "  %s  %04X\n",      "Number of Sections   :",
				pImageFileHeader->NumberOfSections);
	printfScreen(screen, "  %s  %08X\n",      "TimeDateStamp        :",
				pImageFileHeader->TimeDateStamp);
	printfScreen(screen, "  %s  %08X\n",      "PointerToSymbolTable :",
				pImageFileHeader->PointerToSymbolTable);
	printfScreen(screen, "  %s  %08X\n",      "NumberOfSymbols      :",
				pImageFileHeader->NumberOfSymbols);
	printfScreen(screen, "  %s  %04X\n",      "SizeOfOptionalHeader :",
				pImageFileHeader->SizeOfOptionalHeader);
	printfScreen(screen, "  %s  %04X\n",      "Characteristics      :",
				pImageFileHeader->Characteristics);
	for ( i=0; i < NUMBER_IMAGE_HEADER_FLAGS; i++ )
	{
		if ( pImageFileHeader->Characteristics &
			 ImageFileHeaderCharacteristics[i].flag )
			printfScreen(screen,  "    %s\n", ImageFileHeaderCharacteristics[i].name );
	}
}

// Bitfield values and names for the DllCharacteritics flags
WORD_FLAG_DESCRIPTIONS DllCharacteristics[] =
{
{ IMAGE_LIBRARY_PROCESS_INIT, "PROCESS_INIT" },
{ IMAGE_LIBRARY_PROCESS_TERM, "PROCESS_TERM" },
{ IMAGE_LIBRARY_THREAD_INIT, "THREAD_INIT" },
{ IMAGE_LIBRARY_THREAD_TERM, "THREAD_TERM" },
};
#define NUMBER_DLL_CHARACTERISTICS (sizeof(DllCharacteristics) / sizeof(WORD_FLAG_DESCRIPTIONS))

// Bitfield values and names for the LoaderFlags flags
DWORD_FLAG_DESCRIPTIONS LoaderFlags[] =
{
{ IMAGE_LOADER_FLAGS_BREAK_ON_LOAD, "BREAK_ON_LOAD" },
{ IMAGE_LOADER_FLAGS_DEBUG_ON_LOAD, "DEBUG_ON_LOAD" }
};
#define NUMBER_LOADER_FLAGS (sizeof(LoaderFlags) / sizeof(DWORD_FLAG_DESCRIPTIONS))

// Names of the data directory elements that are defined
char *ImageDirectoryNames[] = {
	"EXPORT", "IMPORT", "RESOURCE", "EXCEPTION", "SECURITY", "BASERELOC",
	"DEBUG", "COPYRIGHT", "GLOBALPTR", "TLS", "LOAD_CONFIG" };

#define NUMBER_IMAGE_DIRECTORY_ENTRYS (sizeof(ImageDirectoryNames)/sizeof(char *))

//
// Dump the IMAGE_OPTIONAL_HEADER from a PE file
//
void DumpOptionalHeader(SCREEN *screen, PIMAGE_OPTIONAL_HEADER optionalHeader)
{
	char *s;
	UINT i;

	printfScreen(screen, "Optional Header\n");

	printfScreen(screen, "  %s  %04X\n",    "Magic                ", optionalHeader->Magic);
	printfScreen(screen, "  %s  %u.%02u\n", "linker version       ",
		optionalHeader->MajorLinkerVersion,
		optionalHeader->MinorLinkerVersion);
	printfScreen(screen, "  %s  %X\n",      "size of code         ", optionalHeader->SizeOfCode);
	printfScreen(screen, "  %s  %X\n",      "size of init data    ",
		optionalHeader->SizeOfInitializedData);
	printfScreen(screen, "  %s  %X\n",      "size of uninit data  ",
		optionalHeader->SizeOfUninitializedData);
	printfScreen(screen, "  %s  %X\n",      "entrypoint RVA       ",
		optionalHeader->AddressOfEntryPoint);
	printfScreen(screen, "  %s  %X\n",      "base of code         ", optionalHeader->BaseOfCode);
	printfScreen(screen, "  %s  %X\n",      "base of data         ", optionalHeader->BaseOfData);
	printfScreen(screen, "  %s  %X\n",      "image base           ", optionalHeader->ImageBase);

	printfScreen(screen, "  %s  %X\n",      "section align        ",
		optionalHeader->SectionAlignment);
	printfScreen(screen, "  %s  %X\n",      "file align           ", optionalHeader->FileAlignment);
	printfScreen(screen, "  %s  %u.%02u\n", "required OS version  ",
		optionalHeader->MajorOperatingSystemVersion,
		optionalHeader->MinorOperatingSystemVersion);
	printfScreen(screen, "  %s  %u.%02u\n", "image version        ",
		optionalHeader->MajorImageVersion,
		optionalHeader->MinorImageVersion);
	printfScreen(screen, "  %s  %u.%02u\n", "subsystem version    ",
		optionalHeader->MajorSubsystemVersion,
		optionalHeader->MinorSubsystemVersion);
	printfScreen(screen, "  %s  %X\n",      "Reserved1            ",
		optionalHeader->Reserved1);
	printfScreen(screen, "  %s  %X\n",      "size of image        ",
		optionalHeader->SizeOfImage);
	printfScreen(screen, "  %s  %X\n",      "size of headers      ",
			optionalHeader->SizeOfHeaders);
	printfScreen(screen, "  %s  %X\n",      "checksum             ",
		optionalHeader->CheckSum);
	switch( optionalHeader->Subsystem )
	{
		case IMAGE_SUBSYSTEM_NATIVE: s = "Native"; break;
		case IMAGE_SUBSYSTEM_WINDOWS_GUI: s = "Windows GUI"; break;
		case IMAGE_SUBSYSTEM_WINDOWS_CUI: s = "Windows character"; break;
		case IMAGE_SUBSYSTEM_OS2_CUI: s = "OS/2 character"; break;
		case IMAGE_SUBSYSTEM_POSIX_CUI: s = "Posix character"; break;
		default: s = "unknown";
	}
	printfScreen(screen, "  %s  %04X (%s)\n", "Subsystem            ",
			optionalHeader->Subsystem, s);

	printfScreen(screen, "  %s  %04X\n",    "DLL flags            ",
			optionalHeader->DllCharacteristics);
	for ( i=0; i < NUMBER_DLL_CHARACTERISTICS; i++ )
	{
		if ( optionalHeader->DllCharacteristics &
			 DllCharacteristics[i].flag )
			printfScreen(screen,  "  %s", DllCharacteristics[i].name );
	}
	if ( optionalHeader->DllCharacteristics )
		printfScreen(screen, "\n");

	printfScreen(screen, "  %s  %X\n",      "stack reserve size   ",
		optionalHeader->SizeOfStackReserve);
	printfScreen(screen, "  %s  %X\n",      "stack commit size    ",
		optionalHeader->SizeOfStackCommit);
	printfScreen(screen, "  %s  %X\n",      "heap reserve size    ",
		optionalHeader->SizeOfHeapReserve);
	printfScreen(screen, "  %s  %X\n",      "heap commit size     ",
		optionalHeader->SizeOfHeapCommit);

	printfScreen(screen, "  %s  %08X\n",    "loader flags         ",
		optionalHeader->LoaderFlags);

	for ( i=0; i < NUMBER_LOADER_FLAGS; i++ )
	{
		if ( optionalHeader->LoaderFlags &
			 LoaderFlags[i].flag )
			printfScreen(screen,  "  %s", LoaderFlags[i].name );
	}
	if ( optionalHeader->LoaderFlags )
		printfScreen(screen, "\n");

	printfScreen(screen, "  %s  %X\n", "RVAs & sizes",
		optionalHeader->NumberOfRvaAndSizes);

	printfScreen(screen, "\nData Directory\n");
	for ( i=0; i < optionalHeader->NumberOfRvaAndSizes; i++)
	{
		printfScreen(screen,  "  rva: %08X  size: %08X  %s\n",
			optionalHeader->DataDirectory[i].VirtualAddress,
			optionalHeader->DataDirectory[i].Size,
			(i >= NUMBER_IMAGE_DIRECTORY_ENTRYS)
			? "unused" : ImageDirectoryNames[i]);
	}
}

// Bitfield values and names for the IMAGE_SECTION_HEADER flags
DWORD_FLAG_DESCRIPTIONS SectionCharacteristics[] =
{
{ IMAGE_SCN_CNT_CODE, "CODE" },
{ IMAGE_SCN_CNT_INITIALIZED_DATA, "INITIALIZED_DATA" },
{ IMAGE_SCN_CNT_UNINITIALIZED_DATA, "UNINITIALIZED_DATA" },
{ IMAGE_SCN_LNK_INFO, "LNK_INFO" },
{ IMAGE_SCN_LNK_OVERLAY, "LNK_OVERLAY" },
{ IMAGE_SCN_LNK_REMOVE, "LNK_REMOVE" },
{ IMAGE_SCN_LNK_COMDAT, "LNK_COMDAT" },
{ IMAGE_SCN_MEM_DISCARDABLE, "MEM_DISCARDABLE" },
{ IMAGE_SCN_MEM_NOT_CACHED, "MEM_NOT_CACHED" },
{ IMAGE_SCN_MEM_NOT_PAGED, "MEM_NOT_PAGED" },
{ IMAGE_SCN_MEM_SHARED, "MEM_SHARED" },
{ IMAGE_SCN_MEM_EXECUTE, "MEM_EXECUTE" },
{ IMAGE_SCN_MEM_READ, "MEM_READ" },
{ IMAGE_SCN_MEM_WRITE, "MEM_WRITE" },
};

#define NUMBER_SECTION_CHARACTERISTICS  (sizeof(SectionCharacteristics) / sizeof(DWORD_FLAG_DESCRIPTIONS))

//
// Dump the section table from a PE file or an OBJ
//
void DumpSectionTable(SCREEN *screen, PIMAGE_SECTION_HEADER section,
					  unsigned cSections,
					  BOOL IsEXE)
{
	unsigned i, j;

	printfScreen(screen, "Section Table\n");
	
	for ( i=1; i <= cSections; i++, section++ )
	{
		printfScreen(screen,  "  %02X %s  %s: %08X  VirtAddr: %08X\n",
				i, section->Name,
				IsEXE ? "VirtSize" : "PhysAddr",
				section->Misc.VirtualSize, section->VirtualAddress);
		printfScreen(screen,  "    raw data offs:   %08X  raw data size: %08X\n",
				section->PointerToRawData, section->SizeOfRawData );
		printfScreen(screen,  "    relocation offs: %08X  relocations:   %08X\n",
				section->PointerToRelocations, section->NumberOfRelocations );
		printfScreen(screen,  "    line # offs:     %08X  line #'s:      %08X\n",
				section->PointerToLinenumbers, section->NumberOfLinenumbers );
		printfScreen(screen,  "    characteristics: %08X\n", section->Characteristics);

		printfScreen(screen, "    ");
		for ( j=0; j < NUMBER_SECTION_CHARACTERISTICS; j++ )
		{
			if ( section->Characteristics & 
				SectionCharacteristics[j].flag )
				printfScreen(screen,  "  %s", SectionCharacteristics[j].name );
		}
		printfScreen(screen, "\n\n");
	}
}

//
// Used by the DumpSymbolTable() routine.  It purpose is to filter out
// the non-normal section numbers and give them meaningful names.
//
void GetSectionName(WORD section, PSTR buffer, unsigned cbBuffer)
{
	char tempbuffer[10];
	
	switch ( (SHORT)section )
	{
		case IMAGE_SYM_UNDEFINED: strcpy(tempbuffer, "UNDEF"); break;
		case IMAGE_SYM_ABSOLUTE:  strcpy(tempbuffer, "ABS  "); break;
		case IMAGE_SYM_DEBUG:	  strcpy(tempbuffer, "DEBUG"); break;
		default: sprintf(tempbuffer, "%5X", section);
	}

	strncpy(buffer, tempbuffer, cbBuffer-1);
}

// The names of the first group of possible symbol table storage classes
char * SzStorageClass1[] = {
"NULL","AUTOMATIC","EXTERNAL","STATIC","REGISTER","EXTERNAL_DEF","LABEL",
"UNDEFINED_LABEL","MEMBER_OF_STRUCT","ARGUMENT","STRUCT_TAG",
"MEMBER_OF_UNION","UNION_TAG","TYPE_DEFINITION","UNDEFINED_STATIC",
"ENUM_TAG","MEMBER_OF_ENUM","REGISTER_PARAM","BIT_FIELD"
};

// The names of the second group of possible symbol table storage classes
char * SzStorageClass2[] = {
"BLOCK","FUNCTION","END_OF_STRUCT","FILE","SECTION","WEAK_EXTERNAL"
};

//
// Given a symbol storage class value, return a descriptive ASCII string
//
PSTR GetSZStorageClass(BYTE storageClass)
{
	if ( storageClass <= IMAGE_SYM_CLASS_BIT_FIELD )
		return SzStorageClass1[storageClass];
	else if ( (storageClass >= IMAGE_SYM_CLASS_BLOCK)
		      && (storageClass <= IMAGE_SYM_CLASS_WEAK_EXTERNAL) )
		return SzStorageClass2[storageClass-IMAGE_SYM_CLASS_BLOCK];
	else
		return "???";
}

//
// Dumps the auxillary symbol for a regular symbol.  It takes a pointer
// to the regular symbol, since the routine needs the information in
// that record.
//
void DumpAuxSymbols(SCREEN *screen, PIMAGE_SYMBOL pSymbolTable)
{
	PIMAGE_AUX_SYMBOL auxSym;
	
	auxSym = (PIMAGE_AUX_SYMBOL)(pSymbolTable+1);
	
	if ( pSymbolTable->StorageClass == IMAGE_SYM_CLASS_FILE )
		printfScreen(screen, "     * %s\n", auxSym);
	else if ( (pSymbolTable->StorageClass == IMAGE_SYM_CLASS_EXTERNAL) )
	{
		if ( (pSymbolTable->Type & 0xF0) == (IMAGE_SYM_DTYPE_FUNCTION << 4))
		{	
		printfScreen(screen, "     * tag: %04X  size: %04X  Line #'s: %08X  next fn: %04X\n",
			auxSym->Sym.TagIndex, auxSym->Sym.Misc.TotalSize,
			auxSym->Sym.FcnAry.Function.PointerToLinenumber,
			auxSym->Sym.FcnAry.Function.PointerToNextFunction);
		}
	}
	else if ( (pSymbolTable->StorageClass == IMAGE_SYM_CLASS_STATIC) )
	{
		printfScreen(screen,
			"     * Section: %04X  Len: %05X  Relocs: %04X  LineNums: %04X\n",
			auxSym->Section.Number, auxSym->Section.Length,
			auxSym->Section.NumberOfRelocations,
			auxSym->Section.NumberOfLinenumbers);
	}
}

//
// Given a COFF symbol table index, look up its name.  This function assumes
// that the COFFSymbolCount and PCOFFSymbolTable variables have been already
// set up.
//
BOOL LookupSymbolName(DWORD index, PSTR buffer, UINT length)
{
	PSTR stringTable;

	if ( index >= COFFSymbolCount )
		return FALSE;
	
	if ( PCOFFSymbolTable == 0 )
		return FALSE;
	
	if ( PCOFFSymbolTable[index].N.Name.Short != 0 )
	{
		strncpy(buffer, PCOFFSymbolTable[index].N.ShortName, min(8, length));
		buffer[8] = 0;
	}
	else
	{
		stringTable = (PSTR)&PCOFFSymbolTable[COFFSymbolCount]; 
		strncpy(buffer,
				stringTable + PCOFFSymbolTable[index].N.Name.Long, length);
		buffer[length-1] = 0;
	}
	
	return TRUE;
}

//
// Dumps a COFF symbol table from an EXE or OBJ
//
void DumpSymbolTable(SCREEN *screen, PIMAGE_SYMBOL pSymbolTable, unsigned cSymbols)
{
	unsigned i;
	PSTR stringTable;
	char sectionName[10];
	
	printfScreen(screen, "Symbol Table - %X entries  (* = auxillary symbol)\n", cSymbols);

	printfScreen(screen,
    "Indx Name                 Value    Section    cAux  Type    Storage\n"
    "---- -------------------- -------- ---------- ----- ------- --------\n");

	// The string table apparently starts right after the symbol table
	stringTable = (PSTR)&pSymbolTable[cSymbols]; 
		
	for ( i=0; i < cSymbols; i++ )
	{
		printfScreen(screen, "%04X ", i);
		
		if ( pSymbolTable->N.Name.Short != 0 )
			printfScreen(screen, "%-20.8s", pSymbolTable->N.ShortName);
		else
			printfScreen(screen, "%-20s", stringTable + pSymbolTable->N.Name.Long);
		
		printfScreen(screen, " %08X", pSymbolTable->Value);
	
		GetSectionName(pSymbolTable->SectionNumber, sectionName,
						sizeof(sectionName));
		printfScreen(screen, " sect:%s aux:%X type:%02X st:%s\n",
				sectionName,
				pSymbolTable->NumberOfAuxSymbols,
				pSymbolTable->Type,
				GetSZStorageClass(pSymbolTable->StorageClass) );
		
		if ( pSymbolTable->NumberOfAuxSymbols )
			DumpAuxSymbols(screen, pSymbolTable);

		// Take into account any aux symbols
		i += pSymbolTable->NumberOfAuxSymbols;
		pSymbolTable += pSymbolTable->NumberOfAuxSymbols;
		pSymbolTable++;
	}
}

//
// Given a section name, look it up in the section table and return a
// pointer to the start of its raw data area.
//
LPVOID GetSectionPtr(PSTR name, PIMAGE_NT_HEADERS pNTHeader, DWORD imageBase)
{
	PIMAGE_SECTION_HEADER section;
	unsigned i;
	
	section = (PIMAGE_SECTION_HEADER)(pNTHeader+1);
	
	for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++ )
	{
		if ( strnicmp(section->Name, name, IMAGE_SIZEOF_SHORT_NAME) == 0 )
			return (LPVOID)(section->PointerToRawData + imageBase);
	}
	
	return 0;
}

//
// Given a section name, look it up in the section table and return a
// pointer to its IMAGE_SECTION_HEADER
//
PIMAGE_SECTION_HEADER GetSectionHeader(PSTR name, PIMAGE_NT_HEADERS pNTHeader)
{
	PIMAGE_SECTION_HEADER section;
	unsigned i;
	
	section = (PIMAGE_SECTION_HEADER)(pNTHeader+1);
	
	for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++ )
	{
		if ( strnicmp(section->Name, name, IMAGE_SIZEOF_SHORT_NAME) == 0 )
			return section;
	}
	
	return 0;
}

//
// Do a hexadecimal dump of the raw data for all the sections.  You
// could just dump one section by adjusting the PIMAGE_SECTION_HEADER
// and cSections parameters
//
void DumpRawSectionData(SCREEN *screen, PIMAGE_SECTION_HEADER section,
						PVOID base,
						unsigned cSections)
{
	unsigned i;
	char name[IMAGE_SIZEOF_SHORT_NAME + 1];

	printfScreen(screen, "Section Hex Dumps\n");
	
	for ( i=0; i < cSections; i++, section++ )
	{
		// Make a copy of the section name so that we can ensure that
		// it's null-terminated
		strncpy(name, section->Name, IMAGE_SIZEOF_SHORT_NAME);
		name[IMAGE_SIZEOF_SHORT_NAME] = 0;

		// Don't dump sections that don't exist in the file!
		if ( section->PointerToRawData == 0 )
			continue;
		
		printfScreen(screen,  "section %02X (%s)  size: %08X  file offs: %08X\n",
				i, name, section->SizeOfRawData, section->PointerToRawData);

		HexDump( MakePtr(PBYTE, base, section->PointerToRawData),
				 section->SizeOfRawData );
		printfScreen(screen, "\n");
	}
}

//
// Dump a range of line numbers from the COFF debug information
//
void DumpLineNumbers(SCREEN *screen, PIMAGE_LINENUMBER pln, DWORD count)
{
	char buffer[64];
	DWORD i;
	
	printfScreen(screen, "Line Numbers\n");
	
	for (i=0; i < count; i++)
	{
		if ( pln->Linenumber == 0 )	// A symbol table index
		{
			buffer[0] = 0;
			LookupSymbolName(pln->Type.SymbolTableIndex, buffer,
							sizeof(buffer));
			printfScreen(screen, "SymIndex: %X (%s)\n", pln->Type.SymbolTableIndex,
											 buffer);
		}
		else		// A regular line number
			printfScreen(screen, " Addr: %05X  Line: %04X\n",
				pln->Type.VirtualAddress, pln->Linenumber);
		pln++;
	}
}

// Number of hex values displayed per line
#define HEX_DUMP_WIDTH 16

//
// Dump a region of memory in a hexadecimal format
//
void HexDump(PBYTE ptr, DWORD length)
{
	char buffer[256];
	PSTR buffPtr;
	unsigned cOutput, i;
	DWORD bytesToGo=length;

	while ( bytesToGo  )
	{
		cOutput = bytesToGo >= HEX_DUMP_WIDTH ? HEX_DUMP_WIDTH : bytesToGo;

		buffPtr = buffer;
		buffPtr += sprintf(buffPtr, "%08X:  ", length-bytesToGo );
		
		for ( i=0; i < HEX_DUMP_WIDTH; i++ )
		{
			buffPtr += sprintf(buffPtr,
				i < cOutput ? "%02X " : "   ",
				*(ptr+i) );

			// Put an extra space between the 1st and 2nd half of the bytes
			// on each line.
			if ( i == (HEX_DUMP_WIDTH/2)-1 )
				buffPtr += sprintf(buffPtr, " ");
		}

		for ( i=0; i < cOutput; i++ )
		{
			char c;
			
			c = '.';
			if ( isascii(*(ptr + i)) )
				c = isprint(*(ptr + i)) ? *(ptr + i) : '.';
			
			buffPtr += sprintf(buffPtr, "%c", c);
		}

		bytesToGo -= cOutput;
		ptr += HEX_DUMP_WIDTH;
	}
}
