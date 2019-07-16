
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
*   FILE     :  EVENT.H
*   DESCRIP  :  Event Subsystem for MANOS v1.0
*   DATE     :  July 20, 1998
*
*
***************************************************************************/

#include "types.h"

#define MAX_EVENTS  256
#define EVENT_SIGNATURE    0xFFEEFFCC

typedef struct _EVENT_NODE
{
   struct _EVENT_NODE *nextEvent;
   struct _EVENT_NODE *priorEvent;
   void (*eventRoutine)(LONG parm);
   LONG EventSignature;
   LONG Event;
} EVENT_NODE;

typedef struct _EVENT
{
   EVENT_NODE *head;
   EVENT_NODE *tail;
   LONG eventMutex;
   void (*eventRoutine)(LONG parm);
   LONG eventActive;
   LONG numEvents;
   LONG lockFlag;
} EVENT;

//
//   MANOS Events
//

#define  EVENT_ENTER_OS             0x00000000  //
#define  EVENT_EXIT_OS              0x00000001  //
#define  EVENT_ENTER_REAL_MODE      0x00000002  //
#define  EVENT_RETURN_REAL_MODE     0x00000003  //
#define  EVENT_ALLOC_MEMORY         0x00000004
#define  EVENT_FREE_MEMORY          0x00000005
#define  EVENT_ADD_DISK             0x00000006
#define  EVENT_ADD_LAN              0x00000007
#define  EVENT_CREATE_THREAD        0x00000008  //
#define  EVENT_DESTROY_THREAD       0x00000009  //
#define  EVENT_BIND_THREAD          0x0000000A
#define  EVENT_UNBIND_THREAD        0x0000000B
#define  EVENT_BIND_PROTOCOL        0x0000000C
#define  EVENT_UNBIND_PROTOCOL      0x0000000D
#define  EVENT_OPEN_DCB             0x0000000E
#define  EVENT_CLOSE_DCB            0x0000000F
#define  EVENT_LOCK_DCB             0x00000010
#define  EVENT_UNLOCK_DCB           0x00000011
#define  EVENT_UPDATE_SCREEN        0x00000012
#define  EVENT_CREATE_SCREEN        0x00000013
#define  EVENT_DELETE_SCREEN        0x00000014
#define  EVENT_SELECT_SCREEN        0x00000015
#define  EVENT_RESIZE_SCREEN        0x00000016
#define  EVENT_PAGE_OUT             0x00000017
#define  EVENT_PAGE_IN              0x00000018
#define  EVENT_MAP_ADDRESS          0x00000019
#define  EVENT_UNMAP_ADDRESS        0x0000001A
#define  EVENT_COMPRESS_MEMORY      0x0000001B
#define  EVENT_DECOMPRESS_MEMORY    0x0000001C
#define  EVENT_ADVERTISE_SERVICE    0x0000001D
#define  EVENT_ROUTE_PACKET         0x0000001E
#define  EVENT_READ_DISK            0x0000001F
#define  EVENT_WRITE_DISK           0x00000020
#define  EVENT_SEEK_DISK            0x00000021
#define  EVENT_SCAN_DISKS           0x00000022
#define  EVENT_OPEN_FILE            0x00000023
#define  EVENT_CLOSE_FILE           0x00000024
#define  EVENT_SEEK_FILE            0x00000025
#define  EVENT_WRITE_FILE           0x00000026
#define  EVENT_READ_FILE            0x00000027
#define  EVENT_RENAME_FILE          0x00000028
#define  EVENT_DELETE_FILE          0x00000029
#define  EVENT_ATOMIC_LOCK          0x0000002A
#define  EVENT_ATOMIC_UNLOCK        0x0000002B
#define  EVENT_SYNC_DCB_END         0x0000002C
#define  EVENT_AS_SWITCH            0x0000002D
#define  EVENT_LOAD_MODULE          0x0000002E  //
#define  EVENT_UNLOAD_MODULE        0x0000002F  //
#define  EVENT_ENTER_DEBUGGER       0x00000030  //
#define  EVENT_LEAVE_DEBUGGER       0x00000031  //
#define  EVENT_EXPORT_SYMBOL        0x00000032  //
#define  EVENT_IMPORT_SYMBOL        0x00000033  //
#define  EVENT_LOAD_LIBRARY         0x00000034
#define  EVENT_UNLOAD_LIBRARY       0x00000035
#define  EVENT_MAP_SHARED_SEGMENT   0x00000036
#define  EVENT_SCHEDULE_TIMER       0x00000037
#define  EVENT_CANCEL_TIMER         0x00000037
#define  EVENT_CALL_TIMER           0x00000037
#define  EVENT_SCHEDULE_LWP         0x00000038
#define  EVENT_CANCEL_LWP           0x00000039
#define  EVENT_CALL_SLEEP_LWP       0x0000003A
#define  EVENT_CALL_RT_LWP          0x0000003B
#define  EVENT_SCHEDULE_SOFT_INT    0x00000038
#define  EVENT_CANCEL_SOFT_INT      0x00000039
#define  EVENT_CALL_SOFT_INT        0x0000003B
#define  EVENT_HARDWARE_INT         0x0000003C
#define  EVENT_XCALL                0x0000003D  //
#define  EVENT_XCALL_INT            0x0000003E  //
#define  EVENT_PROCESSOR_ONLINE     0x0000003F  //
#define  EVENT_PROCESSOR_OFFLINE    0x00000040  //
#define  EVENT_DIRECTED_EXT_INT     0x00000041
#define  EVENT_END_OF_INTERRUPT     0x00000042
#define  EVENT_BIOS16_ENTRY         0x00000043
#define  EVENT_BIOS16_EXIT          0x00000044
#define  EVENT_BIOS32_ENTRY         0x00000045
#define  EVENT_BIOS32_EXIT          0x00000046
#define  EVENT_BIOS64_ENTRY         0x00000047
#define  EVENT_BIOS64_EXIT          0x00000048
#define  EVENT_DIVIDE_ZERO          0x00000049  //
#define  EVENT_DEBUGGER_EXCEPTION   0x0000004A  //
#define  EVENT_NMI                  0x0000004B  //
#define  EVENT_DEBUGGER_BREAK       0x0000004C  //
#define  EVENT_OVERFLOW             0x0000004D  //
#define  EVENT_BOUNDS_CHECK         0x0000004E  //
#define  EVENT_INVL_OPCODE          0x0000004F  //
#define  EVENT_NO_COPROCESSOR       0x00000050  //
#define  EVENT_DOUBLE_FAULT         0x00000051  //
#define  EVENT_COPS                 0x00000052  //
#define  EVENT_TSS_CHECK            0x00000053  //
#define  EVENT_SEGMENT_FAULT        0x00000054  //
#define  EVENT_STACK_EXCEPTION      0x00000055  //
#define  EVENT_GENERAL_PROTECT      0x00000056  //
#define  EVENT_PAGE_FAULT           0x00000057  //
#define  EVENT_COPROCESSOR_ERROR    0x00000058  //
#define  EVENT_ALIGNMENT_CHECK      0x00000059  //
#define  EVENT_MACHINE_CHECK        0x0000005A  //
#define  EVENT_INVL_INT             0x0000005B  //
#define  EVENT_REMOVE_EXPORT        0x0000005C  //
#define  EVENT_LOCK_FILE            0x0000005D
#define  EVENT_UNLOCK_FILE          0x0000005E
#define  EVENT_START_TRANSACTION    0x0000005F
#define  EVENT_END_TRANSACTION      0x00000060
#define  EVENT_ABORT_TRANSACTION    0x00000061
#define  EVENT_KALLOC_MEMORY        0x00000062
#define  EVENT_KFREE_MEMORY         0x00000063
#define  EVENT_PALLOC_MEMORY        0x00000064
#define  EVENT_PFREE_MEMORY         0x00000065
#define  EVENT_STALLOC_MEMORY       0x00000066
#define  EVENT_STFREE_MEMORY        0x00000067
#define  EVENT_KEY_PRESSED          0x00000068
#define  EVENT_KEY_RELEASED         0x00000069

extern LONG AddEventToRegistry(LONG Event);
extern LONG AddSystemEventToRegistry(LONG Event, LONG lockFlag);
extern LONG EventNotify(LONG Event, LONG EventData);
extern LONG RegisterEventNotification(LONG Event, void (*EventRoutine)(LONG));
extern LONG DeRegisterEventNotification(LONG EventID);



