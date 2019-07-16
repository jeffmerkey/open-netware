
;***************************************************************************
;
;   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
;                            Reserved.
;
;   AUTHOR   :  Jeff V. Merkey
;   FILE     :  HAL.INC
;   DESCRIP  :  HAL Include for MANOS v1.0
;   DATE     :  March 12, 1997
;
;
;***************************************************************************

PROCESSOR_INACTIVE      EQU    0
PROCESSOR_ACTIVE        EQU    1
PROCESSOR_DEBUG         EQU    2
PROCESSOR_SIGNAL        EQU    3
PROCESSOR_FATAL         EQU    4
PROCESSOR_INIT          EQU    5
PROCESSOR_SHUTDOWN      EQU    6
PROCESSOR_RESUME        EQU    7

PIC_MODE                EQU    0
APIC_MODE               EQU    1
CBUS_MODE               EQU    2
COMPAQ_MODE             EQU    3

HAL struc
   ProcessorNumber             dd    ?
   ProcessorState              dd    ?
   DebugSignal                 dd    ?
   ShutdownSignal              dd    ?

   ;  HAL Table of Functions provided by HAL Modules

   SetSpl                      dd    ?
   GetSpl                      dd    ?
   GetCPUID                    dd    ?
   InitializeProcessorHardware dd    ?
   ShutdownProcessorHardware   dd    ?
   OnlineProcessor             dd    ?
   OfflineProcessor            dd    ?
   OfflineSelf                 dd    ?
   SetInterrupt                dd    ?
   ClearInterrupt              dd    ?
   MaskInterrupt               dd    ?
   UnmaskInterrupt             dd    ?
   StartTimer                  dd    ?
   StopTimer                   dd    ?
   MaskTimer                   dd    ?
   UnmaskTimer                 dd    ?
   MaskInterruptSubsystem      dd    ?
   UnmaskInterruptSubsystem    dd    ?
   EndOfInterrupt              dd    ?
   DirectedNMI                 dd    ?
   XCall                       dd    ?

   ; paging system entry

   SetPageRegister             dd    ?
   GetPageRegister             dd    ?
   PageFaultEntry              dd    ?
   InvalidatePage              dd    ?
   InitializeMMU               dd    ?

   ; atomic functions

   pspin_lock                  dd    ?
   pspin_unlock                dd    ?
   pspin_try_lock              dd    ?

   ; real mode fuynctions

   ExitSystem                  dd     ?
   RealModeInt                 dd     ?
   ProtectedModeInterrupt      dd     ?

   xcall_mutex                 dd     ?
   xcall_command               dd     ?
   xcall_arg1                  dd     ?
   xcall_arg2                  dd     ?
   xcall_arg3                  dd     ?
   xcall_total		       dd     ?
   xcall_fail                  dd     ?
   xcall_busy                  dd     ?
   xcall_ack                   dd     ?

   debug_command               dd     ?

   totalInterrupts             dd     ?
   TLBShootdowns               dd     ?
   lockAssertions              dd     ?
   pageFaults                  dd     ?

   HalReserved                 dd     100 dup (?)

HAL  ends

