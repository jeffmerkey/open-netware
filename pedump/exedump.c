

#include <stdio.h>
#include <dos.h>
#include <bios.h>
#include <stdlib.h>
#include <conio.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>


#define TRUE 1
#define FALSE 0

typedef unsigned char BYTE;
typedef unsigned int WORD;
typedef unsigned int UINT;
typedef unsigned long LONG;
typedef unsigned long DWORD;
typedef short SHORT;
typedef int BOOL;
typedef BYTE far* LPBYTE;
typedef BYTE * PBYTE;
typedef char * PSTR;
typedef char far * LPSTR;
typedef void * PVOID;
typedef void far* LPVOID;
typedef DWORD far* LPDWORD;
typedef WORD far* LPWORD;
typedef char CHAR;
typedef unsigned int WCHAR;
typedef int INT;
typedef WORD * PWORD;
typedef WORD far * LPWORD;
typedef DWORD * PDWORD;
typedef DWORD far * LPDWORD;

#define VOID void

#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD)(ptr) + (addValue) )


#include "peexe.h"
#include "extrnvar.h"
#include "neexe.h"

typedef PIMAGE_COFF_SYMBOLS_HEADER PIMAGE_DEBUG_INFO;

PIMAGE_DEBUG_INFO PCOFFDebugInfo = 0;

char *SzDebugFormats[] = {
"UNKNOWN/BORLAND","COFF","CODEVIEW","FPO","MISC","EXCEPTION","FIXUP" };

void DumpHeader(PIMAGE_FILE_HEADER pImageFileHeader);
void DumpOptionalHeader(PIMAGE_OPTIONAL_HEADER optionalHeader);
void DumpSectionTable(PIMAGE_SECTION_HEADER section, unsigned cSections, BOOL IsEXE);
void GetSectionName(WORD section, PSTR buffer, unsigned cbBuffer);
PSTR GetSZStorageClass(BYTE storageClass);
void DumpAuxSymbols(PIMAGE_SYMBOL pSymbolTable);
BOOL LookupSymbolName(DWORD index, PSTR buffer, UINT length);
void DumpSymbolTable(PIMAGE_SYMBOL pSymbolTable, unsigned cSymbols);
LPVOID GetSectionPtr(PSTR name, PIMAGE_NT_HEADERS pNTHeader, DWORD imageBase);
PIMAGE_SECTION_HEADER GetSectionHeader(PSTR name, PIMAGE_NT_HEADERS pNTHeader);
void DumpRawSectionData(PIMAGE_SECTION_HEADER section, PVOID base, unsigned cSections);
void DumpLineNumbers(PIMAGE_LINENUMBER pln, DWORD count);
void HexDump(PBYTE ptr, DWORD length);
WideCharToMultiByte();
BOOL IsBadReadPtr(void far* lp, UINT cb);

//
// Dump the debug directory in a PE file.
//
void DumpDebugDirectory(DWORD base, PIMAGE_NT_HEADERS pNTHeader)
{
	PIMAGE_DEBUG_DIRECTORY debugDir;
	PIMAGE_SECTION_HEADER header;
	unsigned cDebugFormats, i;
	DWORD offsetInto_rdata;
	DWORD va_debug_dir;
	PSTR szDebugFormat;

	// This line was so long that we had to break it up
	va_debug_dir = pNTHeader->OptionalHeader.
						DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].
						VirtualAddress;
	if ( va_debug_dir == 0 )
		return;

	// If we found a .debug section, and the debug directory is at the
	// beginning of this section, it looks like a Borland file
	header = GetSectionHeader(".debug", pNTHeader);
	if ( header && (header->VirtualAddress == va_debug_dir) )
	{
		debugDir = (PIMAGE_DEBUG_DIRECTORY)(header->PointerToRawData+base);
		cDebugFormats = pNTHeader->OptionalHeader.
							DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
	}
	else	// Look for microsoft debug directory in the .rdata section
	{
		header = GetSectionHeader(".rdata", pNTHeader);
		if ( !header )
			return;

		// See if there's even any debug directories to speak of...
		cDebugFormats = pNTHeader->OptionalHeader.
							DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size
						/ sizeof(IMAGE_DEBUG_DIRECTORY);
		if ( cDebugFormats == 0 )
			return;
	
		offsetInto_rdata = va_debug_dir - header->VirtualAddress;
		debugDir = MakePtr(PIMAGE_DEBUG_DIRECTORY, base,
							header->PointerToRawData + offsetInto_rdata);
	}
    	
	printf(
	"Debug Formats in File\n"
    "  Type            Size     Address  FilePtr  Charactr TimeData Version\n"
    "  --------------- -------- -------- -------- -------- -------- --------\n"
	);
	
	for ( i=0; i < cDebugFormats; i++ )
	{
		szDebugFormat = (debugDir->Type <= 6)
						? SzDebugFormats[debugDir->Type] : "???";

		printf("  %-15s %08X %08X %08X %08X %08X %u.%02u\n",
			szDebugFormat, debugDir->SizeOfData, debugDir->AddressOfRawData,
			debugDir->PointerToRawData, debugDir->Characteristics,
			debugDir->TimeDateStamp, debugDir->MajorVersion,
			debugDir->MinorVersion);

		// If COFF debug info, save its address away for later.  We
		// do the check for "PointerToSymbolTable" because some files
		// have bogus values for the COFF header offset.
		if ( (debugDir->Type == IMAGE_DEBUG_TYPE_COFF) &&
		     pNTHeader->FileHeader.PointerToSymbolTable )
		{
			PCOFFDebugInfo =
				(PIMAGE_DEBUG_INFO)(base+ debugDir->PointerToRawData);
		}
		
		debugDir++;
	}
}

// Function prototype (necessary because two functions recurse)
void DumpResourceDirectory
(
	PIMAGE_RESOURCE_DIRECTORY resDir, DWORD resourceBase,
	DWORD level, DWORD resourceType
);

// The predefined resource types
char *SzResourceTypes[] = {
"???_0", "CURSOR", "BITMAP", "ICON", "MENU", "DIALOG", "STRING", "FONTDIR",
"FONT", "ACCELERATORS", "RCDATA", "MESSAGETABLE", "GROUP_CURSOR",
"???_13", "GROUP_ICON", "???_15", "VERSION"
};

// Get an ASCII string representing a resource type
void GetResourceTypeName(DWORD type, PSTR buffer, UINT cBytes)
{
	if ( type <= 16 )
		strncpy(buffer, SzResourceTypes[type], cBytes);
	else
		sprintf(buffer, "%X", type);
}

//
// If a resource entry has a string name (rather than an ID), go find
// the string and convert it from unicode to ascii.
//
void GetResourceNameFromId
(
	DWORD id, DWORD resourceBase, PSTR buffer, UINT cBytes
)
{
	PIMAGE_RESOURCE_DIR_STRING_U prdsu;

	// If it's a regular ID, just format it.
	if ( !(id & IMAGE_RESOURCE_NAME_IS_STRING) )
	{
		sprintf(buffer, "%X", id);
		return;
	}
	
	id &= 0x7FFFFFFF;
	prdsu = (PIMAGE_RESOURCE_DIR_STRING_U)(resourceBase + id);

//	// prdsu->Length is the number of unicode characters
//	WideCharToMultiByte(CP_ACP, 0, prdsu->NameString, prdsu->Length,
//						buffer, cBytes,	0, 0);

	buffer[ min(cBytes-1,prdsu->Length) ] = 0;	// Null terminate it!!!

}

//
// Dump the information about one resource directory entry.  If the
// entry is for a subdirectory, call the directory dumping routine
// instead of printing information in this routine.
//
void DumpResourceEntry
(
	PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry,
	DWORD resourceBase,
	DWORD level
)
{
	UINT i;
	char nameBuffer[128];
		
	if ( resDirEntry->u2.OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY )
	{
		DumpResourceDirectory( (PIMAGE_RESOURCE_DIRECTORY)
			((resDirEntry->u2.OffsetToData & 0x7FFFFFFF) + resourceBase),
			resourceBase, level, resDirEntry->u1.Name);
		return;
	}

	// Spit out the spacing for the level indentation
	for ( i=0; i < level; i++ )
		printf("    ");

	if ( resDirEntry->u1.Name & IMAGE_RESOURCE_NAME_IS_STRING )
	{
		GetResourceNameFromId(resDirEntry->u1.Name, resourceBase, nameBuffer,
							  sizeof(nameBuffer));
		printf("Name: %s  Offset: %08X\n",
			nameBuffer, resDirEntry->u2.OffsetToData);
	}
	else
	{
		printf("ID: %08X  Offset: %08X\n",
				resDirEntry->u1.Name, resDirEntry->u2.OffsetToData);
	}
}

//
// Dump the information about one resource directory.
//
void DumpResourceDirectory
(
	PIMAGE_RESOURCE_DIRECTORY resDir,
	DWORD resourceBase,
	DWORD level,
	DWORD resourceType
)
{
	PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry;
	char szType[64];
	UINT i;

	// Spit out the spacing for the level indentation
	for ( i=0; i < level; i++ )
		printf("    ");

	// Level 1 resources are the resource types
	if ( level == 1 && !(resourceType & IMAGE_RESOURCE_NAME_IS_STRING) )
	{
		GetResourceTypeName( resourceType, szType, sizeof(szType) );
	}
	else	// Just print out the regular id or name
	{
		GetResourceNameFromId( resourceType, resourceBase, szType,
							   sizeof(szType) );
	}
	
	printf(
		"ResDir (%s) Named:%02X ID:%02X TimeDate:%08X Vers:%u.%02u Char:%X\n",
		szType,	resDir->NumberOfNamedEntries, resDir->NumberOfIdEntries,
		resDir->TimeDateStamp, resDir->MajorVersion,
		resDir->MinorVersion,resDir->Characteristics);

	resDirEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resDir+1);
	
	for ( i=0; i < resDir->NumberOfNamedEntries; i++, resDirEntry++ )
		DumpResourceEntry(resDirEntry, resourceBase, level+1);

	for ( i=0; i < resDir->NumberOfIdEntries; i++, resDirEntry++ )
		DumpResourceEntry(resDirEntry, resourceBase, level+1);
}

//
// Top level routine called to dump out the entire resource hierarchy
//
void DumpResourceSection(DWORD base, PIMAGE_NT_HEADERS pNTHeader)
{
	PIMAGE_RESOURCE_DIRECTORY resDir;
	
	resDir = (PIMAGE_RESOURCE_DIRECTORY) GetSectionPtr(".rsrc", pNTHeader, (DWORD)base);
	if ( !resDir )
		return;
	
 	printf("Resources\n");
	DumpResourceDirectory(resDir, (DWORD)resDir, 0, 0);
}

//
// Dump the imports table (the .idata section) of a PE file
//
void DumpImportsSection(DWORD base, PIMAGE_NT_HEADERS pNTHeader)
{
	PIMAGE_IMPORT_DESCRIPTOR importDesc;
	PIMAGE_SECTION_HEADER header;
	PIMAGE_THUNK_DATA thunk;
	PIMAGE_IMPORT_BY_NAME pOrdinalName;
	DWORD exportsStartRVA, exportsEndRVA;
        IMAGE_DATA_DIRECTORY    dir;
	
	INT delta;

 	printf("Imports Table:\n");
		
	dir=pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	delta = dir.VirtualAddress;
	delta -= 0x200;

	importDesc = MakePtr(PIMAGE_IMPORT_DESCRIPTOR, base, delta);
	
	delta = 0x200;
	
	while ( 1 )
	{
		// See if we've reached an empty IMAGE_IMPORT_DESCRIPTOR
		if ( (importDesc->TimeDateStamp==0 ) && (importDesc->Name==0) )
			break;
		
		printf("  %s\n", (PBYTE)(importDesc->Name) - delta + base);

		printf("  Hint/Name Table: %08X\n", importDesc->u.Characteristics);
 		printf("  TimeDateStamp:   %08X\n", importDesc->TimeDateStamp);
 		printf("  ForwarderChain:  %08X\n", importDesc->ForwarderChain);
 		printf("  First thunk RVA: %08X\n", importDesc->FirstThunk);
		printf("\n");
	
		thunk = (PIMAGE_THUNK_DATA) importDesc->u.Characteristics;
		thunk = (PIMAGE_THUNK_DATA) thunk - delta + base;

		while (1) {
			if (thunk->u1.AddressOfData == 0)
			   break;
			pOrdinalName = thunk->u1.AddressOfData;
			pOrdinalName = (PIMAGE_IMPORT_BY_NAME) pOrdinalName - delta + base;
			printf("    %4.4x %s\n",
				pOrdinalName->Hint,pOrdinalName->Name);

			thunk++;
		}
		importDesc++;	// advance to next IMAGE_IMPORT_DESCRIPTOR
		printf("\n");
	}
}

//
// Dump the exports table (the .edata section) of a PE file
//
void DumpExportsSection(DWORD base, PIMAGE_NT_HEADERS pNTHeader)
{
	PIMAGE_EXPORT_DIRECTORY exportDir;
	PIMAGE_SECTION_HEADER header;
	INT delta; 
	PSTR filename;
	DWORD i;
	PDWORD functions;
	PWORD ordinals;
	PSTR *name;

	header = GetSectionHeader(".edata", pNTHeader);
	if ( !header )
		return;
	exportDir = MakePtr(PIMAGE_EXPORT_DIRECTORY, base,
						 header->PointerToRawData);
	delta = (INT)(header->VirtualAddress - header->PointerToRawData);
	
	filename = (PSTR)(exportDir->Name - delta + base);
		
 	printf("exports table:\n\n");
	printf("  Name:            %s\n", filename);
	printf("  Characteristics: %08X\n", exportDir->Characteristics);
	printf("  TimeDateStamp:   %08X\n", exportDir->TimeDateStamp);
	printf("  Version:         %u.%02u\n", exportDir->MajorVersion,
			exportDir->MinorVersion);
	printf("  Ordinal base:    %08X\n", exportDir->Base);
	printf("  # of functions:  %08X\n", exportDir->NumberOfFunctions);
	printf("  # of Names:      %08X\n", exportDir->NumberOfNames);
	
	functions = (PDWORD)((DWORD)exportDir->AddressOfFunctions - delta + base);
	ordinals = (PWORD)((DWORD)exportDir->AddressOfNameOrdinals - delta + base);
	name = (PSTR *)((DWORD)exportDir->AddressOfNames - delta + base);

	printf("\n  Entry Pt  Ordn  Name\n");
	for ( i=0; i < exportDir->NumberOfNames; i++ )
	{
		printf("  %08X  %4u  %s\n", *functions,
				*ordinals + exportDir->Base,
				(*name - delta + base));
		name++;			// Bump each pointer to the next array element
		ordinals++;
		functions++;
	}
}

// The names of the available base relocations
char *SzRelocTypes[] = {
"ABSOLUTE","HIGH","LOW","HIGHLOW","HIGHADJ","MIPS_JMPADDR",
"I860_BRADDR","I860_SPLIT" };

//
// Dump the base relocation table of a PE file
//
void DumpBaseRelocationsSection(DWORD base, PIMAGE_NT_HEADERS pNTHeader)
{
	PIMAGE_BASE_RELOCATION baseReloc;
	
	baseReloc = (PIMAGE_BASE_RELOCATION) GetSectionPtr(".reloc", pNTHeader, base);
	if ( !baseReloc )
		return;

	printf("base relocations:\n\n");

	while ( baseReloc->SizeOfBlock != 0 )
	{
		unsigned i, cEntries;
		PWORD pEntry;
		char *szRelocType;
		WORD relocType;
		
		cEntries = (baseReloc->SizeOfBlock-sizeof(*baseReloc))/sizeof(WORD);
		pEntry = MakePtr( PWORD, baseReloc, sizeof(*baseReloc) );
		
		printf("Virtual Address: %08X  size: %08X\n",
				baseReloc->VirtualAddress, baseReloc->SizeOfBlock);
			
		for ( i=0; i < cEntries; i++ )
		{
		   // Extract the top 4 bits of the relocation entry.  Turn those 4
		   // bits into an appropriate descriptive string (szRelocType)

		   relocType = (*pEntry & 0xF000) >> 12;
		   szRelocType = (relocType < 8) ? SzRelocTypes[relocType] : "unknown";

		   printf("%s  %08X\n", (char *)szRelocType,
					(DWORD)(*pEntry & 0x0FFF)+baseReloc->VirtualAddress);

		   pEntry++;	// Advance to next relocation entry

		}
		baseReloc = MakePtr( PIMAGE_BASE_RELOCATION, baseReloc,
				     baseReloc->SizeOfBlock);
	}
}

//
// Dump the COFF debug information header
//
void DumpCOFFHeader(PIMAGE_DEBUG_INFO pDbgInfo)
{
	printf("COFF Debug Info Header\n");
	printf("  NumberOfSymbols:      %08X\n", pDbgInfo->NumberOfSymbols);
	printf("  LvaToFirstSymbol:     %08X\n", pDbgInfo->LvaToFirstSymbol);
	printf("  NumberOfLinenumbers:  %08X\n", pDbgInfo->NumberOfLinenumbers);
	printf("  LvaToFirstLinenumber: %08X\n", pDbgInfo->LvaToFirstLinenumber);
	printf("  RvaToFirstByteOfCode: %08X\n", pDbgInfo->RvaToFirstByteOfCode);
	printf("  RvaToLastByteOfCode:  %08X\n", pDbgInfo->RvaToLastByteOfCode);
	printf("  RvaToFirstByteOfData: %08X\n", pDbgInfo->RvaToFirstByteOfData);
	printf("  RvaToLastByteOfData:  %08X\n", pDbgInfo->RvaToLastByteOfData);
}

//
// top level routine called from PEDUMP.C to dump the components of a PE file
//
void DumpExeFile( PIMAGE_DOS_HEADER dosHeader )
{
	PIMAGE_NT_HEADERS pNTHeader;
	DWORD base = (DWORD)dosHeader;
	
	pNTHeader = MakePtr( PIMAGE_NT_HEADERS, dosHeader,
								dosHeader->e_lfanew );

	// First, verify that the e_lfanew field gave us a reasonable
	// pointer, then verify the PE signature.
	if ( IsBadReadPtr(pNTHeader, sizeof(IMAGE_NT_HEADERS)) ||
	     pNTHeader->Signature != IMAGE_NT_SIGNATURE )
	{
		printf("Unhandled EXE type, or invalid .EXE\n");
		return;
	}
	
	DumpHeader((PIMAGE_FILE_HEADER)&pNTHeader->FileHeader);
	printf("\n");

	DumpOptionalHeader((PIMAGE_OPTIONAL_HEADER)&pNTHeader->OptionalHeader);
	printf("\n");

	DumpSectionTable( (PIMAGE_SECTION_HEADER)(pNTHeader+1),
						pNTHeader->FileHeader.NumberOfSections, TRUE);
	printf("\n");

	DumpDebugDirectory(base, pNTHeader);
	printf("\n");

	DumpResourceSection(base, pNTHeader);
	printf("\n");

	DumpImportsSection(base, pNTHeader);
	printf("\n");

	DumpExportsSection(base, pNTHeader);
	printf("\n");

	if ( fShowRelocations )
	{
		DumpBaseRelocationsSection(base, pNTHeader);
		printf("\n");
	}

	//
	// Initialize these vars here since we'll need them in DumpLineNumbers
	//
	PCOFFSymbolTable = MakePtr(PIMAGE_SYMBOL, base,
						pNTHeader->FileHeader.PointerToSymbolTable);
	COFFSymbolCount = pNTHeader->FileHeader.NumberOfSymbols;

	if ( fShowSymbolTable && PCOFFDebugInfo )
	{
		DumpCOFFHeader( PCOFFDebugInfo );
		printf("\n");
	}
	
	if ( fShowLineNumbers && PCOFFDebugInfo )
	{
		DumpLineNumbers( MakePtr(PIMAGE_LINENUMBER, PCOFFDebugInfo,
							PCOFFDebugInfo->LvaToFirstLinenumber),
							PCOFFDebugInfo->NumberOfLinenumbers);
		printf("\n");
	}

	if ( fShowSymbolTable )
	{
		if ( pNTHeader->FileHeader.NumberOfSymbols 
			&& pNTHeader->FileHeader.PointerToSymbolTable)
		{
			DumpSymbolTable(PCOFFSymbolTable, COFFSymbolCount);
			printf("\n");
		}
	}
	
	if ( fShowRawSectionData )
	{
		DumpRawSectionData( (PIMAGE_SECTION_HEADER)(pNTHeader+1),
							dosHeader,
							pNTHeader->FileHeader.NumberOfSections);
	}

}
