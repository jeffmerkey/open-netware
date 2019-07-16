
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
*   FILE     :  TYPES.H
*   DESCRIP  :  Typedefs for MANOS v1.0
*   DATE     :  December 10, 1997
*
*
***************************************************************************/

typedef unsigned long LONG;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int UINT;
typedef unsigned long LONG;
typedef unsigned long DWORD;
typedef short SHORT;
typedef int BOOL;
typedef BYTE * LPBYTE;
typedef BYTE * PBYTE;
typedef char * PSTR;
typedef char * LPSTR;
typedef void * PVOID;
typedef void * LPVOID;
typedef DWORD * LPDWORD;
typedef WORD * LPWORD;
typedef char CHAR;
typedef unsigned int WCHAR;
typedef int INT;
typedef WORD * PWORD;
typedef WORD * LPWORD;
typedef DWORD * PDWORD;
typedef DWORD * LPDWORD;
#define VOID void

#define	uint32	unsigned long
#define uint8 	unsigned char
#define TRUE    1
#define FALSE   0

#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD)(ptr) + (addValue) )
