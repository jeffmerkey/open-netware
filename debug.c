
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  DEBUG.C
*   DESCRIP  :  Multi-Processing Debugger for MANOS v1.0
*   DATE     :  December 28, 1997
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
#include "os.h"
#include "dos.h"
#include "tss.h"
#include "mps.h"
#include "hal.h"
#include "xcall.h"
#include "window.h"
#include "line.h"
#include "loader.h"
#include "menu.h"
#include "rlock.h"
#include "event.h"
#include "debug.h"
#include "dparse.h"
#include "bus.h"
#include "ia32.h"
#include "debcmd.h"
#include "debkern.h"
#include "sld.h"

// debugger commands

DEBUGGER_PARSER QPE = {
0, 0, exitToDOS, 0, 0, "Q", 0, -1,
"" , 0, &serverModuleHandle };

DEBUGGER_PARSER XPE = {
0, 0, exitToDOS, 0, 0, "X", 0, -1,
"" , 0, &serverModuleHandle };

DEBUGGER_PARSER QuitPE = {
0, 0, exitToDOS, 0, 0, "QUIT", 0, 0,
"exit to MS-DOS" , 0, &serverModuleHandle };

DEBUGGER_PARSER ExitPE = {
0, 0, exitToDOS, 0, 0, "EXIT", 0, 0,
"exit to MS-DOS" , 0, &serverModuleHandle };

DEBUGGER_PARSER HPE = {
0, 0, displayDebuggerHelp, displayDebuggerHelpHelp, 0, "HELP", 0, 0,
"this help screen (type HELP <command> for specific help)" , 0, &serverModuleHandle };

DEBUGGER_PARSER HelpPE = {
0, 0, displayDebuggerHelp, displayDebuggerHelpHelp, 0, "H", 0, 0,
"this help screen" , 0, &serverModuleHandle };

DEBUGGER_PARSER DisplaySema = {
0, 0, displaySema, displaySyncHelp, 0, "SEMA", 0, 0,
"display sema_t object(s)" , 0, &serverModuleHandle };

DEBUGGER_PARSER DisplayMutex = {
0, 0, displayMutex, displaySyncHelp, 0, "MUTEX", 0, 0,
"display mutex_t object(s)" , 0, &serverModuleHandle };

DEBUGGER_PARSER DisplayRMutex = {
0, 0, displayRMutex, displaySyncHelp, 0, "RMUTEX", 0, 0,
"display rmutex_t object(s)" , 0, &serverModuleHandle };

DEBUGGER_PARSER DisplayRWLock = {
0, 0, displayRWLock, displaySyncHelp, 0, "RWLOCK", 0, 0,
"display rwlock_t object(s)" , 0, &serverModuleHandle };

DEBUGGER_PARSER DisplaySpin = {
0, 0, displaySpin, displaySyncHelp, 0, "SPIN", 0, 0,
"display spin_t object(s)" , 0, &serverModuleHandle };

DEBUGGER_PARSER clearScreenPE = {
0, 0, clearDebuggerScreen, clearScreenHelp, 0, "CLS", 0, 0,
"clear the screen" , 0, &serverModuleHandle };

DEBUGGER_PARSER asciiTablePE = {
0, 0, displayASCTable, ascTableHelp, 0, "A", 0, 0,
"display ASCII Table" , 0, &serverModuleHandle };

DEBUGGER_PARSER displayScreenStructPE = {
0, 0, DisplayScreenStructure, displayScreenStructHelp, 0, ".S", 0, 0,
"display screen structure(s)" , 0, &serverModuleHandle };

DEBUGGER_PARSER TUTogglePE = {
0, 0, ProcessTUToggle, displayToggleHelp, 0, ".TU", 0, 0,
"toggles unasm debug display (ON | OFF)" , 0, &serverModuleHandle };

DEBUGGER_PARSER TDTogglePE = {
0, 0, ProcessTDToggle, displayToggleHelp, 0, ".TD", 0, 0,
"toggles full dereference display (ON | OFF)" , 0, &serverModuleHandle };

DEBUGGER_PARSER TLTogglePE = {
0, 0, ProcessTLToggle, displayToggleHelp, 0, ".TL", 0, 0,
"toggles source line display (ON | OFF)" , 0, &serverModuleHandle };

DEBUGGER_PARSER TGTogglePE = {
0, 0, ProcessTGToggle, displayToggleHelp, 0, ".TG", 0, 0,
"toggles general registers (ON | OFF)" , 0, &serverModuleHandle };

DEBUGGER_PARSER TCTogglePE = {
0, 0, ProcessTCToggle, displayToggleHelp, 0, ".TC", 0, 0,
"toggles control registers (ON | OFF)" , 0, &serverModuleHandle };

DEBUGGER_PARSER TNTogglePE = {
0, 0, ProcessTNToggle, displayToggleHelp, 0, ".TN", 0, 0,
"toggles coprocessor registers (ON | OFF)"  , 0, &serverModuleHandle };

DEBUGGER_PARSER TRTogglePE = {
0, 0, ProcessTRToggle, displayToggleHelp, 0, ".TR", 0, 0,
"toggles display of break reason (ON | OFF)"  , 0, &serverModuleHandle };

DEBUGGER_PARSER TSTogglePE = {
0, 0, ProcessTSToggle, displayToggleHelp, 0, ".TS", 0, 0,
"toggles segment registers (ON | OFF)"  , 0, &serverModuleHandle };

DEBUGGER_PARSER TATogglePE = {
0, 0, ProcessTAToggle, displayToggleHelp, 0, ".TA", 0, 0,
"toggles all registers (ON | OFF)" , 0, &serverModuleHandle };

DEBUGGER_PARSER TTogglePE = {
0, 0, ProcessTToggle, displayToggleHelp, 0, ".T", 0, 0,
"display task state segment (tss)" , 0, &serverModuleHandle };

DEBUGGER_PARSER versionPE = {
0, 0, DisplayDebuggerVersion, displayDebuggerVersionHelp, 0, ".V", 0, 0,
"display version info" , 0, &serverModuleHandle };

DEBUGGER_PARSER KernelProcessPE = {
0, 0, displayKernelProcess, displayKernelProcessHelp, 0, ".KP", 0, 0,
"display kernel processes or a selected process" , 0, &serverModuleHandle };

DEBUGGER_PARSER KernelQueuePE = {
0, 0, displayKernelQueue, displayKernelQueueHelp, 0, ".K", 0, 0,
"display kernel queues or a processor queue" , 0, &serverModuleHandle };

DEBUGGER_PARSER SymbolsPE = {
0, 0, displaySymbols, displaySymbolsHelp, 0, ".Z", 0, 0,
"display symbol(s)" , 0, &serverModuleHandle };

DEBUGGER_PARSER LoaderMapPE = {
0, 0, displayLoaderMap, displayLoaderMapHelp, 0, ".L", 0, 0,
"display loaded module map" , 0, &serverModuleHandle };

DEBUGGER_PARSER ModulePE = {
0, 0, displayModuleInfo, displayModuleHelp, 0, ".M", 0, 0,
"display loaded modules" , 0, &serverModuleHandle };

DEBUGGER_PARSER ProcessesPE = {
0, 0, displayProcesses, displayProcessesHelp, 0, ".P", 0, 0,
"display kernel processes or a selected process" , 0, &serverModuleHandle };

DEBUGGER_PARSER ControlPE = {
0, 0, displayControlRegisters, displayRegistersHelp, 0, "RC", 0, 0,
"display control registers" , 0, &serverModuleHandle };

DEBUGGER_PARSER AllPE = {
0, 0, displayAllRegisters, displayRegistersHelp, 0, "RA", 0, 0,
"display all registers" , 0, &serverModuleHandle };

DEBUGGER_PARSER SegmentPE = {
0, 0, displaySegmentRegisters, displayRegistersHelp, 0, "RS", 0, 0,
"display segment registers" , 0, &serverModuleHandle };

DEBUGGER_PARSER NumericPE = {
0, 0, displayNumericRegisters, displayRegistersHelp, 0, "RN", 0, 0,
"display coprocessor registers" , 0, &serverModuleHandle };

DEBUGGER_PARSER GeneralPE = {
0, 0, displayGeneralRegisters, displayRegistersHelp, 0, "RG", 0, 0,
"display general registers" , 0, &serverModuleHandle };

DEBUGGER_PARSER DefaultPE = {
0, 0, displayDefaultRegisters, displayRegistersHelp, 0, "R", 0, 0,
"display registers for a processor" , 0, &serverModuleHandle };

DEBUGGER_PARSER APICPE = {
0, 0, displayAPICInfo, displayAPICHelp, 0, "APIC", 0, 0,
"display local/remote apic regs"  , 0, &serverModuleHandle };

DEBUGGER_PARSER MPSPE = {
0, 0, displayMPS, displayMPSHelp, 0, "MPS", 0, 0,
"display MPS tables"  , 0, &serverModuleHandle };

DEBUGGER_PARSER SearchMemoryPE = {
0, 0, SearchMemory, SearchMemoryHelp, 0, "S", 0, 0,
"search memory for pattern (bytes)"  , 0, &serverModuleHandle };

DEBUGGER_PARSER SearchMemoryBPE = {
0, 0, SearchMemoryB, SearchMemoryHelp, 0, "SB", 0, 0,
"search memory for pattern (bytes)"  , 0, &serverModuleHandle };

DEBUGGER_PARSER SearchMemoryWPE = {
0, 0, SearchMemoryW, SearchMemoryHelp, 0, "SW", 0, 0,
"search memory for pattern (words)"  , 0, &serverModuleHandle };

DEBUGGER_PARSER SearchMemoryDPE = {
0, 0, SearchMemoryD, SearchMemoryHelp, 0, "SD", 0, 0,
"search memory for pattern (dwords)"  , 0, &serverModuleHandle };

DEBUGGER_PARSER ChangeWordPE = {
0, 0, changeWordValue, changeMemoryHelp, 0, "CW", 0, 0,
"change words at address"  , 0, &serverModuleHandle };

DEBUGGER_PARSER ChangeDoublePE = {
0, 0, changeDoubleValue, changeMemoryHelp, 0, "CD", 0, 0,
"change dwords at address"  , 0, &serverModuleHandle };

DEBUGGER_PARSER ChangeBytePE = {
0, 0, changeByteValue, changeMemoryHelp, 0, "CB", 0, 0,
"change bytes at address"  , 0, &serverModuleHandle };

DEBUGGER_PARSER ChangeDefaultPE = {
0, 0, changeDefaultValue, changeMemoryHelp, 0, "C", 0, 0,
"change bytes at address"  , 0, &serverModuleHandle };

DEBUGGER_PARSER CloseSymbolsPE = {
0, 0, displayCloseSymbols, displayCloseHelp, 0, "?", 0, 0,
"display closest symbols to <address>" , 0, &serverModuleHandle };

DEBUGGER_PARSER INTRPE = {
0, 0, displayInterruptTable, displayINTRHelp, 0, "INTR", 0, 0,
"system interrupt table"  , 0, &serverModuleHandle };

DEBUGGER_PARSER ViewScreensPE = {
0, 0, displayScreenList, viewScreensHelp, 0, "V", 0, 0,
"view screens"  , 0, &serverModuleHandle };

DEBUGGER_PARSER IOAPICPE = {
0, 0, displayIOAPICInfo, displayIOAPICHelp, 0, "IOAPIC", 0, 0,
"display specified ioapic [#] regs"  , 0, &serverModuleHandle };

DEBUGGER_PARSER WalkPE = {
0, 0, debuggerWalkStack, displayDumpHelp, 0, "W", 0, 0,
"display symbols on the stack"  , 0, &serverModuleHandle };

DEBUGGER_PARSER DumpLinkedPE = {
0, 0, debuggerDumpLinkedList, displayDumpHelp, 0, "DL", 0, 0,
"dump linked list"  , 0, &serverModuleHandle };

DEBUGGER_PARSER DumpWordPE = {
0, 0, debuggerDumpWord, displayDumpHelp, 0, "DW", 0, 0,
"dump memory as words"  , 0, &serverModuleHandle };

DEBUGGER_PARSER DumpStackPE = {
0, 0, debuggerDumpStack, displayDumpHelp, 0, "DS", 0, 0,
"dump stack"  , 0, &serverModuleHandle };

DEBUGGER_PARSER DumpDoubleStackPE = {
0, 0, debuggerDumpDoubleStack, displayDumpHelp, 0, "DDS", 0, 0,
"dump stack double word"  , 0, &serverModuleHandle };

DEBUGGER_PARSER DumpDoublePE = {
0, 0, debuggerDumpDouble, displayDumpHelp, 0, "DD", 0, 0,
"dump memory as double words" , 0, &serverModuleHandle };

DEBUGGER_PARSER DumpBytePE = {
0, 0, debuggerDumpByte, displayDumpHelp, 0, "DB", 0, 0,
"dump memory as bytes"  , 0, &serverModuleHandle };

DEBUGGER_PARSER DumpDefaultPE = {
0, 0, debuggerDumpByte, displayDumpHelp, 0, "D", 0, 0,
"dump memory as bytes"  , 0, &serverModuleHandle };

DEBUGGER_PARSER Diss16PE = {
0, 0, processDisassemble16, displayDisassembleHelp, 0, "UU", 0, 0,
"unassemble code (16-bit)" , 0, &serverModuleHandle };

DEBUGGER_PARSER Diss32PE = {
0, 0, processDisassemble32, displayDisassembleHelp, 0, "U", 0, 0,
"unassemble code (32-bit)"  , 0, &serverModuleHandle };

DEBUGGER_PARSER ProceedPE = {
0, 0, processProceed, executeCommandHelp, 0, "P", 0, 0,
"proceed"  , -1, &serverModuleHandle };

DEBUGGER_PARSER TracePE = {
0, 0, processTrace, executeCommandHelp, 0, "T", 0, 0,
"trace"  , -1, &serverModuleHandle };

DEBUGGER_PARSER GoPE = {
0, 0, processGo, executeCommandHelp, 0, "G", 0, 0,
"go or go til <address> match"  , -1, &serverModuleHandle };

DEBUGGER_PARSER BreakProcessorPE = {
0, 0, breakProcessor, processorCommandHelp, 0, "LB", 0, 0,
"signal a specific processor"  , -1, &serverModuleHandle };

DEBUGGER_PARSER BreakNMIProcessorPE = {
0, 0, breakNMIProcessors, processorCommandHelp, 0, "LN", 0, 0,
"nmi a specific processor"  , -1, &serverModuleHandle };

DEBUGGER_PARSER ListProcessorsPE = {
0, 0, listProcessors, processorCommandHelp, 0, "LP", 0, 0,
"list processor status"  , 0, &serverModuleHandle };

DEBUGGER_PARSER ListProcessorFramePE = {
0, 0, listProcessorFrame, processorCommandHelp, 0, "LR", 0, 0,
"display processor registers"  , 0, &serverModuleHandle };

DEBUGGER_PARSER EAXPE = {
0, 0, ChangeEAXRegister, displayEAXHelp, 0, "EAX", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER EBXPE = {
0, 0, ChangeEBXRegister, displayEBXHelp, 0, "EBX", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER ECXPE = {
0, 0, ChangeECXRegister, displayECXHelp, 0, "ECX", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER EDXPE = {
0, 0, ChangeEDXRegister, displayEDXHelp, 0, "EDX", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER ESIPE = {
0, 0, ChangeESIRegister, displayESIHelp, 0, "ESI", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER EDIPE = {
0, 0, ChangeEDIRegister, displayEDIHelp, 0, "EDI", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER EBPPE = {
0, 0, ChangeEBPRegister, displayEBPHelp, 0, "EBP", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER ESPPE = {
0, 0, ChangeESPRegister, displayESPHelp, 0, "ESP", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER EIPPE = {
0, 0, ChangeEIPRegister, displayEIPHelp, 0, "EIP", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER CSPE = {
0, 0, ChangeCSRegister, displayCSHelp, 0, "CS", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER DSPE = {
0, 0, ChangeDSRegister, displayDSHelp, 0, "DS", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER ESPE = {
0, 0, ChangeESRegister, displayESHelp, 0, "ES", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER FSPE = {
0, 0, ChangeFSRegister, displayFSHelp, 0, "FS", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER GSPE = {
0, 0, ChangeGSRegister, displayGSHelp, 0, "GS", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER SSPE = {
0, 0, ChangeSSRegister, displaySSHelp, 0, "SS", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER RFPE = {
0, 0, ChangeRFFlag, displayRFHelp, 0, "RF", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER TFPE = {
0, 0, ChangeTFFlag, displayTFHelp, 0, "TF", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER ZFPE = {
0, 0, ChangeZFFlag, displayZFHelp, 0, "ZF", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER SFPE = {
0, 0, ChangeSFFlag, displaySFHelp, 0, "SF", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER PFPE = {
0, 0, ChangePFFlag, displayPFHelp, 0, "PF", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER CFPE = {
0, 0, ChangeCFFlag, displayCFHelp, 0, "CF", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER OFPE = {
0, 0, ChangeOFFlag, displayOFHelp, 0, "OF", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER IFPE = {
0, 0, ChangeIFFlag, displayIFHelp, 0, "IF", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER IDPE = {
0, 0, ChangeIDFlag, displayIDHelp, 0, "CPUID", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER DFPE = {
0, 0, ChangeDFFlag, displayDFHelp, 0, "DF", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER NTPE = {
0, 0, ChangeNTFlag, displayNTHelp, 0, "NT", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER VMPE = {
0, 0, ChangeVMFlag, displayVMHelp, 0, "VM", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER VIFPE = {
0, 0, ChangeVIFFlag, displayVIFHelp, 0, "VIF", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER VIPPE = {
0, 0, ChangeVIPFlag, displayVIPHelp, 0, "VIP", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER AFPE = {
0, 0, ChangeAFFlag, displayAFHelp, 0, "AF", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER ACPE = {
0, 0, ChangeACFlag, displayACHelp, 0, "AC", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER MTRRPE = {
0, 0, DisplayMTRRRegisters, displayMTRRHelp, 0, "MTRR", 0, 0,
"display memory type range registers" , 0, &serverModuleHandle };

DEBUGGER_PARSER GDTPE = {
0, 0, displayGDT, displayGDTHelp, 0, ".G", 0, 0,
"display global descriptor table" , 0, &serverModuleHandle };

DEBUGGER_PARSER IDTPE = {
0, 0, displayIDT, displayIDTHelp, 0, ".I", 0, 0,
"display interrupt descriptor table"  , 0, &serverModuleHandle };

DEBUGGER_PARSER EvaluatePE = {
0, 0, evaluateExpression, evaluateExpressionHelp, 0, ".E", 0, 0,
"evaluate expression (.e for help)"  , 0, &serverModuleHandle };

DEBUGGER_PARSER DOSTablePE = {
0, 0, displayDOSTable, displayDOSTableHelp, 0, ".D", 0, 0,
"display dos table" , 0, &serverModuleHandle };

DEBUGGER_PARSER InputWordPE = {
0, 0, inputWordPort, portCommandHelp, 0, "IW", 0, 0,
"input word from port" , 0, &serverModuleHandle };

DEBUGGER_PARSER InputDoublePE = {
0, 0, inputDoublePort, portCommandHelp, 0, "ID", 0, 0,
"input double word from port" , 0, &serverModuleHandle };

DEBUGGER_PARSER InputBytePE = {
0, 0, inputBytePort, portCommandHelp, 0, "IB", 0, 0,
"input byte from port" , 0, &serverModuleHandle };

DEBUGGER_PARSER InputPE = {
0, 0, inputPort, portCommandHelp, 0, "I", 0, 0,
"input byte from port" , 0, &serverModuleHandle };

DEBUGGER_PARSER OutputWordPE = {
0, 0, outputWordPort, portCommandHelp, 0, "OW", 0, 0,
"output word to port" , 0, &serverModuleHandle };

DEBUGGER_PARSER OutputDoublePE = {
0, 0, outputDoublePort, portCommandHelp, 0, "OD", 0, 0,
"output double word to port" , 0, &serverModuleHandle };

DEBUGGER_PARSER OutputBytePE = {
0, 0, outputBytePort, portCommandHelp, 0, "OB", 0, 0,
"output byte to port" , 0, &serverModuleHandle };

DEBUGGER_PARSER OutputPE = {
0, 0, outputPort, portCommandHelp, 0, "O", 0, 0,
"output byte to port" , 0, &serverModuleHandle };

DEBUGGER_PARSER BreakClearAllPE = {
0, 0, breakpointClearAll, breakpointCommandHelp, 0, "BCA", 0, 0,
"clear all breakpoints" , 0, &serverModuleHandle };

DEBUGGER_PARSER BreakClearPE = {
0, 0, breakpointClear, breakpointCommandHelp, 0, "BC", 0, 0,
"clear breakpoint" , 0, &serverModuleHandle };

DEBUGGER_PARSER BreakMaskPE = {
0, 0, breakpointMask, breakpointCommandHelp, 0, "BM", 0, 0,
"mask breaks for specific processor" , 0, &serverModuleHandle };

DEBUGGER_PARSER BW1PE = {
0, 0, breakpointWord1, breakpointCommandHelp, 0, "BW1", 0, -1,
"" , 0, &serverModuleHandle };

DEBUGGER_PARSER BW2PE = {
0, 0, breakpointWord2, breakpointCommandHelp, 0, "BW2", 0, -1,
"" , 0, &serverModuleHandle };

DEBUGGER_PARSER BW4PE = {
0, 0, breakpointWord4, breakpointCommandHelp, 0, "BW4", 0, -1,
"" , 0, &serverModuleHandle };

DEBUGGER_PARSER BWPE = {
0, 0, breakpointWord, breakpointCommandHelp, 0, "BW", 0, 0,
"set write only breakpoint #=1,2 or 4 byte len" , 0, &serverModuleHandle };

DEBUGGER_PARSER BR1PE = {
0, 0, breakpointRead1, breakpointCommandHelp, 0, "BR1", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER BR2PE = {
0, 0, breakpointRead2, breakpointCommandHelp, 0, "BR2", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER BR4PE = {
0, 0, breakpointRead4, breakpointCommandHelp, 0, "BR4", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER BRPE = {
0, 0, breakpointRead, breakpointCommandHelp, 0, "BR", 0, 0,
"set read/write breakpoint #=1,2 or 4 byte len" , 0, &serverModuleHandle };

DEBUGGER_PARSER BI1PE = {
0, 0, breakpointIO1, breakpointCommandHelp, 0, "BI1", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER BI2PE = {
0, 0, breakpointIO2, breakpointCommandHelp, 0, "BI2", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER BI4PE = {
0, 0, breakpointIO4, breakpointCommandHelp, 0, "BI4", 0, -1,
"", 0, &serverModuleHandle };

DEBUGGER_PARSER BIPE = {
0, 0, breakpointIO, breakpointCommandHelp, 0, "BI", 0, 0,
"set io address breakpoint #=1,2 or 4 byte len"  , 0, &serverModuleHandle };

DEBUGGER_PARSER breakpointExecutePE = {
0, 0, breakpointExecute, breakpointCommandHelp, 0, "B", 0, 0,
"display all/set execute breakpoint" , 0, &serverModuleHandle };

DEBUGGER_PARSER breakShowTemp = {
0, 0, breakpointShowTemp, breakpointCommandHelp, 0, "BST", 0, 0,
"displays temporary breakpoints (proceed/go)" , 0, &serverModuleHandle };

// SLD accelerators

ACCELERATOR SLDregistersACC = {
0, 0, 0, 0, 0, F1, 0,
"F1 - Lexical Scope", &serverModuleHandle };

ACCELERATOR registersACC = {
0, 0, activateRegisterDisplayACC, 0, 0, F1, 0,
"F1 - Registers", &serverModuleHandle };

ACCELERATOR setBreakACC = {
0, 0, SLDSetBreak, 0, 0, F2, 0,
"F2 - Breakpoint", &serverModuleHandle };

ACCELERATOR searchACC = {
0, 0, SLDSearch, 0, 0, F3, 0,
"F3 - Search", &serverModuleHandle };

ACCELERATOR currentACC = {
0, 0, SLDCurrent, 0, 0, F4, 0,
"F4 - Current", &serverModuleHandle };

ACCELERATOR screenACC = {
0, 0, SLDScreens, 0, 0, F5, 0,
"F5 - Screens", &serverModuleHandle };

ACCELERATOR tabsetACC = {
0, 0, SLDTab, 0, 0, TAB, 0,
"TAB - Set Tabsize", &serverModuleHandle };

ACCELERATOR pageupACC = {
0, 0, SLDPageUp, 0, 0, PG_UP, 0,
"PG_UP - Page Up", &serverModuleHandle };

ACCELERATOR pagedownACC = {
0, 0, SLDPageDown, 0, 0, PG_DOWN, 0,
"PG_DN - Page Down", &serverModuleHandle };

ACCELERATOR pageHomeACC = {
0, 0, SLDCtrlPageUp, 0, 0, LEFT_CTRL_MASK | PG_UP, 0,
"[NUM] PG_UP - Page Up", &serverModuleHandle };

ACCELERATOR pageEndACC = {
0, 0, SLDCtrlPageDown, 0, 0, LEFT_CTRL_MASK | PG_DOWN, 0,
"[NUM] PG_DN - Page Down", &serverModuleHandle };

ACCELERATOR arrowupACC = {
0, 0, SLDScrollUp, 0, 0, UP_ARROW, 0,
"UP - Scroll Up", &serverModuleHandle };

ACCELERATOR arrowdownACC = {
0, 0, SLDScrollDown, 0, 0, DOWN_ARROW, 0,
"DN - Scroll Down", &serverModuleHandle };

ACCELERATOR arrowLeftACC = {
0, 0, SLDLeftArrow, 0, 0, LEFT_ARROW, 0,
"LEFT_ARROW - Move Indent Left", &serverModuleHandle };

ACCELERATOR arrowRightACC = {
0, 0, SLDRightArrow, 0, 0, RIGHT_ARROW, 0,
"RIGHT_ARROW - Move Indent Right", &serverModuleHandle };

ACCELERATOR homeACC = {
0, 0, SLDHome, 0, 0, HOME, 0,
"HOME - Indent Start of Line", &serverModuleHandle };

ACCELERATOR endACC = {
0, 0, SLDEnd, 0, 0, END, 0,
"END - Indent End of Line", &serverModuleHandle };

ACCELERATOR NpageupACC = {
0, 0, SLDPageUp, 0, 0, PG_UP | NUMERIC_PAD_MASK, 0,
"[NUM] PG_UP - Page Up", &serverModuleHandle };

ACCELERATOR NpagedownACC = {
0, 0, SLDPageDown, 0, 0, PG_DOWN | NUMERIC_PAD_MASK, 0,
"[NUM] PG_DN - Page Down", &serverModuleHandle };

ACCELERATOR NpageHomeACC = {
0, 0, SLDCtrlPageUp, 0, 0, LEFT_CTRL_MASK | PG_UP | NUMERIC_PAD_MASK, 0,
"[NUM] PG_UP - Page Up", &serverModuleHandle };

ACCELERATOR NpageEndACC = {
0, 0, SLDCtrlPageDown, 0, 0, LEFT_CTRL_MASK | PG_DOWN | NUMERIC_PAD_MASK, 0,
"[NUM] PG_DN - Page Down", &serverModuleHandle };

ACCELERATOR NarrowupACC = {
0, 0, SLDScrollUp, 0, 0, UP_ARROW | NUMERIC_PAD_MASK, 0,
"[NUM] UP - Scroll Up", &serverModuleHandle };

ACCELERATOR NarrowdownACC = {
0, 0, SLDScrollDown, 0, 0, DOWN_ARROW | NUMERIC_PAD_MASK, 0,
"[NUM] DN - Scroll Down", &serverModuleHandle };

ACCELERATOR NarrowLeftACC = {
0, 0, SLDLeftArrow, 0, 0, LEFT_ARROW | NUMERIC_PAD_MASK, 0,
"[NUM] LEFT_ARROW - Move Indent Left", &serverModuleHandle };

ACCELERATOR NarrowRightACC = {
0, 0, SLDRightArrow, 0, 0, RIGHT_ARROW | NUMERIC_PAD_MASK, 0,
"[NUM] RIGHT_ARROW - Move Indent Right", &serverModuleHandle };

ACCELERATOR NhomeACC = {
0, 0, SLDHome, 0, 0, HOME | NUMERIC_PAD_MASK, 0,
"[NUM] HOME - Indent Start of Line", &serverModuleHandle };

ACCELERATOR NendACC = {
0, 0, SLDEnd, 0, 0, END | NUMERIC_PAD_MASK, 0,
"[NUM] END - Indent End of Line", &serverModuleHandle };

ACCELERATOR traceSLDACC = {
0, 0, SLDprocessTraceACC, 0, 0, F7, 0,
"F7 - Trace", &serverModuleHandle };

ACCELERATOR proceedSLDACC = {
0, 0, SLDprocessProceedACC, 0, 0, F8, 0,
"F8 - Proceed", &serverModuleHandle };

ACCELERATOR goSLDACC = {
0, 0, SLDprocessGoACC, 0, 0, F9, 0,
"F9 - Go", &serverModuleHandle };

ACCELERATOR exitSLDACC = {
0, 0, SLDexitToDOSACC, 0, 0, F10, 0,
"F10 - Exit", &serverModuleHandle };


// interactive debugger accelerators

ACCELERATOR traceACC = {
0, 0, processTraceACC, 0, 0, F7, 0,
"F7 - Trace", &serverModuleHandle };

ACCELERATOR proceedACC = {
0, 0, processProceedACC, 0, 0, F8, 0,
"F8 - Proceed", &serverModuleHandle };

ACCELERATOR goACC = {
0, 0, processGoACC, 0, 0, F9, 0,
"F9 - Go", &serverModuleHandle };

ACCELERATOR exitACC = {
0, 0, exitToDOSACC, 0, 0, F10, 0,
"F10 - Exit", &serverModuleHandle };

ACCELERATOR toggleSLDACC = {
0, 0, SLDInteractive, 0, 0, F6, 0,
"F6 - Toggle Source Level Debugger", &serverModuleHandle };

ACCELERATOR toggleACC = {
0, 0, SLDInteractive, 0, 0, F6, 0,
"F6 - Toggle Source Level Debugger", &serverModuleHandle };

ACCELERATOR enterACC = {            // this accelerator handles repeat command
0, 0, enterKeyACC, 0, 0, ENTER, 0,  // processing
"Enter - Execute or Repeat a Command", &serverModuleHandle };

extern BYTE *BreakDescription[];
extern BYTE *ExceptionDescription[];

BYTE *processState[]={
  "PS_INIT  ", "PS_ACTIVE", "PS_SLEEP ", "PS_SYNC  ", "PS_LWP   ",
  "PS_HALT  ", "  ?????  ", "  ?????  ", "  ?????  ", "  ?????  ",
  "  ?????  ", "  ?????  ", "  ?????  ", "  ?????  ", "  ?????  ",
  "  ?????  ", "  ?????  "
};

BYTE *procState[]={
   "PROCESSOR_INACTIVE", "PROCESSOR_ACTIVE  ", "PROCESSOR_DEBUG   ",
   "PROCESSOR_SIGNAL  ", "PROCESSOR_FATAL   ", "PROCESSOR_INIT    ",
   "PROCESSOR_SHUTDOWN", "PROCESSOR_RESUME  ", "PROCESSOR_LOCK    ",
   "PROCESSOR_CURRENT ", "PROCESSOR_SUSPEND ", "??????            ",
   "??????            ", "??????            ", "??????            ",
   "??????            ", "??????            ", "??????            ",
   "??????            "
};

SOURCE_LINE_INFO executeLineInfo;
SOURCE_LINE_INFO logicalLineInfo;
MODULE_HANDLE *executeModuleHandle;
MODULE_HANDLE *logicalModuleHandle;
BYTE **logicalSourceFile = 0;
LONG logicalModuleIndex = 0;
LONG logicalFramePosition = 0;
LONG logicalLineAddress = -1;
LONG Indent = 0;
LONG debugInfoPresent;

BYTE *lastDumpAddress = 0, *lastLinkAddress = 0;
LONG lastUnasmAddress = 0;
LONG displayLength;
BYTE lastCommand = 0;
BYTE lastDebugCommand[100] = {""};
LONG lastDisplayLength = 0;
BYTE debugCommand[100] = {""};
LONG nextUnasmAddress = 0;
BYTE IBuffer[80 * 50 * 2];
BYTE TBuffer[80 * 50 * 2];
BYTE WBuffer[80 * 50 * 2];
LONG pic1Value;
LONG pic2Value;
LONG BreakReserved[4];
LONG BreakPoints[4];
LONG BreakType[4];
LONG BreakLength[4];
LONG BreakTemp[4];
LONG BreakGo[4];
LONG BreakProceed[4];
LONG BreakMask[MAX_PROCESSORS];
StackFrame *CurrentFrame[MAX_PROCESSORS];
LONG NestedInterrupts[MAX_PROCESSORS];
LONG registerDisplayActive = 0;
SCREEN *oldScreen = 0;
BYTE BreakCondition[4][100];
LONG ConditionalBreakpoint[4];
StackFrame lastStackFrame = { 0x00 };
LONG lastCR0 = 0;
LONG lastCR2 = 0;
LONG lastCR4 = 0;
LONG CurrentDR7 = 0;
LONG CurrentDR6[MAX_PROCESSORS];
LONG SourceDebuggerActive = 0;
LONG debuggerActive = 0;
LONG ExitOSFlag = 0;
rlock_t debug_mutex = { 0, 0, 0, 0 };
LONG repeatCommand = 0;
LONG totalLines;
LONG currentException;
BYTE *currentPanicMsg;

void InitializeDebugger(LONG DebuggerPageTable, LONG OSPageTable)
{
   register LONG i;
   extern void InitializeDebuggerTables(LONG DebuggerPageTable, LONG OSPageTable);
   extern void InitializeDebuggerRegisters(void);

   debuggerActive = 0;
   registerDisplayActive = 0;
   lastCommand = 0;
   lastDisplayLength = 0;

   for (i=0; i < MAX_PROCESSORS; i++)
      BreakMask[i] = 0;

   for (i=0; i < 4; i++)
   {
      BreakReserved[i] = 0;
      BreakPoints[i] = 0;
      BreakType[i] = 0;
      BreakLength[i] = 0;
      BreakProceed[i] = 0;
      BreakGo[i] = 0;
      BreakTemp[i] = 0;
      ConditionalBreakpoint[i] = 0;
      BreakCondition[i][0] = '\0';
   }

   InitializeDebuggerTables(DebuggerPageTable, OSPageTable);
   InitializeDebuggerRegisters();

   create_window_internal(debugScreen, 0, "Registers",  0, 63, 24, 79, 1, 0,
			  LTGREEN, 0, LTCYAN, &WBuffer[0], 1);

   create_window_internal(sourceScreen, 1, " Search for <address/symbol> ",
			  12, 2, 16, 78, 1, 0,
			  LTGREEN, 0, LTCYAN, &IBuffer[0], 0);

   create_window_internal(sourceScreen, 2, " Set Tab Size <2-8> ", 15, 8, 19,
			  72, 1, 0, LTCYAN, 0, LTCYAN, &TBuffer[0], 0);

   AddDebuggerCommandParser(&DisplaySema);
   AddDebuggerCommandParser(&DisplayMutex);
   AddDebuggerCommandParser(&DisplayRMutex);
   AddDebuggerCommandParser(&DisplayRWLock);
   AddDebuggerCommandParser(&DisplaySpin);
   AddDebuggerCommandParser(&HPE);
   AddDebuggerCommandParser(&HelpPE);
   AddDebuggerCommandParser(&QPE);
   AddDebuggerCommandParser(&XPE);
   AddDebuggerCommandParser(&QuitPE);
   AddDebuggerCommandParser(&ExitPE);
   AddDebuggerCommandParser(&clearScreenPE);
   AddDebuggerCommandParser(&asciiTablePE);
   AddDebuggerCommandParser(&displayScreenStructPE);
   AddDebuggerCommandParser(&TUTogglePE);
   AddDebuggerCommandParser(&TDTogglePE);
   AddDebuggerCommandParser(&TLTogglePE);
   AddDebuggerCommandParser(&TGTogglePE);
   AddDebuggerCommandParser(&TCTogglePE);
   AddDebuggerCommandParser(&TNTogglePE);
   AddDebuggerCommandParser(&TRTogglePE);
   AddDebuggerCommandParser(&TSTogglePE);
   AddDebuggerCommandParser(&TATogglePE);
   AddDebuggerCommandParser(&TTogglePE);
   AddDebuggerCommandParser(&versionPE);
   AddDebuggerCommandParser(&KernelProcessPE);
   AddDebuggerCommandParser(&KernelQueuePE);
   AddDebuggerCommandParser(&SymbolsPE);
   AddDebuggerCommandParser(&LoaderMapPE);
   AddDebuggerCommandParser(&ModulePE);
   AddDebuggerCommandParser(&ProcessesPE);
   AddDebuggerCommandParser(&ControlPE);
   AddDebuggerCommandParser(&AllPE);
   AddDebuggerCommandParser(&SegmentPE);
   AddDebuggerCommandParser(&NumericPE);
   AddDebuggerCommandParser(&GeneralPE);
   AddDebuggerCommandParser(&DefaultPE);
   AddDebuggerCommandParser(&APICPE);
   AddDebuggerCommandParser(&MPSPE);
   AddDebuggerCommandParser(&SearchMemoryPE);
   AddDebuggerCommandParser(&SearchMemoryBPE);
   AddDebuggerCommandParser(&SearchMemoryWPE);
   AddDebuggerCommandParser(&SearchMemoryDPE);
   AddDebuggerCommandParser(&ChangeWordPE);
   AddDebuggerCommandParser(&ChangeDoublePE);
   AddDebuggerCommandParser(&ChangeBytePE);
   AddDebuggerCommandParser(&ChangeDefaultPE);
   AddDebuggerCommandParser(&CloseSymbolsPE);
   AddDebuggerCommandParser(&INTRPE);
   AddDebuggerCommandParser(&ViewScreensPE);
   AddDebuggerCommandParser(&IOAPICPE);
   AddDebuggerCommandParser(&WalkPE);
   AddDebuggerCommandParser(&DumpLinkedPE);
   AddDebuggerCommandParser(&DumpWordPE);
   AddDebuggerCommandParser(&DumpStackPE);
   AddDebuggerCommandParser(&DumpDoubleStackPE);
   AddDebuggerCommandParser(&DumpDoublePE);
   AddDebuggerCommandParser(&DumpBytePE);
   AddDebuggerCommandParser(&DumpDefaultPE);
   AddDebuggerCommandParser(&Diss16PE);
   AddDebuggerCommandParser(&Diss32PE);
   AddDebuggerCommandParser(&ProceedPE);
   AddDebuggerCommandParser(&TracePE);
   AddDebuggerCommandParser(&GoPE);
   AddDebuggerCommandParser(&BreakProcessorPE);
   AddDebuggerCommandParser(&BreakNMIProcessorPE);
   AddDebuggerCommandParser(&ListProcessorsPE);
   AddDebuggerCommandParser(&ListProcessorFramePE);
   AddDebuggerCommandParser(&EAXPE);
   AddDebuggerCommandParser(&EBXPE);
   AddDebuggerCommandParser(&ECXPE);
   AddDebuggerCommandParser(&EDXPE);
   AddDebuggerCommandParser(&ESIPE);
   AddDebuggerCommandParser(&EDIPE);
   AddDebuggerCommandParser(&EBPPE);
   AddDebuggerCommandParser(&ESPPE);
   AddDebuggerCommandParser(&EIPPE);
   AddDebuggerCommandParser(&CSPE);
   AddDebuggerCommandParser(&DSPE);
   AddDebuggerCommandParser(&ESPE);
   AddDebuggerCommandParser(&FSPE);
   AddDebuggerCommandParser(&GSPE);
   AddDebuggerCommandParser(&SSPE);
   AddDebuggerCommandParser(&RFPE);
   AddDebuggerCommandParser(&TFPE);
   AddDebuggerCommandParser(&ZFPE);
   AddDebuggerCommandParser(&SFPE);
   AddDebuggerCommandParser(&PFPE);
   AddDebuggerCommandParser(&CFPE);
   AddDebuggerCommandParser(&OFPE);
   AddDebuggerCommandParser(&IFPE);
   AddDebuggerCommandParser(&IDPE);
   AddDebuggerCommandParser(&DFPE);
   AddDebuggerCommandParser(&NTPE);
   AddDebuggerCommandParser(&VMPE);
   AddDebuggerCommandParser(&VIFPE);
   AddDebuggerCommandParser(&VIPPE);
   AddDebuggerCommandParser(&AFPE);
   AddDebuggerCommandParser(&ACPE);
   AddDebuggerCommandParser(&MTRRPE);
   AddDebuggerCommandParser(&GDTPE);
   AddDebuggerCommandParser(&IDTPE);
   AddDebuggerCommandParser(&EvaluatePE);
   AddDebuggerCommandParser(&DOSTablePE);
   AddDebuggerCommandParser(&InputWordPE);
   AddDebuggerCommandParser(&InputDoublePE);
   AddDebuggerCommandParser(&InputBytePE);
   AddDebuggerCommandParser(&InputPE);
   AddDebuggerCommandParser(&OutputWordPE);
   AddDebuggerCommandParser(&OutputDoublePE);
   AddDebuggerCommandParser(&OutputBytePE);
   AddDebuggerCommandParser(&OutputPE);
   AddDebuggerCommandParser(&BreakClearAllPE);
   AddDebuggerCommandParser(&BreakClearPE);
   AddDebuggerCommandParser(&BreakMaskPE);
   AddDebuggerCommandParser(&BW1PE);
   AddDebuggerCommandParser(&BW2PE);
   AddDebuggerCommandParser(&BW4PE);
   AddDebuggerCommandParser(&BWPE);
   AddDebuggerCommandParser(&BR1PE);
   AddDebuggerCommandParser(&BR2PE);
   AddDebuggerCommandParser(&BR4PE);
   AddDebuggerCommandParser(&BRPE);
   AddDebuggerCommandParser(&BI1PE);
   AddDebuggerCommandParser(&BI2PE);
   AddDebuggerCommandParser(&BI4PE);
   AddDebuggerCommandParser(&BIPE);
   AddDebuggerCommandParser(&breakpointExecutePE);

   AddAccelRoutine(debugScreen, &traceACC);
   AddAccelRoutine(debugScreen, &proceedACC);
   AddAccelRoutine(debugScreen, &goACC);
   AddAccelRoutine(debugScreen, &exitACC);
   AddAccelRoutine(debugScreen, &toggleACC);
   AddAccelRoutine(debugScreen, &enterACC);
   AddAccelRoutine(debugScreen, &registersACC);

   AddAccelRoutine(sourceScreen, &traceSLDACC);
   AddAccelRoutine(sourceScreen, &proceedSLDACC);
   AddAccelRoutine(sourceScreen, &goSLDACC);
   AddAccelRoutine(sourceScreen, &exitSLDACC);
   AddAccelRoutine(sourceScreen, &SLDregistersACC);
   AddAccelRoutine(sourceScreen, &setBreakACC);
   AddAccelRoutine(sourceScreen, &searchACC);
   AddAccelRoutine(sourceScreen, &currentACC);
   AddAccelRoutine(sourceScreen, &screenACC);
   AddAccelRoutine(sourceScreen, &toggleSLDACC);
   AddAccelRoutine(sourceScreen, &tabsetACC);
   AddAccelRoutine(sourceScreen, &pageupACC);
   AddAccelRoutine(sourceScreen, &pagedownACC);
   AddAccelRoutine(sourceScreen, &pageHomeACC);
   AddAccelRoutine(sourceScreen, &pageEndACC);
   AddAccelRoutine(sourceScreen, &arrowupACC);
   AddAccelRoutine(sourceScreen, &arrowdownACC);
   AddAccelRoutine(sourceScreen, &arrowLeftACC);
   AddAccelRoutine(sourceScreen, &arrowRightACC);
   AddAccelRoutine(sourceScreen, &homeACC);
   AddAccelRoutine(sourceScreen, &endACC);
   AddAccelRoutine(sourceScreen, &NpageupACC);
   AddAccelRoutine(sourceScreen, &NpagedownACC);
   AddAccelRoutine(sourceScreen, &NpageHomeACC);
   AddAccelRoutine(sourceScreen, &NpageEndACC);
   AddAccelRoutine(sourceScreen, &NarrowupACC);
   AddAccelRoutine(sourceScreen, &NarrowdownACC);
   AddAccelRoutine(sourceScreen, &NarrowLeftACC);
   AddAccelRoutine(sourceScreen, &NarrowRightACC);
   AddAccelRoutine(sourceScreen, &NhomeACC);
   AddAccelRoutine(sourceScreen, &NendACC);
   return;
}

void ClearDebuggerState(void)
{
   extern void ClearDebuggerRegisters(void);

   ClearDebuggerRegisters();
   return;
}

void OpenRegistersWindow(LONG num, StackFrame *stackFrame)
{
    if (registerDisplayActive)
    {
       activate_window(num);
       WindowRegisterFrame(num, stackFrame);
    }
    return;
}

void CloseRegistersWindow(LONG num)
{
    if (registerDisplayActive)
       close_window(num);
    return;
}

void SaveLastCommandInfo(void)
{
    repeatCommand = 0;
    lastCommand = debugCommand[0];
    lastDisplayLength = displayLength;
    CopyData((LONG *)debugCommand, (LONG *)lastDebugCommand, 80);
    return;
}

LONG debugger_command_entry(LONG processor, LONG Exception,
			    StackFrame *stackFrame, BYTE *panicMsg)
{

    BYTE verbBuffer[100];
    register BYTE *verb, *pp, *vp;
    register LONG i, count, retCode, key;
    extern LONG reason_toggle;
    extern LONG control_toggle;
    extern LONG numeric_toggle;
    extern LONG segment_toggle;
    extern LONG general_toggle;
    extern LONG SLDUpdate(StackFrame *stackFrame, LONG Exception, BYTE *panicMsg);

    if (Exception > 23)
       Exception = 20;

    currentException = Exception;
    currentPanicMsg = panicMsg;

    if (debuggerActive)
    {
       printfScreenWithAttribute(debugScreen, LTCYAN, "WARNING!!!:  Nested Debugger Exception detected.\n");
       printfScreenWithAttribute(debugScreen, LTCYAN, "Continue ? <Y/N> ");
       key = GetKey();
       printfScreen(debugScreen, "%c\n", toupper(key & 0xFF));
       if (toupper(key & 0xFF) == 'N')
	  return 0;
    }
    debuggerActive++;

    lastUnasmAddress = (LONG) stackFrame->tEIP;
    lastLinkAddress = lastDumpAddress = (BYTE *) stackFrame->tESP;
    lastDisplayLength = displayLength = debugScreen->nLines - 5;

    if (reason_toggle)
       ConsoleDisplayBreakReason(debugScreen, stackFrame, Exception, panicMsg, processor, lastCommand);
    if (control_toggle)
       DisplayControlRegisters(debugScreen, processor, stackFrame);
    if (numeric_toggle)
       DisplayNPXRegisters(debugScreen, processor);
    if (segment_toggle)
       DisplaySegmentRegisters(debugScreen, stackFrame);
    if (general_toggle)
       DisplayGeneralRegisters(debugScreen, stackFrame);
    nextUnasmAddress = disassemble(debugScreen, stackFrame, lastUnasmAddress, 1, BRITEWHITE, 1);

    retCode = SLDUpdate(stackFrame, Exception, panicMsg);
    if (retCode == (LONG) -1)
       return 0;

    if (activeScreen != debugScreen && activeScreen != sourceScreen)
       oldScreen = GetVidOwner();

    if (SourceDebuggerActive)
    {
       if (activeScreen != sourceScreen)
	  SetVideoOwner(sourceScreen);

       ClearTempBreakpoints();
       while (1)
       {
SLDInput:;
	  key = GetKey();
	  if (key)
	  {
	     retCode = AccelRoutine(sourceScreen, key, stackFrame);
	     if (retCode == (LONG) -1)
		return 0;

	     if (retCode && !SourceDebuggerActive)
		goto InteractiveInput;

	     if (retCode)
		continue;
	  }
       }
    }
    else
    {
       if (activeScreen != debugScreen)
	  SetVideoOwner(debugScreen);

       ClearTempBreakpoints();
       while (1)
       {
InteractiveInput:;
	  printfScreenWithAttribute(debugScreen, LTCYAN, "(%i)> ", get_processor_id());
	  SaveLastCommandInfo();
	  OpenRegistersWindow(0, stackFrame);
	  key = ScreenInputFromKeyboard(debugScreen, &debugCommand[0], 0, 80, BRITEWHITE);
	  CloseRegistersWindow(0);
	  if (key)
	  {
	     retCode = AccelRoutine(debugScreen, key, stackFrame);
	     if (retCode == (LONG) -1)
		return 0;

	     if (retCode && SourceDebuggerActive)
		goto SLDInput;

	     if (retCode)
		continue;
	  }
	  if (*debugCommand)
	  {
	     count = 0;
	     pp = debugCommand;
	     vp = verb = &verbBuffer[0];
	     while (*pp && *pp == ' ' && count++ < 80)
		pp++;

	     while (*pp && *pp != ' ' && *pp != '=' && count++ < 80)
		*vp++ = *pp++;
	     *vp = '\0';

	     while (*pp && *pp == ' ' && count++ < 80)
		pp++;

	     retCode = DebuggerParserRoutine(debugScreen, verb, debugCommand, stackFrame, Exception);
	     if (retCode == (LONG) -1)
		return 0;
	  }
       }
    }

}

void ClearTempBreakpoints(void)
{

   register LONG i;

   for (i=0; i < 4; i++)
   {
      if (BreakTemp[i])
      {
	 BreakTemp[i] = 0;
	 BreakReserved[i] = 0;
	 BreakPoints[i] = 0;
	 BreakType[i] = 0;
	 BreakLength[i] = 0;
	 BreakGo[i] = 0;
	 BreakProceed[i] = 0;
      }
   }
   SetDebugRegisters();
   return;

}

LONG ValidBreakpoint(LONG address)
{

   register LONG i;

   for (i=0; i < 4; i++)
   {
      if (!BreakTemp[i])
	 if (BreakPoints[i] == address)
	    return 1;
   }
   return 0;

}

LONG debug_lock(rlock_t *rlock, LONG proc)
{
    while (rspin_try_lock(rlock))
    {
       if (processor_table[proc].ProcessorState == PROCESSOR_SHUTDOWN)
	  return 0;
    }
    return 1;
}

void debug_unlock(rlock_t *rlock)
{
    rspin_unlock(rlock);
}

