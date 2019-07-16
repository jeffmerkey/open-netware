

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   This program is an unpublished work of TRG, Inc. and contains
*   trade secrets and other proprietary information.  Unauthorized
*   use, copying, disclosure, or distribution of this file without the
*   consent of TRG, Inc. can subject the offender to severe criminal
*   and/or civil penalities.
*
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  OBJDUMP.C
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

typedef struct _i386RelocTypes {
   WORD type;
   PSTR name;
} i386RelocTypes;

// ASCII names for the various relocations used in i386 COFF OBJs
i386RelocTypes i386Relocations[] =
{
{ IMAGE_REL_I386_ABSOLUTE, "ABSOLUTE" },
{ IMAGE_REL_I386_DIR16, "DIR16" },
{ IMAGE_REL_I386_REL16, "REL16" },
{ IMAGE_REL_I386_DIR32, "DIR32" },
{ IMAGE_REL_I386_DIR32NB, "DIR32NB" },
{ IMAGE_REL_I386_SEG12, "SEG12" },
{ IMAGE_REL_I386_SECTION, "SECTION" },
{ IMAGE_REL_I386_SECREL, "SECREL" },
{ IMAGE_REL_I386_REL32, "REL32" }
};
#define I386RELOCTYPECOUNT (sizeof(i386Relocations) / sizeof(i386RelocTypes))

//
// Given an i386 OBJ relocation type, return its ASCII name in a buffer
//

void GetObjRelocationName(WORD type, char *buffer, DWORD cBytes)
{
	DWORD i;
	
	for ( i=0; i < I386RELOCTYPECOUNT; i++ )
		if ( type == i386Relocations[i].type )
		{
			strncpy(buffer, i386Relocations[i].name, cBytes);
			return;
		}
		
	sprintf( buffer, "???_%X", type);
}

//
// Dump the relocation table for one COFF section
//
void DumpObjRelocations(SCREEN *screen, PIMAGE_RELOCATION pRelocs, DWORD count)
{
	DWORD i;
	char szTypeName[32];
	
	for ( i=0; i < count; i++ )
	{
		GetObjRelocationName(pRelocs->Type, szTypeName, sizeof(szTypeName));
		printfScreen(screen, "  Address: %08X  SymIndex: %08X  Type: %s\n",
				pRelocs->u.VirtualAddress, pRelocs->SymbolTableIndex,
				szTypeName);
		pRelocs++;
	}
}

//
// top level routine called from PEDUMP.C to dump the components of a
// COFF OBJ file.
//
void DumpObjFile(SCREEN *screen, PIMAGE_FILE_HEADER pImageFileHeader )
{
	unsigned i;
	PIMAGE_SECTION_HEADER pSections;
	
	DumpHeader(screen, pImageFileHeader);
	printfScreen(screen, "\n");

	pSections = (PIMAGE_SECTION_HEADER)(pImageFileHeader+1);

	DumpSectionTable(screen, pSections, pImageFileHeader->NumberOfSections, FALSE);
	printfScreen(screen, "\n");

	if ( fShowRelocations )
	{
		for ( i=0; i < pImageFileHeader->NumberOfSections; i++ )
		{
			if ( pSections[i].PointerToRelocations == 0 )
				continue;
		
			printfScreen(screen, "Section %02X (%.8s) relocations\n", i, pSections[i].Name);
			DumpObjRelocations(screen, MakePtr(PIMAGE_RELOCATION, pImageFileHeader,
								       pSections[i].PointerToRelocations),
								       pSections[i].NumberOfRelocations );
			printfScreen(screen, "\n");
		}
	}

	PCOFFSymbolTable = MakePtr(PIMAGE_SYMBOL, pImageFileHeader, pImageFileHeader->PointerToSymbolTable);
	COFFSymbolCount = pImageFileHeader->NumberOfSymbols;

	if ( fShowLineNumbers )
	{
		// Walk through the section table...
		for (i=0; i < pImageFileHeader->NumberOfSections; i++)
		{
			// if there's any line numbers for this section, dump'em
			if ( pSections->NumberOfLinenumbers )
			{
				DumpLineNumbers(screen, MakePtr(PIMAGE_LINENUMBER, pImageFileHeader,
										 pSections->PointerToLinenumbers),
								 pSections->NumberOfLinenumbers );
				printfScreen(screen, "\n");
			}
			pSections++;
		}
	}

	if ( fShowSymbolTable )
	{
		DumpSymbolTable(screen, PCOFFSymbolTable, COFFSymbolCount);
		printfScreen(screen, "\n");
	}
	
	if ( fShowRawSectionData )
	{
		DumpRawSectionData( screen, (PIMAGE_SECTION_HEADER)(pImageFileHeader+1),
							pImageFileHeader,
							pImageFileHeader->NumberOfSections);
	}
}
