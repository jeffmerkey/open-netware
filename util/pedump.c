
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

extern void DumpExeFile( PIMAGE_DOS_HEADER dosHeader );
extern void DumpObjFile( PIMAGE_FILE_HEADER pImageFileHeader );

//
// Open up a file, memory map it, and call the appropriate dumping routine
//
void DumpFile(char *filename)
{
	FILE *fp;
	char far *hFileMapping;
	char far *lpFileBase;
	LONG size = 0, index = 0;
	PIMAGE_DOS_HEADER dosHeader;

	if ((fp = fopen(filename, "rb")) == NULL)
	{
	   printf("could not open file %s\n", filename);
	   return;
	}

	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	size = ((size + 511) * 512) / 512; // round up to 512 boundry
	hFileMapping = (char far *) _fmalloc(size);
	if (!hFileMapping)
	{
	   fclose(fp);
	   printf("could not alloc memory for file %s\n", filename);
	   return;
	}

	index = 0;
	while (!feof(fp))
	{
	   fread((void *)&hFileMapping[index], 512, 1, fp);
	   index += 512L;
	}

	lpFileBase = hFileMapping;
	printf("Dump of file %s\n\n", filename);

	dosHeader = (PIMAGE_DOS_HEADER) lpFileBase;
	if ( dosHeader->e_magic == IMAGE_DOS_SIGNATURE )
	{
	   DumpExeFile( dosHeader );
	}
	else if ( (dosHeader->e_magic == 0x014C)	// Does it look like a i386
		   && (dosHeader->e_sp == 0) )		// COFF OBJ file???
	{
		// The two tests above aren't what they look like.  They're
		// really checking for IMAGE_FILE_HEADER.Machine == i386 (0x14C)
		// and IMAGE_FILE_HEADER.SizeOfOptionalHeader == 0;
			
		DumpObjFile( (PIMAGE_FILE_HEADER) lpFileBase);
	}
	else
		printf("unrecognized file format\n");

	_ffree(hFileMapping);
	fclose(fp);

}

//
// process all the command line arguments and return a pointer to
// the filename argument.
//
char *ProcessCommandLine(int argc, char *argv[])
{
	int i;

	for ( i=1; i < argc; i++ )
	{
	   strupr(argv[i]);

	   // Is it a switch character?
	   if ( (argv[i][0] == '-') || (argv[i][0] == '/') )
	   {
		if ( argv[i][1] == 'A' )
		{
			fShowRelocations = TRUE;
			fShowRawSectionData = TRUE;
			fShowSymbolTable = TRUE;
			fShowLineNumbers = TRUE;
		}
		else if ( argv[i][1] == 'H' )
			fShowRawSectionData = TRUE;
		else if ( argv[i][1] == 'L' )
			fShowLineNumbers = TRUE;
		else if ( argv[i][1] == 'R' )
			fShowRelocations = TRUE;
		else if ( argv[i][1] == 'S' )
			fShowSymbolTable = TRUE;
	   }
	   else	// Not a switch character.  Must be the filename
	   {
	      return argv[i];
	   }
	}
	return 0;

}

int main(int argc, char *argv[])
{
	char *filename;

	if ( argc == 1 )
	{
	      printf(	HelpText );
	      return 1;
	}

	filename = ProcessCommandLine(argc, argv);
	if ( filename )
		DumpFile( filename );

	return 0;
}
