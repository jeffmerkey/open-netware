
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
*   FILE     :  DEBUG.H
*   DESCRIP  :  Multi-Processing Debugger for MANOS v1.0
*   DATE     :  December 28, 1997
*
*
***************************************************************************/

#include "types.h"

#define SYMBOL_DEBUG             0
#define LINE_INFO_WINDOW         1
#define LOGICAL_LINE_WINDOW      5
#define POOLED_RESOURCES         32
#define POOLED_NMI_RESOURCES     16
#define POOLED_STACK_SIZE        0x4000
#define POOLED_STACK_ADJUST      32
#define POOLED_STACK_SIGNATURE   0xFEEDBEEF

#define EXT_NMI_PORT             0x0461
#define NMI_IO_PORT              0x0462
#define NMI_CONTROL_PORT         0x0C6E
#define NMI_PORT                 0x61
#define PIC1_DEBUG_MASK          0xFC
#define PIC2_DEBUG_MASK          0xFF
#define EXCEPTION_ENTRIES  19
#define RESUME             0x00010000
#define NESTED_TASK        0x00004000
#define SINGLE_STEP        0x00000100
#define INVALID_EXPRESSION  0
#define NUMERIC_EXPRESSION  1
#define BOOLEAN_EXPRESSION  2

// DR7 Breakpoint Type and Length Fields

#define BREAK_EXECUTE    0
#define BREAK_WRITE      1
#define BREAK_IOPORT     2
#define BREAK_READWRITE  3
#define ONE_BYTE_FIELD   0
#define TWO_BYTE_FIELD   1
#define UNDEFINED_FIELD  2
#define FOUR_BYTE_FIELD  3

// DR7 Register

#define L0_BIT   0x00000001
#define G0_BIT   0x00000002
#define L1_BIT   0x00000004
#define G1_BIT   0x00000008
#define L2_BIT   0x00000010
#define G2_BIT   0x00000020
#define L3_BIT   0x00000040
#define G3_BIT   0x00000080
#define LEXACT   0x00000100
#define GEXACT   0x00000200
#define GDETECT  0x00002000
#define DR7DEF   0x00000400

// DR6 Register

#define B0_BIT   0x00000001
#define B1_BIT   0x00000002
#define B2_BIT   0x00000004
#define B3_BIT   0x00000008

#define BD_BIT   0x00002000
#define BS_BIT   0x00004000
#define BT_BIT   0x00008000

// Memory Type Range Registers (MTRR)

#define MTRR_PHYS_BASE_0    0x200
#define MTRR_PHYS_MASK_0    0x201
#define MTRR_PHYS_BASE_1    0x202
#define MTRR_PHYS_MASK_1    0x203
#define MTRR_PHYS_BASE_2    0x204
#define MTRR_PHYS_MASK_2    0x205
#define MTRR_PHYS_BASE_3    0x206
#define MTRR_PHYS_MASK_3    0x207
#define MTRR_PHYS_BASE_4    0x208
#define MTRR_PHYS_MASK_4    0x209
#define MTRR_PHYS_BASE_5    0x20A
#define MTRR_PHYS_MASK_5    0x20B
#define MTRR_PHYS_BASE_6    0x20C
#define MTRR_PHYS_MASK_6    0x20D
#define MTRR_PHYS_BASE_7    0x20E
#define MTRR_PHYS_MASK_7    0x20F

// IA32 flags settings

#define   CF_FLAG      0x00000001
#define   PF_FLAG      0x00000004
#define   AF_FLAG      0x00000010
#define   ZF_FLAG      0x00000040
#define   SF_FLAG      0x00000080
#define   TF_FLAG      0x00000100  // ss flag
#define   IF_FLAG      0x00000200
#define   DF_FLAG      0x00000400
#define   OF_FLAG      0x00000800
#define   NT_FLAG      0x00004000
#define   RF_FLAG      0x00010000  // resume flag
#define   VM_FLAG      0x00020000
#define   AC_FLAG      0x00040000
#define   VIF_FLAG     0x00080000
#define   VIP_FLAG     0x00100000
#define   ID_FLAGS     0x00200000

typedef struct _GDT {
    WORD Limit;	    //	0xFFFF
    WORD Base1;	    //  0
    BYTE Base2;	    //	0
    BYTE GDTType;   //	10010010b
    BYTE OtherType; //	11001111b
    BYTE Base3;	    //	0
} GDT;

typedef struct _IDT {
    WORD IDTLow;     //	0
    WORD IDTSegment; //	0x08
    BYTE IDTSkip;    //	0
    BYTE IDTFlags;   //	10001110b
    WORD IDTHigh;    //	0
} IDT;

typedef struct _TSS {
    WORD TSSLimit;	// 0x0080
    WORD TSSBase1;	// 0
    BYTE TSSBase2;	// 0
    BYTE TSSType;	// 10001001b
    BYTE TSSOtherType;	// 00000000b
    BYTE TSSBase3;	// 0
} TSS;

typedef struct _TSS_GATE {
    WORD TSSRes1;  	// 0
    WORD TSSSelector;	// 0
    BYTE TSSRes2; 	// 0
    BYTE TSSFlags;	// 10000101b
    WORD TSSRes3; 	// 0
} TSS_GATE;

typedef struct _LDT {
    WORD LDTLimit;	// 0xFFFF
    WORD LDTBase1;	// 0
    BYTE LDTBase2;	// 0
    BYTE LDTGDTType;	// 10000010b
    BYTE LDTOtherType;	// 10001111b
    BYTE LDTBase3;	// 0
} LDT;


typedef struct _POOL_HEADER {
   struct _POOL_HEADER *next;
   struct _POOL_HEADER *prior;
   BYTE *StackTop;
   LONG Signature;
   StackFrame TSS;
   BYTE Stack[1];
} POOL_HEADER;


extern POOL_HEADER *remove_pool(void);
extern void insert_pool(POOL_HEADER *p);
extern LONG FreePooledResource(POOL_HEADER *pool);
extern LONG AllocPooledResource(StackFrame *stackFrame);
extern POOL_HEADER *remove_pool_nmi(void);
extern void insert_pool_nmi(POOL_HEADER *p);
extern LONG FreeNMIPooledResource(POOL_HEADER *pool);
extern LONG AllocNMIPooledResource(StackFrame *stackFrame);
extern LONG disassemble(SCREEN *screen, StackFrame *stackFrame, LONG p, LONG count, LONG attr, LONG use);
extern void InitializeDebugger(LONG DebuggerPageTable, LONG OSPageTable);
extern void ClearDebuggerState(void);
extern LONG StopProcessorsExclSelf(LONG self);
extern LONG FreeProcessorsExclSelf(LONG self);
extern void DisplayKernel(SCREEN *screen, LONG p);
extern void debug_mps_ints(SCREEN *screen);
extern LONG debug_mps_ioapics(SCREEN *screen);
extern LONG debug_mps_procs(SCREEN *screen);
extern void debug_mps_buses(SCREEN *screen);
extern LONG debug_mps_table(SCREEN *screen);
extern void displayMPSTables(SCREEN *screen);
extern void displayMTRRRegisters(SCREEN *screen);
extern void DisplayLoaderMap(SCREEN *screen);
extern void DisplayGDT(SCREEN *screen, BYTE *GDT_ADDRESS);
extern void DisplayIDT(SCREEN *screen, BYTE *IDT_ADDRESS);
extern void SetDebugRegisters(void);
extern void LoadDebugRegisters(void);
extern void ClearTempBreakpoints(void);
extern LONG ValidBreakpoint(LONG address);
extern void DisplayDOSTable(SCREEN *screen, DOS_TABLE *dos);
extern BYTE *dump(SCREEN *screen, BYTE *p, LONG count);
extern BYTE *dumpWord(SCREEN *screen, BYTE *p, LONG count);
extern BYTE *dumpDouble(SCREEN *screen, BYTE *p, LONG count);
extern BYTE *dumpLinkedList(SCREEN *screen, BYTE *p, LONG count);
extern BYTE *dumpDoubleStack(SCREEN *screen, StackFrame *stackFrame, BYTE *p, LONG count);
extern BYTE *dumpStack(SCREEN *screen, StackFrame *stackFrame, BYTE *p, LONG count);
extern void DisplayTSS(SCREEN *screen, StackFrame *stackFrame);
extern void WindowRegisterFrame(LONG num, StackFrame *stackFrame);
extern void DisplayGeneralRegisters(SCREEN *screen, StackFrame *stackFrame);
extern void DisplaySegmentRegisters(SCREEN *screen, StackFrame *stackFrame);
extern void DisplayControlRegisters(SCREEN *screen, LONG processor, StackFrame *stackFrame);
extern double ldexp(double v, int e);
extern void DisplayNPXRegisters(SCREEN *screen, LONG processor);
extern LONG debug_lock(rlock_t *rlock, LONG proc);
extern void debug_unlock(rlock_t *rlock);
extern LONG debugger_setup(LONG processor, LONG Exception, StackFrame *stackFrame, BYTE *panicMsg);
extern LONG TLBShootDown(void);
extern LONG VMFlushTLB(void);
extern LONG xcall_processor(LONG p, LONG cmd, LONG arg1, LONG arg2, LONG arg3);
extern LONG nmiSetup(LONG processor, LONG Exception, StackFrame *stackFrame, BYTE *panicMsg);
extern LONG debugger_entry(LONG Exception, StackFrame *stackFrame, LONG FaultAddress, BYTE *panicMsg);
extern LONG moduleDebugAction(SCREEN *screen, LONG value, BYTE *option);
extern LONG debugger_command_entry(LONG processor, LONG Exception, StackFrame *stackFrame, BYTE *panicMsg);
extern void ConsoleDisplayBreakReason(SCREEN *debugScreen, StackFrame *stackFrame,
			       LONG Exception, BYTE *panicMsg, LONG processor,
			       LONG lastCommand);
extern void SourceDisplayBreakReason(SCREEN *sourceScreen, StackFrame *stackFrame,
			       LONG Exception, BYTE *panicMsg, LONG processor,
			       LONG lastCommand);

extern void outb(LONG, LONG);
extern void outw(LONG, LONG);
extern void outd(LONG, LONG);
extern BYTE inb(LONG);
extern WORD inw(LONG);
extern LONG ind(LONG);
extern LONG ReadDR0(void);
extern LONG ReadDR1(void);
extern LONG ReadDR2(void);
extern LONG ReadDR3(void);
extern LONG ReadDR6(void);
extern LONG ReadDR7(void);
extern LONG WriteDR0(LONG);
extern LONG WriteDR1(LONG);
extern LONG WriteDR2(LONG);
extern LONG WriteDR3(LONG);
extern LONG WriteDR6(LONG);
extern LONG WriteDR7(LONG);
extern LONG ReadCR0(void);
extern LONG ReadCR2(void);
extern LONG ReadCR3(void);
extern LONG ReadCR4(void);
extern void ReadGDTR(LONG *);
extern void ReadIDTR(LONG *);
extern LONG ReadLDTR(void);
extern LONG ReadTR(void);
extern LONG GetKey(void);
extern LONG unassemble(SCREEN *screen, StackFrame *stackFrame, LONG ip, LONG use, LONG attr);
extern void DisplayKernel(SCREEN *, LONG);
extern LONG get_flags(void);
extern void ExitOS(void);
extern LONG get_processor_id(void);
extern LONG get_physical_processor(void);
extern LONG dspin_try_lock(LONG *lock);
extern LONG dspin_lock(LONG *lock);
extern LONG dspin_unlock(LONG *lock);
extern void dump_ioapic(SCREEN *screen, LONG num);
extern void dump_local_apic(SCREEN *screen);
extern void dump_remote_apic(SCREEN *screen, LONG proc);
extern void dump_int_table(SCREEN *screen);
extern void ReadMSR(LONG msr, LONG *val1, LONG *val2);
extern void WriteMSR(LONG msr, LONG *val1, LONG *val2);
extern void MTRROpen(void);
extern void MTRRClose(void);
extern void MaskKeyboard(void);
extern void UnmaskKeyboard(void);
extern void DisplayModuleMap(SCREEN *screen, LONG attr, LONG extendedInfo);
extern void DumpOSSymbolTableMatch(SCREEN *screen, BYTE *match);
extern void save_npx(NUMERIC_FRAME *npx);
extern void load_npx(NUMERIC_FRAME *npx);
extern LONG inRealModeFlag;
extern LONG PageFaultHandler(LONG address, LONG process_id,
			     LONG processor, LONG FaultNestingCount,
			     LONG context);
extern void DisplayModule(SCREEN *screen, LONG attr, LONG address);
extern void DisplayScreen(SCREEN *screen, SCREEN *targetScreen);
extern displayVersionInfo(SCREEN *, BYTE *);
extern void DisplayASCIITable(SCREEN *screen);

extern StackFrame TSSSegments[EXCEPTION_ENTRIES * MAX_PROCESSORS];
extern StackFrame TSSDefaultSegments[MAX_PROCESSORS];
extern BYTE StackPool[];
extern BYTE NMIStackPool[];
extern MODULE_HANDLE *moduleListHead;
extern MODULE_HANDLE *moduleListTail;
extern SCREEN *screenListHead;
extern SCREEN *activeScreen;
extern SCREEN *debugScreen;
extern DOS_TABLE *DosDataTable;
extern LONG needs_proceed;
extern LONG RealModeSegment16;
extern LONG BaseSegment;
extern LONG CodeSegment;
extern LONG DataSegment;
extern LONG RelocSegment;
extern LONG DebugSegment;
extern LONG DebugSize;
extern LONG LoaderCodeSegment;
extern LONG LoaderDataSegment;
extern LONG StartOfHighMemory;
extern LONG HighMemoryLength;
extern LONG StartupMemory;
extern LONG StartupLength;
extern LONG StartOfLowMemory;
extern LONG LowMemoryLength;
extern LONG IDTSegments[];
extern LONG IDTPointers[];
extern LONG ExceptionTaskIndex[];


