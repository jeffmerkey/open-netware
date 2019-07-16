
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
*   FILE     :  EVENT.C
*   DESCRIP  :  Event Subsystem for MANOS v1.0
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

EVENT EventArray[MAX_EVENTS];

void RegisterOSEvents(void)
{
    extern void ExitOSProcessorShutdown(LONG key);
    extern void ExitOSShutdownSMP(LONG key);
    extern void ExitOSResetKeyboard(LONG key);
    extern void ExitOSClearDebuggerState(LONG key);

    RegisterEventNotification(EVENT_EXIT_OS, ExitOSProcessorShutdown);
    RegisterEventNotification(EVENT_EXIT_OS, ExitOSShutdownSMP);
    RegisterEventNotification(EVENT_EXIT_OS, ExitOSResetKeyboard);
    RegisterEventNotification(EVENT_EXIT_OS, ExitOSClearDebuggerState);
}

void InitializeEvents(void)
{

   AddEventToRegistry(EVENT_ENTER_OS);
   AddEventToRegistry(EVENT_EXIT_OS);
   AddEventToRegistry(EVENT_ENTER_REAL_MODE);
   AddEventToRegistry(EVENT_RETURN_REAL_MODE);
   AddEventToRegistry(EVENT_ALLOC_MEMORY);
   AddEventToRegistry(EVENT_FREE_MEMORY);
   AddEventToRegistry(EVENT_ADD_DISK);
   AddEventToRegistry(EVENT_ADD_LAN);
   AddEventToRegistry(EVENT_CREATE_THREAD);
   AddEventToRegistry(EVENT_DESTROY_THREAD);
   AddEventToRegistry(EVENT_BIND_THREAD);
   AddEventToRegistry(EVENT_UNBIND_THREAD);
   AddEventToRegistry(EVENT_BIND_PROTOCOL);
   AddEventToRegistry(EVENT_UNBIND_PROTOCOL);
   AddEventToRegistry(EVENT_OPEN_DCB);
   AddEventToRegistry(EVENT_CLOSE_DCB);
   AddEventToRegistry(EVENT_LOCK_DCB);
   AddEventToRegistry(EVENT_UNLOCK_DCB);
   AddEventToRegistry(EVENT_UPDATE_SCREEN);
   AddEventToRegistry(EVENT_CREATE_SCREEN);
   AddEventToRegistry(EVENT_DELETE_SCREEN);
   AddEventToRegistry(EVENT_SELECT_SCREEN);
   AddEventToRegistry(EVENT_RESIZE_SCREEN);
   AddEventToRegistry(EVENT_PAGE_OUT);
   AddEventToRegistry(EVENT_PAGE_IN);
   AddEventToRegistry(EVENT_MAP_ADDRESS);
   AddEventToRegistry(EVENT_UNMAP_ADDRESS);
   AddEventToRegistry(EVENT_COMPRESS_MEMORY);
   AddEventToRegistry(EVENT_DECOMPRESS_MEMORY);
   AddEventToRegistry(EVENT_ADVERTISE_SERVICE);
   AddEventToRegistry(EVENT_ROUTE_PACKET);
   AddEventToRegistry(EVENT_READ_DISK);
   AddEventToRegistry(EVENT_WRITE_DISK);
   AddEventToRegistry(EVENT_SEEK_DISK);
   AddEventToRegistry(EVENT_SCAN_DISKS);
   AddEventToRegistry(EVENT_OPEN_FILE);
   AddEventToRegistry(EVENT_CLOSE_FILE);
   AddEventToRegistry(EVENT_SEEK_FILE);
   AddEventToRegistry(EVENT_WRITE_FILE);
   AddEventToRegistry(EVENT_READ_FILE);
   AddEventToRegistry(EVENT_RENAME_FILE);
   AddEventToRegistry(EVENT_DELETE_FILE);
   AddEventToRegistry(EVENT_ATOMIC_LOCK);
   AddEventToRegistry(EVENT_ATOMIC_UNLOCK);
   AddEventToRegistry(EVENT_SYNC_DCB_END);
   AddEventToRegistry(EVENT_AS_SWITCH);
   AddEventToRegistry(EVENT_LOAD_MODULE);
   AddEventToRegistry(EVENT_UNLOAD_MODULE);
   AddEventToRegistry(EVENT_ENTER_DEBUGGER);
   AddEventToRegistry(EVENT_LEAVE_DEBUGGER);
   AddEventToRegistry(EVENT_EXPORT_SYMBOL);
   AddEventToRegistry(EVENT_IMPORT_SYMBOL);
   AddEventToRegistry(EVENT_LOAD_LIBRARY);
   AddEventToRegistry(EVENT_UNLOAD_LIBRARY);
   AddEventToRegistry(EVENT_MAP_SHARED_SEGMENT);
   AddEventToRegistry(EVENT_SCHEDULE_TIMER);
   AddEventToRegistry(EVENT_CANCEL_TIMER);
   AddEventToRegistry(EVENT_CALL_TIMER);
   AddEventToRegistry(EVENT_SCHEDULE_LWP);
   AddEventToRegistry(EVENT_CANCEL_LWP);
   AddEventToRegistry(EVENT_CALL_SLEEP_LWP);
   AddEventToRegistry(EVENT_CALL_RT_LWP);
   AddEventToRegistry(EVENT_SCHEDULE_SOFT_INT);
   AddEventToRegistry(EVENT_CANCEL_SOFT_INT);
   AddEventToRegistry(EVENT_CALL_SOFT_INT);
   AddEventToRegistry(EVENT_HARDWARE_INT);
   AddEventToRegistry(EVENT_XCALL);
   AddEventToRegistry(EVENT_XCALL_INT);
   AddEventToRegistry(EVENT_PROCESSOR_ONLINE);
   AddEventToRegistry(EVENT_PROCESSOR_OFFLINE);
   AddEventToRegistry(EVENT_DIRECTED_EXT_INT);
   AddEventToRegistry(EVENT_END_OF_INTERRUPT);
   AddEventToRegistry(EVENT_BIOS16_ENTRY);
   AddEventToRegistry(EVENT_BIOS16_EXIT);
   AddEventToRegistry(EVENT_BIOS32_ENTRY);
   AddEventToRegistry(EVENT_BIOS32_EXIT);
   AddEventToRegistry(EVENT_BIOS64_ENTRY);
   AddEventToRegistry(EVENT_BIOS64_EXIT);

   AddSystemEventToRegistry(EVENT_DIVIDE_ZERO, 0);
   AddSystemEventToRegistry(EVENT_DEBUGGER_EXCEPTION, 0);
   AddSystemEventToRegistry(EVENT_NMI, 0);
   AddSystemEventToRegistry(EVENT_DEBUGGER_BREAK, 0);
   AddSystemEventToRegistry(EVENT_OVERFLOW, 0);
   AddSystemEventToRegistry(EVENT_BOUNDS_CHECK, 0);
   AddSystemEventToRegistry(EVENT_INVL_OPCODE, 0);
   AddSystemEventToRegistry(EVENT_NO_COPROCESSOR, 0);
   AddSystemEventToRegistry(EVENT_DOUBLE_FAULT, 0);
   AddSystemEventToRegistry(EVENT_COPS, 0);
   AddSystemEventToRegistry(EVENT_TSS_CHECK, 0);
   AddSystemEventToRegistry(EVENT_SEGMENT_FAULT, 0);
   AddSystemEventToRegistry(EVENT_STACK_EXCEPTION, 0);
   AddSystemEventToRegistry(EVENT_GENERAL_PROTECT, 0);
   AddSystemEventToRegistry(EVENT_PAGE_FAULT, 0);
   AddSystemEventToRegistry(EVENT_COPROCESSOR_ERROR, 0);
   AddSystemEventToRegistry(EVENT_ALIGNMENT_CHECK, 0);
   AddSystemEventToRegistry(EVENT_MACHINE_CHECK, 0);
   AddSystemEventToRegistry(EVENT_INVL_INT, 0);

   AddEventToRegistry(EVENT_REMOVE_EXPORT);
   AddEventToRegistry(EVENT_LOCK_FILE);
   AddEventToRegistry(EVENT_UNLOCK_FILE);
   AddEventToRegistry(EVENT_START_TRANSACTION);
   AddEventToRegistry(EVENT_END_TRANSACTION);
   AddEventToRegistry(EVENT_ABORT_TRANSACTION);
   AddEventToRegistry(EVENT_KALLOC_MEMORY);
   AddEventToRegistry(EVENT_KFREE_MEMORY);
   AddEventToRegistry(EVENT_PALLOC_MEMORY);
   AddEventToRegistry(EVENT_PFREE_MEMORY);
   AddEventToRegistry(EVENT_STALLOC_MEMORY);
   AddEventToRegistry(EVENT_STFREE_MEMORY);
   AddEventToRegistry(EVENT_KEY_PRESSED);
   AddEventToRegistry(EVENT_KEY_RELEASED);


   return;

}

LONG AddEventToRegistry(LONG Event)
{
   if (Event > MAX_EVENTS - 1)
      return -1;

   if (EventArray[Event].eventActive)
      return -1;

   EventArray[Event].eventActive = TRUE;
   EventArray[Event].head = EventArray[Event].tail = 0;
   EventArray[Event].eventMutex = 0;
   EventArray[Event].lockFlag = TRUE;

   return 0;
}

LONG AddSystemEventToRegistry(LONG Event, LONG lockFlag)
{
   if (Event > MAX_EVENTS - 1)
      return -1;

   if (EventArray[Event].eventActive)
      return -1;

   EventArray[Event].eventActive = TRUE;
   EventArray[Event].head = EventArray[Event].tail = 0;
   EventArray[Event].eventMutex = 0;
   EventArray[Event].lockFlag = lockFlag;

   return 0;
}

LONG EventNotify(LONG Event, LONG EventData)
{
   register LONG flags;
   register EVENT_NODE *event;

   if (Event > MAX_EVENTS - 1)
      return -1;

   if (!EventArray[Event].eventActive)
      return -1;

   flags = get_flags();
   spin_lock(&EventArray[Event].eventMutex);

   event = EventArray[Event].head;
   while (event)
   {
      if (event->EventSignature != EVENT_SIGNATURE)
	 panic("EVENT NODE WAS CORRUPT !!!");

      if (event->eventRoutine)
	 (event->eventRoutine)(EventData);
      event = event->nextEvent;
   }
   spin_unlock(&EventArray[Event].eventMutex);
   set_flags(flags);
   return 0;
}

LONG RegisterEventNotification(LONG Event, void (*EventRoutine)(LONG))
{
   register LONG flags;
   register EVENT_NODE *event, *eventSearch;

   if (Event > MAX_EVENTS - 1)
      return -1;

   if (!EventArray[Event].eventActive)
      return -1;

   event = (EVENT_NODE *) kmalloc(sizeof(EVENT_NODE));
   if (event)
   {
      SetData((LONG *)event, 0, sizeof(EVENT_NODE));
      event->EventSignature = EVENT_SIGNATURE;
      event->eventRoutine = (void (*)(LONG))EventRoutine;
      event->Event = Event;

      flags = get_flags();
      spin_lock(&EventArray[Event].eventMutex);

      if (!EventArray[Event].head)
      {
	 EventArray[Event].head = event;
	 EventArray[Event].tail = event;
	 event->nextEvent = event->priorEvent = 0;
      }
      else
      {
	 EventArray[Event].tail->nextEvent = event;
	 event->nextEvent = 0;
	 event->priorEvent = EventArray[Event].tail;
	 EventArray[Event].tail = event;
      }
      EventArray[Event].numEvents++;

      spin_unlock(&EventArray[Event].eventMutex);
      set_flags(flags);

      return (LONG) event;
   }
   return 0;

}

LONG DeRegisterEventNotification(LONG EventID)
{

   register LONG flags, Event;
   register EVENT_NODE *event = (EVENT_NODE *) EventID;
   register EVENT_NODE *eventSearch;

   if (EventID > MAX_EVENTS - 1)
      return -1;

   if (event->EventSignature != EVENT_SIGNATURE)
      return -1;

   Event = event->Event;
   if (!EventArray[Event].eventActive)
      return -1;

   flags = get_flags();
   spin_lock(&EventArray[event->Event].eventMutex);
   eventSearch = EventArray[event->Event].head;
   while (eventSearch)
   {
      if (eventSearch == event)
      {
	 if (EventArray[Event].head == event)
	 {
	    EventArray[Event].head = (void *) event->nextEvent;
	    if (EventArray[Event].head)
	       EventArray[Event].head->priorEvent = NULL;
	    else
	       EventArray[Event].tail = NULL;
	 }
	 else
	 {
	    event->priorEvent->nextEvent = event->nextEvent;
	    if (event != EventArray[Event].tail)
	       event->nextEvent->priorEvent = event->priorEvent;
	    else
	       EventArray[Event].tail = event->priorEvent;
	 }
	 if (EventArray[Event].numEvents)
	    EventArray[Event].numEvents--;

	 spin_unlock(&EventArray[event->Event].eventMutex);
	 set_flags(flags);
	 kfree(event);
	 return 0;
      }
      eventSearch = eventSearch->nextEvent;
   }
   spin_unlock(&EventArray[event->Event].eventMutex);
   set_flags(flags);
   return -1;

}




