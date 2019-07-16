

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  PEDUMP.C
*   DESCRIP  :  Portable Executable Dump Utility for MANOS v1.0
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
#include "dosfile.h"
#include "ifs.h"

// Global variables set here, and used in EXEDUMP.C and OBJDUMP.C
int fShowRelocations = FALSE;
int fShowRawSectionData = FALSE;
int fShowSymbolTable = FALSE;
int fShowLineNumbers = FALSE;

char HelpText[] =
"PEDUMP - Win32/COFF .EXE/.OBJ file dumper - 1993 Matt Pietrek\n\n"
"Syntax: PEDUMP [switches] filename\n\n"
"  /A    show everything in dump\n"
"  /H    show hex dump of sections\n"
"  /L    show line number information\n"
"  /R    show base relocations\n"
"  /S    show symbol table\n";

//
// Open up a file, memory map it, and call the appropriate dumping routine
//

LONG DumpPEFile(SCREEN *screen, BYTE *filename)
{
	LONG fd, retCode, bytesRead;
	BYTE *FileBase, *fp;
	LONG size, i;
	PIMAGE_DOS_HEADER dosHeader;

	fd = IFSOpenFile(filename, O_RDONLY | O_DENYNONE);
	if (!fd)
	   return -1;

	size = IFSFileSizeHandled(fd);
	size = ((size + 511) * 512) / 512;

	FileBase = (BYTE *) malloc(size);
	if (!FileBase)
	{
	   IFSCloseFile(fd);
	   return -1;
	}

	printf("reading file %s ....\n", filename);
	fp = FileBase;
	for (i=0; i < size; i += 512)
	{
	   bytesRead = IFSReadFile(fd, fp, 512);
	   if (!bytesRead)
	      break;
	   fp = (BYTE *)((LONG)fp + (LONG)512);
	}
	IFSCloseFile(fd);

	printf("Dump of file %s\n", filename);
	dosHeader = (PIMAGE_DOS_HEADER) FileBase;
	if ( dosHeader->e_magic == IMAGE_DOS_SIGNATURE )
	{
	   DumpExeFile( screen, dosHeader );
	}
	else if ( (dosHeader->e_magic == 0x014C)	// Does it look like a i386
		   && (dosHeader->e_sp == 0) )		// COFF OBJ file???
	{
	   // The two tests above aren't what they look like.  They're
	   // really checking for IMAGE_FILE_HEADER.Machine == i386 (0x14C)
	   // and IMAGE_FILE_HEADER.SizeOfOptionalHeader == 0;

	   DumpObjFile( screen, (PIMAGE_FILE_HEADER) FileBase);
	}
	else
	   printf("\nunrecognized file format\n");

	free(FileBase);
	return 0;

}

BYTE *dumpFileData(SCREEN *screen, BYTE *p, LONG count, LONG *offset)
{

    BYTE *symbolName;
    register LONG i, r, total;
    BYTE str[9];

    printfScreenWithAttribute(screen, BRITEWHITE, "\n           0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");

    for (r=0; r < count; r++)
    {
       printfScreenWithAttribute(screen, BRITEWHITE, "\n%08X ", *offset);
       for (total = 0, i=0; i < 16; i++, total++)
       {
	  printfScreenWithAttribute(screen, LTCYAN, " %02X", (BYTE) p[i]);
       }
       printfScreen(screen, "  ");
       for (i=0; i < total; i++)
       {
	  if (p[i] < 32 || p[i] > 126) printfScreenWithAttribute(screen, BRITEWHITE, ".");
	  else printfScreenWithAttribute(screen, BRITEWHITE, "%c", p[i]);
       }
       p = (void *)((LONG) p + (LONG) total);
       *offset += total;
    }
    printfScreen(screen, "\n");

    return p;

}

LONG DumpFile(SCREEN *screen, BYTE *filename)
{
    LONG fd, retCode, bytesRead, offset;
    BYTE *FileBase, *fp;
    LONG size, i;

    fd = IFSOpenFile(filename, O_RDONLY | O_DENYNONE);
    if (!fd)
       return -1;

    size = IFSFileSizeHandled(fd);
    size = ((size + 511) * 512) / 512;

    FileBase = (BYTE *) malloc(size);
    if (!FileBase)
    {
       IFSCloseFile(fd);
       return -1;
    }

    printf("reading file %s ....\n", filename);
    fp = FileBase;
    for (i=0; i < size; i += 512)
    {
       bytesRead = IFSReadFile(fd, fp, 512);
       if (!bytesRead)
	  break;
       fp = (BYTE *)((LONG)fp + (LONG)512);
    }
    IFSCloseFile(fd);

    printf("Dump of file %s\n", filename);
    fp = FileBase;

    for (offset = i = 0; i < size; i += 256)
    {
       fp = dumpFileData(screen, fp, 256 / 16, &offset);
       if (Pause())
	  break;
    }
    free(FileBase);
    return 0;

}

