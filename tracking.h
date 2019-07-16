
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  TRACKING.H
*   DESCRIP  :  Resource Tracking Subsystem for MANOS v1.0
*   DATE     :  July 30, 1998
*
*
***************************************************************************/

#include "types.h"

#define MAX_TRACKED_CATEGORIES  100
#define TRACKING_SIGNATURE    0xFFEEFFCC

typedef struct _TRACKING_NODE
{
   struct _TRACKING_NODE *nextCategory;
   struct _TRACKING_NODE *priorCategory;
   struct _TRACKING_NODE *nextModule;
   struct _TRACKING_NODE *priorModule;
   LONG CategorySignature;
   LONG Category;
   BYTE *ElementName;
   void *ElementPointer;
   void *ElementContext;
   LONG ElementValue;
   void *ElementOwner;
} TRACKING_NODE;

typedef struct _TRACKING
{
   TRACKING_NODE *head;
   TRACKING_NODE *tail;
   LONG trackMutex;
   void (*trackRoutine)(LONG parm);
   LONG trackActive;
   LONG numCategories;
   LONG lockFlag;
   BYTE *CategoryName;
   LONG (*CategoryRelease)(void *tracking);
} TRACKING;

//
//
//

#define   ALLOCATED_MEMORY     0x00000001
#define   SPIN_LOCK            0x00000002
#define   TIMER_CALLBACK       0x00000003

LONG RegisterTrackedCategory(LONG Category, BYTE *CategoryName, LONG (*CategoryRelease)(void *tracking));
LONG RegisterTrackedElement(LONG Category, BYTE *ElementName, void *ElementPointer,
			    void *ElementContext, LONG ElementValue, void *ElementOwner);
LONG DeRegisterTrackedElement(LONG ElementID);
LONG AddTrackedElementToModule(LONG ElementID);
LONG RemoveTrackedElementFromModule(LONG ElementID);
TRACKING_NODE *GetTrackedList(LONG Category);
TRACKING_NODE *GetTrackedModule(MODULE_HANDLE *module);
LONG *GetTrackedModuleLock(MODULE_HANDLE *module);
LONG GetTrackedModuleCount(MODULE_HANDLE *module);
