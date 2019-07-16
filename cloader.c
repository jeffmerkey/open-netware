

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  CLOADER.C
*   DESCRIP  :  Initialization Code for MANOS v1.0
*   DATE     :  November 2, 1997
*
*
***************************************************************************/

#include "version.h"
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "types.h"
#include "emit.h"
#include "string.h"
#include "dos.h"
#include "tss.h"
#include "os.h"
#include "hal.h"
#include "kernel.h"
#include "screen.h"
#include "peexe.h"
#include "extrnvar.h"
#include "dosfile.h"
#include "mps.h"

#define  FILE_NOT_FOUND     10
#define  BAD_FILE           11
#define  FILE_SEEK_ERROR    12
#define  FILE_READ_ERROR    13
#define  FILE_MODULO_ERROR  14

extern void ExitToDOS(void);
extern void OSMain(DOS_TABLE *, PM_TABLE *, LONG);

IMAGE_NT_HEADERS pNTHeader;
IMAGE_DOS_HEADER dosHeader;
IMAGE_SECTION_HEADER section;
BASE_RELOCATION baseReloc;
LONG pe_base;
LONG data_virt_size, data_offset, data_blocks, data_size, data_rva;
LONG code_virt_size, code_offset, code_blocks, code_size, code_rva;
LONG reloc_virt_size, reloc_offset, reloc_blocks, reloc_size, reloc_rva;
LONG debug_virt_size, debug_offset, debug_blocks, debug_size, debug_rva;
LONG export_virt_size, export_offset, export_blocks, export_size, export_rva;
PM_TABLE pm_table;

BYTE sector[4096];

LONG LoadOS(DOS_TABLE *dos);

LONG MajorVersion = MAJOR_VERSION;
LONG MinorVersion = MINOR_VERSION;
LONG BuildVersion = BUILD_VERSION;

void ExitLoader(void)
{
    extern void ResetKeyboard(void);

    ResetKeyboard();
    ExitToDOS();   //  call does not return
}

LONG loader_main(DOS_TABLE *dos)
{

    LONG key, ret, TEMP;
    BYTE commandBuffer[100];
    LONG *ebda_addr, *bmem_addr;
    LONG ebda_base, bmem_base, dosMem;

    pm_table.LoaderCodeSegment = dos->PM_CODE_SEGMENT;
    pm_table.LoaderDataSegment = dos->PM_DATA_SEGMENT;

    printf("MANOS LOADER\r\n");
    printf("v%02d.%02d.%02d\r\n", MajorVersion, MinorVersion, BuildVersion);
    printf("Copyright (C) 1997, 1998 Timpanogas Research Group, Inc.\r\n");
    printf("All Rights Reserved.\r\n");

    //
    //  get extended BIOS data address and base memory address and compute
    //  which is smaller.  If either are greater than 0xA0000 or less than
    //  0x7F800, then they are invalid, and set to zero.
    //  Low memory is that memory which exists between the end of
    //  the MANOS loader and the beginnning of either of these regions
    //  whichever is the smaller.
    //

    ebda_addr = (LONG *) EBDA_PTR;
    ebda_base = *ebda_addr;
    ebda_base = (ebda_base << 4) & 0xFFFFF;
    bmem_addr = (LONG *) BMEM_PTR;
    bmem_base = *bmem_addr;
    bmem_base = (bmem_base * 1024) & 0xFFFFF;

    if (bmem_base > 0x000A0000 || bmem_base < 0x0007F800)
       bmem_base = 0;

    if (ebda_base > 0x000A0000 || ebda_base < 0x0007F800)
       ebda_base = 0;

    printf("\nInitializing Low Memory...\r\n");
    dos->MEMORY_LOW_START = ((dos->MEMORY_LOW_START + 0xFFF) & 0xFFFFF000);

    if (ebda_base || bmem_base)
    {
       if (ebda_base && bmem_base)
       {
	  if (ebda_base < bmem_base)
	     dos->MEMORY_LOW = ebda_base - dos->MEMORY_LOW_START;
	  else
	  if (bmem_base < ebda_base)
	     dos->MEMORY_LOW = bmem_base - dos->MEMORY_LOW_START;
	  else
	  if (ebda_base == bmem_base)
	     dos->MEMORY_LOW = ebda_base - dos->MEMORY_LOW_START;
       }
       else
       if (ebda_base)
	  dos->MEMORY_LOW = ebda_base - dos->MEMORY_LOW_START;
       else
       if (bmem_base)
	  dos->MEMORY_LOW = bmem_base - dos->MEMORY_LOW_START;
    }
    else
       dos->MEMORY_LOW = 0x000A0000 - dos->MEMORY_LOW_START;
    SetData((LONG *) dos->MEMORY_LOW_START, 0x00000000, dos->MEMORY_LOW);

    printf("Initializing High Memory...\r\n");
    TEMP = ((dos->MEMORY_HIGH_START + 0xFFF) & 0xFFFFF000);
    dos->MEMORY_HIGH = dos->MEMORY_HIGH - (TEMP - dos->MEMORY_HIGH_START);
    dos->MEMORY_HIGH_START = TEMP;
    SetData((LONG *) dos->MEMORY_HIGH_START, 0x00000000, dos->MEMORY_HIGH);

    sti(); // do not enable interrupts before this point

    //  patch dos real mode interrupt entry point into dos table

    dos->REAL_MODE_INT = (LONG) int86x;

    if (dos->SYSTEM_OFFSET)
    {
       printf("system image offset %08X (%d)\r\n",
	       dos->SYSTEM_OFFSET,
	       dos->SYSTEM_OFFSET);

       printf("\nLoading Operating System ... \r\n");

       ret = LoadOS(dos);
       if (ret) {};
	  printf("\nError Loading Operating System : %d\r\n", ret);
    }
    else
    {
       printf("\nOperating System Image Not Found\r\n");
       ExitLoader();  // this call does not return
    }

    return 0;

}

LPVOID GetSectionPtr(PSTR name, PIMAGE_NT_HEADERS pNTHeader, LONG fp)
{

	unsigned i;

	DOSRead(fp, &section, sizeof(IMAGE_SECTION_HEADER));
	for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++)
	{
	   if (strnicmp(section.Name, name, IMAGE_SIZEOF_SHORT_NAME) == 0 )
	      return (LPVOID)(section.PointerToRawData);
	   DOSRead(fp, &section, sizeof(IMAGE_SECTION_HEADER));
	}
	return 0;
}


LONG get_section(DOS_TABLE *dos, PSTR name, LONG fp)
{

	LONG ep;

	DOSLseek(fp, dos->SYSTEM_OFFSET, SEEK_SET);

	// read MZ header
	DOSRead(fp, &dosHeader, sizeof(IMAGE_DOS_HEADER));

	// seek to PE header
	DOSLseek(fp, (LONG)dosHeader.e_lfanew + dos->SYSTEM_OFFSET, SEEK_SET);

	// read PE header
	DOSRead(fp, &pNTHeader, sizeof(IMAGE_NT_HEADERS));

	if (pNTHeader.Signature != IMAGE_NT_SIGNATURE)
	{
	   printf("image is an unhandled EXE type, or invalid .EXE\n");
	   return 0;
	}
	return (LONG) GetSectionPtr(name, &pNTHeader, fp);

}

LPVOID GetSectionSz(PSTR name, PIMAGE_NT_HEADERS pNTHeader, LONG fp)
{

	unsigned i;

	DOSRead(fp, &section, sizeof(IMAGE_SECTION_HEADER));
	for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++)
	{
	   if ( strnicmp(section.Name, name, IMAGE_SIZEOF_SHORT_NAME) == 0 )
	      return (LPVOID)(section.SizeOfRawData);
	   DOSRead(fp, &section, sizeof(IMAGE_SECTION_HEADER));
	}
	return 0;
}


LONG get_section_size(DOS_TABLE *dos, PSTR name, LONG fp)
{

	LONG ep;

	DOSLseek(fp, dos->SYSTEM_OFFSET, SEEK_SET);

	// read MZ header
	DOSRead(fp, &dosHeader, sizeof(IMAGE_DOS_HEADER));

	// seek to PE header
	DOSLseek(fp, (LONG)dosHeader.e_lfanew + dos->SYSTEM_OFFSET, SEEK_SET);

	// read PE header
	DOSRead(fp, &pNTHeader, sizeof(IMAGE_NT_HEADERS));

	if (pNTHeader.Signature != IMAGE_NT_SIGNATURE)
	{
	   printf("image is an unhandled EXE type, or invalid .EXE\n");
	   return 0;
	}
	return (LONG) GetSectionSz(name, &pNTHeader, fp);

}

LPVOID GetSectionVirtSz(PSTR name, PIMAGE_NT_HEADERS pNTHeader, LONG fp)
{

	unsigned i;

	DOSRead(fp, &section, sizeof(IMAGE_SECTION_HEADER));
	for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++)
	{
	   if ( strnicmp(section.Name, name, IMAGE_SIZEOF_SHORT_NAME) == 0 )
	      return (LPVOID)(section.Misc.VirtualSize);
	   DOSRead(fp, &section, sizeof(IMAGE_SECTION_HEADER));
	}
	return 0;
}


LONG get_section_virt_size(DOS_TABLE *dos, PSTR name, LONG fp)
{

	LONG ep;

	DOSLseek(fp, dos->SYSTEM_OFFSET, SEEK_SET);

	// read MZ header
	DOSRead(fp, &dosHeader, sizeof(IMAGE_DOS_HEADER));

	// seek to PE header
	DOSLseek(fp, (LONG)dosHeader.e_lfanew + dos->SYSTEM_OFFSET, SEEK_SET);

	// read PE header
	DOSRead(fp, &pNTHeader, sizeof(IMAGE_NT_HEADERS));

	if (pNTHeader.Signature != IMAGE_NT_SIGNATURE)
	{
	   printf("image is an unhandled EXE type, or invalid .EXE\n");
	   return 0;
	}
	return (LONG) GetSectionVirtSz(name, &pNTHeader, fp);

}



LONG get_pe_base(DOS_TABLE *dos, LONG fp)
{

	LONG ep;

	DOSLseek(fp, dos->SYSTEM_OFFSET, SEEK_SET);

	// read MZ header
	DOSRead(fp, &dosHeader, sizeof(IMAGE_DOS_HEADER));

	// seek to PE header
	DOSLseek(fp, (LONG)dosHeader.e_lfanew + dos->SYSTEM_OFFSET, SEEK_SET);

	// read PE header
	DOSRead(fp, &pNTHeader, sizeof(IMAGE_NT_HEADERS));

	if (pNTHeader.Signature != IMAGE_NT_SIGNATURE)
	{
	   printf("image is an unhandled EXE type, or invalid .EXE\n");
	   return 0;
	}
	return (LONG) pNTHeader.OptionalHeader.ImageBase;

}

LONG GetSectionRVA(PSTR name, PIMAGE_NT_HEADERS pNTHeader, LONG fp)
{

	unsigned i;

	DOSRead(fp, &section, sizeof(IMAGE_SECTION_HEADER));
	for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++)
	{
	   if ( strnicmp(section.Name, name, IMAGE_SIZEOF_SHORT_NAME) == 0 )
	      return (LONG)(section.VirtualAddress);
	   DOSRead(fp, &section, sizeof(IMAGE_SECTION_HEADER));
	}
	return 0;
}


LONG get_section_rva(DOS_TABLE *dos, PSTR name, LONG fp)
{

	LONG ep;

	DOSLseek(fp, dos->SYSTEM_OFFSET, SEEK_SET);

	// read MZ header
	DOSRead(fp, &dosHeader, sizeof(IMAGE_DOS_HEADER));

	// seek to PE header
	DOSLseek(fp, (LONG)dosHeader.e_lfanew + dos->SYSTEM_OFFSET, SEEK_SET);

	// read PE header
	DOSRead(fp, &pNTHeader, sizeof(IMAGE_NT_HEADERS));

	if (pNTHeader.Signature != IMAGE_NT_SIGNATURE)
	{
	   printf("image is an unhandled EXE type, or invalid .EXE\n");
	   return 0;
	}
	return (LONG) GetSectionRVA(name, &pNTHeader, fp);

}


LONG LoadOS(DOS_TABLE *dos)
{

     register int i;
     LONG fd, rc;
     PBASE_RELOCATION baseReloc;
     LONG CodeSegmentPtr;
     LONG DataSegmentPtr;
     LONG RelocSegmentPtr;
     LONG DebugSegmentPtr;
     LONG ExportSegmentPtr;

     fd = DOSOpen("MANOS.EXE", O_RDONLY | O_DENYNONE);
     if (!fd)
	return (FILE_NOT_FOUND);

     rc = DOSLseek(fd, dos->SYSTEM_OFFSET, SEEK_SET);
     if (!rc)
     {
	DOSClose(fd);
	return (FILE_SEEK_ERROR);
     }

     pe_base = get_pe_base(dos, fd);
     if (!pe_base)
       pe_base = 0x400000;

     code_virt_size = get_section_virt_size(dos, "CODE", fd);
     code_offset = get_section(dos, "CODE", fd);
     code_rva = get_section_rva(dos, "CODE", fd);
     code_size = get_section_size(dos, "CODE", fd);
     code_blocks = (code_size + 4095) / 4096; // compute total code blocks
     if (code_size % 512)
     {
	printf("\r\ncode_size %08X (%d) modulo 512 error\n", code_size, code_size);
	DOSClose(fd);
	return FILE_MODULO_ERROR;
     }

     data_virt_size = get_section_virt_size(dos, "DATA", fd);
     data_offset = get_section(dos, "DATA", fd);
     data_rva = get_section_rva(dos, "DATA", fd);
     data_size = get_section_size(dos, "DATA", fd);
     data_blocks = (data_size + 4095) / 4096; // compute total data blocks
     if (data_size % 512)
     {
	printf("\r\ndata_size %08X (%d) modulo 512 error\n", data_size, data_size);
	DOSClose(fd);
	return FILE_MODULO_ERROR;
     }

     reloc_virt_size = get_section_virt_size(dos, ".reloc", fd);
     reloc_offset = get_section(dos, ".reloc", fd);
     reloc_rva = get_section_rva(dos, ".reloc", fd);
     reloc_size = get_section_size(dos, ".reloc", fd);
     reloc_blocks = (reloc_size + 4095) / 4096; // compute total reloc blocks
     if (reloc_size % 512)
     {
	printf("\r\nreloc_size %08X (%d) modulo 512 error\n", reloc_size, reloc_size);
	DOSClose(fd);
	return FILE_MODULO_ERROR;
     }

     export_virt_size = get_section_virt_size(dos, ".edata", fd);
     export_offset = get_section(dos, ".edata", fd);
     export_rva = get_section_rva(dos, ".edata", fd);
     export_size = get_section_size(dos, ".edata", fd);
     export_blocks = (export_size + 4095) / 4096; // compute total reloc blocks

     debug_virt_size = get_section_virt_size(dos, ".debug", fd);
     debug_offset = get_section(dos, ".debug", fd);
     debug_rva = get_section_rva(dos, ".debug", fd);
     debug_size = get_section_size(dos, ".debug", fd);
     debug_blocks = (debug_size + 4095) / 4096; // compute total debug blocks

     pm_table.BaseSegment = pm_table.CodeSegment = CodeSegmentPtr = ((dos->MEMORY_HIGH_START + 0xFFFFF) & 0xFFF00000);
     pm_table.CodeRVA = code_rva;
     pm_table.DataSegment = DataSegmentPtr = (CodeSegmentPtr + (data_rva - code_rva));
     pm_table.RelocSegment = RelocSegmentPtr = (((DataSegmentPtr + data_virt_size) + 0xFFF) & 0xFFFFF000);
     if (export_virt_size)
     {
	pm_table.ExportSegment = ExportSegmentPtr = (((RelocSegmentPtr + reloc_virt_size) + 0xFFF) & 0xFFFFF000);
	pm_table.ExportSize = export_size;
	pm_table.ExportRVA = export_rva;
     }
     else
     {
	pm_table.ExportSegment = 0;
	pm_table.ExportSize = 0;
	pm_table.ExportRVA = 0;
     }
     pm_table.DebugSegment = DebugSegmentPtr = (((ExportSegmentPtr + export_virt_size) + 0xFFF) & 0xFFFFF000);
     pm_table.DebugSize = debug_size;
     pm_table.StartOfHighMemory = (((DebugSegmentPtr + debug_virt_size) + 0xFFF) & 0xFFFFF000);
     pm_table.HighMemoryLength = (dos->MEMORY_HIGH - (pm_table.StartOfHighMemory - dos->MEMORY_HIGH_START));
     pm_table.StartOfLowMemory = ((dos->MEMORY_LOW_START + 0xFFF) & 0xFFFFF000);
     pm_table.LowMemoryLength = (dos->MEMORY_LOW - (pm_table.StartOfLowMemory - dos->MEMORY_LOW_START));

     DOSLseek(fd, code_offset + dos->SYSTEM_OFFSET, SEEK_SET);
     for (i=0; i < code_blocks; i++)
     {
	DOSRead(fd, sector, 4096);
	CopyData((LONG *)sector, (LONG *)CodeSegmentPtr, 4096);
	CodeSegmentPtr += 4096;
     }

     DOSLseek(fd, data_offset + dos->SYSTEM_OFFSET, SEEK_SET);
     for (i=0; i < data_blocks; i++)
     {
	DOSRead(fd, sector, 4096);
	CopyData((LONG *)sector, (LONG *)DataSegmentPtr, 4096);
	DataSegmentPtr += 4096;
     }

     DOSLseek(fd, reloc_offset + dos->SYSTEM_OFFSET, SEEK_SET);
     for (i=0; i < reloc_blocks; i++)
     {
	DOSRead(fd, sector, 4096);
	CopyData((LONG *)sector, (LONG *)RelocSegmentPtr, 4096);
	RelocSegmentPtr += 4096;
     }

     DOSLseek(fd, export_offset + dos->SYSTEM_OFFSET, SEEK_SET);
     for (i=0; i < export_blocks; i++)
     {
	DOSRead(fd, sector, 4096);
	CopyData((LONG *)sector, (LONG *)ExportSegmentPtr, 4096);
	ExportSegmentPtr += 4096;
     }

     DOSLseek(fd, debug_offset + dos->SYSTEM_OFFSET, SEEK_SET);
     for (i=0; i < debug_blocks; i++)
     {
	DOSRead(fd, sector, 4096);
	CopyData((LONG *)sector, (LONG *)DebugSegmentPtr, 4096);
	DebugSegmentPtr += 4096;
     }

     DOSClose(fd);

     baseReloc = (PBASE_RELOCATION) pm_table.RelocSegment;
     while ( baseReloc->SizeOfBlock != 0 )
     {
	LONG i, cEntries;
	PWORD pEntry;
	LONG ImageOffset, *ImagePtr;
	LONG ImageEntry;
	WORD relocType;

	cEntries = (baseReloc->SizeOfBlock - sizeof(BASE_RELOCATION))/sizeof(WORD);
	pEntry = MakePtr(PWORD, baseReloc, sizeof(BASE_RELOCATION));
	for ( i=0; i < cEntries; i++ )
	{
	   relocType = (*pEntry & 0xF000) >> 12;
	   switch (relocType)
	   {
	     case 0:   // ABSOLUTE fixup entry.  ingore absolute fixups
		break;  // on Intel

	     case 3:   // HIGHLOW Intel IA32 fixup entry
		ImageOffset = (LONG)(*pEntry & 0x0FFF) + baseReloc->VirtualAddress;
		ImageOffset -= code_rva;
		ImageOffset += pm_table.BaseSegment;
		ImagePtr = (LONG *) ImageOffset;
		ImageEntry = *ImagePtr;
		ImageEntry -= (pe_base + code_rva);
		ImageEntry += pm_table.BaseSegment;
		*ImagePtr = ImageEntry;
		break;

	     default:
		break;
	   }
	   pEntry++;	// Advance to next relocation entry
	}
	baseReloc = MakePtr(PBASE_RELOCATION, baseReloc, baseReloc->SizeOfBlock);
     }

     OSMain(dos, &pm_table, pm_table.CodeSegment);  // this function does not return

     return 0;

}


