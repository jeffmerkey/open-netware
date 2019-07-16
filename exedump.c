

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  EXEDUMP.C
*   DESCRIP  :  PE EXE Support for MANOS v1.0
*   DATE     :  January 10, 1998
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

PIMAGE_DEBUG_INFO PCOFFDebugInfo = 0;
char *SzDebugFormats[] = {
"BORLAND","COFF","CODEVIEW","FPO","MISC","EXCEPTION","FIXUP" };

//
// Dump the debug directory in a PE file.
//
void DumpDebugDirectory(SCREEN *screen, DWORD base, PIMAGE_NT_HEADERS pNTHeader)
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
    	
	printfScreen(screen, "Debug Formats in File\n");
	printfScreen(screen, "  Type            Size     Address  FilePtr  Charactr TimeData Version\n");
	printfScreen(screen, "  --------------- -------- -------- -------- -------- -------- --------\n");

	for ( i=0; i < cDebugFormats; i++ )
	{
		szDebugFormat = (debugDir->Type <= 6)
						? SzDebugFormats[debugDir->Type] : "???";
		printfScreen(screen, "  %s %08X %08X %08X %08X %08X %u.%02u\n",
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
void DumpResourceDirectory(SCREEN *screen, PIMAGE_RESOURCE_DIRECTORY resDir, DWORD resourceBase,
	DWORD level, DWORD resourceType);

// The predefined resource types
char *SzResourceTypes[] =
{
   "???_0",
   "CURSOR",
   "BITMAP",
   "ICON",
   "MENU",
   "DIALOG",
   "STRING",
   "FONTDIR",
   "FONT",
   "ACCELERATORS",
   "RCDATA",
   "MESSAGETABLE",
   "GROUP_CURSOR",
   "???_13",
   "GROUP_ICON",
   "???_15",
   "VERSION"
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
void GetResourceNameFromId(DWORD id, DWORD resourceBase, PSTR buffer,
			   UINT cBytes)
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
void DumpResourceEntry(SCREEN *screen, PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry,
			DWORD resourceBase,
			DWORD level)
{
	UINT i;
	char nameBuffer[128];
		
	if ( resDirEntry->u2.OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY )
	{
		DumpResourceDirectory( screen, (PIMAGE_RESOURCE_DIRECTORY)
			((resDirEntry->u2.OffsetToData & 0x7FFFFFFF) + resourceBase),
			resourceBase, level, resDirEntry->u1.Name);
		return;
	}

	// Spit out the spacing for the level indentation
	for ( i=0; i < level; i++ )
		printfScreen(screen, "    ");

	if ( resDirEntry->u1.Name & IMAGE_RESOURCE_NAME_IS_STRING )
	{
		GetResourceNameFromId(resDirEntry->u1.Name, resourceBase, nameBuffer,
							  sizeof(nameBuffer));
		printfScreen(screen, "Name: %s  Offset: %08X\n",
			nameBuffer, resDirEntry->u2.OffsetToData);
	}
	else
	{
		printfScreen(screen, "ID: %08X  Offset: %08X\n",
				resDirEntry->u1.Name, resDirEntry->u2.OffsetToData);
	}
}

//
// Dump the information about one resource directory.
//
void DumpResourceDirectory(SCREEN *screen, PIMAGE_RESOURCE_DIRECTORY resDir,
	DWORD resourceBase,
	DWORD level,
	DWORD resourceType)
{
	PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry;
	char szType[64];
	UINT i;

	// Spit out the spacing for the level indentation
	for ( i=0; i < level; i++ )
	   printfScreen(screen, "    ");

	// Level 1 resources are the resource types
	if ( level == 1 && !(resourceType & IMAGE_RESOURCE_NAME_IS_STRING) )
	{
	   GetResourceTypeName( resourceType, szType, sizeof(szType));
	}
	else	// Just print out the regular id or name
	{
	   GetResourceNameFromId( resourceType, resourceBase, szType, sizeof(szType));
	}
	
	printfScreen(screen,
		     "ResDir (%s) Named:%02X ID:%02X TimeDate:%08X Vers:%u.%02u Char:%X\n",
		     szType,
		     resDir->NumberOfNamedEntries,
		     resDir->NumberOfIdEntries,
		     resDir->TimeDateStamp, resDir->MajorVersion,
		     resDir->MinorVersion,resDir->Characteristics);

	resDirEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resDir+1);

	for ( i=0; i < resDir->NumberOfNamedEntries; i++, resDirEntry++ )
	   DumpResourceEntry(screen, resDirEntry, resourceBase, level+1);

	for ( i=0; i < resDir->NumberOfIdEntries; i++, resDirEntry++ )
	   DumpResourceEntry(screen, resDirEntry, resourceBase, level+1);
}

//
// Top level routine called to dump out the entire resource hierarchy
//
void DumpResourceSection(SCREEN *screen, DWORD base, PIMAGE_NT_HEADERS pNTHeader)
{
	PIMAGE_RESOURCE_DIRECTORY resDir;
	
	resDir = (PIMAGE_RESOURCE_DIRECTORY) GetSectionPtr(".rsrc", pNTHeader, (DWORD)base);
	if ( !resDir )
		return;

	printfScreen(screen, "Resources\n");
	DumpResourceDirectory(screen, resDir, (DWORD)resDir, 0, 0);
}

#define DEBUG_IMPORT_DUMP  1

void DumpImportsSection(SCREEN *screen, DWORD base, PIMAGE_NT_HEADERS pNTHeader)
{
    PIMAGE_SECTION_HEADER header;
    PIMAGE_IMPORT_DESCRIPTOR importDesc;
    PIMAGE_THUNK_DATA ImportNameTable;
    PIMAGE_IMPORT_BY_NAME pOrdinalName;
    PBYTE moduleName;
    IMAGE_DATA_DIRECTORY ImageDirectory;
    LONG delta = 0;

    header = GetSectionHeader(".idata", pNTHeader);
    if (!header)
    {
       ImageDirectory = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
       if (!ImageDirectory.Size)
	  return;
       importDesc = MakePtr(PIMAGE_IMPORT_DESCRIPTOR, base, ImageDirectory.VirtualAddress);
    }
    else
    {
       importDesc = MakePtr(PIMAGE_IMPORT_DESCRIPTOR, base, header->PointerToRawData);
       delta = (LONG)(header->VirtualAddress - header->PointerToRawData);
    }

    printfScreen(screen, "Imports Table:\n");
    while (importDesc->Name != 0)
    {
       if (importDesc->u.Characteristics)
       {
	  ImportNameTable = (PIMAGE_THUNK_DATA)((LONG)importDesc->u.Characteristics - delta + base);
	  moduleName = (PBYTE)(importDesc->Name) - delta + base;
	  printfScreen(screen, "\n%s\n", moduleName);
	  printfScreen(screen, "Import Table    : %08X\n", ImportNameTable);
	  while (ImportNameTable->u1.AddressOfData != 0)
	  {
	     if (ImportNameTable->u1.Ordinal & 0x80000000)
	     {
		printfScreen(screen, "\n[%08X] ordinal %s|%04X.%04X",
		       (LONG)ImportNameTable - (LONG)base + (LONG)delta,
		       (BYTE *)(LONG)(importDesc->Name) - delta + base,
		       (LONG)(ImportNameTable->u1.Ordinal >> 16) & 0xFFFF,
		       (LONG)ImportNameTable->u1.Ordinal & 0xFFFF);
	     }
	     else
	     {
		pOrdinalName = (PIMAGE_IMPORT_BY_NAME)((LONG)ImportNameTable->u1.AddressOfData - delta + base);
		printfScreen(screen, "[%08X] ordinal %04x %s\n",
			(LONG)ImportNameTable - (LONG)base + (LONG)delta,
			pOrdinalName->Hint,
			pOrdinalName->Name);
	     }
	     ImportNameTable++;
	  }
       }
       else if (importDesc->FirstThunk)
       {
	  ImportNameTable = (PIMAGE_THUNK_DATA)((LONG)importDesc->FirstThunk - delta + base);
	  moduleName = (PBYTE)(importDesc->Name) - delta + base;
	  printfScreen(screen, "\n%s\n", moduleName);
	  printfScreen(screen, "Import Table    : %08X\n", ImportNameTable);
	  while (ImportNameTable->u1.AddressOfData != 0)
	  {
	     if (ImportNameTable->u1.Ordinal & 0x80000000)
	     {
		printfScreen(screen, "\n[%08X] ordinal %s|%04X.%04X",
			    (LONG)ImportNameTable - (LONG)base + (LONG)delta,
			    (BYTE *)((LONG)importDesc->Name) - delta + base,
			    (LONG)(ImportNameTable->u1.Ordinal >> 16) & 0xFFFF,
			    (LONG)ImportNameTable->u1.Ordinal & 0xFFFF);
	     }
	     else
	     {
		pOrdinalName = (PIMAGE_IMPORT_BY_NAME)((LONG)ImportNameTable->u1.AddressOfData - delta + base);
		printfScreen(screen, "[%08X] ordinal %04x %s\n",
			    (LONG)ImportNameTable - (LONG)base + (LONG)delta,
			    pOrdinalName->Hint,
			    pOrdinalName->Name);
	     }
	     ImportNameTable++;
	  }
       }
       importDesc++;
    }

}

//
// Dump the exports table (the .edata section) of a PE file
//

void DumpExportsSection(SCREEN *screen, DWORD base, PIMAGE_NT_HEADERS pNTHeader)
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
	if (!header)
	   return;

	exportDir = MakePtr(PIMAGE_EXPORT_DIRECTORY, base, header->PointerToRawData);
	delta = (INT)(header->VirtualAddress - header->PointerToRawData);
	filename = (PSTR)(exportDir->Name - delta + base);

	printfScreen(screen, "exports table:\n\n");
	printfScreen(screen, "  Name:            %s\n", filename);
	printfScreen(screen, "  Characteristics: %08X\n", exportDir->Characteristics);
	printfScreen(screen, "  TimeDateStamp:   %08X\n", exportDir->TimeDateStamp);
	printfScreen(screen, "  Version:         %u.%02u\n", exportDir->MajorVersion,
			exportDir->MinorVersion);
	printfScreen(screen, "  Ordinal base:    %08X\n", exportDir->Base);
	printfScreen(screen, "  # of functions:  %08X\n", exportDir->NumberOfFunctions);
	printfScreen(screen, "  # of Names:      %08X\n", exportDir->NumberOfNames);
	
	functions = (PDWORD)((DWORD)exportDir->AddressOfFunctions - delta + base);
	ordinals = (PWORD)((DWORD)exportDir->AddressOfNameOrdinals - delta + base);
	name = (PSTR *)((DWORD)exportDir->AddressOfNames - delta + base);

	printfScreen(screen, "\n  Entry Pt  Ordn  Name\n");
	for ( i=0; i < exportDir->NumberOfNames; i++ )
	{
		printfScreen(screen, "  %08X  %4u  %s\n", *functions, *ordinals + exportDir->Base,
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
void DumpBaseRelocationsSection(SCREEN *screen, DWORD base, PIMAGE_NT_HEADERS pNTHeader)
{
	PIMAGE_BASE_RELOCATION baseReloc;
	
	baseReloc = (PIMAGE_BASE_RELOCATION) GetSectionPtr(".reloc", pNTHeader, base);
	if ( !baseReloc )
		return;

	printfScreen(screen, "base relocations:\n\n");

	while ( baseReloc->SizeOfBlock != 0 )
	{
		unsigned i, cEntries;
		PWORD pEntry;
		char *szRelocType;
		WORD relocType;
		
		cEntries = (baseReloc->SizeOfBlock-sizeof(*baseReloc))/sizeof(WORD);
		pEntry = MakePtr( PWORD, baseReloc, sizeof(*baseReloc) );
		
		printfScreen(screen, "Virtual Address: %08X  size: %08X\n",
				baseReloc->VirtualAddress, baseReloc->SizeOfBlock);
			
		for ( i=0; i < cEntries; i++ )
		{
		   // Extract the top 4 bits of the relocation entry.  Turn those 4
		   // bits into an appropriate descriptive string (szRelocType)

		   relocType = (*pEntry & 0xF000) >> 12;
		   szRelocType = (relocType < 8) ? SzRelocTypes[relocType] : "unknown";

		   printfScreen(screen, "%s  %08X\n", (char *)szRelocType,
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
void DumpCOFFHeader(SCREEN *screen, PIMAGE_DEBUG_INFO pDbgInfo)
{
	printfScreen(screen, "COFF Debug Info Header\n");
	printfScreen(screen, "  NumberOfSymbols:      %08X\n", pDbgInfo->NumberOfSymbols);
	printfScreen(screen, "  LvaToFirstSymbol:     %08X\n", pDbgInfo->LvaToFirstSymbol);
	printfScreen(screen, "  NumberOfLinenumbers:  %08X\n", pDbgInfo->NumberOfLinenumbers);
	printfScreen(screen, "  LvaToFirstLinenumber: %08X\n", pDbgInfo->LvaToFirstLinenumber);
	printfScreen(screen, "  RvaToFirstByteOfCode: %08X\n", pDbgInfo->RvaToFirstByteOfCode);
	printfScreen(screen, "  RvaToLastByteOfCode:  %08X\n", pDbgInfo->RvaToLastByteOfCode);
	printfScreen(screen, "  RvaToFirstByteOfData: %08X\n", pDbgInfo->RvaToFirstByteOfData);
	printfScreen(screen, "  RvaToLastByteOfData:  %08X\n", pDbgInfo->RvaToLastByteOfData);
}

//
// top level routine called from PEDUMP.C to dump the components of a PE file
//
void DumpExeFile( SCREEN *screen, PIMAGE_DOS_HEADER dosHeader )
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
		printfScreen(screen, "Unhandled EXE type, or invalid .EXE\n");
		return;
	}
	
	DumpHeader(screen, (PIMAGE_FILE_HEADER)&pNTHeader->FileHeader);
	printfScreen(screen, "\n");

	DumpOptionalHeader(screen, (PIMAGE_OPTIONAL_HEADER)&pNTHeader->OptionalHeader);
	printfScreen(screen, "\n");

	DumpSectionTable(screen,  (PIMAGE_SECTION_HEADER)(pNTHeader+1),
						pNTHeader->FileHeader.NumberOfSections, TRUE);
	printfScreen(screen, "\n");

	DumpDebugDirectory(screen, base, pNTHeader);
	printfScreen(screen, "\n");

	DumpResourceSection(screen, base, pNTHeader);
	printfScreen(screen, "\n");

	DumpImportsSection(screen, base, pNTHeader);
	printfScreen(screen, "\n");

	DumpExportsSection(screen, base, pNTHeader);
	printfScreen(screen, "\n");

	if ( fShowRelocations )
	{
	   DumpBaseRelocationsSection(screen, base, pNTHeader);
	   printfScreen(screen, "\n");
	}

	//
	// Initialize these vars here since we'll need them in DumpLineNumbers
	//

	PCOFFSymbolTable = MakePtr(PIMAGE_SYMBOL, base,	pNTHeader->FileHeader.PointerToSymbolTable);
	COFFSymbolCount = pNTHeader->FileHeader.NumberOfSymbols;

	if ( fShowSymbolTable && PCOFFDebugInfo )
	{
		DumpCOFFHeader( screen, PCOFFDebugInfo );
		printfScreen(screen, "\n");
	}

	if ( fShowLineNumbers && PCOFFDebugInfo )
	{
		DumpLineNumbers( screen, MakePtr(PIMAGE_LINENUMBER, PCOFFDebugInfo,
							PCOFFDebugInfo->LvaToFirstLinenumber),
							PCOFFDebugInfo->NumberOfLinenumbers);
		printfScreen(screen, "\n");
	}

	if ( fShowSymbolTable )
	{
		if ( pNTHeader->FileHeader.NumberOfSymbols
			&& pNTHeader->FileHeader.PointerToSymbolTable)
		{
			DumpSymbolTable(screen, PCOFFSymbolTable, COFFSymbolCount);
			printfScreen(screen, "\n");
		}
	}

	if ( fShowRawSectionData )
	{
		DumpRawSectionData( screen, (PIMAGE_SECTION_HEADER)(pNTHeader+1),
							dosHeader,
							pNTHeader->FileHeader.NumberOfSections);
	}

}
