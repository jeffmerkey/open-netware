
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  CODEVIEW.H
*   DESCRIP  :  Microsoft/Borland Codeview Support for MANOS v1.0
*   DATE     :  January 10, 1998
*
***************************************************************************/

#include "types.h"

// CodeView Subsection Header Types

#define  MODULE        0x120
#define  TYPES         0x121
#define  PUBLIC        0x122
#define  PUBLIC_SYM    0x123
#define  SYMBOLS       0x124
#define  ALIGN_SYM     0x125
#define  SRC_LINE_SEG  0x126
#define  SRC_MODULE    0x127
#define  LIBRARIES     0x128
#define  GLOBAL_SYMS   0x129
#define  GLOBAL_TYPES  0x12B
#define  MPC           0x12C
#define  NAMES         0x130

// CodeView Symbol Types

#define  SIZE_LENGTH_FIELD   2
#define  SIZE_NAME_FIELD     2

#define  S_COMPILE      0x0001
#define  S_REGISTER     0x0002
#define  S_CONST        0x0003
#define  S_UDT          0x0004
#define  S_SSEARCH      0x0005
#define  S_END          0x0006
#define  S_SKIP         0x0007
#define  S_CVRESERVE    0x0008
#define  S_OBJNAME      0x0009
#define  S_GPROCREF     0x0020
#define  S_GDATAREF     0x0021
#define  S_EDATA        0x0022
#define  S_EPROC        0x0023

#define  S_BPREL16      0x0100
#define  S_LDATA16      0x0101
#define  S_GDATA16      0x0102
#define  S_PUB16        0x0103
#define  S_LPROC16      0x0104
#define  S_GPROC16      0x0105
#define  S_THUNK16      0x0106
#define  S_BLOCK16      0x0107
#define  S_WITH16       0x0108
#define  S_LABEL16      0x0109
#define  S_CEXMODEL16   0x010A
#define  S_VFTPATH16    0x010B

#define  S_BPREL32      0x0200
#define  S_LDATA32      0x0201
#define  S_GDATA32      0x0202
#define  S_PUB32        0x0203
#define  S_LPROC32      0x0204
#define  S_GPROC32      0x0205
#define  S_THUNK32      0x0206
#define  S_BLOCK32      0x0207
#define  S_WITH32       0x0208
#define  S_LABEL32      0x0209
#define  S_CEXMODEL32   0x020A
#define  S_VFTPATH32    0x020B
#define  S_ENTRY32      0x0210
#define  S_OPTVAR32     0x0211
#define  S_PROCRET32    0x0212
#define  S_SAVEREGS32   0x0213

// CodeView Leaf Indice Types

#define  LF_MODIFIER    0x0001
#define  LF_POINTER     0x0002
#define  LF_ARRAY       0x0003
#define  LF_CLASS       0x0004
#define  LF_STRUCTURE   0x0005
#define  LF_UNION       0x0006
#define  LF_ENUM        0x0007
#define  LF_PROCEDURE   0x0008
#define  LF_MFUNCTION   0x0009
#define  LF_VTSHAPE     0x000A
#define  LF_COBOL0      0x000B
#define  LF_COBOL1      0x000C
#define  LF_BARRAY      0x000D
#define  LF_LABEL       0x000E
#define  LF_NULL        0x000F
#define  LF_NOTTRAN     0x0010
#define  LF_DIMARRAY    0x0011
#define  LF_VFTPATH     0x0012

#define  LF_SKIP        0x0200
#define  LF_ARGLIST     0x0201
#define  LF_DEFARG      0x0202
#define  LF_LIST        0x0203
#define  LF_FIELDLIST   0x0204
#define  LF_DERIVED     0x0205
#define  LF_BITFIELD    0x0206
#define  LF_METHODLIST  0x0207
#define  LF_DIMCONU     0x0208
#define  LF_DIMCONLU    0x0209
#define  LF_DIMVARU     0x020A
#define  LF_DIMVARLU    0x020B
#define  LF_REFSYM      0x020C

#define  LF_BCLASS      0x0400
#define  LF_VBCCLASS    0x0401
#define  LF_IVBCLASS    0x0402
#define  LF_ENUMERATE   0x0403
#define  LF_FRIENDFCH   0x0404
#define  LF_INDEX       0x0405
#define  LF_MEMBER      0x0406
#define  LF_STMEMBER    0x0407
#define  LF_METHOD      0x0408
#define  LF_NESTTYPE    0x0409
#define  LF_VFUNCTAB    0x040A
#define  LF_FRIENDCLS   0x040B

#define  LF_NUMERIC     0x8000
#define  LF_CHAR        0x8000
#define  LF_SHORT       0x8001
#define  LF_USHORT      0x8002
#define  LF_LONG        0x8003
#define  LF_ULONG       0x8004
#define  LF_REAL32      0x8005
#define  LF_REAL64      0x8006
#define  LF_REAL80      0x8007
#define  LF_REAL128     0x8008
#define  LF_QUADWORD    0x8009
#define  LF_NQUADWORD   0x800A
#define  LF_REAL48      0x800B

#define  LF_PAD0        0xF0
#define  LF_PAD1        0xF1
#define  LF_PAD2        0xF2
#define  LF_PAD3        0xF3
#define  LF_PAD4        0xF4
#define  LF_PAD5        0xF5
#define  LF_PAD6        0xF6
#define  LF_PAD7        0xF7
#define  LF_PAD8        0xF8
#define  LF_PAD9        0xF9
#define  LF_PAD10       0xFA
#define  LF_PAD11       0xFB
#define  LF_PAD12       0xFC
#define  LF_PAD13       0xFD
#define  LF_PAD14       0xFE
#define  LF_PAD15       0xFF

//
//   BORLAND CV Header Structures
//

typedef struct _DEBUG_SIGNATURE {
   LONG Version;
   LONG Offset;
} DEBUG_SIGNATURE;

typedef struct _SUBSECTION_HEADER {
   WORD headerLength;
   WORD entryLength;
   LONG numberOfEntries;
   LONG nextDirectory;
   LONG flags;
} SUBSECTION_HEADER;

typedef struct _DIRECTORY_HEADER {
   WORD directoryType;
   WORD moduleIndex;
   LONG entryOffset;
   LONG sizeOfEntry;
} DIRECTORY_HEADER;

typedef struct _NAME_SECTION {
   LONG NumberOfNames;
} NAME_SECTION;

typedef struct _SYMBOL_HEADER {
   WORD length;
   WORD symbolType;
} SYMBOL_HEADER;

typedef struct _NAME_HEADER {
   BYTE length;
   BYTE Name[1];
} NAME_HEADER;

//
//  Global Symbol Table Data Structures
//

typedef struct _GLOBAL_SYM_HEADER {
   LONG Reserved1;
   LONG SizeOfSymbols;
   LONG Reserved2;
   LONG Reserved3;
   LONG NumberOfUDTs;
   LONG NumberOfOthers;
   LONG TotalSymbols;
   WORD SymbolHash;
   WORD AddressHash;
} GLOBAL_SYM_HEADER;

typedef struct _DEBUGREF_TYPE {
   WORD length;
   WORD symbolType;
   LONG Offset;
   LONG Type;
   LONG Name;
   LONG browseOffset;
   LONG symbolOffset;
   LONG symbolSegment;
} DEBUGREF;

typedef struct _UDT_TYPE {
   WORD length;
   WORD symbolType;
   LONG Type;
   WORD Property;
   LONG Name;
   LONG bOffset;
} UDT;

typedef struct _GPROCREF_TYPE {
   WORD length;
   WORD symbolType;
   LONG Offset;
   LONG Type;
   LONG Name;
   LONG bOffset;
   LONG cOffset;
   LONG segment;
} GPROCREF;

typedef struct _GDATAREF_TYPE {
   WORD length;
   WORD symbolType;
   LONG Offset;
   LONG Type;
   LONG Name;
   LONG bOffset;
   LONG dOffset;
   LONG segment;
} GDATAREF;

typedef struct _ENTRY32_TYPE {
   WORD length;
   WORD symbolType;
   LONG Offset;
   LONG Segment;
} ENTRY32;

typedef struct _GPROC32_TYPE {
   WORD length;
   WORD symbolType;
   LONG Parent;
   LONG End;
   LONG Next;
   LONG ProcLength;
   LONG DebugStart;
   LONG DebugEnd;
   LONG Offset;
   WORD Segment;
   LONG Type;
   BYTE NearFar;
   LONG Name;
} GPROC32;

typedef struct _GDATA32_TYPE {
   WORD length;
   WORD symbolType;
   LONG Offset;
   LONG Segment;
   LONG Type;
   LONG Name;
} GDATA32;

//
//  Line Information Structures
//

typedef struct _SOURCE_HEADER {
   WORD NumberOfFiles;
   WORD NumberOfSegments;
} SOURCE_HEADER;

typedef struct _START_END {
   LONG Start;
   LONG End;
} START_END;

typedef struct _SOURCE_INDICE_TABLE {
   LONG Offset;
} SOURCE_INDICE_TABLE;

typedef struct _SEGMENT_TABLE {
   WORD Offset;
} SEGMENT_TABLE;

typedef struct _MODULE_INFO {
   WORD Overlay;
   WORD LibraryIndice;
   WORD Segment;
   WORD Style;
   LONG Name;
   LONG TimeStamp;
   LONG Reserved[3];
   LONG SegmentInfo[1];
} MODULE_INFO;

typedef struct _SOURCE_FILE_TABLE {
   WORD NumberOfSegments;
   LONG Name;
} SOURCE_FILE_TABLE;

typedef struct _BASE_SOURCE_LINE {
   LONG BaseSourceLine;
} BASE_SOURCE_LINE;

typedef struct _LINE_NUMBER_DATA {
   WORD Segment;
   WORD NumberOfLinePairs;
} LINE_NUMBER_DATA;

typedef struct _LINE_OFFSET {
   LONG Offset;
} LINE_OFFSET;

typedef struct _LINE_NUMBER {
   WORD LineNumber;
} LINE_NUMBER;

