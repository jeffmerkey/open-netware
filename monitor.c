

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  MONITOR.C
*   DESCRIP  :  Console Monitor for MANOS v1.0
*   DATE     :  January 10, 1998
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
#include "window.h"
#include "menu.h"
#include "dosfile.h"
#include "timer.h"
#include "peexe.h"
#include "line.h"
#include "loader.h"
#include "malloc.h"
#include "free.h"
#include "event.h"

extern	uint32	mpt_total_pages;
extern	uint32  mpt_reserved_pages;
extern	uint32  mpt_physical_pages;
extern	uint32  mpt_logical_pages;
extern	uint32  mpt_special_pages;
extern	uint32  mpt_available_pages;
extern	uint32	mpt_system_page_count;
extern	uint32	mpt_system_page_use_count;
extern	uint32	mpt_move_counter;
extern	uint32	mpt_alloc_physical_count;
extern	uint32	mpt_return_physical_count;

extern	LONG	page_fault_success;
extern	LONG	page_fault_failure;
extern	LONG	page_fault_move_counter;
extern	LONG	page_fault_alloc_error;
extern	LONG	*mpt_physical_block_list_head;
extern	LONG	mpt_physical_block_activity;
extern	LONG	*mpt_system_list_head;
extern	LONG	mpt_system_page_activity;
extern	LONG	*mpt_list_head;
extern	LONG	*mpt_activity_counter;

LONG	last_mpt_move_counter;
LONG	last_page_fault_success;
LONG	last_page_fault_failure;
LONG	last_page_fault_move_counter;
LONG	last_page_fault_alloc_error;
LONG	last_alloc_physical_count;
LONG	last_return_physical_count;
LONG	last_physical_block_count;
LONG	last_physical_block_activity;
LONG	last_system_page_count;
LONG	last_system_page_activity;
LONG	last_regular_page_count;
LONG   	last_regular_page_activity;
LONG	mpt_regular_page_count;
LONG	mpt_regular_page_activity;

#define PENTIUM_EVENT        0x0011
#define PENTIUM_COUNTER_1    0x0012
#define PENTIUM_COUNTER_2    0x0013

#define PPII_COUNTER_1       0x00C1
#define PPII_COUNTER_2       0x00C2
#define PPII_EVENT_1         0x0186
#define PPII_EVENT_2         0x0187

#define PENTIUM_PPII_STDC    0x0010

#define EMON_CALL	     0x01C0
#define EMON_C3		     0x0180
#define EMON_C3_NON	     0x0140
#define EMON_ALL	     0x00C0
#define EMON_R3		     0x0080
#define EMON_R3_NON	     0x0040
#define EMON_NONE	     0x0000

LONG PreemptiveEvents = 0;
LONG PagingCount = 0;
LONG TotalInterrupts = 0;
LONG LockAssertions = 0;
LONG PageFaults = 0;
LONG TLBShootdowns = 0;
LONG TotalXCalls;
LONG XCallFailures;
LONG XCallBusyCount;

LONG MonitorActive = 0;
LONG DaysUp = 0;
LONG HoursUp = 0;
LONG MinutesUp = 0;
LONG SecondsUp = 0;
LONG TicksUp = 0;
LONG UpdateStats = 0;
LONG EMONUpdateOK = 0;

BYTE *kernelStateArray[]={
    "PS_INIT  ",
    "PS_ACTIVE",
    "PS_SLEEP ",
    "PS_SYNC  ",
    "PS_LWP   ",
    "PS_HALT  ",
    "  ?????  ",
    "  ?????  ",
    "  ?????  ",
    "  ?????  ",
    "  ?????  ",
    "  ?????  ",
    "  ?????  ",
    "  ?????  ",
    "  ?????  ",
    "  ?????  ",
    "  ?????  "
};

BYTE *engineState[]={
   "INACTIVE",
   "ACTIVE  ",
   "DEBUG   ",
   "SIGNAL  ",
   "FATAL   ",
   "INIT    ",
   "SHUTDOWN",
   "RESUME  ",
   "LOCK    ",
   "CURRENT ",
   "SUSPEND ",
   "??????  ",
   "??????  ",
   "??????  ",
   "??????  ",
   "??????  ",
   "??????  ",
   "??????  ",
   "??????  "
};

BYTE widgets[4]={ '|', '/', '-', '\\' };

LONG EMONClocks[MAX_PROCESSORS];
LONG EMONCounter1[MAX_PROCESSORS];
LONG EMONCounter2[MAX_PROCESSORS];
LONG EMONRegisters[2] = { -1, -1 };
LWP  EMONLWP[MAX_PROCESSORS];
LWP  EMONLWPSet[MAX_PROCESSORS];

BYTE *PentiumEMON[]={
     "Data Read                          ",
     "Data Write                         ",
     "Data Read or Data Write            ",
     "Data Read Miss                     ",
     "Data Write Miss                    ",
     "Data Read Miss or Data Write Miss  ",
     "Data TLB Miss                      ",
     "Write (hit) to M or E state lines  ",
     "Data Cache Lines Written Back      ",
     "Data Cache Snoops                  ",
     "Data Cache Snoop Hits              ",
     "Memory Accesses In Both Pipes      ",
     "Bank Conflicts                     ",
     "Misaligned Data Memory References  ",
     "Code Read                          ",
     "Code TLB Miss                      ",
     "Code Cache Miss                    ",
     "Any Segment Register Load          ",
     "Branches                           ",
     "BTB Hits                           ",
     "Taken Branch or BTB Hit            ",
     "Pipeline Flushes                   ",
     "Instructions Executed              ",
     "Instructions Executed in the v-pipe",
     "Bus Utilization (clocks)           ",
     "Pipeline Stalled by Write Backup   ",
     "Pipeline Stalled by Data Mem Read  ",
     "Pentium Stalled by write to E or M ",
     "Locked Bus Cycle                   ",
     "I/O Read or Write Cycle            ",
     "Non-cacheable memory references    ",
     "AGI (Address Generation Interlock) ",
     "Floating Point Operations          ",
     "Breakpoint 0 Match                 ",
     "Breakpoint 1 Match                 ",
     "Breakpoint 2 Match                 ",
     "Breakpoint 3 Match                 ",
     "Hardware Interrupts                "



};

BYTE *PentiumProEMON[]={
     "Violations of Strong Ordering Detected    ",
     "Number of Store Buffer Forwards           ",
     "Number of Store Buffer Blocks             ",
     "Number of Store Buffer Drain Cycles       ",
     "Misaligned Data References                ",
     "Segment Register Loads                    ",
     "Floating Point Operations (0)             ",
     "Floating Point Exceptions                 ",
     "Number of Multiplies (1)                  ",
     "Number of Divides    (1)                  ",
     "Number Busy Divider Cycles (0)            ",
     "Number of L2 Address Strobes              ",
     "L2 Cache Data Bus Busy Cycles             ",
     "L2 Cache Data Bus Busy During Transfer    ",
     "L2 Cache Allocated Lines                  ",
     "L2 Cache Allocated M State Lines          ",
     "L2 Cache Evicted Lines                    ",
     "L2 Cache M State Evicted Lines            ",
     "L2 Cache Instruction Fetches              ",
     "L2 Cache Loads (Reads)                    ",
     "L2 Cache Stores (Writes)                  ",
     "L2 Cache Locks                            ",
     "L2 Cache Data Requests                    ",
     "L2 Cache Reads                            ",
     "L2 Cache Requests                         ",
     "L1 Cache Data Reads                       ",
     "L1 Cache Data Writes                      ",
     "L1 Cache Data Locks                       ",
     "Data Memory References                    ",
     "L1 Cache Data Requests                    ",
     "Lines Brought Into L1 Cache               ",
     "L1 Cache Allocated M State Lines          ",
     "L1 Cache Evicted M State Lines            ",
     "L1 Data Cache Misses Outstanding          ",
     "Data TLB Misses                           ",
     "Breakpoint Matches                        ",
     "External Bus Requests Outstanding         ",
     "Bus Clock Cycles Driving BNR Pin          ",
     "Bus Clock Cycles Asserting DRDY           ",
     "Bus Clock Cycles with LOCK# Asserted      ",
     "Bus Clock Cycles During PROC Data Receive ",
     "Bus Burst Read Transactions               ",
     "Bus Read for Ownership Transactions       ",
     "Bus WB or M State Lines Evicted/writeback ",
     "Bus Burst Instruction Fetches             ",
     "Bus Invalidate Transactions               ",
     "Bus Partial Write Transactions            ",
     "Bus Partial Mem Transactions/MemMappedIO  ",
     "Bus IO Transactions (IN/OUT)              ",
     "Bus Deferred Reply Transactions           ",
     "Bus Burst Transactions                    ",
     "Bus Memory Transactions                   ",
     "Bus Transactions                          ",
     "Burst Reads Snooped By L1                 ",
     "Invalidate Transactions Snooped by L1     ",
     "Bus Requests Snooped by L1                ",
     "Burst Reads Snooped by L1 and L2          ",
     "Invalidates Snooped by L1 and L2          ",
     "Bus Requests Snooped by L1 and L2         ",
     "Snoop Responses to Transactions (ext bus) ",
     "Number of Data Bus Stalls                 ",
     "CPU Not Halted Cycles                     ",
     "Number HIT pin Assertions                 ",
     "Number HITM pin Assertions                ",
     "Number AERR pin Assertions                ",
     "Number BERR pin Assertions                ",
     "BINIT pin Assertions/Bus Snoop Stalls     ",
     "Instruction Fetches                       ",
     "Instruction Fetch Misses                  ",
     "Uncacheable Instruction Fetches           ",
     "Prefetch Hits                             ",
     "Breakpoint Matches                        ",
     "Instruction TLB Misses                    ",
     "Cycles Instruction Fetch is Stalled       ",
     "Cycles Instruction Fetch is Stalled (pipe)",
     "UOPs Dispatched from Resv. Stn            ",
     "Cycles during UOP Dispatch                ",
     "Resource Related Stalls                   ",
     "Instructions Retired                      ",
     "Floating Point Operations Retired (0)     ",
     "UOPs Retired                              ",
     "Self Modifying Code Detected              ",
     "Branch Instructions Retired               ",
     "Branch Miss-Predictions Retired           ",
     "Clocks While Interrupts are Masked        ",
     "Clocks While Interrupts are Masked/Pending",
     "Hardware Interrupts                       ",
     "Taken Branches Retired                    ",
     "Taken Mispredictions Retired              ",
     "Instructions Decoded                      ",
     "OUPs Decoded                              ",
     "Register Renaming (Partial) Stalls        ",
     "Cycles that Register Renaming Stalls Occur",
     "Branch Instructions Decoded               ",
     "BTB Hits                                  ",
     "BTB Misses                                ",
     "Return Stack Buffer Hits                  ",
     "Bogus Branches Detected                   ",
     "Mispredicted Returns Retired              ",
     "Number of BACLEARS# Asserted              "
};

LONG PentiumEMONCodes[] = {
  0x00, 0x01, 0x28, 0x03, 0x04, 0x29, 0x02, 0x05, 0x06, 0x07, 0x08, 0x09,
  0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x22, 0x23, 0x24, 0x25,
  0x26, 0x27
};

LONG PentiumProEMONCodes[] =
{
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x10, 0x11, 0x12, 0x13, 0x14, 0x21,
  0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,
  0x2E, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B,
  0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
  0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x80, 0x81, 0x82, 0x83, 0x84,
  0x85, 0x86, 0x87, 0xA0, 0xA1, 0xA2, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5,
  0xC6, 0xC7, 0xC8, 0xC9, 0xCa, 0xD0, 0xD1, 0xD2, 0xD3, 0xE0, 0xE1, 0xE2,
  0xE3, 0xE4, 0xE5, 0xE6
};

extern LONG NumberOfOpenHandles;
extern LONG TotalSystemMemory;
extern LONG HighMemoryLength;
extern LONG LowMemoryLength;
extern BYTE kernelState[];
extern LONG ConnectionsAllowed;
extern LONG ConnectionsOpen;
extern LONG systemThreadCount;
extern BYTE ServerName[];
extern void ReadMSR(LONG msr, LONG *val1, LONG *val2);
extern void WriteMSR(LONG msr, LONG *val1, LONG *val2);

LONG gwNum;
LONG resizeEvent = 0;
SCREEN *monitorScreen = 0;

void VMUpdate(LONG wNum)
{

    BYTE displayBuffer[100] = { "" };
    register LONG i;

    while (1)
    {
	  sprintf(displayBuffer,
		   "              Total Pages: %8u", mpt_total_pages);
	  write_portal(wNum, displayBuffer, 1, 1, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
		   "           Reserved Pages: %8u", mpt_reserved_pages);
	  write_portal(wNum, displayBuffer, 2, 1, BRITEWHITE | BGBLUE);


	  if (mpt_system_list_head[1] < last_system_page_count)
	  {
		  sprintf(displayBuffer,
			   "             System Pages: %8u   -%8u  [%8u]",
			   mpt_system_list_head[1],
			   last_system_page_count - mpt_system_list_head[1],
			   mpt_system_page_activity - last_system_page_activity);
	  }
	  else
	  {
		  sprintf(displayBuffer,
			   "             System Pages: %8u   +%8u  [%8u]",
			   mpt_system_list_head[1],
			   mpt_system_list_head[1] - last_system_page_count,
			   mpt_system_page_activity - last_system_page_activity);
	  }
	  last_system_page_activity = mpt_system_page_activity;
	  last_system_page_count = mpt_system_list_head[1];
	  write_portal(wNum, displayBuffer, 3, 1, BRITEWHITE | BGBLUE);

	  mpt_regular_page_count = mpt_list_head[(0 * 1024) + 1] +
				   mpt_list_head[(1 * 1024) + 1] +
				   mpt_list_head[(2 * 1024) + 1] +
				   mpt_list_head[(3 * 1024) + 1];

	  mpt_regular_page_activity = mpt_activity_counter[(0 * 1024) + 1] +
				   mpt_activity_counter[(1 * 1024) + 1] +
				   mpt_activity_counter[(2 * 1024) + 1] +
				   mpt_activity_counter[(3 * 1024) + 1];

	  if (mpt_regular_page_count < last_regular_page_count)
	  {
		  sprintf(displayBuffer,
			   "          Available Pages: %8u   -%8u  [%8u]",
			   mpt_regular_page_count,
			   last_regular_page_count - mpt_regular_page_count,
			   mpt_regular_page_activity - last_regular_page_activity);
	  }
	  else
	  {
		  sprintf(displayBuffer,
			   "          Available Pages: %8u   +%8u  [%8u]",
			   mpt_regular_page_count,
			   mpt_regular_page_count - last_regular_page_count,
			   mpt_regular_page_activity - last_regular_page_activity);
	  }
	  last_regular_page_activity = mpt_regular_page_activity;
	  last_regular_page_count = mpt_regular_page_count;
	  write_portal(wNum, displayBuffer, 4, 1, BRITEWHITE | BGBLUE);


	  if (mpt_physical_block_list_head[1] < last_physical_block_count)
	  {
		  sprintf(displayBuffer,
			   "          Physical Blocks: %8u   -%8u  [%8u]",
			   mpt_physical_block_list_head[1],
			   last_physical_block_count - mpt_physical_block_list_head[1],
			   mpt_physical_block_activity - last_physical_block_activity);
	  }
	  else
	  {
		  sprintf(displayBuffer,
			   "          Physical Blocks: %8u   +%8u  [%8u]",
			   mpt_physical_block_list_head[1],
			   mpt_physical_block_list_head[1] - last_physical_block_count,
			   mpt_physical_block_activity - last_physical_block_activity);
	  }
	  last_physical_block_activity = mpt_physical_block_activity;
	  last_physical_block_count = mpt_physical_block_list_head[1];
	  write_portal(wNum, displayBuffer, 5, 1, BRITEWHITE | BGBLUE);



	  sprintf(displayBuffer,
		   "              Moved Pages: %8u  %8u", mpt_move_counter -  last_mpt_move_counter, mpt_move_counter);
	  write_portal(wNum, displayBuffer, 8, 1, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
		   "              Page Faults: %8u  %8u", page_fault_success -  last_page_fault_success, page_fault_success);
	  write_portal(wNum, displayBuffer, 9, 1, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
		   "     Page Faults Failures: %8u  %8u", page_fault_failure -  last_page_fault_failure, page_fault_failure);
	  write_portal(wNum, displayBuffer, 10, 1, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
		   "        Page Faults Moves: %8u  %8u", page_fault_move_counter -  last_page_fault_move_counter, page_fault_move_counter);
	  write_portal(wNum, displayBuffer, 11, 1, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
		   "     Page Fault Postpones: %8u  %8u", page_fault_alloc_error -  last_page_fault_alloc_error, page_fault_alloc_error);
	  write_portal(wNum, displayBuffer, 12, 1, BRITEWHITE | BGBLUE);

	last_mpt_move_counter = mpt_move_counter;
	last_page_fault_success = page_fault_success;
	last_page_fault_failure = page_fault_failure;
	last_page_fault_move_counter = page_fault_move_counter;
	last_page_fault_alloc_error = page_fault_alloc_error;

       update_portal(wNum);
       delayThread(18);
    }

}

void VMPortalProcess(SCREEN *screen)
{

    register LONG i, key;
    register LONG wNum;
    register PROCESS *child;

    UpdateStats = 0;

    wNum = make_portal(screen,
		       " Virtual Memory Information ",
		       3,   // window start
		       0,
		       screen->nLines - 1,
		       screen->nCols - 1,  // window end
		       100, // number of lines
		       1,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0);

    if (wNum)
    {
       child = createThread("Virtual Memory Update", (LONG (*)()) VMUpdate,
			     8192 * 2, (void *) wNum, 0);
       if (child)
       {
	  activate_portal(wNum);
	  killThread(child);
	  free_portal(wNum);
       }
    }

    UpdateStats = TRUE;

    return;

}

void UpdateTime(SCREEN *screen)
{

    BYTE displayBuffer[100] = { "" };
    LONG date, time;

    while (1)
    {
       GetCMOSDate(&date);
       GetCMOSTime(&time);

       sprintf(displayBuffer,
	       "%02X-%02X-%02X  %2X:%02X:%02X%c",
	       (date >> 16) & 0xFF,
	       (date >> 8) & 0xFF,
	       (date >> 24) & 0xFF,
	       (time >> 16) & 0xFF,
	       (time >> 8) & 0xFF,
	       time & 0xFF);

       PutVidString(screen, displayBuffer, 60, 0, BLUE | BGCYAN);
       delayThread(18);
    }

}


void ConsoleUpdate(LONG wNum)
{

    register LONG i;
    register LONG cW = 0;
    BYTE displayBuffer[100] = { "" };

    TotalInterrupts = 0;
    LockAssertions = 0;
    TLBShootdowns = 0;
    PageFaults = 0;
    TotalXCalls = 0;
    XCallFailures = 0;
    XCallBusyCount = 0;

    for (i=0; i < active_processors; i++)
    {
       processor_table[i].totalInterrupts = 0;
       processor_table[i].lockAssertions = 0;
       processor_table[i].TLBShootdowns = 0;
       processor_table[i].pageFaults = 0;
       processor_table[i].xcall_total = 0;
       processor_table[i].xcall_fail = 0;
       processor_table[i].xcall_busy = 0;
    }

    while (1)
    {
       if (UpdateStats)
       {
	  TotalInterrupts = 0;
	  LockAssertions = 0;
	  TLBShootdowns = 0;
	  PageFaults = 0;
	  TotalXCalls = 0;
	  XCallFailures = 0;
	  XCallBusyCount = 0;

	  for (i=0; i < active_processors; i++)
	  {
	     TotalInterrupts += processor_table[i].totalInterrupts;
	     processor_table[i].totalInterrupts = 0;

	     LockAssertions += processor_table[i].lockAssertions;
	     processor_table[i].lockAssertions = 0;

	     TLBShootdowns += processor_table[i].TLBShootdowns;
	     processor_table[i].TLBShootdowns = 0;

	     PageFaults += processor_table[i].pageFaults;
	     processor_table[i].pageFaults = 0;

	     TotalXCalls += processor_table[i].xcall_total;
	     processor_table[i].xcall_total = 0;

	     XCallFailures += processor_table[i].xcall_fail;
	     XCallBusyCount += processor_table[i].xcall_busy;

	  }

	  sprintf(displayBuffer,
	       " Number Of Processors    : %6u   Server Up Time      :  %4d:%02d:%02d:%02d [%c]",
	       num_procs,
	       DaysUp,
	       HoursUp,
	       MinutesUp,
	       SecondsUp,
	       widgets[cW++ & 3]);
	  write_portal(wNum, displayBuffer, 0, 0, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
	       " Active Processors       : %6u   Total System Memory : %12u bytes",
	       active_processors,
	       TotalSystemMemory);
	  write_portal(wNum, displayBuffer, 1, 0, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
	       " Utilization             : %6u%%  Memory Above 1MB    : %12u bytes",
	       systemUtilization,
	       HighMemoryLength);
	  write_portal(wNum, displayBuffer, 2, 0, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
	       " Total XCalls            : %6u   Memory Below 1MB    : %12u bytes",
	       TotalXCalls,
	       LowMemoryLength);
	  write_portal(wNum, displayBuffer, 3, 0, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
	       " XCall Failures          : %6u   TLB Shootdowns      : %12u",
	       XCallFailures,
	       TLBShootdowns);
	  write_portal(wNum, displayBuffer, 4, 0, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
	       " Preemptive Events       : %6u   Page Faults         : %12u",
	       PreemptiveEvents,
	       PageFaults);
	  write_portal(wNum, displayBuffer, 5, 0, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
	       " Interrupts/second       : %6u   Locks/second        : %12u",
	       TotalInterrupts,
	       LockAssertions);
	  write_portal(wNum, displayBuffer, 6, 0, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
	       " Paging Count            : %6u   XCall Busy Count    : %12u",
	       PagingCount,
	       XCallBusyCount);
	  write_portal(wNum, displayBuffer, 7, 0, BRITEWHITE | BGBLUE);

	  PreemptiveEvents = 0;
	  PagingCount = 0;

	  update_static_portal(wNum);
       }
       delayThread(18);

    }


}


void ProcessorUpdate(LONG wNum)
{

    BYTE displayBuffer[100] = { "" };
    register LONG i;
    register LONG x, y;

    while (1)
    {
       for (i=0; i < MAX_PROCESSORS; i++)
       {

	  if (i & 1)
	     y = 40;
	  else
	     y = 2;

	  switch (i)
	  {
	     case 0:
	     case 1:
		x = 0;
		break;

	     case 2:
	     case 3:
		x = 9;
		break;

	     case 4:
	     case 5:
		x = 18;
		break;

	     case 6:
	     case 7:
		x = 27;
		break;
	  }

	  sprintf(displayBuffer,
		   " Processor        : %6u", processor_table[i].ProcessorNumber);
	  write_portal(wNum, displayBuffer, x + 1, y, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
		   " Processor State  : %s", engineState[processor_table[i].ProcessorState & 0xF]);
	  write_portal(wNum, displayBuffer, x + 2, y, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
		   " Utilization      : %6u%%", processorUtilization[i]);
	  write_portal(wNum, displayBuffer, x + 3, y, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
		   " Interrupts       : %6u", processor_table[i].totalInterrupts);
	  write_portal(wNum, displayBuffer, x + 4, y, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
		   " Locks            : %6u", processor_table[i].lockAssertions);
	  write_portal(wNum, displayBuffer, x + 5, y, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
		   " TLB Shootdowns   : %6u", processor_table[i].TLBShootdowns);
	  write_portal(wNum, displayBuffer, x + 6, y, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
		   " Page Faults      : %6u", processor_table[i].pageFaults);
	  write_portal(wNum, displayBuffer, x + 7, y, BRITEWHITE | BGBLUE);

	  processor_table[i].totalInterrupts = 0;
	  processor_table[i].lockAssertions = 0;
	  processor_table[i].TLBShootdowns = 0;
	  processor_table[i].pageFaults = 0;

       }
       update_portal(wNum);

       delayThread(18);

    }




}

void ProcessorPortalProcess(SCREEN *screen)
{

    register LONG i, key;
    register LONG wNum;
    register PROCESS *child;

    UpdateStats = 0;

    wNum = make_portal(screen,
		       " Processor Information ",
		       3,
		       0,
		       screen->nLines - 1,
		       screen->nCols - 1,
		       36,
		       1,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0);

    if (wNum)
    {
       child = createThread("Processor Update", (LONG (*)()) ProcessorUpdate,
			     8192 * 2, (void *) wNum, 0);
       if (child)
       {
	  activate_portal(wNum);
	  killThread(child);
	  free_portal(wNum);
       }
    }

    UpdateStats = TRUE;

    return;

}

BYTE lDelim[] = {
    0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4,
    0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4,
    0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4,
    0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4,
    0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4,
    0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4,
    0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4,
    0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4,
    0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4,
    0xC4, 0xC4
};


LONG moduleAction(SCREEN *screen, LONG value, BYTE *option)
{

     BYTE buf[255];
     register LONG wNum;
     MODULE_HANDLE *module;

     if (screen) {};
     if (option) {};

     sprintf(buf, " %s Module Information ", option);
     wNum = create_window(screen,
			 buf,
			 5,
			 4,
			 22,
			 75,
			 1,
			 YELLOW | BGBLUE,
			 YELLOW | BGBLUE,
			 BRITEWHITE | BGBLUE,
			 BRITEWHITE | BGBLUE,
			 1);

     if (!wNum)
	return -1;

     activate_window(wNum);

     module = (MODULE_HANDLE *) value;

     sprintf(buf, "%s", module->ModuleName);
     window_write_string(wNum, buf, 1, 1, BRITEWHITE | BGBLUE);

     sprintf(buf,
	     "Short Name : %s   Entry : %08X  Size : %08X",
	     module->ModuleShortName,
	     module->entryPoint,
	     module->BaseSize);
     window_write_string(wNum, buf, 2, 1, BRITEWHITE | BGBLUE);

     sprintf(buf,
	     "Range      : %08X-%08X   BaseRVA  : %08X",
	     module->ModuleStart,
	     module->ModuleEnd,
	     module->PEBase);
     window_write_string(wNum, buf, 3, 1, BRITEWHITE | BGBLUE);

     sprintf(buf,
	     "Import Directory : %08X   Export Directory  : %08X",
	     module->ImportDirectory,
	     module->ExportDirectory);
     window_write_string(wNum, buf, 4, 1, BRITEWHITE | BGBLUE);

     sprintf(buf,
	     "Code Segment   : %08X  Code Size   : (%08X) %u bytes",
	     module->CodeSegmentPtr,
	     module->CodeSegmentSize,
	     module->CodeSegmentSize);
     window_write_string(wNum, buf, 5, 1, BRITEWHITE | BGBLUE);

     sprintf(buf,
	     "Data Segment   : %08X  Data Size   : (%08X) %u bytes",
	     module->DataSegmentPtr,
	     module->DataSegmentSize,
	     module->DataSegmentSize);
     window_write_string(wNum, buf, 6, 1, BRITEWHITE | BGBLUE);

     sprintf(buf,
	     "Reloc Segment  : %08X  Reloc Size  : (%08X) %u bytes",
	     module->RelocSegmentPtr,
	     module->RelocSegmentSize,
	     module->RelocSegmentSize);
     window_write_string(wNum, buf, 7, 1, BRITEWHITE | BGBLUE);

     sprintf(buf,
	     "Init Segment   : %08X  Init Size   : (%08X) %u bytes",
	     module->InitSegmentPtr,
	     module->InitSegmentSize,
	     module->InitSegmentSize);
     window_write_string(wNum, buf, 8, 1, BRITEWHITE | BGBLUE);

     sprintf(buf,
	     "Import Segment : %08X  Import Size : (%08X) %u bytes",
	     module->ImportSegmentPtr,
	     module->ImportSegmentSize,
	     module->ImportSegmentSize);
     window_write_string(wNum, buf, 9, 1, BRITEWHITE | BGBLUE);

     sprintf(buf,
	     "Export Segment : %08X  Export Size : (%08X) %u bytes",
	     module->ExportSegmentPtr,
	     module->ExportSegmentSize,
	     module->ExportSegmentSize);
     window_write_string(wNum, buf, 10, 1, BRITEWHITE | BGBLUE);

     sprintf(buf,
	     "Shared Segment : %08X  Shared Size : (%08X) %u bytes",
	     module->SharedSegmentPtr,
	     module->SharedSegmentSize,
	     module->SharedSegmentSize);
     window_write_string(wNum, buf, 11, 1, BRITEWHITE | BGBLUE);

     sprintf(buf,
	     "RPC Segment    : %08X  RPC Size    : (%08X) %u bytes",
	     module->RPCSegmentPtr,
	     module->RPCSegmentSize,
	     module->RPCSegmentSize);
     window_write_string(wNum, buf, 12, 1, BRITEWHITE | BGBLUE);

     sprintf(buf,
	     "Resource Seg   : %08X  Resource Sz : (%08X) %u bytes",
	     module->ResourceSegmentPtr,
	     module->ResourceSegmentSize,
	     module->ResourceSegmentSize);
     window_write_string(wNum, buf, 13, 1, BRITEWHITE | BGBLUE);

     sprintf(buf,
	     "Debug Segment  : %08X  Debug Size  : (%08X) %u bytes",
	     module->DebugSegmentPtr,
	     module->DebugSegmentSize,
	     module->DebugSegmentSize);
     window_write_string(wNum, buf, 14, 1, BRITEWHITE | BGBLUE);

     ReadKeyboard(screen);

     free_window(wNum);

     return 0;

}

void ModuleInfo(SCREEN *screen)
{

    register LONG mNum;
    register LONG i;
    register MODULE_HANDLE *module;

    UpdateStats = 0;

    mNum = make_menu(screen,
		     " Loaded Modules ",
		     2,
		     30,
		     18,
		     1,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     moduleAction,
		     0);

    if (mNum)
    {
       module = moduleListHead;
       for (i=0; i < NumberOfModules && module; i++)
       {
	  add_item_to_menu(mNum, module->ModuleShortName, (LONG) module);
	  module = module->next;
       }
       activate_menu(mNum);
       free_menu(mNum);
    }
    UpdateStats = TRUE;

    return;

}

void LANWANInfo(SCREEN *screen)
{

    register LONG mNum;
    register LONG i;
    register MODULE_HANDLE *module;

    UpdateStats = 0;

    mNum = make_portal(screen,
		       " LAN/WAN Driver Information ",
		       3,
		       1,
		       screen->nLines - 2,
		       screen->nCols - 2,
		       40,
		       1,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0);

    if (mNum)
    {
       activate_portal(mNum);
       free_portal(mNum);
    }

    UpdateStats = TRUE;

    return;

}

void DiskInfo(SCREEN *screen)
{

    register LONG mNum;
    register LONG i;
    register MODULE_HANDLE *module;

    UpdateStats = 0;

    mNum = make_portal(screen,
		       " Disk Device Information ",
		       3,
		       1,
		       screen->nLines - 2,
		       screen->nCols - 2,
		       40,
		       1,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0);

    if (mNum)
    {
       activate_portal(mNum);
       free_portal(mNum);
    }

    UpdateStats = TRUE;

    return;

}

LONG ReadMSRRegister(LONG rNum)
{
   LONG value1, value2;

   ReadMSR(rNum, &value1, &value2);

   return (value1);

}

void WriteMSRRegister(LONG rNum, LONG value1)
{
   LONG value2;

   WriteMSR(rNum, &value1, &value2);

}

LONG ReadClocks(LONG proc)
{

   LONG value;

   EMONClocks[proc] = ReadMSRRegister(PENTIUM_PPII_STDC);
   WriteMSRRegister(PENTIUM_PPII_STDC, 0);

   return (value);

}

void SetEMONRegisters(LONG proc)
{

    register LONG mask, value, emon1, emon2;

    if (DosDataTable->CPU_TYPE == 5)
    {
       emon1 = EMONRegisters[0];
       if (emon1 != (LONG) -1)
       {
	  value = ReadMSRRegister(PENTIUM_EVENT);
	  mask = PentiumEMONCodes[emon1] + EMON_ALL;
	  value = value & 0xFFFFFE00;
	  value = value + mask;
	  WriteMSRRegister(PENTIUM_EVENT, value);
	  WriteMSRRegister(PENTIUM_COUNTER_1, 0);
       }

       emon2 = EMONRegisters[1];
       if (emon2 != (LONG) -1)
       {
	  value = ReadMSRRegister(PENTIUM_EVENT);
	  mask = PentiumEMONCodes[emon2] + EMON_ALL;
	  value = value & 0xFE00FFFF;
	  value = value + (mask << 16);
	  WriteMSRRegister(PENTIUM_EVENT, value);
	  WriteMSRRegister(PENTIUM_COUNTER_2, 0);
       }
       else
	  EMONCounter2[proc] = 0;
    }
    else
    if (DosDataTable->CPU_TYPE == 6)
    {
       emon1 = EMONRegisters[0];
       if (emon1 != (LONG) -1)
       {
	  value = ReadMSRRegister(PPII_EVENT_1);
	  mask = PentiumProEMONCodes[emon1];
	  value &= 0x00420000;
	  value |= (mask | 0x00420000);
	  WriteMSRRegister(PPII_EVENT_1, value);
	  WriteMSRRegister(PPII_COUNTER_1, 0);
       }

       emon2 = EMONRegisters[1];
       if (emon2 != (LONG) -1)
       {
	  value = ReadMSRRegister(PPII_EVENT_2);
	  mask = PentiumProEMONCodes[emon2];
	  value &= 0x00420000;
	  value |= (mask | 0x00420000);
	  WriteMSRRegister(PPII_EVENT_2, value);
	  WriteMSRRegister(PPII_COUNTER_2, 0);
       }
    }

}

void ReadEMONStats(LONG proc)
{
    register LONG mask, value, emon1, emon2;

    if (DosDataTable->CPU_TYPE == 5)
    {
       ReadClocks(proc);
       emon1 = EMONRegisters[0];
       if (emon1 != (LONG) -1)
       {
	  EMONCounter1[proc] = ReadMSRRegister(PENTIUM_COUNTER_1);

	  value = ReadMSRRegister(PENTIUM_EVENT);
	  mask = PentiumEMONCodes[emon1] + EMON_ALL;
	  value = value & 0xFFFFFE00;
	  value = value + mask;
	  WriteMSRRegister(PENTIUM_EVENT, value);
	  WriteMSRRegister(PENTIUM_COUNTER_1, 0);
       }
       else
	  EMONCounter1[proc] = 0;

       emon2 = EMONRegisters[1];
       if (emon2 != (LONG) -1)
       {
	  EMONCounter2[proc] = ReadMSRRegister(PENTIUM_COUNTER_2);

	  value = ReadMSRRegister(PENTIUM_EVENT);
	  mask = PentiumEMONCodes[emon2] + EMON_ALL;
	  value = value & 0xFE00FFFF;
	  value = value + (mask << 16);
	  WriteMSRRegister(PENTIUM_EVENT, value);
	  WriteMSRRegister(PENTIUM_COUNTER_2, 0);
       }
       else
	  EMONCounter2[proc] = 0;
    }
    else
    if (DosDataTable->CPU_TYPE == 6)
    {
       ReadClocks(proc);
       emon1 = EMONRegisters[0];
       if (emon1 != (LONG) -1)
       {
	  EMONCounter1[proc] = ReadMSRRegister(PPII_COUNTER_1);

	  value = ReadMSRRegister(PPII_EVENT_1);
	  mask = PentiumProEMONCodes[emon1];
	  value &= 0x00420000;
	  value |= (mask | 0x00420000);
	  WriteMSRRegister(PPII_EVENT_1, value);
	  WriteMSRRegister(PPII_COUNTER_1, 0);
       }
       else
	  EMONCounter1[proc] = 0;

       emon2 = EMONRegisters[1];
       if (emon2 != (LONG) -1)
       {
	  EMONCounter2[proc] = ReadMSRRegister(PPII_COUNTER_2);

	  value = ReadMSRRegister(PPII_EVENT_2);
	  mask = PentiumProEMONCodes[emon2];
	  value &= 0x00420000;
	  value |= (mask | 0x00420000);
	  WriteMSRRegister(PPII_EVENT_2, value);
	  WriteMSRRegister(PPII_COUNTER_2, 0);
       }
       else
	  EMONCounter2[proc] = 0;
    }

}

void EMONUpdate(LONG wNum)
{

    BYTE displayBuffer[100] = { "" };
    register LONG i, count1, count2, emon1, emon2;
    register LONG x, y;

    EMONUpdateOK = 1;

    while (1)
    {
       for (x = 0, y = 2, i=0; i < MAX_PROCESSORS; i++, x += 6)
       {
	  sprintf(displayBuffer,
		   " Processor %u",
		   processor_table[i].ProcessorNumber);
	  write_portal(wNum, displayBuffer, x + 1, y, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer,
		   " %s ",
		   engineState[processor_table[i].ProcessorState & 0xF]);
	  write_portal(wNum, displayBuffer, x + 1, y + 43, BRITEWHITE | BGBLUE);

	  emon1 = EMONRegisters[0];
	  emon2 = EMONRegisters[1];
	  count1 = EMONCounter1[i];
	  count2 = EMONCounter2[i];

	  if (DosDataTable->CPU_TYPE == 5)
	  {
	     sprintf(displayBuffer,
		   " %s ",
		   emon1 == (LONG) -1
		   ? "Counter Not Selected                     "
		   : PentiumEMON[emon1]);
	     write_portal(wNum, displayBuffer, x + 2, y, BRITEWHITE | BGBLUE);

	     sprintf_comma(displayBuffer,
		   " %u                  ",
		   count1);
	     write_portal(wNum, displayBuffer, x + 2, y + 43, BRITEWHITE | BGBLUE);

	     sprintf(displayBuffer,
		   " %s ",
		   emon2 == (LONG) -1
		   ? "Counter Not Selected                     "
		   : PentiumEMON[emon2]);
	     write_portal(wNum, displayBuffer, x + 3, y, BRITEWHITE | BGBLUE);

	     sprintf_comma(displayBuffer,
		   " %u                  ",
		   count2);
	     write_portal(wNum, displayBuffer, x + 3, y + 43, BRITEWHITE | BGBLUE);

	     sprintf(displayBuffer, " Total Clock Cycles ");
	     write_portal(wNum, displayBuffer, x + 4, y, BRITEWHITE | BGBLUE);

	     sprintf_comma(displayBuffer, " %u                 ", EMONClocks[i]);
	     write_portal(wNum, displayBuffer, x + 4, y + 43, BRITEWHITE | BGBLUE);

	  }
	  else
	  if (DosDataTable->CPU_TYPE == 6)
	  {
	     sprintf(displayBuffer,
		   " %s ",
		   emon1 == (LONG) -1
		   ? "Counter Not Selected                     "
		   : PentiumProEMON[emon1]);
	     write_portal(wNum, displayBuffer, x + 2, y, BRITEWHITE | BGBLUE);

	     sprintf_comma(displayBuffer,
		   " %u                  ",
		   count1);
	     write_portal(wNum, displayBuffer, x + 2, y + 43, BRITEWHITE | BGBLUE);

	     sprintf(displayBuffer,
		   " %s ",
		   emon2 == (LONG) -1
		   ? "Counter Not Selected                     "
		   : PentiumProEMON[emon2]);
	     write_portal(wNum, displayBuffer, x + 3, y, BRITEWHITE | BGBLUE);

	     sprintf_comma(displayBuffer,
		   " %u                  ",
		   count2);
	     write_portal(wNum, displayBuffer, x + 3, y + 43, BRITEWHITE | BGBLUE);

	     sprintf(displayBuffer, " Total Clock Cycles ");
	     write_portal(wNum, displayBuffer, x + 4, y, BRITEWHITE | BGBLUE);

	     sprintf_comma(displayBuffer, " %u                 ", EMONClocks[i]);
	     write_portal(wNum, displayBuffer, x + 4, y + 43, BRITEWHITE | BGBLUE);

	  }

	  if (processor_table[i].ProcessorState == PROCESSOR_ACTIVE)
	  {
	     EMONLWP[i].procedureAddress = (LONG (*)(void *)) ReadEMONStats;
	     EMONLWP[i].lwpArgument = (void *) i;
	     scheduleBoundLWP(&EMONLWP[i], i);
	  }
       }

       if (EMONUpdateOK)
	  update_portal(wNum);

       delayThread(18);

    }

}

LONG EMONKeys(SCREEN *screen, LONG key)
{

     EMONUpdateOK = 0;

     switch (key & 0xFF)
     {
	case F2:
	   if (DosDataTable->CPU_TYPE == 5)
	   {
	      register LONG mNum;
	      register LONG i;
	      register MODULE_HANDLE *module;

	      mNum = make_menu(screen,
			       " P5 (Pentium) EMON Stats ",
			       2,
			       20,
			       18,
			       1,
			       YELLOW | BGBLUE,
			       YELLOW | BGBLUE,
			       BRITEWHITE | BGBLUE,
			       BRITEWHITE | BGBLUE,
			       0,
			       0);

	      if (mNum)
	      {
		 register LONG choice;

		 for (i=0; i < (sizeof(PentiumEMON) / sizeof(LONG)); i++)
		 {
		    if (i != EMONRegisters[0] && i != EMONRegisters[1])
		       add_item_to_menu(mNum, PentiumEMON[i], i);
		 }

		 choice = activate_menu(mNum);
		 if (choice != (LONG) -1 && choice != EMONRegisters[0] &&
		     choice != EMONRegisters[1])
		 {
		    if (EMONRegisters[0] == (LONG) -1)
		       EMONRegisters[0] = choice;
		    else
		    if (EMONRegisters[1] == (LONG) -1)
		       EMONRegisters[1] = choice;

		 }
		 free_menu(mNum);
	      }
	      break;
	   }
	   else
	   if (DosDataTable->CPU_TYPE == 6)
	   {
	      register LONG mNum;
	      register LONG i;
	      register MODULE_HANDLE *module;

	      mNum = make_menu(screen,
			       " P6 (Pentium Pro/II) EMON Stats ",
			       2,
			       16,
			       18,
			       1,
			       YELLOW | BGBLUE,
			       YELLOW | BGBLUE,
			       BRITEWHITE | BGBLUE,
			       BRITEWHITE | BGBLUE,
			       0,
			       0);

	      if (mNum)
	      {
		 register LONG choice;

		 for (i=0; i < (sizeof(PentiumProEMON) / sizeof(LONG)); i++)
		 {
		    if (i != EMONRegisters[0] && i != EMONRegisters[1])
		       add_item_to_menu(mNum, PentiumProEMON[i], i);
		 }

		 choice = activate_menu(mNum);
		 if (choice != (LONG) -1 && choice != EMONRegisters[0] &&
		     choice != EMONRegisters[1])
		 {
		    // some of the floating point stats are specific
		    // to a particular counter.  Four those cases,
		    // adjust the counter assignment

		    switch (choice)
		    {
		       case 8:  // case 8,9 must run on EventSelect1
		       case 9:
			  if (EMONRegisters[0] == (LONG) -1 || EMONRegisters[1] == (LONG) -1)
			  {
			     if (EMONRegisters[1] != (LONG) -1)
			     {
				if (EMONRegisters[1] == 8 || EMONRegisters[1] == 9)
				   break;

				EMONRegisters[0] = EMONRegisters[1];
				EMONRegisters[1] = choice;
			     }
			     else
			     {
				EMONRegisters[1] = choice;
			     }
			  }
			  break;

		       case 6:   // case 6, 10, 79 must run on EventSelect2
		       case 10:
		       case 79:
			  if (EMONRegisters[0] == (LONG) -1 || EMONRegisters[1] == (LONG) -1)
			  {
			     if (EMONRegisters[0] != (LONG) -1)
			     {
				if (EMONRegisters[0] == 6 || EMONRegisters[0] == 10 ||
				    EMONRegisters[0] == 79)
				   break;

				EMONRegisters[1] = EMONRegisters[0];
				EMONRegisters[0] = choice;
			     }
			     else
			     {
				EMONRegisters[0] = choice;
			     }
			  }
			  break;

		       default:
			  if (EMONRegisters[0] == (LONG) -1)
			     EMONRegisters[0] = choice;
			  else
			  if (EMONRegisters[1] == (LONG) -1)
			     EMONRegisters[1] = choice;
			  break;
		    }

		 }

		 free_menu(mNum);
	      }
	      break;
	   }
	   break;

	case F3:
	   if (DosDataTable->CPU_TYPE == 5)
	   {
	      register LONG mNum;
	      register LONG i;
	      register MODULE_HANDLE *module;

	      mNum = make_menu(screen,
			       " De-Select EMON Statistic (P5) ",
			       2,
			       20,
			       18,
			       1,
			       YELLOW | BGBLUE,
			       YELLOW | BGBLUE,
			       BRITEWHITE | BGBLUE,
			       BRITEWHITE | BGBLUE,
			       0,
			       0);

	      if (mNum)
	      {
		 register LONG choice, curr1, curr2;

		 curr1 = EMONRegisters[0];
		 if (curr1 != (LONG) -1)
		    add_item_to_menu(mNum, PentiumEMON[curr1], curr1);

		 curr2 = EMONRegisters[1];
		 if (curr2 != (LONG) -1)
		    add_item_to_menu(mNum, PentiumEMON[curr2], curr2);

		 choice = activate_menu(mNum);
		 if (EMONRegisters[0] == choice)
		    EMONRegisters[0] = -1;
		 else
		 if (EMONRegisters[1] == choice)
		    EMONRegisters[1] = -1;

		 free_menu(mNum);
	      }
	      break;
	   }
	   else
	   if (DosDataTable->CPU_TYPE == 6)
	   {
	      register LONG mNum;
	      register LONG i;
	      register MODULE_HANDLE *module;

	      mNum = make_menu(screen,
			       " De-Select EMON Statistic (P6/II) ",
			       2,
			       16,
			       18,
			       1,
			       YELLOW | BGBLUE,
			       YELLOW | BGBLUE,
			       BRITEWHITE | BGBLUE,
			       BRITEWHITE | BGBLUE,
			       0,
			       0);

	      if (mNum)
	      {
		 register LONG choice, curr1, curr2;

		 curr1 = EMONRegisters[0];
		 if (curr1 != (LONG) -1)
		    add_item_to_menu(mNum, PentiumProEMON[curr1], curr1);

		 curr2 = EMONRegisters[1];
		 if (curr2 != (LONG) -1)
		    add_item_to_menu(mNum, PentiumProEMON[curr2], curr2);

		 choice = activate_menu(mNum);
		 if (EMONRegisters[0] == choice)
		    EMONRegisters[0] = -1;
		 else
		 if (EMONRegisters[1] == choice)
		    EMONRegisters[1] = -1;

		 free_menu(mNum);
	      }
	      break;
	   }
	   break;

	default:
	   break;
     }

     EMONUpdateOK = 1;

     return 0;

}


void EMONInfo(SCREEN *screen)
{

    register LONG i, key;
    register PROCESS *p;
    register LONG wNum;

    UpdateStats = 0;

    wNum = make_portal(screen,
		       " Processor Performance Counters - <F2> add <F3> delete ",
		       3,
		       0,
		       screen->nLines - 2,
		       screen->nCols - 1,
		       50,
		       1,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       EMONKeys);

    if (wNum)
    {
       p = createThread("EMON Update", (LONG (*)()) EMONUpdate,
			     8192 * 2, (void *) wNum, 0);
       if (p)
       {
	  activate_portal(wNum);
	  killThread(p);
	  free_portal(wNum);
       }
    }

    UpdateStats = TRUE;

    return;

}

void KernelUpdate(LONG wNum)
{

    register PROCESS *p;
    register LONG flags, i, maxSize = 0, j;
    BYTE displayBuffer[100] = { "" };
    BYTE spaceBuffer[100] = { "" };

    SetDataB((LONG *) spaceBuffer, 0x20, 80);
    spaceBuffer[78] = '\0';

    while (1)
    {
       for (j = 0; j < maxSize; j++)
	  write_portal(wNum, spaceBuffer, j, 0, BRITEWHITE | BGBLUE);
       maxSize = 0;

       flags = get_flags();
       spin_lock(&systemThreadmutex);

       p = systemThreadHead;
       i = 2;
       while (p)
       {
	  write_portal(wNum, " PID       State      Proc  Elg     Name                              Util",
			      0, 1, YELLOW | BGBLUE);
	  write_portal(wNum, lDelim, 1, 2, YELLOW | BGBLUE);

	  sprintf(displayBuffer, " %08X  %s  p%02d  %s  %s",
		  p,
		  kernelStateArray[p->threadState & 0xF],
		  p->lastProcessor,
		  (p == processorSet[p->lastProcessor].running_process)
		  ? "RUNNING"
		  : "IDLE   ",
		  p->processName,
		  0, 0);

	  write_portal(wNum, displayBuffer, i, 1, BRITEWHITE | BGBLUE);

	  sprintf(displayBuffer, "%3d.%02d%%",
		  p->threadUtilization,
		  p->threadSubUtil);
	  write_portal(wNum, displayBuffer, i, 69, BRITEWHITE | BGBLUE);

	  if (i++ > 100)
	     break;

	  p = p->kernelNext;
       }
       spin_unlock(&systemThreadmutex);
       set_flags(flags);

       if (i > maxSize)
	  maxSize = i;

       update_portal(wNum);
       delayThread(18);

    }

}

void KernelProcess(SCREEN *screen)
{

    register LONG wNum;
    register PROCESS *child;

    UpdateStats = 0;


    wNum = make_portal(screen,
		       " Kernel Information ",
		       3,
		       0,
		       screen->nLines - 1,
		       screen->nCols - 1,
		       100,
		       1,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0);

    if (wNum)
    {
       child = createThread("Kernel Process Update", (LONG (*)()) KernelUpdate,
			     8192 * 2, (void *) wNum, 0);
       if (child)
       {
	  activate_portal(wNum);
	  killThread(child);
	  free_portal(wNum);
       }
    }


    UpdateStats = TRUE;

    return;

}

LONG warnAction(SCREEN *screen)
{

    register LONG mNum, retCode;

    mNum = make_menu(screen,
		     " Exit Monitor ",
		     14,
		     32,
		     2,
		     1,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     0,
		     0);

    add_item_to_menu(mNum, "Yes", 1);
    add_item_to_menu(mNum, "No", 0);

    retCode = activate_menu(mNum);
    if (retCode == (LONG) -1)
       retCode = 0;

    free_menu(mNum);

    return retCode;


}

LONG menuAction(SCREEN *screen, LONG value, BYTE *option)
{

   if (option) {};

   switch (value)
   {
      case 1:
	 ProcessorPortalProcess(screen);
	 break;

      case 2:
	 KernelProcess(screen);
	 break;

      case 3:
	 ModuleInfo(screen);
	 break;

      case 4:
	 LANWANInfo(screen);
	 break;

      case 5:
	 DiskInfo(screen);
	 break;

      case 6:
	 DiskInfo(screen);
	 break;

      case 7:
	 VMPortalProcess(screen);
	 break;

      case 8:
	 DiskInfo(screen);
	 break;

      case 9:
	 DiskInfo(screen);
	 break;

      case 10:
	 DiskInfo(screen);
	 break;

      case 11:
	 DiskInfo(screen);
	 break;

      case 12:
	 EMONInfo(screen);
	 break;

      default:
	 break;
   }
   return value;
}

void MonitorProcess(void)
{

    register PROCESS *child = 0, *time = 0;
    register LONG i, key, retCode;
    register LONG mNum = 0, wNum;
    BYTE displayBuffer[100] = { "" };

    if (MonitorActive)
       return;

    monitorScreen = createScreen("System Monitor");
    if (!monitorScreen)
       return;

    MonitorActive = TRUE;
    SetVideoOwner(monitorScreen);

    while (MonitorActive)
    {
       for (i=0; i < monitorScreen->nLines - 1; i++)
	  PutVidCharCleol(monitorScreen, 0xB0, i, CYAN | BGBLUE);

       sprintf(displayBuffer, "  MANOS System Monitor v1.0");
       PutVidStringCleol(monitorScreen, displayBuffer, 0, BLUE | BGCYAN);

       sprintf(displayBuffer, "  Server Name: %s", ServerName);
       PutVidStringCleol(monitorScreen, displayBuffer, 1, BLUE | BGCYAN);

       sprintf(displayBuffer, "  Metropolitan Area Network Operating System v1.00.10");
       PutVidStringCleol(monitorScreen, displayBuffer, 2, BLUE | BGCYAN);

       sprintf(displayBuffer, " ALT-ESC: Switch Screens");
       write_screen_comment_line(monitorScreen, displayBuffer, BLUE | BGWHITE);

       DisableCursor(monitorScreen);

       UpdateStats = TRUE;

       wNum = make_portal(monitorScreen,
		       " General Information ",
		       3,
		       0,
		       13,
		       monitorScreen->nCols - 1,
		       8,
		       1,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       0);
       if (!wNum)
	  goto ErrorExit;

       child = createThread("Console Update", (LONG (*)()) ConsoleUpdate, 8192 * 2,
			 (void *) wNum, 0);
       if (!child)
	  goto ErrorExit;

       activate_static_portal(wNum);

       time = createThread("Time Process", (LONG (*)()) UpdateTime, 8192 * 2, monitorScreen, 0);
       if (!time)
	  goto ErrorExit;

       mNum = make_menu(monitorScreen,
		     " Available Options ",
		     14,
		     25,
		     6,
		     1,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     menuAction,
		     warnAction);

       if (!mNum)
	  goto ErrorExit;

       add_item_to_menu(mNum, "Multi-Processor Information", 1);
       add_item_to_menu(mNum, "SMP Kernel Information", 2);
       add_item_to_menu(mNum, "System Module Information", 3);
       add_item_to_menu(mNum, "LAN/WAN Information", 4);
       add_item_to_menu(mNum, "Disk Device Information", 5);
       add_item_to_menu(mNum, "Protocol Information", 6);
       add_item_to_menu(mNum, "Virtual Memory Information", 7);
       add_item_to_menu(mNum, "Connection Information ", 8);
       add_item_to_menu(mNum, "Cache Information ", 9);
       add_item_to_menu(mNum, "Resource Tracking Information ", 10);
       add_item_to_menu(mNum, "File System Information", 11);

       if (DosDataTable->CPU_TYPE >= 5)
	  add_item_to_menu(mNum, "Processor Performance Stats", 12);

       retCode = activate_menu(mNum);
       if (retCode) {};

ErrorExit:;
       sprintf(displayBuffer, " Exiting ... ");
       PutVidStringCleol(monitorScreen, displayBuffer,
		      monitorScreen->nLines - 1, BLUE | BGWHITE);

       if (child)
	  killThread(child);

       if (time)
	  killThread(time);

       if (wNum)
       {
	  deactivate_static_portal(wNum);
	  free_portal(wNum);
       }

       if (mNum)
	  free_menu(mNum);

       MonitorActive = 0;

    }
    deleteScreen(monitorScreen);

    return;

}





