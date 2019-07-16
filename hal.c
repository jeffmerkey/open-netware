
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
*   FILE     :  HAL.C
*   DESCRIP  :  Hardware Abstraction Layer for MANOS v1.0
*   DATE     :  December 10, 1997
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
#include "hal.h"
#include "mps.h"
#include "os.h"

HAL processor_table[MAX_PROCESSORS];
HAL *pindex[MAX_PROCESSORS];

extern LONG ReadCR3(void);
extern void LoadPDE(LONG);

void std_ret(void) { return; };
LONG null_ret(void) { return 0; };
LONG true_ret(void) { return 1; };
LONG unsupported_ret(void) { return -1; };
LONG SMPInitialized = 0;

LONG InitializeHAL(void)
{

     LONG systemType;
     register int i;

     //
     //   system default is PIC mode
     //

     systemType = PIC_MODE;
     num_procs = 1;
     for (i=0; i < MAX_PROCESSORS; i++)
     {
	SetData((LONG *) &processor_table[i], 0, sizeof(HAL));
	pindex[i] = (HAL *) &processor_table[i];
	processor_table[i].ProcessorNumber = i;
	if (!i)
	   processor_table[i].ProcessorState = PROCESSOR_ACTIVE;
	else
	   processor_table[i].ProcessorState = PROCESSOR_INACTIVE;
	processor_table[i].DebugSignal = 0;
	processor_table[i].SetSpl = (LONG (*)()) null_ret;
	processor_table[i].GetSpl =  (LONG (*)()) null_ret;
	processor_table[i].GetCPUID =  (LONG (*)(void)) null_ret;
	processor_table[i].InitializeProcessorHardware =  (LONG (*)(LONG)) null_ret;
	processor_table[i].ShutdownProcessorHardware =  (LONG (*)(LONG)) null_ret;
	processor_table[i].OnlineProcessor =  (LONG (*)(LONG, LONG)) null_ret;
	processor_table[i].OfflineProcessor =  (LONG (*)()) null_ret;
	processor_table[i].OfflineSelf =  (LONG (*)()) null_ret;
	processor_table[i].SetInterrupt =  (LONG (*)()) pic_set_int;
	processor_table[i].ClearInterrupt =  (LONG (*)()) pic_clear_int;
	processor_table[i].MaskInterrupt =  (LONG (*)()) mask_pic;
	processor_table[i].UnmaskInterrupt =  (LONG (*)()) unmask_pic;
	processor_table[i].MaskInterruptSubsystem =  (LONG (*)()) mask_system;
	processor_table[i].StartTimer =  (LONG (*)()) null_ret;
	processor_table[i].StopTimer =  (LONG (*)()) null_ret;
	processor_table[i].MaskTimer =  (LONG (*)()) mask_pic_timer;
	processor_table[i].UnmaskTimer =  (LONG (*)()) unmask_pic_timer;
	processor_table[i].UnmaskInterruptSubsystem =  (LONG (*)()) unmask_system;
	processor_table[i].EndOfInterrupt =  (LONG (*)(LONG)) pic_eoi;
	processor_table[i].DirectedNMI =  (LONG (*)()) null_ret;
	processor_table[i].XCall =  (LONG (*)()) null_ret;
	processor_table[i].SetPageRegister =  (LONG (*)()) LoadPDE;
	processor_table[i].GetPageRegister =  (LONG (*)()) ReadCR3;
	processor_table[i].PageFaultEntry =  (LONG (*)()) null_ret;
	processor_table[i].InvalidatePage =  (LONG (*)()) null_ret;
	processor_table[i].spin_lock =  (LONG (*)(LONG *)) spin_lock;
	processor_table[i].spin_unlock =  (LONG (*)(LONG *)) spin_unlock;
	processor_table[i].spin_try_lock =  (LONG (*)(LONG *)) spin_try_lock;
	processor_table[i].ExitSystem = (LONG (*)()) ExitOS;
	processor_table[i].RealModeInt = (LONG (*)()) null_ret;
	processor_table[i].ProtectedModeInterrupt = (LONG (*)()) null_ret;
	processor_table[i].xcall_mutex = 0;
	processor_table[i].xcall_command = 0;
	processor_table[i].xcall_arg1 = 0;
	processor_table[i].xcall_arg2 = 0;
	processor_table[i].xcall_arg3 = 0;
     }

     return systemType;

}

LONG InitializeSMPHAL(void)
{

     LONG retCode;
     register int i;

     retCode = MPSDetect();
     if (retCode)
     {
	//
	//  MPS detected, use APIC_MODE
	//

	systemType = APIC_MODE;
	for (i=0; i < MAX_PROCESSORS; i++)
	{
	   processor_table[i].DebugSignal = 0;
	   processor_table[i].SetSpl = (LONG (*)()) unsupported_ret;
	   processor_table[i].GetSpl =  (LONG (*)()) unsupported_ret;
	   processor_table[i].GetCPUID =  (LONG (*)(void)) get_apic_id;
	   processor_table[i].InitializeProcessorHardware =  (LONG (*)(LONG)) apic_init;
	   processor_table[i].ShutdownProcessorHardware =  (LONG (*)(LONG)) apic_close;
	   processor_table[i].OnlineProcessor =  (LONG (*)(LONG, LONG)) apic_activate_processor;
	   processor_table[i].OfflineProcessor =  (LONG (*)()) true_ret;
	   processor_table[i].OfflineSelf =  (LONG (*)()) apic_close;
	   processor_table[i].SetInterrupt =  (LONG (*)()) apic_set_int;
	   processor_table[i].ClearInterrupt =  (LONG (*)()) apic_clear_int;
	   processor_table[i].MaskInterrupt =  (LONG (*)()) apic_mask_int;
	   processor_table[i].UnmaskInterrupt =  (LONG (*)()) apic_unmask_int;
	   processor_table[i].StartTimer =  (LONG (*)()) apic_timer_start;
	   processor_table[i].StopTimer =  (LONG (*)()) apic_timer_stop;
	   processor_table[i].MaskTimer =  (LONG (*)()) apic_mask_timer;
	   processor_table[i].UnmaskTimer =  (LONG (*)()) apic_unmask_timer;
	   processor_table[i].MaskInterruptSubsystem =  (LONG (*)()) disable_ioapic_ints;
	   processor_table[i].UnmaskInterruptSubsystem =  (LONG (*)()) enable_ioapic_ints;
	   processor_table[i].EndOfInterrupt =  (LONG (*)(LONG)) apic_eoi;
	   processor_table[i].DirectedNMI =  (LONG (*)(LONG)) apic_directed_nmi;
	   processor_table[i].XCall =  (LONG (*)()) apic_xcall;
	   processor_table[i].SetPageRegister =  (LONG (*)()) LoadPDE;
	   processor_table[i].GetPageRegister =  (LONG (*)()) ReadCR3;
	   processor_table[i].PageFaultEntry =  (LONG (*)()) null_ret;
	   processor_table[i].InvalidatePage =  (LONG (*)()) null_ret;
	   processor_table[i].spin_lock =  (LONG (*)(LONG *)) spin_lock;
	   processor_table[i].spin_unlock =  (LONG (*)(LONG *)) spin_unlock;
	   processor_table[i].spin_try_lock =  (LONG (*)(LONG *)) spin_try_lock;
	   processor_table[i].ExitSystem = (LONG (*)()) ExitOS;
	   processor_table[i].RealModeInt = (LONG (*)()) null_ret;
	   processor_table[i].ProtectedModeInterrupt = (LONG (*)()) null_ret;
	   processor_table[i].xcall_mutex = 0;
	   processor_table[i].xcall_command = 0;
	   processor_table[i].xcall_arg1 = 0;
	   processor_table[i].xcall_arg2 = 0;
	   processor_table[i].xcall_arg3 = 0;
	}
     }
     return retCode;

}

void InitializeSMP(void)
{
    register LONG flags;

    flags = get_flags();
    (processor_table[0].InitializeProcessorHardware)(0);
    set_flags(flags);

    SMPInitialized = 1;

    return;
}

void ShutdownSMP(void)
{

    register LONG flags;

    flags = get_flags();
    (processor_table[0].ShutdownProcessorHardware)(0);
    set_flags(flags);

    return;

}


void ExitOSShutdownSMP(void)
{
    if (SMPInitialized)
       ShutdownSMP();
}