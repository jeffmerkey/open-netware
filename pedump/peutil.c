

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

BOOL IsBadReadPtr(void)
{
     return 0;
}

