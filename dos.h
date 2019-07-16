

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
*   FILE     :  DOS.H
*   DESCRIP  :  DOS Table Defines for MANOS v1.0
*   DATE     :  November 2, 1997
*
*
***************************************************************************/

#include "types.h"

typedef volatile struct _SYSTEM_DOS_TABLE {
   LONG DOS_TABLE;
   LONG GDT_TABLE;
   LONG GDT_POINTER;
   LONG DOS_IDT_TABLE;
   LONG DOS_CE_SEGMENT;
   LONG DOS_CE_OFFSET;
   LONG DOS_CC_SEGMENT;
   LONG DOS_CC_OFFSET;
   LONG DOS_DATA_SEGMENT;
   LONG DOS_STACK_SEGMENT;
   LONG DOS_EXTRA_SEGMENT;
   LONG DOS_FS_SEGMENT;
   LONG DOS_STACK_OFFSET;
   LONG DOS_SYSTEM_BUS_TYPE;
   LONG DOS_CR0;
   LONG REAL_MODE_INT;
   LONG DOS_EXIT;
   BYTE VENDOR_ID[12];
   LONG CPU_TYPE;
   LONG CPU_MODEL;
   LONG STEPPING;
   LONG ID_FLAG;
   LONG FEATURE_FLAGS;
   LONG XMS_FUNCTION;
   LONG XMS_MEMORY;
   LONG XMS_SIZE;
   LONG XMS_HANDLE;
   LONG XMS_BASE;
   LONG MEMORY_HIGH;
   LONG MEMORY_HIGH_START;
   LONG MEMORY_LOW;
   LONG MEMORY_LOW_START;
   LONG FP_STATUS;
   LONG DOS_DEFAULT_DRIVE;
   LONG MASK_8259_A;
   LONG MASK_8259_B;
   LONG FPU_TYPE;
   LONG INTEL_PROC;
   LONG RESERVED;
   LONG LINE_20_ON;
   LONG LINE_20_OFF;
   LONG PM_STACK;
   LONG PM_CODE_SEGMENT;
   LONG PM_DATA_SEGMENT;
   LONG JUMP16_SEGMENT;
   LONG MSDOS_CREATE;
   LONG MSDOS_OPEN;
   LONG MSDOS_LSEEK;
   LONG MSDOS_READ;
   LONG MSDOS_WRITE;
   LONG MSDOS_CLOSE;
   LONG MSDOS_UNLINK;
   LONG PE_HEADER_ADDR;
   LONG RELOC_OFFSET;
   LONG RELOC32_OFFSET;
   LONG CODE_RVA;
   LONG DATA_RVA;
   LONG CODE_SIZE;
   LONG DATA_SIZE;
   LONG VIDEO_ADDRESS;
   LONG VIDEO_CURSOR_MODE;
   LONG VIDEO_PORT_ADDRESS;
   LONG VIDEO_SCREEN_TYPE;
   LONG VIDEO_COLOR_FLAG;
   LONG START_CODE_16;
   LONG END_CODE_16;
   LONG DEBUG_TABLE;
   LONG CODE_VIRT_SIZE;
   LONG DATA_VIRT_SIZE;
   LONG EXTENDED_MEMORY_LEN;
   LONG EXTENDED_MEMORY_ADDR;
   LONG CODE_16_ENTRY;
   LONG DEBUG_SIZE;
   LONG STARTUP_SEGMENT;
   LONG STARTUP_CODE;
   LONG STARTUP_JUMP;
   LONG RELOC_SIZE;
   LONG SYSTEM_OFFSET;
   LONG REAL_INT_SEGMENT;
   LONG CURRENT_DIRECTORY;
   LONG DISPLAY_CODE;
   BYTE DISPLAY_STATE[128];
} DOS_TABLE;

typedef struct _PM_TABLE {
   LONG BaseSegment;
   LONG CodeSegment;
   LONG CodeRVA;
   LONG DataSegment;
   LONG RelocSegment;
   LONG DebugSegment;
   LONG DebugSize;
   LONG ExportSegment;
   LONG ExportSize;
   LONG ExportRVA;
   LONG LoaderCodeSegment;
   LONG LoaderDataSegment;
   LONG StartOfHighMemory;
   LONG HighMemoryLength;
   LONG StartupMemory;
   LONG StartupLength;
   LONG StartOfLowMemory;
   LONG LowMemoryLength;
   LONG SourceCodeSegment;
   LONG SourceCodeSegmentLength;
   LONG SourceCodeIndex;
   LONG SourceCodeIndexLength;
} PM_TABLE;

extern DOS_TABLE *DosDataTable;

