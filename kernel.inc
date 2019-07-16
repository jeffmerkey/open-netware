
;***************************************************************************
;
;   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
;                           Reserved.
;
;   This program is an unpublished work of TRG, Inc. and contains
;   trade secrets and other proprietary information.  Unauthorized
;   use, copying, disclosure, or distribution of this file without the
;   consent of TRG, Inc. can subject the offender to severe criminal
;   and/or civil penalities.
;
;
;   AUTHOR   :  Jeff V. Merkey
;   FILE     :  KERNEL.INC
;   DESCRIP  :  Kernel equates and structures for MANOS v1.0
;   DATE     :  November 29, 1997
;
;
;**************************************************************************/

include version.inc

LWP_INACTIVE    EQU   0
LWP_ACTIVE      EQU   1

PS_HALT         EQU   0
PS_ACTIVE       EQU   1
PS_SLEEP        EQU   2
PS_SYNC         EQU   3

if (IA32_OPCODE)

;  124 bytes total size numeric register context

NUMERIC_REGISTER_CONTEXT struc
  sig0      dw  ?
  sig1      dw  ?
  sig2      dw  ?
  sig3      dw  ?
  exp_sign  dw  ?
NUMERIC_REGISTER_CONTEXT ends

NUMERIC_FRAME struc

  control   dd   ?
  status    dd   ?
  tag       dd   ?
  eip       dd   ?
  cs        dd   ?
  dataptr   dd   ?
  datasel   dd   ?

  num1      NUMERIC_REGISTER_CONTEXT  <?>
  num2      NUMERIC_REGISTER_CONTEXT  <?>
  num3      NUMERIC_REGISTER_CONTEXT  <?>
  num4      NUMERIC_REGISTER_CONTEXT  <?>
  num5      NUMERIC_REGISTER_CONTEXT  <?>
  num6      NUMERIC_REGISTER_CONTEXT  <?>
  num7      NUMERIC_REGISTER_CONTEXT  <?>
  num8      NUMERIC_REGISTER_CONTEXT  <?>

  pad       dd   5 dup (?)

NUMERIC_FRAME ends

;  104 bytes total size register context

CONTEXT_FRAME struc
    cBackLink          dw     ?
    cTSSReserved       dw     ?
    cESP0              dd     ?
    cSS0               dw     ?
    cSS0res            dw     ?
    cESP1              dd     ?
    cSS1               dw     ?
    cSS1res            dw     ?
    cESP2              dd     ?
    cSS2               dw     ?
    cSS2res            dw     ?
    cCR3               dd     ?
    cEIP 	       dd     ?
    cSystemFlags       dd     ?
    cEAX       	       dd     ?
    cECX               dd     ?
    cEDX 	       dd     ?
    cEBX     	       dd     ?
    cESP     	       dd     ?
    cEBP       	       dd     ?
    cESI    	       dd     ?
    cEDI     	       dd     ?
    cES                dw     2 dup (?)
    cCS                dw     2 dup (?)
    cSS                dw     2 dup (?)
    cDS        	       dw     2 dup (?)
    cFS   	       dw     2 dup (?)
    cGS       	       dw     2 dup (?)
    cLDT       	       dw     2 dup (?)
    cIOPermissMap      dw     2 dup (?)
    cPad               dd     6 dup (?)
CONTEXT_FRAME ends

endif

SEMAPHORE struc
   SEMA_next         dd   ?
   SEMA_prior        dd   ?
   SEMA_kernelNext   dd   ?
   SEMA_kernelPrior  dd   ?
   sema_value        dd   ?
   sema_head         dd   ?
   sema_tail         dd   ?
   sema_mutex        dd   ?
   sema_waiters      dd   ?
   sema_signature    dd   ?
   sema_timerID      dd   ?
SEMAPHORE ends

PROCESS_CONTROL struc

   ps_next              dd  ?
   ps_prior             dd  ?
   kernelNext           dd  ?
   kernelPrior          dd  ?
   syncNext             dd  ?
   syncPrior            dd  ?

   processID            dd  ?
   typeID               dd  ?

   stackPointer         dd  ?
   stackLimit           dd  ?
   stackEnd             dd  ?
   processSignature     dd  ?


   refCount             dd  ?
   threadMutex          dd  ?
   processorBinding     dd  ?
   realTimePriority     dd  ?
   inheritedPriority    dd  ?
   setMemberPriority    dd  ?
   processName          db  64 dup (?)
   startAddress         dd  ?
   startParm            dd  ?
   ps_utilizationCount  dd  ?
   threadState          dd  ?
   threadType           dd  ?
   threadFlags          dd  ?
   codeSegment          dd  ?
   threadDataArea       dd  10 dup (?)

   lastProcessor        dd  ?
   resourceFlag         dd  ?

   threadSemaphore      dd  ?
   threadUtilization    dd  ?

   delayNext            dd  ?
   delayPrior           dd  ?
   delaySignature       dd  ?
   delayWaitTime        dd  ?
   delayCurrTime        dd  ?
   delayParameter       dd  ?
   delayFunction        dd  ?
   delayFlags           dd  ?
   delayStamp           dd  ?

   keyBuffer            dd  ?
   typeAheadCount       dd  ?

   threadSubUtil        dd  ?
   threadPreempt        dd  ?
   bindswFlags          dd  ?

   ProcessContext       CONTEXT_FRAME  <?>
   NumericContext       NUMERIC_FRAME  <?>

   tModuleContext       dd   ?
   tAddressSpace        dd   ?

   thread_sema          SEMAPHORE <?>
   syncObject           dd  ?
   syncState            dd  ?
   syncFlag             dd  ?

PROCESS_CONTROL ends

LIGHT_WEIGHT_PROCESS struc

   lwp_next              dd  ?
   lwp_prior             dd  ?
   lwp_procedureAddress  dd  ?
   lwp_State             dd  ?
   lwp_Delay             dd  ?
   lwp_Flags             dd  ?
   lwp_Argument          dd  ?

LIGHT_WEIGHT_PROCESS ends

PROCESSOR_QUEUE struc
   idle_process           dd   ?
   running_process        dd   ?
   local_queue_head       dd   ?
   local_queue_tail       dd   ?
   bind_queue_head        dd   ?
   bind_queue_tail        dd   ?
   bind_queue_mutex       dd   ?
   lwp_queue_head         dd   ?
   lwp_queue_tail         dd   ?
   lwp_bind_queue_head    dd   ?
   lwp_bind_queue_tail    dd   ?
   lwp_bind_queue_mutex   dd   ?
   lsoft_int_head         dd   ?
   lsoft_int_tail         dd   ?
   lsoft_int_mutex        dd   ?
   lfast_lwp_head         dd   ?
   lfast_lwp_tail         dd   ?
   lfast_lwp_mutex        dd   ?
   lwp_worker_queue_head  dd   ?
   lwp_worker_queue_tail  dd   ?
   local_queue_count      dd   ?
   bind_queue_count       dd   ?
   lwp_queue_count        dd   ?
   lwpExecCount           dd   ?
   idleCount              dd   ?
   utilizationCount       dd   ?
   sampleCount            dd   ?
   processorUtilization   dd   ?
   eligibleCount          dd   ?
   activeCount            dd   ?
   lwpProcessCounter      dd   ?
   sleepDisallowed        dd   ?
   processorNumber        dd   ?
   bindsw_queue_head      dd   ?
   bindsw_queue_tail      dd   ?
   bindsw_queue_mutex     dd   ?
   ModuleContext          dd   ?
   AddressSpace           dd   ?
PROCESSOR_QUEUE ends


