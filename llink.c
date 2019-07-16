

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  LLINK.C
*   DESCRIP  :  Loader Stub Linker for MANOS v1.0
*   DATE     :  November 9, 1997
*
*
***************************************************************************/


#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include "kernel.h"
#include "tss.h"
#include "screen.h"
#include "keyboard.h"
#include "types.h"
#include "emit.h"
#include "dos.h"
#include "mps.h"
#include "hal.h"
#include "timer.h"
#include "line.h"
#include "loader.h"
#include "free.h"
#include "ifs.h"
#include "event.h"
#include "exports.h"

#define VERBOSE  0
#define TRUE 1
#define FALSE 0

#include "peexe.h"
#include "extrnvar.h"

typedef PIMAGE_COFF_SYMBOLS_HEADER PIMAGE_DEBUG_INFO;

#define MZ_SIGNATURE  ('M' | ('Z' << 8))
#define NE_SIGNATURE  ('N' | ('E' << 8))
#define PE_SIGNATURE  ('P' | ('E' << 8))

#define SMALL_EXE_HEADER 0

#if SMALL_EXE_HEADER
   #define EXE_HEADER_SIZE 32
   #define EXE_HEADER_BLOCKS 1
#else
   #define EXE_HEADER_SIZE 512
   #define EXE_HEADER_BLOCKS 32
#endif

typedef union _DOS_EXE_HEADER {
   struct _EXE {
      unsigned int signature;
      unsigned int bytes_in_last_block;
      unsigned int blocks_in_file;
      unsigned int num_relocs;
      unsigned int header_paragraphs;
      unsigned int min_extra_paragraphs;
      unsigned int max_extra_paragraphs;
      unsigned int ss;
      unsigned int sp;
      unsigned int checksum;
      unsigned int ip;
      unsigned int cs;
      unsigned int reloc_table_offset;
      unsigned int overlay_number;
      unsigned int reserved[4];
      unsigned int oem_id;
      unsigned int oem_info;
      unsigned int res_2[10];
      unsigned long new_file_address;
   } p;
   unsigned char header[EXE_HEADER_SIZE];
} EXE;

struct DOS_EXE_RELOC {
   unsigned int offset;
   unsigned int segment;
};


BYTE dumpbuf[1024];
int dumpoff = 0;

/*
 * Old MZ header for DOS programs.  Actually just a couple of fields
 * from it, so that we can find the start of the NE header.
 */
struct mz_header_s
{
    WORD mz_magic;         /* MZ Header signature */
    BYTE dont_care[0x3a];  /* MZ Header stuff */
    WORD ne_offset;        /* Offset to extended header */
};

/*
 * This is the Windows executable (NE) header.
 */
struct ne_header_s
{
    WORD  ne_magic;             /* 00 NE signature 'NE' */
    BYTE  linker_version;   /* 02 Linker version number */
    BYTE  linker_revision;  /* 03 Linker revision number */
    WORD  entry_tab_offset; /* 04 Offset to entry table relative to NE */
    WORD  entry_tab_length; /* 06 Length of entry table in bytes */
    DWORD reserved1;        /* 08 Reserved by Microsoft */
    WORD  format_flags;         /* 0c Flags about segments in this file */
    WORD  auto_data_seg;    /* 0e Automatic data segment number */
    WORD  local_heap_length;    /* 10 Initial size of local heap */
    WORD  stack_length;         /* 12 Initial size of stack */
    WORD  ip;           /* 14 Initial IP */
    WORD  cs;           /* 16 Initial CS */
    WORD  sp;           /* 18 Initial SP */
    WORD  ss;           /* 1a Initial SS */
    WORD  n_segment_tab;    /* 1c # of entries in segment table */
    WORD  n_mod_ref_tab;    /* 1e # of entries in module reference tab. */
    WORD  nrname_tab_length;    /* 20 Length of nonresident-name table     */
    WORD  segment_tab_offset;   /* 22 Offset to segment table */
    WORD  resource_tab_offset;  /* 24 Offset to resource table */
    WORD  rname_tab_offset; /* 26 Offset to resident-name table */
    WORD  moduleref_tab_offset; /* 28 Offset to module reference table */
    WORD  iname_tab_offset; /* 2a Offset to imported name table */
    DWORD nrname_tab_offset;    /* 2c Offset to nonresident-name table */
    WORD  n_mov_entry_points;   /* 30 # of movable entry points */
    WORD  align_shift_count;    /* 32 Logical sector alignment shift count */
    WORD  n_resource_seg;   /* 34 # of resource segments */
    BYTE  operating_system; /* 36 Flags indicating target OS */
    BYTE  additional_flags; /* 37 Additional information flags */
    WORD  fastload_offset;  /* 38 Offset to fast load area */
    WORD  fastload_length;  /* 3a Length of fast load area */
    WORD  reserved2;        /* 3c Reserved by Microsoft */
    WORD  expect_version;   /* 3e Expected Windows version number */
};

unsigned char sector[8192];

EXE iexe;
EXE texe;

typedef struct _MANOS_HEADER {
    WORD blocks_in_file;
    WORD virtual_blocks;
    LONG offset_of_pe_header;
    LONG offset_of_32_code;
    LONG offset_of_32_data;
    LONG offset_of_16_code;
    LONG offset_of_stack;
    LONG offset_reloc;
    LONG offset_reloc32;
    LONG code_rva;
    LONG data_rva;
    LONG code_size;
    LONG data_size;
    LONG pe_base;
    LONG offset_debug;
    LONG code_virt_size;
    LONG data_virt_size;
    LONG reloc_size;
    LONG debug_size;
    LONG system_offset;
    LONG system_size;
    WORD psp_next_segment;
    WORD total_paragraphs;
    LONG initrd_offset;
    LONG initrd_size;
} MANOS;

MANOS manos_header;

IMAGE_NT_HEADERS pNTHeader;
IMAGE_DOS_HEADER dosHeader;
IMAGE_SECTION_HEADER section;
BASE_RELOCATION baseReloc;

int is_nepe(FILE *f)
{
	struct mz_header_s mz_header;
	struct ne_header_s ne_header;

	fseek(f,0,SEEK_SET);
	if(fread(&mz_header, 1, sizeof(mz_header), f) !=sizeof(mz_header))
		return 0;
	if(mz_header.mz_magic != MZ_SIGNATURE)
		return 0;
	fseek(f,mz_header.ne_offset,SEEK_SET);
	if(fread(&ne_header, 1, sizeof(ne_header), f) !=sizeof(ne_header))
		return 0;
	if(ne_header.ne_magic == PE_SIGNATURE) return 1;
	if(ne_header.ne_magic == NE_SIGNATURE) return 1;
	return 0;
}

DWORD LLGetSectionPtr(PSTR name, PIMAGE_NT_HEADERS pNTHeader, FILE *fp)
{

	unsigned i;

	fread(&section, sizeof(IMAGE_SECTION_HEADER), 1, fp);
	for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++)
	{
	   if ( strnicmp(section.Name, name, IMAGE_SIZEOF_SHORT_NAME) == 0 )
	      return (DWORD)(section.PointerToRawData);
	   fread(&section, sizeof(IMAGE_SECTION_HEADER), 1, fp);
	}
	return 0;
}


LONG get_section(PSTR name, FILE *fp)
{

	LONG ep;

	fseek(fp, 0, SEEK_SET);

	// read MZ header
	fread(&dosHeader, sizeof(IMAGE_DOS_HEADER), 1, fp);

	// seek to PE header
	fseek(fp, (LONG)dosHeader.e_lfanew, SEEK_SET);

	// read PE header
	fread(&pNTHeader, sizeof(IMAGE_NT_HEADERS), 1, fp);

	if (pNTHeader.Signature != IMAGE_NT_SIGNATURE)
	{
	   printf("Unhandled EXE type, or invalid .EXE\n");
	   return 0;
	}
	return (LONG) LLGetSectionPtr(name, &pNTHeader, fp);

}

DWORD GetSectionSz(PSTR name, PIMAGE_NT_HEADERS pNTHeader, FILE *fp)
{

	unsigned i;

	fread(&section, sizeof(IMAGE_SECTION_HEADER), 1, fp);
	for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++)
	{
	   if ( strnicmp(section.Name, name, IMAGE_SIZEOF_SHORT_NAME) == 0 )
	      return (DWORD)(section.SizeOfRawData);
	   fread(&section, sizeof(IMAGE_SECTION_HEADER), 1, fp);
	}
	return 0;
}


LONG get_section_size(PSTR name, FILE *fp)
{

	LONG ep;

	fseek(fp, 0, SEEK_SET);

	// read MZ header
	fread(&dosHeader, sizeof(IMAGE_DOS_HEADER), 1, fp);

	// seek to PE header
	fseek(fp, (LONG)dosHeader.e_lfanew, SEEK_SET);

	// read PE header
	fread(&pNTHeader, sizeof(IMAGE_NT_HEADERS), 1, fp);

	if (pNTHeader.Signature != IMAGE_NT_SIGNATURE)
	{
	   printf("Unhandled EXE type, or invalid .EXE\n");
	   return 0;
	}
	return (LONG) GetSectionSz(name, &pNTHeader, fp);

}

DWORD GetSectionVirtSz(PSTR name, PIMAGE_NT_HEADERS pNTHeader, FILE *fp)
{

	unsigned i;

	fread(&section, sizeof(IMAGE_SECTION_HEADER), 1, fp);
	for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++)
	{
	   if ( strnicmp(section.Name, name, IMAGE_SIZEOF_SHORT_NAME) == 0 )
	      return (DWORD)(section.Misc.VirtualSize);
	   fread(&section, sizeof(IMAGE_SECTION_HEADER), 1, fp);
	}
	return 0;
}


LONG get_section_virt_size(PSTR name, FILE *fp)
{

	LONG ep;

	fseek(fp, 0, SEEK_SET);

	// read MZ header
	fread(&dosHeader, sizeof(IMAGE_DOS_HEADER), 1, fp);

	// seek to PE header
	fseek(fp, (LONG)dosHeader.e_lfanew, SEEK_SET);

	// read PE header
	fread(&pNTHeader, sizeof(IMAGE_NT_HEADERS), 1, fp);

	if (pNTHeader.Signature != IMAGE_NT_SIGNATURE)
	{
	   printf("Unhandled EXE type, or invalid .EXE\n");
	   return 0;
	}

	return (LONG) GetSectionVirtSz(name, &pNTHeader, fp);

}



LONG get_pe_base(FILE *fp)
{

	LONG ep;

	fseek(fp, 0, SEEK_SET);

	// read MZ header
	fread(&dosHeader, sizeof(IMAGE_DOS_HEADER), 1, fp);

	// seek to PE header
	fseek(fp, (LONG)dosHeader.e_lfanew, SEEK_SET);

	// read PE header
	fread(&pNTHeader, sizeof(IMAGE_NT_HEADERS), 1, fp);

	if (pNTHeader.Signature != IMAGE_NT_SIGNATURE)
	{
	   printf("Unhandled EXE type, or invalid .EXE\n");
	   return 0;
	}
	return (LONG) pNTHeader.OptionalHeader.ImageBase;

}

LONG GetSectionRVA(PSTR name, PIMAGE_NT_HEADERS pNTHeader, FILE *fp)
{

	unsigned i;

	fread(&section, sizeof(IMAGE_SECTION_HEADER), 1, fp);
	for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++)
	{
	   if ( strnicmp(section.Name, name, IMAGE_SIZEOF_SHORT_NAME) == 0 )
	      return (LONG)(section.VirtualAddress);
	   fread(&section, sizeof(IMAGE_SECTION_HEADER), 1, fp);
	}
	return 0;
}


LONG get_section_rva(PSTR name, FILE *fp)
{

	LONG ep;

	fseek(fp, 0, SEEK_SET);

	// read MZ header
	fread(&dosHeader, sizeof(IMAGE_DOS_HEADER), 1, fp);

	// seek to PE header
	fseek(fp, (LONG)dosHeader.e_lfanew, SEEK_SET);

	// read PE header
	fread(&pNTHeader, sizeof(IMAGE_NT_HEADERS), 1, fp);

	if (pNTHeader.Signature != IMAGE_NT_SIGNATURE)
	{
	   printf("Unhandled EXE type, or invalid .EXE\n");
	   return 0;
	}
	return (LONG) GetSectionRVA(name, &pNTHeader, fp);

}

char *SzRelocTypes[] = {
   "ABSOLUTE","HIGH","LOW","HIGHLOW","HIGHADJ","MIPS_JMPADDR",
   "I860_BRADDR","I860_SPLIT" };

void dump_reloc(FILE *fp)
{

	LONG reloc;

	fseek(fp, 0, SEEK_SET);

	// read MZ header
	fread(&dosHeader, sizeof(IMAGE_DOS_HEADER), 1, fp);

	// seek to PE header
	fseek(fp, (LONG)dosHeader.e_lfanew, SEEK_SET);

	// read PE header
	fread(&pNTHeader, sizeof(IMAGE_NT_HEADERS), 1, fp);

	if (pNTHeader.Signature != IMAGE_NT_SIGNATURE)
	{
	   printf("Unhandled EXE type, or invalid .EXE\n");
	   return;
	}

	reloc = (LONG) LLGetSectionPtr(".reloc", &pNTHeader, fp);
	if (reloc)
	{
	   fseek(fp, reloc, SEEK_SET);
	   fread(&baseReloc, sizeof(BASE_RELOCATION), 1, fp);
	   printf("base relocations:\n\n");
	   while ( baseReloc.SizeOfBlock != 0 )
	   {
		unsigned i, cEntries;
		WORD pEntry;
		char *szRelocType;
		WORD relocType;

		cEntries = (baseReloc.SizeOfBlock - sizeof(BASE_RELOCATION)) / sizeof(WORD);

		printf("Virtual Address: %08lX  size: %08lX\n",
				baseReloc.VirtualAddress,
				baseReloc.SizeOfBlock);

		for ( i=0; i < cEntries; i++ )
		{
		   // Extract the top 4 bits of the relocation entry.  Turn those 4
		   // bits into an appropriate descriptive string (szRelocType)

		   fread(&pEntry, sizeof(WORD), 1, fp);
		   relocType = (pEntry & 0xF000) >> 12;
		   szRelocType = (relocType < 8) ? SzRelocTypes[relocType] : "unknown";
		   printf("%s  %08lX\n", (char *)szRelocType,
				     (DWORD)(pEntry & 0x0FFF) + baseReloc.VirtualAddress);
		}
		fread(&baseReloc, sizeof(BASE_RELOCATION), 1, fp);
	   }
	}
}

LONG get_file_size(FILE *fp)
{

    LONG size;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    return size;

}

unsigned int loader_blocks;
LONG total_blocks, total_size, image_blocks, initrd_blocks;
FILE *init, *loader, *target, *image, *initrd;
LONG manos_found = 0, manos_offset = 0;
LONG init_offset = 0, init_size, image_present = 0, initrd_present = 0;
LONG image_size, initrd_size, system_offset, init_blocks, initrd_offset;
LONG data_virt_size, data_offset, data_blocks, data_size;
LONG code_virt_size, code_offset, code_blocks, code_size;
LONG reloc_virt_size, reloc_offset, reloc_blocks, reloc_size;
LONG debug_size, data_rva, code_rva, debug_offset;
LONG code_target_offset, data_target_offset, reloc_target_offset;
LONG debug_virt_size, debug_blocks, debug_rva, debug_target_offset;
LONG code_virt_blocks, data_virt_blocks, reloc_virt_blocks, debug_virt_blocks;
LONG adj_blocks = 0, virt_blocks;

int main(int argc, char *argv[])
{

     register unsigned long i;
     init = loader = target = image = NULL;

     if (argc < 3 || !argv[1][0] || !argv[2][0])
     {
	    printf("USAGE:  llink <init> <loader> [optional <image> <initrd>]\n");
	    return (0);
     }

     if ((init = fopen(argv[1], "rb")) == NULL)
     {
	    printf("could not open init file %s\n", argv[1]);
	    return 0;
     }

     if ((loader = fopen(argv[2], "rb")) == NULL)
     {
	    printf("could not open loader file %s\n", argv[2]);
	    fclose(init);
	    return 0;
     }

     if ((image = fopen(argv[3], "rb")) == NULL)
     {
	     printf("output being written to DRLXLOAD.EXE\n");
	     image_present = 0;
	     if ((target = fopen("DRLXLOAD.EXE", "wb")) == NULL)
	     {
	        fclose(init);
	        fclose(loader);
	        return 0;
	     }
     }
     else
     {
	    image_present = 1;
	    if ((target = fopen(argv[4], "wb")) == NULL)
	    {
	       fclose(init);
	       fclose(loader);
	       fclose(image);
	       printf("could not open output file %s\n", argv[4]);
	       return 0;
	    }
	    printf("output being written to %s\n", argv[4]);
     }

     if (argv[5] && ((initrd = fopen(argv[5], "rb")) == NULL))
     {
	    printf("could not open initrd file %s\n", argv[5]);
	    fclose(init);
	    fclose(loader);
	    fclose(image);
	    fclose(target);
	    return 0;
     }

     if (initrd)
        initrd_present = 1;

     if (init)
     {
	if (is_nepe(init))
	       printf("%s is a Windows NE/LE/PE executable format\n", argv[1]);
        else
	       printf("%s is an DR-DOS executable format\n", argv[1]);
     }

     if (loader)
	 {
	    if (is_nepe(loader))
	       printf("%s is a Windows executable format\n", argv[2]);
        else
	       printf("%s is an DR-DOS executable format\n", argv[2]);
     }

     if (image)
     {
	     printf("%s is an objdump binary image\n", argv[3]);
     }

     if (initrd)
     {
	     printf("%s is an initrd image\n", argv[5]);
     }

     code_virt_size = get_section_virt_size("CODE", loader);
     code_blocks = (code_virt_size + 511) / 512; // compute total code blocks
     code_offset = get_section("CODE", loader);
     code_rva = get_section_rva("CODE", loader);
     code_size = get_section_size("CODE", loader);
     manos_header.code_size = code_size;
     manos_header.code_rva = code_rva;
     manos_header.code_virt_size = code_virt_size;
     if (code_size % 512)
     {
	     printf("LLINK FATAL: code_segment error modulo 512\n");
	     fclose(target);
	     fclose(init);
	     fclose(loader);
	     if (image_present)
	        fclose(image);
	     if (initrd_present)
	        fclose(initrd);
	     return 0;
     }

     data_virt_size = get_section_virt_size("DATA", loader);
     data_blocks = (data_virt_size + 511) / 512; // compute total data blocks
     data_offset = get_section("DATA", loader);
     data_rva = get_section_rva("DATA", loader);
     data_size = get_section_size("DATA", loader);
     manos_header.data_size = data_size;
     manos_header.data_rva = data_rva;
     manos_header.data_virt_size = data_virt_size;
     if (data_size % 512)
     {
	    printf("LLINK FATAL: data_segment error modulo 512\n");
	    fclose(target);
	    fclose(init);
	    fclose(loader);
	    if (image_present)
	       fclose(image);
	     if (initrd_present)
	        fclose(initrd);
	    return 0;
     }

     reloc_virt_size = get_section_virt_size(".reloc", loader);
     reloc_blocks = (reloc_virt_size + 511) / 512; // compute total reloc blocks
     reloc_offset = get_section(".reloc", loader);
     reloc_size = get_section_size(".reloc", loader);
     manos_header.reloc_size = reloc_size;
     if (reloc_size % 512)
     {
	    printf("LLINK FATAL: reloc_segment error modulo 512\n");
	    fclose(target);
	    fclose(init);
	    fclose(loader);
	    if (image_present)
	       fclose(image);
	     if (initrd_present)
	        fclose(initrd);
	    return 0;
     }

     debug_virt_size = get_section_virt_size(".debug", loader);
     debug_blocks = (debug_virt_size + 511) / 512; // compute total debug blocks
     debug_offset = get_section(".debug", loader);
     debug_rva = get_section_rva(".debug", loader);
     debug_size = get_section_size(".debug", loader);
     manos_header.debug_size = debug_size;
     if (debug_rva) {};

     init_size = get_file_size(init);
     init_blocks = (init_size + 511) / 512; // compute total image blocks

     if (image_present)
     {
	    image_size = get_file_size(image);
	    image_blocks = (image_size + 4095) / 4096; // compute total image blocks
     }

     if (initrd_present)
     {
	    initrd_size = get_file_size(initrd);
	    initrd_blocks = (initrd_size + 4095) / 4096; // compute total image blocks
     }

     fseek(init, 0L, SEEK_SET);
     fread(&iexe, EXE_HEADER_SIZE, 1, init);
     iexe.p.blocks_in_file = init_blocks;
     iexe.p.bytes_in_last_block = 0;    // round to a whole page

     for (i=0; i < EXE_HEADER_SIZE; i++)  // copy init header
	    texe.header[i] = iexe.header[i];

     loader_blocks = (code_blocks + data_blocks + reloc_blocks + debug_blocks);
     texe.p.blocks_in_file += loader_blocks;
     texe.p.bytes_in_last_block = 0;    // round to a whole page

     virt_blocks = (code_virt_size + data_virt_size + reloc_virt_size +
                   debug_virt_size + 511) / 512;
     if (virt_blocks > loader_blocks)
        adj_blocks = virt_blocks - loader_blocks;


     printf("added %i loader blocks, and is now %i blocks virt_adj-%i\n",
	    loader_blocks, texe.p.blocks_in_file, adj_blocks);

     total_blocks = texe.p.blocks_in_file;
     total_size = (LONG) texe.p.blocks_in_file * 512;

     printf("total size %ld bytes (0x%08lX)\n", total_size, total_size);

     // write header and search for INITCODE_START in the code segment
     // the thunk header is below this label

     fseek(target, 0, SEEK_SET);
     fwrite(&texe, EXE_HEADER_SIZE, 1, target);  // write modified header

     for (i=0; i < init_blocks - 1; i++)  // less one for the header
     {
	    memset(sector, 0x00, 512);
	    fread(sector, 512L, 1, init);
	    if (!manos_found)
	    {
	       BYTE s[16] = { 'I', 'N', 'I', 'T', '_', 'C', 'O', 'D',
			              'E', '_', 'S', 'T', 'A', 'R', 'T', '_' };
	       register int r;

	       for (r=0; r < 512; r++)
	       {
	           if (!memcmp(&s[0], &sector[r], 16))
	           {
		          init_offset = ftell(target) + r;
		          manos_offset = ftell(target) + r + 16;
		          manos_found = 1;
		          printf("manos header at %08lX, init code at %08lX\n",
			              manos_offset, init_offset);
		          break;
	           }
	       }
	    }
	    fwrite(sector, 512L, 1, target);
     }

     code_target_offset = ftell(target);
     fseek(loader, code_offset, SEEK_SET);
     for (i=0; i < code_blocks; i++)
     {
	    memset(sector, 0x00, 512);
	    if (code_size)
	    {
	       fread(sector, 512L, 1, loader);
	       code_size -= 512;
	    }
	    fwrite(sector, 512L, 1, target);
     }

     data_target_offset = ftell(target);
     fseek(loader, data_offset, SEEK_SET);
     for (i=0; i < data_blocks; i++)
     {
	    memset(sector, 0x00, 512);
	    if (data_size)
	    {
	       fread(sector, 512L, 1, loader);
	       data_size -= 512;
	    }
	    fwrite(sector, 512L, 1, target);
     }

     reloc_target_offset = ftell(target);
     fseek(loader, reloc_offset, SEEK_SET);
     for (i=0; i < reloc_blocks; i++)
     {
	    memset(sector, 0x00, 512);
	    if (reloc_size)
	    {
	       fread(sector, 512L, 1, loader);
	       reloc_size -= 512;
	    }
	    fwrite(sector, 512L, 1, target);
     }

     debug_target_offset = ftell(target);
     fseek(loader, debug_offset, SEEK_SET);
     for (i=0; i < debug_blocks; i++)
     {
	    memset(sector, 0x00, 512);
	    fread(sector, 512L, 1, loader);
	    fwrite(sector, 512L, 1, target);
     }

     //
     //  append the OS image to the end of the file
     //

     if (image_present)
     {

	    system_offset = ftell(target);
	    fseek(image, 0L, SEEK_SET);
	    for (i=0; i < image_blocks; i++)
	    {
	       memset(sector, 0x00, 4096);
	       fread(sector, 4096L, 1, image);
	       fwrite(sector, 4096L, 1, target);
	    }
     }
     else
     {
        system_offset = 0;
     }

     if (initrd_present)
     {

	    initrd_offset = ftell(target);
	    fseek(initrd, 0L, SEEK_SET);
	    for (i=0; i < initrd_blocks; i++)
	    {
	       memset(sector, 0x00, 4096);
	       fread(sector, 4096L, 1, initrd);
	       fwrite(sector, 4096L, 1, target);
	    }
     }
     else
     {
        initrd_offset = 0;
     }

     //  these fixups are relative to the start of INITCODE

     manos_header.blocks_in_file      = texe.p.blocks_in_file;
     manos_header.virtual_blocks      = texe.p.blocks_in_file + adj_blocks;
     manos_header.offset_of_16_code   = init_offset - init_offset;
     manos_header.offset_of_32_code   = code_target_offset - init_offset;
     manos_header.offset_of_32_data   = data_target_offset - init_offset;
     manos_header.offset_reloc        = reloc_target_offset - init_offset;
     manos_header.offset_debug        = debug_target_offset - init_offset;
     manos_header.system_offset       = system_offset;
     manos_header.system_size         = image_size;
     manos_header.initrd_offset       = initrd_offset;
     manos_header.initrd_size         = initrd_size;
     manos_header.pe_base             = get_pe_base(loader);
     if (!manos_header.pe_base)
	   manos_header.pe_base = 0x400000;

     fseek(target, manos_offset, SEEK_SET);
     fwrite(&manos_header, sizeof(MANOS), 1, target);

     printf("loader 16-bit code offset  :: %08lX  :: %08lX\n", manos_header.offset_of_16_code,
								init_offset);
     printf("loader header offset       :: %08lX  :: %08lX\n", manos_offset - init_offset,
								manos_offset);
     printf("loader 'CODE'              :: %08lX  :: %08lX  :: %08lX\n", manos_header.offset_of_32_code,
								code_target_offset, code_offset);
     printf("loader 'DATA'              :: %08lX  :: %08lX  :: %08lX\n", manos_header.offset_of_32_data,
								data_target_offset, data_offset);
     printf("loader reloc table         :: %08lX  :: %08lX  :: %08lX\n", manos_header.offset_reloc,
								reloc_target_offset, reloc_offset);
     printf("loader debug offset        :: %08lX  :: %08lX  :: %08lX\n", manos_header.offset_debug,
								debug_target_offset, debug_offset);
     printf("loader .code RVA           :: %08lX\n", manos_header.code_rva);
     printf("loader .data RVA           :: %08lX\n", manos_header.data_rva);
     printf("loader virtual code size   :: %08lX\n", manos_header.code_virt_size);
     printf("loader virtual data size   :: %08lX\n", manos_header.data_virt_size);
     printf("loader raw code size       :: %08lX\n", manos_header.code_size);
     printf("loader raw data size       :: %08lX\n", manos_header.data_size);
     printf("loader reloc size          :: %08lX\n", manos_header.reloc_size);
     printf("loader debug size          :: %08lX\n", manos_header.debug_size);
     printf("loader image offset        :: %08lX\n", manos_header.system_offset);
     printf("loader image size          :: %08lX\n", manos_header.system_size);
     printf("loader initrd offset       :: %08lX\n", manos_header.initrd_offset);
     printf("loader initrd size         :: %08lX\n", manos_header.initrd_size);

     fclose(init);
     fclose(loader);
     if (image_present)
	    fclose(image);
     if (initrd_present)
            fclose(initrd);
     fclose(target);

     return 0;


}



