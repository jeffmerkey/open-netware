

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
*   FILE     :  CODEVIEW.C
*   DESCRIP  :  Microsoft/Borland Codeview Support for MANOS v1.0
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
#include "dosfile.h"
#include "peexe.h"
#include "line.h"
#include "loader.h"
#include "malloc.h"
#include "free.h"
#include "trees.h"
#include "symbols.h"
#include "exports.h"
#include "codeview.h"

// OS initialization only
extern LONG StartOfHighMemory;
extern LONG HighMemoryLength;

LONG CVStartOfHighMemory;
LONG CVHighMemoryLength;
LONG CVMemorySize = 0;

BYTE *GetSymbolDescription(LONG type)
{
    switch (type)
    {
       case S_GDATA32:
	  return "S_GDATA32   ";
       case S_GPROC32:
	  return "S_GPROC32   ";
       case S_GPROCREF:
	  return "S_GPROCREF  ";
       case S_GDATAREF:
	  return "S_GDATAREF  ";
       case S_UDT:
	  return "S_UDT       ";
       case S_ENTRY32:
	  return "S_ENTRY32   ";
       case S_COMPILE:
	  return "S_COMPILE   ";
       case S_REGISTER:
	  return "S_REGISTER  ";
       case S_CONST:
	  return "S_CONST     ";
       case S_SSEARCH:
	  return "S_SSEARCH   ";
       case S_END:
	  return "S_END       ";
       case S_SKIP:
	  return "S_SKIP      ";
       case S_CVRESERVE:
	  return "S_CVRESERVE ";
       case S_OBJNAME:
	  return "S_OBJNAME   ";
       case S_EDATA:
	  return "S_EDATA     ";
       case S_EPROC:
	  return "S_EPROC     ";
       case S_BPREL16:
	  return "S_BPREL16   ";
       case S_LDATA16:
	  return "S_LDATA16   ";
       case S_GDATA16:
	  return "S_GDATA16   ";
       case S_PUB16:
	  return "S_PUB16     ";
       case S_LPROC16:
	  return "S_LPROC16   ";
       case S_GPROC16:
	  return "S_GPROC16   ";
       case S_THUNK16:
	  return "S_THUNK16   ";
       case S_BLOCK16:
	  return "S_BLOCK16   ";
       case S_WITH16:
	  return "S_WITH16    ";
       case S_LABEL16:
	  return "S_LABEL16   ";
       case S_CEXMODEL16:
	  return "S_CEXMODEL16";
       case S_VFTPATH16:
	  return "S_VFTPATH16 ";
       case S_BPREL32:
	  return "S_BPREL32   ";
       case S_LDATA32:
	  return "S_LDATA32   ";
       case S_PUB32:
	  return "S_PUB32     ";
       case S_LPROC32:
	  return "S_LPROC32   ";
       case S_THUNK32:
	  return "S_THUNK32   ";
       case S_BLOCK32:
	  return "S_BLOCK32   ";
       case S_WITH32:
	  return "S_WITH32    ";
       case S_LABEL32:
	  return "S_LABEL32   ";
       case S_CEXMODEL32:
	  return "S_CEXMODEL32";
       case S_VFTPATH32:
	  return "S_VFTPATH32 ";
       case S_OPTVAR32:
	  return "S_OPTVAR32  ";
       case S_PROCRET32:
	  return "S_PROCRET32 ";
       case S_SAVEREGS32:
	  return "S_SAVEREGS32";

       default:
	  return "UNKNOWN_TYPE";

    }

}

LONG InitAddModuleSymbols(MODULE_HANDLE *module)
{
    register LONG i, address, type, segment, nameIndex;
    register SYMBOL_HEADER *symHeader;
    register DEBUGREF *debugRef;
    register ENTRY32 *entry;
    register GDATA32 *gdata;
    register GPROC32 *gproc;
    register UDT *udt;
    register BYTE *name;
    register SYMBOL *symbol;

    //  NOTE:  symbols that are imported are annotated with a segment
    //  index of '0xFFFF' rather than a valid segment number.  If any
    //  symbol has an advertized segment greater than the number of module
    //  sections, then ignore the symbol and do not add it.  It is assumed
    //  that the symbol is already exported somewhere else.

    for (i=1; i < module->NumberOfSymbols + 1; i++)
    {
       symHeader = (SYMBOL_HEADER *) module->SymbolIndexTable[i];
       type = symHeader->symbolType;
       switch (type)
       {
	  case S_GDATA32:
	     gdata = (GDATA32 *) symHeader;
	     segment = (gdata->Segment <= module->NumberOfSections)
		       ? gdata->Segment
		       : 0;
	     nameIndex = (gdata->Name <= module->NumberOfNames)
		       ? gdata->Name
		       : 0;
	     if (segment && nameIndex)
	     {
		address = module->SegmentIndexTable[segment] + gdata->Offset;
		name = (BYTE *) module->NameIndexTable[nameIndex];
		symbol = InitCreatePublicSymbol(module, name, address, type,
				    FORMAT_BORLAND, 0, gdata->Type,
				    SYMBOL_DATA);
		if (!symbol)
		   return -1;
	     }
	     break;

	  case S_GPROC32:
	     gproc = (GPROC32 *) symHeader;
	     segment = (gproc->Segment <= module->NumberOfSections)
		       ? gproc->Segment
		       : 0;
	     nameIndex = (gproc->Name <= module->NumberOfNames)
		       ? gproc->Name
		       : 0;
	     if (segment && nameIndex)
	     {
		address = module->SegmentIndexTable[segment] + gproc->Offset;
		name = (BYTE *) module->NameIndexTable[nameIndex];
		symbol = InitCreatePublicSymbol(module, name, address, type,
				    FORMAT_BORLAND, 0, gproc->Type,
				    SYMBOL_PROCEDURE);
		if (!symbol)
		   return -1;
	     }
	     break;

	  case S_GPROCREF:
	     debugRef = (DEBUGREF *) symHeader;
	     segment = (debugRef->symbolSegment <= module->NumberOfSections)
		       ? debugRef->symbolSegment
		       : 0;
	     nameIndex = (debugRef->Name <= module->NumberOfNames)
		       ? debugRef->Name
		       : 0;
	     if (segment && nameIndex)
	     {
		address = module->SegmentIndexTable[segment] + debugRef->symbolOffset;
		name = (BYTE *) module->NameIndexTable[nameIndex];
		symbol = InitCreatePublicSymbol(module, name, address, type,
				    FORMAT_BORLAND, 0, debugRef->Type,
				    SYMBOL_PROCEDURE);
		if (!symbol)
		   return -1;
	     }
	     break;

	  case S_GDATAREF:
	     debugRef = (DEBUGREF *) symHeader;
	     segment = (debugRef->symbolSegment <= module->NumberOfSections)
		       ? debugRef->symbolSegment
		       : 0;
	     nameIndex = (debugRef->Name <= module->NumberOfNames)
		       ? debugRef->Name
		       : 0;
	     if (segment && nameIndex)
	     {
		address = module->SegmentIndexTable[segment] + debugRef->symbolOffset;
		name = (BYTE *) module->NameIndexTable[nameIndex];
		symbol = InitCreatePublicSymbol(module, name, address, type,
				    FORMAT_BORLAND, 0, debugRef->Type,
				    SYMBOL_DATA);
		if (!symbol)
		   return -1;
	     }
	     break;

	  case S_UDT:
	     udt = (UDT *) symHeader;
	     nameIndex = (udt->Name <= module->NumberOfNames)
		       ? udt->Name
		       : 0;
	     if (nameIndex)
	     {
		address = udt->bOffset;
		name = (BYTE *) module->NameIndexTable[nameIndex];
		symbol = InitCreatePublicSymbol(module, name, address, type,
				    FORMAT_BORLAND, 0, udt->Type,
				    SYMBOL_CONTROL);
		if (!symbol)
		   return -1;
	     }
	     break;

	  case S_ENTRY32:
	     entry = (ENTRY32 *) symHeader;
	     segment = (entry->Segment <= module->NumberOfSections)
		       ? entry->Segment
		       : 0;
	     if (segment)
	     {
		address = module->SegmentIndexTable[segment] + entry->Offset;
		symbol = InitCreatePublicSymbol(module, "ENTRY", address, type,
				    FORMAT_BORLAND, 0, 0,
				    SYMBOL_ENTRY);
		if (!symbol)
		   return -1;
	     }
	     break;

	  case S_COMPILE:
	  case S_REGISTER:
	  case S_CONST:
	  case S_SSEARCH:
	  case S_END:
	  case S_SKIP:
	  case S_CVRESERVE:
	  case S_OBJNAME:
	  case S_EDATA:
	  case S_EPROC:

	  case S_BPREL16:
	  case S_LDATA16:
	  case S_GDATA16:
	  case S_PUB16:
	  case S_LPROC16:
	  case S_GPROC16:
	  case S_THUNK16:
	  case S_BLOCK16:
	  case S_WITH16:
	  case S_LABEL16:
	  case S_CEXMODEL16:
	  case S_VFTPATH16:

	  case S_BPREL32:
	  case S_LDATA32:
	  case S_PUB32:
	  case S_LPROC32:
	  case S_THUNK32:
	  case S_BLOCK32:
	  case S_WITH32:
	  case S_LABEL32:
	  case S_CEXMODEL32:
	  case S_VFTPATH32:
	  case S_OPTVAR32:
	  case S_PROCRET32:
	  case S_SAVEREGS32:
	     break;

	  default:
	     break;

       }
    }
    return 0;

}

LONG AddModuleSymbols(MODULE_HANDLE *module)
{
    register LONG i, address, type, segment, nameIndex;
    register SYMBOL_HEADER *symHeader;
    register DEBUGREF *debugRef;
    register ENTRY32 *entry;
    register GDATA32 *gdata;
    register GPROC32 *gproc;
    register UDT *udt;
    register BYTE *name;
    register SYMBOL *symbol;

    //  NOTE:  symbols that are imported are annotated with a segment
    //  index of '0xFFFF' rather than a valid segment number.  If any
    //  symbol has an advertized segment greater than the number of module
    //  sections, then ignore the symbol and do not add it.  It is assumed
    //  that the symbol is already exported somewhere else.

    for (i=1; i < module->NumberOfSymbols + 1; i++)
    {
       symHeader = (SYMBOL_HEADER *) module->SymbolIndexTable[i];
       type = symHeader->symbolType;
       switch (type)
       {
	  case S_GDATA32:
	     gdata = (GDATA32 *) symHeader;
	     segment = (gdata->Segment <= module->NumberOfSections)
		       ? gdata->Segment
		       : 0;
	     nameIndex = (gdata->Name <= module->NumberOfNames)
		       ? gdata->Name
		       : 0;
	     if (segment && nameIndex)
	     {
		address = module->SegmentIndexTable[segment] + gdata->Offset;
		name = (BYTE *) module->NameIndexTable[nameIndex];
		symbol = CreatePublicSymbol(module, name, address, type,
				    FORMAT_BORLAND, 0, gdata->Type,
				    SYMBOL_DATA);
		if (!symbol)
		   return -1;
	     }
	     break;

	  case S_GPROC32:
	     gproc = (GPROC32 *) symHeader;
	     segment = (gproc->Segment <= module->NumberOfSections)
		       ? gproc->Segment
		       : 0;
	     nameIndex = (gproc->Name <= module->NumberOfNames)
		       ? gproc->Name
		       : 0;
	     if (segment && nameIndex)
	     {
		address = module->SegmentIndexTable[segment] + gproc->Offset;
		name = (BYTE *) module->NameIndexTable[nameIndex];
		symbol = CreatePublicSymbol(module, name, address, type,
				    FORMAT_BORLAND, 0, gproc->Type,
				    SYMBOL_PROCEDURE);
		if (!symbol)
		   return -1;
	     }
	     break;

	  case S_GPROCREF:
	     debugRef = (DEBUGREF *) symHeader;
	     segment = (debugRef->symbolSegment <= module->NumberOfSections)
		       ? debugRef->symbolSegment
		       : 0;
	     nameIndex = (debugRef->Name <= module->NumberOfNames)
		       ? debugRef->Name
		       : 0;
	     if (segment && nameIndex)
	     {
		address = module->SegmentIndexTable[segment] + debugRef->symbolOffset;
		name = (BYTE *) module->NameIndexTable[nameIndex];
		symbol = CreatePublicSymbol(module, name, address, type,
				    FORMAT_BORLAND, 0, debugRef->Type,
				    SYMBOL_PROCEDURE);
		if (!symbol)
		   return -1;
	     }
	     break;

	  case S_GDATAREF:
	     debugRef = (DEBUGREF *) symHeader;
	     segment = (debugRef->symbolSegment <= module->NumberOfSections)
		       ? debugRef->symbolSegment
		       : 0;
	     nameIndex = (debugRef->Name <= module->NumberOfNames)
		       ? debugRef->Name
		       : 0;
	     if (segment && nameIndex)
	     {
		address = module->SegmentIndexTable[segment] + debugRef->symbolOffset;
		name = (BYTE *) module->NameIndexTable[nameIndex];
		symbol = CreatePublicSymbol(module, name, address, type,
				    FORMAT_BORLAND, 0, debugRef->Type,
				    SYMBOL_DATA);
		if (!symbol)
		   return -1;
	     }
	     break;

	  case S_UDT:
	     udt = (UDT *) symHeader;
	     nameIndex = (udt->Name <= module->NumberOfNames)
		       ? udt->Name
		       : 0;
	     if (nameIndex)
	     {
		address = udt->bOffset;
		name = (BYTE *) module->NameIndexTable[nameIndex];
		symbol = CreatePublicSymbol(module, name, address, type,
				    FORMAT_BORLAND, 0, udt->Type,
				    SYMBOL_CONTROL);
		if (!symbol)
		   return -1;
	     }
	     break;

	  case S_ENTRY32:
	     entry = (ENTRY32 *) symHeader;
	     segment = (entry->Segment <= module->NumberOfSections)
		       ? entry->Segment
		       : 0;
	     if (segment)
	     {
		address = module->SegmentIndexTable[segment] + entry->Offset;
		symbol = CreatePublicSymbol(module, "ENTRY", address, type,
				    FORMAT_BORLAND, 0, 0,
				    SYMBOL_ENTRY);
		if (!symbol)
		   return -1;
	     }
	     break;

	  case S_COMPILE:
	  case S_REGISTER:
	  case S_CONST:
	  case S_SSEARCH:
	  case S_END:
	  case S_SKIP:
	  case S_CVRESERVE:
	  case S_OBJNAME:
	  case S_EDATA:
	  case S_EPROC:

	  case S_BPREL16:
	  case S_LDATA16:
	  case S_GDATA16:
	  case S_PUB16:
	  case S_LPROC16:
	  case S_GPROC16:
	  case S_THUNK16:
	  case S_BLOCK16:
	  case S_WITH16:
	  case S_LABEL16:
	  case S_CEXMODEL16:
	  case S_VFTPATH16:

	  case S_BPREL32:
	  case S_LDATA32:
	  case S_PUB32:
	  case S_LPROC32:
	  case S_THUNK32:
	  case S_BLOCK32:
	  case S_WITH32:
	  case S_LABEL32:
	  case S_CEXMODEL32:
	  case S_VFTPATH32:
	  case S_OPTVAR32:
	  case S_PROCRET32:
	  case S_SAVEREGS32:
	     break;

	  default:
	     break;

       }
    }
    return 0;
}

LONG GetValueFromLine(LONG ModuleIndex,
		      LONG FileIndex,
		      LONG SegmentIndex,
		      LONG LineNumber,
		      MODULE_HANDLE *module,
		      LONG *debugInfoPresent)
{

    register LONG i, r, k, j, y, x, tmp, address;
    register LONG Start, End, lastLine;
    register SOURCE_HEADER *source;
    register SOURCE_INDICE_TABLE *indice;
    register START_END *lineStart;
    register SOURCE_INDICE_TABLE *lineIndice;
    register SOURCE_FILE_TABLE *fileTable;
    register BASE_SOURCE_LINE *baseSourceLine;
    register LINE_NUMBER_DATA *lineNumberData;
    register LINE_OFFSET *lineOffset;
    register LINE_NUMBER *lineNumber;

    if (debugInfoPresent)
       *debugInfoPresent = 0;

    if (!module->SourceNumber)
    {
       if (debugInfoPresent)
	  *debugInfoPresent = 0;
       return -1;
    }

    if (debugInfoPresent)
       *debugInfoPresent = 1;

    if (ModuleIndex > module->SourceNumber)
       return -1;

    i = ModuleIndex;
    source = (SOURCE_HEADER *) module->SourceIndexTable[i];

    tmp = (LONG) source + sizeof(SOURCE_HEADER);
    indice = (SOURCE_INDICE_TABLE *) tmp;

    if (FileIndex > source->NumberOfFiles)
       return -1;

    r = FileIndex;
    tmp = (LONG) source + indice[r].Offset;
    fileTable = (SOURCE_FILE_TABLE *) tmp;

    tmp = (LONG) fileTable + sizeof(SOURCE_FILE_TABLE);
    lineIndice = (SOURCE_INDICE_TABLE *) tmp;

    if (SegmentIndex > fileTable->NumberOfSegments)
       return -1;

    k = SegmentIndex;

    tmp = (LONG) source + lineIndice[k].Offset;
    lineNumberData = (LINE_NUMBER_DATA *) tmp;

    tmp = (LONG) lineNumberData + sizeof(LINE_NUMBER_DATA);
    lineOffset = (LINE_OFFSET *) tmp;

    tmp = (LONG) lineOffset + (lineNumberData->NumberOfLinePairs * 4);
    lineNumber = (LINE_NUMBER *) tmp;

    for (j = 0; j < lineNumberData->NumberOfLinePairs; j++)
    {
       address = lineOffset[j].Offset + module->SegmentIndexTable[k + 1];
       if (LineNumber == lineNumber[j].LineNumber)
	  return address;
    }
    return -1;

}

LONG GetLineInfoFromValue(LONG address,
			  SOURCE_LINE_INFO *info,
			  MODULE_HANDLE **newModule,
			  LONG *totalLines,
			  LONG *debugInfoPresent,
			  LONG *exact)
{

    register LONG i, r, k, j, y, tmp;
    register LONG Start, End, lastLine;
    register SOURCE_HEADER *source;
    register SOURCE_INDICE_TABLE *indice;
    register START_END *lineStart;
    register SOURCE_INDICE_TABLE *lineIndice;
    register SOURCE_FILE_TABLE *fileTable;
    register BASE_SOURCE_LINE *baseSourceLine;
    register LINE_NUMBER_DATA *lineNumberData;
    register LINE_OFFSET *lineOffset;
    register LINE_NUMBER *lineNumber;
    register MODULE_HANDLE *module;

    if (newModule)
       *newModule = 0;

    if (totalLines)
       *totalLines = 0;

    if (debugInfoPresent)
       *debugInfoPresent = 0;

    if (exact)
       *exact = 0;

    module = GetModuleHandleFromValue(address);
    if (!module)
       return 0;

    if (!module->SourceNumber)
    {
       if (newModule)
	  *newModule = module;
       if (debugInfoPresent)
	  *debugInfoPresent = 0;
       return 0;
    }

    if (debugInfoPresent)
       *debugInfoPresent = 1;

    for (i=0; i < module->SourceNumber; i++)
    {
       source = (SOURCE_HEADER *) module->SourceIndexTable[i];

       tmp = (LONG) source + sizeof(SOURCE_HEADER);
       indice = (SOURCE_INDICE_TABLE *) tmp;

       for (r=0; r < source->NumberOfFiles; r++)
       {
	  tmp = (LONG) source + indice[r].Offset;
	  fileTable = (SOURCE_FILE_TABLE *) tmp;

	  tmp = (LONG) fileTable + sizeof(SOURCE_FILE_TABLE);
	  lineIndice = (SOURCE_INDICE_TABLE *) tmp;

	  tmp = (LONG) lineIndice + (fileTable->NumberOfSegments * 4);
	  lineStart = (START_END *) tmp;

	  for (k=0; k < fileTable->NumberOfSegments; k++)
	  {
	     Start = lineStart[k].Start + module->SegmentIndexTable[k + 1];
	     End = lineStart[k].End + module->SegmentIndexTable[k + 1];

	     if (address >= Start && address <= End)
	     {
		tmp = (LONG) source + lineIndice[k].Offset;
		lineNumberData = (LINE_NUMBER_DATA *) tmp;

		tmp = (LONG) lineNumberData + sizeof(LINE_NUMBER_DATA);
		lineOffset = (LINE_OFFSET *) tmp;

		tmp = (LONG) lineOffset + (lineNumberData->NumberOfLinePairs * 4);
		lineNumber = (LINE_NUMBER *) tmp;

		for (lastLine = j = 0; j < lineNumberData->NumberOfLinePairs; j++)
		{
		   register LONG lineAddress = lineOffset[j].Offset + module->SegmentIndexTable[k + 1];

		   if (address == lineAddress && lineNumber[j].LineNumber)
		   {
		      if (exact)
			 *exact = 1;

		      if (j < lineNumberData->NumberOfLinePairs)
		      {
			 info->ModuleName = (BYTE *) module->NameIndexTable[fileTable->Name];
			 info->Segment = k;
			 info->Offset = lineOffset[j].Offset + module->SegmentIndexTable[k + 1];
			 info->LineNumber = lineNumber[j].LineNumber;
			 info->LineAddress = lineOffset[j].Offset + module->SegmentIndexTable[k + 1];
			 info->FileIndex = r;
			 info->SourceIndex = i;
			 info->SourcePresent = 0;
			 info->ModuleIndex = 0;
			 info->LineTable = 0;
			 info->SourceLine = 0;
			 for (y = 1; y < (module->SourceNumber + 1); y++)
			 {
			    if (module->crossIndex[y].Name &&
				!strcmp(info->ModuleName,
				module->crossIndex[y].Name))
			    {
			       register LONG *ndx;

			       info->SourcePresent = 1;
			       info->ModuleIndex = y;
			       info->LineTable = ndx = (LONG *) module->SourceLineIndex[y];
			       if (totalLines)
				  *totalLines = module->SourceSizeIndex[y];
			       if (ndx)
				  info->SourceLine = (BYTE *) ndx[info->LineNumber];
			    }
			 }
			 j++;
		      }
		      else
		      {
			 info->ModuleName = 0;
			 info->Segment = 0;
			 info->Offset = 0;
			 info->LineNumber = 0;
			 info->LineAddress = 0;
			 info->SourcePresent = 0;
			 info->SourceIndex = 0;
			 info->FileIndex = 0;
			 info->ModuleIndex = 0;
			 info->LineTable = 0;
			 info->SourceLine = 0;
		      }

		      if (newModule)
			 *newModule = module;

		      return 1;
		   }
		   else
		   if (address < lineAddress && lineNumber[j].LineNumber)
		   {
		      if (j < lineNumberData->NumberOfLinePairs)
		      {
			 info->ModuleName = (BYTE *) module->NameIndexTable[fileTable->Name];
			 info->Segment = k;
			 info->Offset = lineOffset[lastLine].Offset + module->SegmentIndexTable[k + 1];
			 info->LineNumber = lineNumber[lastLine].LineNumber;
			 info->LineAddress = lineOffset[lastLine].Offset + module->SegmentIndexTable[k + 1];
			 info->FileIndex = r;
			 info->SourceIndex = i;
			 info->SourcePresent = 0;
			 info->ModuleIndex = 0;
			 info->LineTable = 0;
			 info->SourceLine = 0;
			 for (y = 1; y < (module->SourceNumber + 1); y++)
			 {
			    if (module->crossIndex[y].Name &&
				!strcmp(info->ModuleName,
				module->crossIndex[y].Name))
			    {
			       register LONG *ndx;

			       info->SourcePresent = 1;
			       info->ModuleIndex = y;
			       info->LineTable = ndx = (LONG *) module->SourceLineIndex[y];
			       if (totalLines)
				  *totalLines = module->SourceSizeIndex[y];
			       if (ndx)
				  info->SourceLine = (BYTE *) ndx[info->LineNumber];
			    }
			 }
			 j++;
		      }
		      else
		      {
			 info->ModuleName = 0;
			 info->Segment = 0;
			 info->Offset = 0;
			 info->LineNumber = 0;
			 info->LineAddress = 0;
			 info->SourcePresent = 0;
			 info->SourceIndex = 0;
			 info->FileIndex = 0;
			 info->ModuleIndex = 0;
			 info->LineTable = 0;
			 info->SourceLine = 0;
		      }

		      if (newModule)
			 *newModule = module;

		      return 1;
		   }
		   lastLine = j;
		}
	     }
	  }
       }
    }
    return 0;

}

void *InitMalloc(LONG size)
{

    register LONG length;
    void *p = 0;

    // align on 16 byte boundries

    length = (size + 0xF) & 0xFFFFFFF0;
    if (length < HighMemoryLength)
    {
       p = (void *) StartOfHighMemory;
       StartOfHighMemory += length;
       HighMemoryLength -= length;
       CVMemorySize += length;
    }
    return p;

}

// this call will return all memory allocated by the base OS for CodeView

void InitFree(MODULE_HANDLE *module)
{
    SetData((LONG *)CVStartOfHighMemory, 0, CVMemorySize);
    StartOfHighMemory = CVStartOfHighMemory;
    HighMemoryLength = CVHighMemoryLength;
    CVMemorySize = CVStartOfHighMemory = CVHighMemoryLength = 0;
    module->sourceIndex = 0;
    module->SourceNumber = 0;
    module->ModuleNumber = 0;
    module->SourceIndex = 0;
    module->ModuleIndex = 0;
    module->NameSection = 0;
    module->NumberOfSymbols = 0;
    module->NumberOfNames = 0;
    module->NumberOfSections = 0;
    module->DebugSymbolVersion = 0;
    module->GlobalSymbolTable = 0;
    module->GlobalTypeTable = 0;
    module->DebugBaseAddress = 0;
    module->ModuleIndexTable = 0;
    module->SourceIndexTable = 0;
    module->NameIndexTable = 0;
    module->SymbolIndexTable = 0;
    module->SourceCodeIndex = 0;
    module->SourceLineIndex = 0;
    module->SourceSizeIndex = 0;
    module->crossIndex = 0;
    module->defaultProcess = 0;

}

LONG ReadSourceFile(MODULE_HANDLE *module, BYTE *filename)
{

    LONG fd;
    register BYTE *buf;
    register LONG size, bytesRead, total = 0, tmp, filesize;

    fd = DOSOpen(strupr(filename), O_RDONLY | O_DENYNONE);
    if (fd)
    {
       filesize = DOSLseek(fd, 0L, SEEK_END);
       buf = InitMalloc(filesize);
       if (buf)
       {
	  module->SourceCodeIndex[module->sourceIndex] = (LONG) buf;
	  DOSLseek(fd, 0L, SEEK_SET);
	  module->crossIndex[module->sourceIndex].Name = filename;
	  module->crossIndex[module->sourceIndex].Index = module->sourceIndex;
	  bytesRead = 1;
	  while (bytesRead)
	  {
	     bytesRead = DOSRead(fd, buf, 4096);
	     if (bytesRead != 4096)
	     {
		total += bytesRead;
		break;
	     }
	     total += bytesRead;
	     tmp = (LONG) buf + bytesRead;
	     buf = (BYTE *) tmp;
	  }
	  DOSClose(fd);
	  return total;
       }
       module->SourceCodeIndex[module->sourceIndex] = 0;
    }
    return 0;

}

LONG GetSourceFiles(MODULE_HANDLE *module)
{

    register LONG i, r, k, j, t, filesize, lines;
    register LONG Start, End, *ndx;
    register MODULE_INFO *moduleInfo;
    register BYTE *buffer;

    for (module->sourceIndex = 1, i = 1; i < (module->ModuleNumber + 1); i++, \
	 module->sourceIndex++)
    {
       moduleInfo = (MODULE_INFO *) module->ModuleIndexTable[i];
       filesize = ReadSourceFile(module,
			   (BYTE *) module->NameIndexTable[moduleInfo->Name]);
       if (filesize)
       {

	  buffer = (BYTE *) module->SourceCodeIndex[module->sourceIndex];
	  lines = 1;
	  lines++;
	  for (k = 0; k < filesize; k++)
	  {
	     if (buffer[k] == '\n')
		lines++;
	  }

	  module->SourceSizeIndex[module->sourceIndex] = lines;
	  module->SourceLineIndex[module->sourceIndex] = (LONG) InitMalloc(lines * sizeof(LONG));
	  if (!module->SourceLineIndex[module->sourceIndex])
	     return -1;

	  ndx = (LONG *) module->SourceLineIndex[module->sourceIndex];
	  buffer = (BYTE *) module->SourceCodeIndex[module->sourceIndex];
	  ndx[0] = 0;
	  lines = 1;
	  ndx[lines++] = (LONG) &buffer[0];
	  for (k = 0; k < filesize; k++)
	  {
	     if (buffer[k] == '\r')
		buffer[k] = '\0';

	     if (buffer[k] == '\n')
	     {
		buffer[k] = '\0';
		ndx[lines++] = (LONG) &buffer[k+1];
	     }
	  }
	  buffer[k] = '\0';
       }
       else
       {
	  module->SourceCodeIndex[module->sourceIndex] = 0;
	  module->crossIndex[module->sourceIndex].Name = (BYTE *) module->NameIndexTable[moduleInfo->Name];
	  module->crossIndex[module->sourceIndex].Index = module->sourceIndex;
	  module->SourceSizeIndex[module->sourceIndex] = 0;
	  module->SourceLineIndex[module->sourceIndex] = 0;
       }
    }
    return 0;

}

LONG InitializeSymbolTable(MODULE_HANDLE *module)
{

    register LONG i, val, mem;
    register LONG *p, delta;
    register DIRECTORY_HEADER *header;
    register DEBUG_SIGNATURE *sig;
    register GLOBAL_SYM_HEADER *symTable;
    register SYMBOL_HEADER *symHeader;
    register NAME_HEADER *nameHeader;
    register GPROCREF *procRef;
    register GDATAREF *dataRef;
    register NAME_SECTION *NameTableHeader;
    register SUBSECTION_HEADER *SubSectionHeader;

    CVStartOfHighMemory = StartOfHighMemory;
    CVHighMemoryLength = HighMemoryLength;

    if (!module->DebugSegmentSize)
       return -1;

    //  Borland _CV_ lfanew is at endOfDebugSection - 8 bytes

    val = module->DebugSegmentPtr + module->DebugSegmentSize - 8;
    sig = (DEBUG_SIGNATURE *) val;

    // check for Borland debug signature at end of image

    if (strncmp((BYTE *)sig, "FB", 2))
       return -1;

    module->DebugSymbolVersion = sig->Version;
    module->DebugBaseAddress = module->DebugSegmentPtr + module->DebugSegmentSize - sig->Offset;

    sig = (DEBUG_SIGNATURE *) module->DebugBaseAddress;
    val = module->DebugBaseAddress + sig->Offset;
    SubSectionHeader = (SUBSECTION_HEADER *) val;

    val = (LONG) SubSectionHeader + SubSectionHeader->headerLength;
    header = (DIRECTORY_HEADER *) val;

    for (i=1, module->SourceNumber = 0, module->ModuleNumber = 0;
	 i < SubSectionHeader->numberOfEntries + 1; i++)
    {
       if (header->directoryType == SRC_MODULE)
	  module->SourceNumber++;

       if (header->directoryType == MODULE)
	  module->ModuleNumber++;

       if (header->directoryType == GLOBAL_SYMS)
	  module->GlobalSymbolTable = module->DebugBaseAddress \
				      + header->entryOffset;

       if (header->directoryType == GLOBAL_TYPES)
	  module->GlobalTypeTable = module->DebugBaseAddress \
				    + header->entryOffset;

       if (header->directoryType == NAMES)
	  module->NameSection = module->DebugBaseAddress \
				+ header->entryOffset;

       val = (LONG)header + SubSectionHeader->entryLength;
       header = (DIRECTORY_HEADER *) val;

    }

    module->SourceIndexTable = InitMalloc((module->SourceNumber + 1) * sizeof(LONG));
    if (!module->SourceIndexTable)
    {
       InitFree(module);
       return -1;
    }
    else
       SetData(module->SourceIndexTable, 0,
	      (module->SourceNumber + 1) * sizeof(LONG));

    val = (LONG) SubSectionHeader + SubSectionHeader->headerLength;
    header = (DIRECTORY_HEADER *) val;
    for (i=1; i < SubSectionHeader->numberOfEntries + 1; i++)
    {
       if (header->directoryType == SRC_MODULE)
	  module->SourceIndexTable[module->SourceIndex++] = \
	     module->DebugBaseAddress + header->entryOffset;

       val = (LONG)header + SubSectionHeader->entryLength;
       header = (DIRECTORY_HEADER *) val;
    }

    module->ModuleIndexTable =InitMalloc((module->ModuleNumber + 1) * sizeof(LONG));
    if (!module->ModuleIndexTable)
    {
       InitFree(module);
       return -1;
    }
    else
       SetData(module->ModuleIndexTable, 0,
	      (module->ModuleNumber + 1) * sizeof(LONG));

    val = (LONG) SubSectionHeader + SubSectionHeader->headerLength;
    header = (DIRECTORY_HEADER *) val;
    for (i=1; i < SubSectionHeader->numberOfEntries + 1; i++)
    {
       if (header->directoryType == MODULE)
	  module->ModuleIndexTable[i] = module->DebugBaseAddress \
					+ header->entryOffset;
       val = (LONG)header + SubSectionHeader->entryLength;
       header = (DIRECTORY_HEADER *) val;
    }

    NameTableHeader = (NAME_SECTION *) module->NameSection;
    module->NumberOfNames = NameTableHeader->NumberOfNames;
    module->NameIndexTable = InitMalloc((module->NumberOfNames + 1) * sizeof(LONG));
    if (!module->NameIndexTable)
    {
       InitFree(module);
       return -1;
    }
    else
       SetData(module->NameIndexTable, 0,
	      (module->NumberOfNames + 1) * sizeof(LONG));



    val = (LONG) module->NameSection + sizeof(NAME_SECTION);
    nameHeader = (NAME_HEADER *) val;
    for (i=1; i < module->NumberOfNames + 1; i++)
    {
       module->NameIndexTable[i] = (LONG) nameHeader + 1;
       val = (LONG) nameHeader + nameHeader->length + SIZE_NAME_FIELD;
       nameHeader = (NAME_HEADER *) val;
    }

    symTable = (GLOBAL_SYM_HEADER *) module->GlobalSymbolTable;
    module->NumberOfSymbols = symTable->TotalSymbols;
    module->SymbolIndexTable = InitMalloc((module->NumberOfSymbols + 1) * sizeof(LONG));
    if (!module->SymbolIndexTable)
    {
       InitFree(module);
       return -1;
    }
    else
       SetData(module->SymbolIndexTable, 0,
	      (module->NumberOfSymbols + 1) * sizeof(LONG));

    val = (LONG) symTable + sizeof(GLOBAL_SYM_HEADER);
    symHeader = (SYMBOL_HEADER *) val;
    for (i=1; i < module->NumberOfSymbols + 1; i++)
    {
       module->SymbolIndexTable[i] = (LONG) symHeader;
       val = (LONG) symHeader + symHeader->length + SIZE_LENGTH_FIELD;
       symHeader = (SYMBOL_HEADER *) val;
    }

    module->SourceCodeIndex = InitMalloc((module->SourceNumber + 1) * sizeof(LONG));
    if (!module->SourceCodeIndex)
    {
       InitFree(module);
       return -1;
    }
    else
       SetData(module->SourceCodeIndex, 0,
	      (module->SourceNumber + 1) * sizeof(LONG));

    module->SourceLineIndex = InitMalloc((module->SourceNumber + 1) * sizeof(LONG));
    if (!module->SourceLineIndex)
    {
       InitFree(module);
       return -1;
    }
    else
       SetData(module->SourceLineIndex, 0,
	      (module->SourceNumber + 1) * sizeof(LONG));

    module->SourceSizeIndex = InitMalloc((module->SourceNumber + 1) * sizeof(LONG));
    if (!module->SourceSizeIndex)
    {
       InitFree(module);
       return -1;
    }
    else
       SetData(module->SourceSizeIndex, 0,
	      (module->SourceNumber + 1) * sizeof(LONG));

    module->crossIndex = InitMalloc((module->SourceNumber + 1) * sizeof(LONG) * 2);
    if (!module->crossIndex)
    {
       InitFree(module);
       return -1;
    }
    else
       SetData((LONG *)module->crossIndex, 0,
	      (module->SourceNumber + 1) * sizeof(LONG));

    if (GetSourceFiles(module))
    {
       InitFree(module);
       return -1;
    }

    if (InitAddModuleSymbols(module))
    {
       InitFree(module);
       return -1;
    }

    return 0;

}

// Module Initialization

LONG ModuleReadSourceFile(MODULE_HANDLE *module, BYTE *filename)
{

    LONG fd;
    register BYTE *buf;
    register LONG size, bytesRead, total = 0, val, filesize;

    fd = DOSOpen(strupr(filename), O_RDONLY | O_DENYNONE);
    if (fd)
    {
       filesize = DOSLseek(fd, 0L, SEEK_END);
       buf = kmalloc(filesize + sizeof(LONG));
       if (buf)
       {
	  module->SourceCodeIndex[module->sourceIndex] = (LONG) buf;
	  DOSLseek(fd, 0L, SEEK_SET);
	  module->crossIndex[module->sourceIndex].Name = filename;
	  module->crossIndex[module->sourceIndex].Index = module->sourceIndex;
	  bytesRead = 1;
	  while (bytesRead)
	  {
	     bytesRead = DOSRead(fd, buf, 4096);
	     if (bytesRead != 4096)
	     {
		total += bytesRead;
		break;
	     }
	     total += bytesRead;
	     val = (LONG) buf + bytesRead;
	     buf = (BYTE *) val;
	  }
	  DOSClose(fd);
	  return total;
       }
       module->SourceCodeIndex[module->sourceIndex] = 0;
    }
    return 0;

}

LONG ModuleGetSourceFiles(MODULE_HANDLE *module)
{

    register LONG i, r, k, j, t, filesize, lines;
    register LONG Start, End, *ndx;
    register MODULE_INFO *moduleInfo;
    register BYTE *buffer;

    for (module->sourceIndex = 1, i = 1; i < (module->ModuleNumber + 1); i++, \
	 module->sourceIndex++)
    {
       moduleInfo = (MODULE_INFO *) module->ModuleIndexTable[i];
       filesize = ModuleReadSourceFile(module,
			   (BYTE *) module->NameIndexTable[moduleInfo->Name]);
       if (filesize)
       {
	  buffer = (BYTE *) module->SourceCodeIndex[module->sourceIndex];
	  lines = 1;
	  lines++;
	  for (k = 0; k < filesize; k++)
	  {
	     if (buffer[k] == '\n')
		lines++;
	  }

	  module->SourceSizeIndex[module->sourceIndex] = lines;
	  module->SourceLineIndex[module->sourceIndex] = (LONG) kmalloc(lines * sizeof(LONG));
	  if (!module->SourceLineIndex[module->sourceIndex])
	     return -1;

	  ndx = (LONG *) module->SourceLineIndex[module->sourceIndex];
	  buffer = (BYTE *) module->SourceCodeIndex[module->sourceIndex];
	  ndx[0] = 0;
	  lines = 1;
	  ndx[lines++] = (LONG) &buffer[0];
	  for (k = 0; k < filesize; k++)
	  {
	     if (buffer[k] == '\r')
		buffer[k] = '\0';

	     if (buffer[k] == '\n')
	     {
		buffer[k] = '\0';
		ndx[lines++] = (LONG) &buffer[k + 1];
	     }
	  }
	  buffer[k] = '\0';
       }
       else
       {
	  module->SourceCodeIndex[module->sourceIndex] = 0;
	  module->crossIndex[module->sourceIndex].Name = (BYTE *) module->NameIndexTable[moduleInfo->Name];
	  module->crossIndex[module->sourceIndex].Index = module->sourceIndex;
	  module->SourceSizeIndex[module->sourceIndex] = 0;
	  module->SourceLineIndex[module->sourceIndex] = 0;
       }
    }
    return 0;

}

LONG ModuleInitializeSymbolTable(MODULE_HANDLE *module)
{

    register LONG i, val, mem;
    register LONG *p, delta;
    register DIRECTORY_HEADER *header;
    register DEBUG_SIGNATURE *sig;
    register GLOBAL_SYM_HEADER *symTable;
    register SYMBOL_HEADER *symHeader;
    register NAME_HEADER *nameHeader;
    register GPROCREF *procRef;
    register GDATAREF *dataRef;
    register NAME_SECTION *NameTableHeader;
    register SUBSECTION_HEADER *SubSectionHeader;

    if (!module->DebugSegmentSize)
       return -1;

    //  Borland _CV_ lfanew is at endOfDebugSection - 8 bytes

    val = module->DebugSegmentPtr + module->DebugSegmentSize - 8;
    sig = (DEBUG_SIGNATURE *) val;

    // check for Borland debug signature at end of image

    if (strncmp((BYTE *)sig, "FB", 2))
       return -1;

    module->DebugSymbolVersion = sig->Version;
    module->DebugBaseAddress = module->DebugSegmentPtr + \
			       module->DebugSegmentSize - sig->Offset;

    sig = (DEBUG_SIGNATURE *) module->DebugBaseAddress;
    val = module->DebugBaseAddress + sig->Offset;
    SubSectionHeader = (SUBSECTION_HEADER *) val;

    val = (LONG) SubSectionHeader + SubSectionHeader->headerLength;
    header = (DIRECTORY_HEADER *) val;

    for (i=1, module->SourceNumber = 0, module->ModuleNumber = 0;
	 i < SubSectionHeader->numberOfEntries + 1; i++)
    {
       if (header->directoryType == SRC_MODULE)
	  module->SourceNumber++;

       if (header->directoryType == MODULE)
	  module->ModuleNumber++;

       if (header->directoryType == GLOBAL_SYMS)
	  module->GlobalSymbolTable = module->DebugBaseAddress \
				      + header->entryOffset;

       if (header->directoryType == GLOBAL_TYPES)
	  module->GlobalTypeTable = module->DebugBaseAddress \
				    + header->entryOffset;

       if (header->directoryType == NAMES)
	  module->NameSection = module->DebugBaseAddress \
				+ header->entryOffset;

       val = (LONG)header + SubSectionHeader->entryLength;
       header = (DIRECTORY_HEADER *) val;

    }

    module->SourceIndexTable = kmalloc((module->SourceNumber + 1) * sizeof(LONG));
    if (!module->SourceIndexTable)
    {
       ModuleFreeSymbolTable(module);
       return -1;
    }
    else
       SetData(module->SourceIndexTable, 0,
	      (module->SourceNumber + 1) * sizeof(LONG));

    val = (LONG) SubSectionHeader + SubSectionHeader->headerLength;
    header = (DIRECTORY_HEADER *) val;
    for (i=1; i < SubSectionHeader->numberOfEntries + 1; i++)
    {
       if (header->directoryType == SRC_MODULE)
	  module->SourceIndexTable[module->SourceIndex++] = \
	     module->DebugBaseAddress + header->entryOffset;

       val = (LONG)header + SubSectionHeader->entryLength;
       header = (DIRECTORY_HEADER *) val;
    }

    module->ModuleIndexTable = kmalloc((module->ModuleNumber + 1) * sizeof(LONG));
    if (!module->ModuleIndexTable)
    {
       ModuleFreeSymbolTable(module);
       return -1;
    }
    else
       SetData(module->ModuleIndexTable, 0,
	      (module->ModuleNumber + 1) * sizeof(LONG));

    val = (LONG) SubSectionHeader + SubSectionHeader->headerLength;
    header = (DIRECTORY_HEADER *) val;
    for (i=1; i < SubSectionHeader->numberOfEntries + 1; i++)
    {
       if (header->directoryType == MODULE)
	  module->ModuleIndexTable[i] = module->DebugBaseAddress + header->entryOffset;

       val = (LONG)header + SubSectionHeader->entryLength;
       header = (DIRECTORY_HEADER *) val;
    }

    NameTableHeader = (NAME_SECTION *) module->NameSection;
    module->NumberOfNames = NameTableHeader->NumberOfNames;
    module->NameIndexTable = kmalloc((module->NumberOfNames + 1) * sizeof(LONG));
    if (!module->NameIndexTable)
    {
       ModuleFreeSymbolTable(module);
       return -1;
    }
    else
       SetData(module->NameIndexTable, 0,
	      (module->NumberOfNames + 1) * sizeof(LONG));

    val = (LONG) module->NameSection + sizeof(NAME_SECTION);
    nameHeader = (NAME_HEADER *) val;
    for (i=1; i < module->NumberOfNames + 1; i++)
    {
       module->NameIndexTable[i] = (LONG) nameHeader + 1;
       val = (LONG) nameHeader + nameHeader->length + SIZE_NAME_FIELD;
       nameHeader = (NAME_HEADER *) val;
    }

    symTable = (GLOBAL_SYM_HEADER *) module->GlobalSymbolTable;
    module->NumberOfSymbols = symTable->TotalSymbols;
    module->SymbolIndexTable = kmalloc((module->NumberOfSymbols + 1) * sizeof(LONG));
    if (!module->SymbolIndexTable)
    {
       ModuleFreeSymbolTable(module);
       return -1;
    }
    else
       SetData(module->SymbolIndexTable, 0,
	      (module->NumberOfSymbols + 1) * sizeof(LONG));

    val = (LONG) symTable + sizeof(GLOBAL_SYM_HEADER);
    symHeader = (SYMBOL_HEADER *) val;
    for (i=1; i < module->NumberOfSymbols + 1; i++)
    {
       module->SymbolIndexTable[i] = (LONG) symHeader;
       val = (LONG) symHeader + symHeader->length + SIZE_LENGTH_FIELD;
       symHeader = (SYMBOL_HEADER *) val;
    }

    module->SourceCodeIndex = kmalloc((module->SourceNumber + 1) * sizeof(LONG));
    if (!module->SourceCodeIndex)
    {
       ModuleFreeSymbolTable(module);
       return -1;
    }
    else
       SetData(module->SourceCodeIndex, 0,
	      (module->SourceNumber + 1) * sizeof(LONG));

    module->SourceLineIndex = kmalloc((module->SourceNumber + 1) * sizeof(LONG));
    if (!module->SourceLineIndex)
    {
       ModuleFreeSymbolTable(module);
       return -1;
    }
    else
       SetData(module->SourceLineIndex, 0,
	      (module->SourceNumber + 1) * sizeof(LONG));

    module->SourceSizeIndex = kmalloc((module->SourceNumber + 1) * sizeof(LONG));
    if (!module->SourceSizeIndex)
    {
       ModuleFreeSymbolTable(module);
       return -1;
    }
    else
       SetData(module->SourceSizeIndex, 0,
	      (module->SourceNumber + 1) * sizeof(LONG));

    module->crossIndex = kmalloc((module->SourceNumber + 1) * sizeof(LONG) * 2);
    if (!module->crossIndex)
    {
       ModuleFreeSymbolTable(module);
       return -1;
    }
    else
       SetData((LONG *)module->crossIndex, 0,
	      (module->SourceNumber + 1) * sizeof(LONG));

    if (ModuleGetSourceFiles(module))
    {
       ModuleFreeSymbolTable(module);
       return -1;
    }

    if (AddModuleSymbols(module))
    {
       RemoveModuleSymbols(module);
       return -1;
    }
    return 0;

}

LONG ModuleFreeSymbolTable(MODULE_HANDLE *module)
{

    register LONG i;

    RemoveModuleSymbols(module);

    for (i = 1; i < module->ModuleNumber + 1; i++)
    {
       if (module->SourceLineIndex[i])
	  kfree((void *) module->SourceLineIndex[i]);
       if (module->SourceCodeIndex[i])
	  kfree((void *) module->SourceCodeIndex[i]);
    }

    if (module->SourceIndexTable)
	  kfree((void *) module->SourceIndexTable);
    if (module->ModuleIndexTable)
	  kfree((void *) module->ModuleIndexTable);
    if (module->NameIndexTable)
	  kfree((void *) module->NameIndexTable);
    if (module->SymbolIndexTable)
	  kfree((void *) module->SymbolIndexTable);

    if (module->SourceCodeIndex)
	  kfree((void *) module->SourceCodeIndex);
    if (module->SourceLineIndex)
	  kfree((void *) module->SourceLineIndex);
    if (module->SourceSizeIndex)
	  kfree((void *) module->SourceSizeIndex);
    if (module->crossIndex)
	  kfree((void *) module->crossIndex);

    module->sourceIndex = 0;
    module->SourceNumber = 0;
    module->ModuleNumber = 0;
    module->SourceIndex = 0;
    module->ModuleIndex = 0;
    module->NameSection = 0;
    module->NumberOfSymbols = 0;
    module->NumberOfNames = 0;
    module->NumberOfSections = 0;
    module->DebugSymbolVersion = 0;
    module->GlobalSymbolTable = 0;
    module->GlobalTypeTable = 0;
    module->DebugBaseAddress = 0;
    module->ModuleIndexTable = 0;
    module->SourceIndexTable = 0;
    module->NameIndexTable = 0;
    module->SymbolIndexTable = 0;
    module->SourceCodeIndex = 0;
    module->SourceLineIndex = 0;
    module->SourceSizeIndex = 0;
    module->crossIndex = 0;
    module->defaultProcess = 0;

    return 0;

}

