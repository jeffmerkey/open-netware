
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
*   FILE     :  HAL.H
*   DESCRIP  :  Hardware Abstraction Layer for MANOS v1.0
*   DATE     :  December 10, 1997
*
*
***************************************************************************/

#include "types.h"
#include "version.h"

#define PROCESSOR_INACTIVE      0
#define PROCESSOR_ACTIVE        1
#define PROCESSOR_DEBUG         2
#define PROCESSOR_SIGNAL        3
#define PROCESSOR_FATAL         4
#define PROCESSOR_INIT          5
#define PROCESSOR_SHUTDOWN      6
#define PROCESSOR_RESUME        7
#define PROCESSOR_LOCK          8
#define PROCESSOR_CURRENT       9
#define PROCESSOR_SUSPEND       10

#define PIC_MODE         0
#define APIC_MODE        1
#define CBUS_MODE        2
#define COMPAQ_MODE      3

typedef struct _HARDWARE_ABSTRACTION {
   LONG ProcessorNumber;
   LONG ProcessorState;
   LONG DebugSignal;
   LONG ShutdownSignal;

   //  HAL Table of Functions provided by HAL Modules

   LONG (*SetSpl)();
   LONG (*GetSpl)();
   LONG (*GetCPUID)(void);
   LONG (*InitializeProcessorHardware)(LONG);
   LONG (*ShutdownProcessorHardware)(LONG);
   LONG (*OnlineProcessor)(LONG, LONG);
   LONG (*OfflineProcessor)();
   LONG (*OfflineSelf)();
   LONG (*SetInterrupt)(LONG, LONG, LONG, LONG);
   LONG (*ClearInterrupt)(LONG);
   LONG (*MaskInterrupt)(LONG);
   LONG (*UnmaskInterrupt)(LONG);
   LONG (*StartTimer)(void);
   LONG (*StopTimer)(void);
   LONG (*MaskTimer)(void);
   LONG (*UnmaskTimer)(void);
   LONG (*MaskInterruptSubsystem)(void);
   LONG (*UnmaskInterruptSubsystem)(void);
   LONG (*EndOfInterrupt)(LONG);
   LONG (*DirectedNMI)(LONG);
   LONG (*XCall)();

   // paging system entry

   LONG (*SetPageRegister)();
   LONG (*GetPageRegister)();
   LONG (*PageFaultEntry)();
   LONG (*InvalidatePage)();
   LONG (*InitializeMMU)();

   // atomic functions

   LONG (*spin_lock)();
   LONG (*spin_unlock)();
   LONG (*spin_try_lock)();

   // real mode functions

   LONG (*ExitSystem)();
   LONG (*RealModeInt)();
   LONG (*ProtectedModeInterrupt)();

   LONG xcall_mutex;
   LONG xcall_command;
   LONG xcall_arg1;
   LONG xcall_arg2;
   LONG xcall_arg3;
   LONG xcall_total;
   LONG xcall_fail;
   LONG xcall_busy;
   LONG xcall_ack;

   LONG debug_command;

   LONG totalInterrupts;
   LONG TLBShootdowns;
   LONG lockAssertions;
   LONG pageFaults;

   LONG HalReserved[100];

} HAL;

extern LONG InitializeHAL(void);
extern LONG InitializeSMPHAL(void);
extern void InitializePaging(LONG Processor);

extern HAL processor_table[MAX_PROCESSORS];

extern LONG systemType;

