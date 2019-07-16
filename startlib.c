

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  STARTLIB.C
*   DESCRIP  :  System Startup Registry for MANOS v1.0
*   DATE     :  August 20, 1998
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
#include "peexe.h"
#include "window.h"
#include "timer.h"
#include "line.h"
#include "dosfile.h"
#include "loader.h"
#include "event.h"
#include "bus.h"
#include "start.h"


LONG SetupDosDataTableSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    if (node) {};
    if (dos) {};
    if (pm) {};

    DosDataTable = dos;
    return 0;
}

LONG InitializeBusMemorySU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern LONG InitializeBusMemory(DOS_TABLE *dos, PM_TABLE *pm);

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitializeBusMemory(dos, pm);
    return 0;
}

LONG InitializeEventsSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern void InitializeEvents(void);

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitializeEvents();
    return 0;
}

LONG InitializeIFSSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern void InitializeIFS(void);

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitializeIFS();
    return 0;
}

LONG InitializeModuleListSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern void InitializeModuleList(void);

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitializeModuleList();
    return 0;
}

LONG InitializeSymbolTableSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitializeSymbolTable(&serverModuleHandle);
    return 0;
}

LONG InitializeProcessorStartupSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern LONG InitializeProcessorStartup(DOS_TABLE *dos);

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitializeProcessorStartup(dos);
    return 0;
}

LONG InitializeKernelSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitializeKernel();
    return 0;
}

LONG InitializeDebuggerSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern void InitializeDebugger(LONG DebuggerPDE, LONG OSPDE);
    extern LONG OSPDE, DebuggerPDE;

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitializeDebugger(DebuggerPDE, OSPDE);
    return 0;
}

LONG InitializeHALSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitializeHAL();
    return 0;
}

LONG InitializeInterruptsSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern void InitializeInterrupts(void);

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitializeInterrupts();
    return 0;
}

LONG InitializeVideoSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitializeVideo(dos);
    return 0;
}

LONG InitDMASU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern void InitDMA(void);

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitDMA();
    return 0;
}

LONG EnableSystemPagingSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern void EnableSystemPaging(void);

    if (node) {};
    if (dos) {};
    if (pm) {};

    EnableSystemPaging();
    return 0;
}

LONG displayVersionInfoSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern LONG displayVersionInfo(SCREEN *, BYTE *);

    if (node) {};
    if (dos) {};
    if (pm) {};

    displayVersionInfo(consoleScreen, 0);
    return 0;
}

LONG ResetKeyboardSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern void ResetKeboard(void);

    if (node) {};
    if (dos) {};
    if (pm) {};

    ResetKeyboard();
    return 0;
}

LONG InitPagingMemorySU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern void InitPagedMemory(void);

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitPagedMemory();
    return 0;
}

LONG InitializeSMPHALSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitializeSMPHAL();
    return 0;
}

LONG InitializeSMPSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern void InitializeSMP(void);

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitializeSMP();
    return 0;
}

LONG InitializeExportListSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern void InitializeExportList(MODULE_HANDLE *module);

    if (node) {};
    if (dos) {};
    if (pm) {};

    InitializeExportList(&serverModuleHandle);
    return 0;
}

LONG displaySystemMemoryInfoSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern LONG displaySystemMemoryInfo(void);

    if (node) {};
    if (dos) {};
    if (pm) {};

    displaySystemMemoryInfo();
    return 0;
}

LONG MeasureSpeedRatingSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern LONG MeasureSpeedRating(void);

    if (node) {};
    if (dos) {};
    if (pm) {};

    MeasureSpeedRating();
    return 0;
}

LONG CreateConsoleProcessSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern void CreateConsoleProcess(void);

    if (node) {};
    if (dos) {};
    if (pm) {};

    CreateConsoleProcess();
    return 0;
}

LONG EventNotifySU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{

    if (node) {};
    if (dos) {};
    if (pm) {};

    EventNotify(EVENT_ENTER_OS, 0);
    return 0;
}

LONG RegisterOSEventsSU(struct _STARTUP_NODE *node,
			 DOS_TABLE *dos, PM_TABLE *pm)
{
    extern void RegisterOSEvents(void);

    if (node) {};
    if (dos) {};
    if (pm) {};

    RegisterOSEvents();
    return 0;
}

void ExitOSResetKeyboard(LONG value)
{
    if (value) {};
    ResetKeyboard();
}

void ExitOSClearDebuggerState(LONG value)
{
    extern void ClearDebuggerState(void);
    if (value) {};
    ClearDebuggerState();
}

STARTUP_NODE setupDosSUN = { 0, 0, SetupDosDataTableSU, 0, 0, 0, 0 };
STARTUP_NODE initBusMemorySUN = { 0, 0, InitializeBusMemorySU, 0, 0, 0, 0 };
STARTUP_NODE initEventsSUN = { 0, 0, InitializeEventsSU, 0, 0, 0, 0 };
STARTUP_NODE initIFSSUN = { 0, 0, InitializeIFSSU, 0, 0, 0, 0 };
STARTUP_NODE initModulesSUN = { 0, 0, InitializeModuleListSU, 0, 0, 0, 0 };
STARTUP_NODE initSymbolsSUN = { 0, 0, InitializeSymbolTableSU, 0, 0, 0, 0 };
STARTUP_NODE initPStartupSUN = { 0, 0, InitializeProcessorStartupSU, 0, 0, 0, 0 };
STARTUP_NODE initKernelSUN = { 0, 0, InitializeKernelSU, 0, 0, 0, 0 };
STARTUP_NODE initDebugSUN = { 0, 0, InitializeDebuggerSU, 0, 0, 0, 0 };
STARTUP_NODE initHALSUN = { 0, 0, InitializeHALSU, 0, 0, 0, 0 };
STARTUP_NODE initInterruptSUN = { 0, 0, InitializeInterruptsSU, 0, 0, 0, 0 };
STARTUP_NODE initVideoSUN = { 0, 0, InitializeVideoSU, 0, 0, 0, 0 };
STARTUP_NODE initDMASUN = { 0, 0, InitDMASU, 0, 0, 0, 0 };
STARTUP_NODE initPagingSUN = { 0, 0, EnableSystemPagingSU, 0, 0, 0, 0 };
STARTUP_NODE initVersionSUN = { 0, 0, displayVersionInfoSU, 0, 0, 0, 0 };
STARTUP_NODE initKeyboardSUN = { 0, 0, ResetKeyboardSU, 0, 0, 0, 0 };
STARTUP_NODE initPagedMemSUN = { 0, 0, InitPagingMemorySU, 0, 0, 0, 0 };
STARTUP_NODE initSMPHALSUN = { 0, 0, InitializeSMPHALSU, 0, 0, 0, 0 };
STARTUP_NODE initSMPSUN = { 0, 0, InitializeSMPSU, 0, 0, 0, 0 };
STARTUP_NODE initExportsSUN = { 0, 0, InitializeExportListSU, 0, 0, 0, 0 };
STARTUP_NODE initDisplayMemSUN = { 0, 0, displaySystemMemoryInfoSU, 0, 0, 0, 0 };
STARTUP_NODE initSpeedSUN = { 0, 0, MeasureSpeedRatingSU, 0, 0, 0, 0 };
STARTUP_NODE initConsoleSUN = { 0, 0, CreateConsoleProcessSU, 0, 0, 0, 0 };
STARTUP_NODE initNotifyUPSUN = { 0, 0, EventNotifySU, 0, 0, 0, 0 };
STARTUP_NODE initOSEventsSUN = { 0, 0, RegisterOSEventsSU, 0, 0, 0, 0 };

LONG UpdateStartupRegistry(void)
{
   AddStartupRoutine(&setupDosSUN);      // set DosDataTable pointer
   AddStartupRoutine(&initBusMemorySUN); // scan buses and EISA NVRAM memory
   AddStartupRoutine(&initEventsSUN);    // setup Event subsystem
   AddStartupRoutine(&initIFSSUN);       // setup the Installable File System
   AddStartupRoutine(&initModulesSUN);   // setup module loader
   AddStartupRoutine(&initSymbolsSUN);   // setup symbol tables
   AddStartupRoutine(&initPStartupSUN);  // create SMP startup code
   AddStartupRoutine(&initKernelSUN);    // setup the kernel
   AddStartupRoutine(&initDebugSUN);     // setup the debugger
   AddStartupRoutine(&initHALSUN);       // setup Hardware Abstraction Layer
   AddStartupRoutine(&initInterruptSUN); // setup device interrupt tables
   AddStartupRoutine(&initVideoSUN);     // setup video system
   AddStartupRoutine(&initDMASUN);       // setup DMA controllers
   AddStartupRoutine(&initPagingSUN);    // activate paging
   AddStartupRoutine(&initVersionSUN);   // display MANOS version
   AddStartupRoutine(&initKeyboardSUN);  // setup keyboard driver
   AddStartupRoutine(&initPagedMemSUN);  // create system page tables
   AddStartupRoutine(&initSMPHALSUN);    // load SMP Hardware Abstraction
   AddStartupRoutine(&initSMPSUN);       // enable SMP support
   AddStartupRoutine(&initExportsSUN);   // setup exports tables
   AddStartupRoutine(&initDisplayMemSUN);// display system memory totals
   AddStartupRoutine(&initSpeedSUN);     // perform proceesor speed test
   AddStartupRoutine(&initConsoleSUN);   // create console process
   AddStartupRoutine(&initNotifyUPSUN);  // notify OS is initialized
   AddStartupRoutine(&initOSEventsSUN);  // register screen events

   // additional startup routines should be added here

   return 0;
}

