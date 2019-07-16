
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
*   FILE     :  TRACKING.C
*   DESCRIP  :  Tracking Subsystem for MANOS v1.0
*   DATE     :  July 30, 1998
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
#include "malloc.h"
#include "free.h"
#include "tracking.h"

TRACKING TrackingArray[MAX_TRACKED_CATEGORIES];

TRACKING_NODE *GetTrackedList(LONG Category)
{
   if (!TrackingArray[Category].trackActive)
      return 0;
   return (TrackingArray[Category].head);
}

TRACKING_NODE *GetTrackedModule(MODULE_HANDLE *module)
{
   return (module->resourceHead);
}

LONG *GetTrackedModuleLock(MODULE_HANDLE *module)
{
   return (&module->resourceLock);
}

LONG GetTrackedModuleCount(MODULE_HANDLE *module)
{
   return (module->resourceNum);
}

LONG RegisterTrackedCategory(LONG Category,
			     BYTE *CategoryName,
			     LONG (*CategoryRelease)(void *tracking))
{
   if (Category > MAX_TRACKED_CATEGORIES - 1)
      return -1;

   if (TrackingArray[Category].trackActive)
      return -1;

   TrackingArray[Category].trackActive = TRUE;
   TrackingArray[Category].head = TrackingArray[Category].tail = 0;
   TrackingArray[Category].trackMutex = 0;
   TrackingArray[Category].lockFlag = TRUE;
   TrackingArray[Category].CategoryName = CategoryName;
   TrackingArray[Category].CategoryRelease = CategoryRelease;

   return 0;
}

LONG RegisterTrackedElement(LONG Category,
			    BYTE *ElementName,
			    void *ElementPointer,
			    void *ElementContext,
			    LONG ElementValue,
			    void *ElementOwner)
{
   register LONG flags;
   register TRACKING_NODE *track, *trackSearch;

   if (!TrackingArray[Category].trackActive)
      return -1;

   track = (TRACKING_NODE *) kmalloc(sizeof(TRACKING_NODE));
   if (track)
   {
      SetData((LONG *)track, 0, sizeof(TRACKING_NODE));
      track->CategorySignature = TRACKING_SIGNATURE;
      track->ElementName = ElementName;
      track->ElementPointer = ElementPointer;
      track->ElementValue = ElementValue;
      track->ElementContext = ElementContext;
      track->Category = Category;
      track->ElementOwner = ElementOwner;

      flags = get_flags();
      spin_lock(&TrackingArray[Category].trackMutex);

      if (!TrackingArray[Category].head)
      {
	 TrackingArray[Category].head = track;
	 TrackingArray[Category].tail = track;
	 track->nextCategory = track->priorCategory = 0;
      }
      else
      {
	 TrackingArray[Category].tail->nextCategory = track;
	 track->nextCategory = 0;
	 track->priorCategory = TrackingArray[Category].tail;
	 TrackingArray[Category].tail = track;
      }
      TrackingArray[Category].numCategories++;
      spin_unlock(&TrackingArray[Category].trackMutex);
      set_flags(flags);
      return (LONG) track;
   }
   return 0;

}

LONG DeRegisterTrackedElement(LONG ElementID)
{

   register LONG flags, Category;
   register TRACKING_NODE *track = (TRACKING_NODE *) ElementID;
   register TRACKING_NODE *trackSearch;

   if (track->CategorySignature != TRACKING_SIGNATURE)
      return -1;

   Category = track->Category;
   if (!TrackingArray[Category].trackActive)
      return -1;

   flags = get_flags();
   spin_lock(&TrackingArray[track->Category].trackMutex);
   trackSearch = TrackingArray[track->Category].head;
   while (trackSearch)
   {
      if (trackSearch == track)
      {
	 if (TrackingArray[Category].head == track)
	 {
	    TrackingArray[Category].head = (void *) track->nextCategory;
	    if (TrackingArray[Category].head)
	       TrackingArray[Category].head->priorCategory = NULL;
	    else
	       TrackingArray[Category].tail = NULL;
	 }
	 else
	 {
	    track->priorCategory->nextCategory = track->nextCategory;
	    if (track != TrackingArray[Category].tail)
	       track->nextCategory->priorCategory = track->priorCategory;
	    else
	       TrackingArray[Category].tail = track->priorCategory;
	 }
	 if (TrackingArray[Category].numCategories)
	    TrackingArray[Category].numCategories--;

	 spin_unlock(&TrackingArray[track->Category].trackMutex);
	 set_flags(flags);
	 kfree(track);
	 return 0;
      }
      trackSearch = trackSearch->nextCategory;
   }
   spin_unlock(&TrackingArray[track->Category].trackMutex);
   set_flags(flags);
   return -1;

}

LONG AddTrackedElementToModule(LONG ElementID)
{
   register LONG flags, Category;
   register TRACKING_NODE *track = (TRACKING_NODE *) ElementID;
   register TRACKING_NODE *trackSearch, *tail, *search;
   register MODULE_HANDLE *module;

   if (track->CategorySignature != TRACKING_SIGNATURE)
      return -1;

   Category = track->Category;
   if (!TrackingArray[Category].trackActive)
      return -1;

   module = (MODULE_HANDLE *) track->ElementOwner;
   flags = get_flags();
   spin_lock(&module->resourceLock);
   search = module->resourceHead;
   while (search)
   {
      if (search == track)
      {
	 spin_unlock(&module->resourceLock);
	 set_flags(flags);
	 return 1;
      }
      search = search->nextModule;
   }
   if (!module->resourceHead)
   {
      module->resourceHead = track;
      module->resourceTail = track;
      track->nextModule = track->priorModule = 0;
   }
   else
   {
      tail = (TRACKING_NODE *) module->resourceTail;
      tail->nextModule = track;
      track->nextModule = 0;
      track->priorModule = module->resourceTail;
      module->resourceTail = track;
   }
   module->resourceNum++;
   spin_unlock(&module->resourceLock);
   set_flags(flags);
   return 0;

}

LONG RemoveTrackedElementFromModule(LONG ElementID)
{
   register LONG flags, Category;
   register TRACKING_NODE *track = (TRACKING_NODE *) ElementID;
   register TRACKING_NODE *trackSearch, *head;
   register MODULE_HANDLE *module;

   if (track->CategorySignature != TRACKING_SIGNATURE)
      return -1;

   Category = track->Category;
   if (!TrackingArray[Category].trackActive)
      return -1;

   module = (MODULE_HANDLE *) track->ElementOwner;
   flags = get_flags();
   spin_lock(&module->resourceLock);
   trackSearch = TrackingArray[track->Category].head;
   while (trackSearch)
   {
      if (trackSearch == track)
      {
	 if (module->resourceHead == (void *) track)
	 {
	    module->resourceHead = (void *) track->nextModule;
	    if (module->resourceHead)
	    {
	       head = (TRACKING_NODE *) module->resourceHead;
	       head->priorModule = NULL;
	    }
	    else
	       module->resourceTail = NULL;
	 }
	 else
	 {
	    track->priorModule->nextModule = track->nextModule;
	    if (track != (TRACKING_NODE *) module->resourceTail)
	       track->nextModule->priorModule = track->priorModule;
	    else
	       module->resourceTail = track->priorModule;
	 }
	 if (module->resourceNum)
	    module->resourceNum--;
	 spin_unlock(&module->resourceLock);
	 set_flags(flags);
	 return 0;
      }
      trackSearch = trackSearch->nextModule;
   }
   spin_unlock(&module->resourceLock);
   set_flags(flags);
   return -1;

}


