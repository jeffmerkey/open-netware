

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  TSSPOOL.C
*   DESCRIP  :  IA32 TSS Debugger Pools for MANOS v1.0
*   DATE     :  December 28, 1997
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
#include "event.h"
#include "debug.h"
#include "dparse.h"

POOL_HEADER *poolHead;
POOL_HEADER *poolTail;
LONG poolLock = 0;
POOL_HEADER *NMIpoolHead;
POOL_HEADER *NMIpoolTail;
LONG NMIpoolLock = 0;

extern LONG CurrentDR6[MAX_PROCESSORS];

void InitializeDebuggerTables(LONG DebuggerPageTable, LONG OSPageTable)
{

   register LONG i;
   register POOL_HEADER *pool;

   pool = (POOL_HEADER *) &StackPool[0];
   for (i=0; i < POOLED_RESOURCES; i++)
   {
      insert_pool(pool);
      pool->StackTop = (BYTE *) ((LONG) pool + (POOLED_STACK_SIZE - POOLED_STACK_ADJUST));
      pool->Signature = POOLED_STACK_SIGNATURE;
      pool = (POOL_HEADER *) ((LONG) pool + POOLED_STACK_SIZE);
   }

   pool = (POOL_HEADER *) &NMIStackPool[0];
   for (i=0; i < POOLED_NMI_RESOURCES; i++)
   {
      insert_pool_nmi(pool);
      pool->StackTop = (BYTE *) ((LONG) pool + (POOLED_STACK_SIZE - POOLED_STACK_ADJUST));
      pool->Signature = POOLED_STACK_SIGNATURE;
      pool = (POOL_HEADER *) ((LONG) pool + POOLED_STACK_SIZE);
   }

   for (i=0; i < (EXCEPTION_ENTRIES * MAX_PROCESSORS); i++)
   {
      TSSSegments[i].tCR3 = (LONG *) DebuggerPageTable;
      TSSSegments[i].tSystemFlags &= ~IF_FLAG;
   }

   for (i=0; i < (MAX_PROCESSORS); i++)
   {
      TSSDefaultSegments[i].tCR3 = (LONG *) OSPageTable;
      TSSSegments[i].tSystemFlags &= ~IF_FLAG;
      CurrentDR6[i] = 0;
   }

}


POOL_HEADER *remove_pool(void)
{
    POOL_HEADER *pool;

    dspin_lock(&poolLock);
    if (poolHead)
    {
       pool = poolHead;
       poolHead = pool->next;
       if (poolHead)
	  poolHead->prior = NULL;
       else
	  poolTail = NULL;

       dspin_unlock(&poolLock);
       return pool;
    }

    dspin_unlock(&poolLock);
    return pool;

}

void insert_pool(POOL_HEADER *p)
{

    dspin_lock(&poolLock);
    if (!poolHead)
    {
       poolHead = p;
       poolTail = p;
       p->next = p->prior = 0;
    }
    else
    {
       poolTail->next = p;
       p->next = 0;
       p->prior = poolTail;
       poolTail = p;
    }
    dspin_unlock(&poolLock);
    return;

}

LONG FreePooledResource(POOL_HEADER *pool)
{
    if (pool->Signature != POOLED_STACK_SIGNATURE)
       return -1;

    insert_pool(pool);

    return 0;
}

LONG AllocPooledResource(StackFrame *stackFrame)
{

   POOL_HEADER *pool;

   pool = remove_pool();
   if (pool && pool->Signature == POOLED_STACK_SIGNATURE)
   {
      CopyData((LONG *)stackFrame, (LONG *) &pool->TSS, sizeof(StackFrame));
      return (LONG) pool;
   }
   else
      return 0;
}

POOL_HEADER *remove_pool_nmi(void)
{

    POOL_HEADER *pool;

    dspin_lock(&NMIpoolLock);
    if (NMIpoolHead)
    {
       pool = NMIpoolHead;
       NMIpoolHead = pool->next;
       if (NMIpoolHead)
	  NMIpoolHead->prior = NULL;
       else
	  NMIpoolTail = NULL;

       dspin_unlock(&NMIpoolLock);
       return pool;
    }

    dspin_unlock(&NMIpoolLock);
    return pool;

}

void insert_pool_nmi(POOL_HEADER *p)
{

    dspin_lock(&NMIpoolLock);
    if (!NMIpoolHead)
    {
       NMIpoolHead = p;
       NMIpoolTail = p;
       p->next = p->prior = 0;
    }
    else
    {
       NMIpoolTail->next = p;
       p->next = 0;
       p->prior = NMIpoolTail;
       NMIpoolTail = p;
    }
    dspin_unlock(&NMIpoolLock);
    return;

}

LONG FreeNMIPooledResource(POOL_HEADER *pool)
{
    if (pool->Signature != POOLED_STACK_SIGNATURE)
       return -1;

    insert_pool_nmi(pool);

    return 0;
}

LONG AllocNMIPooledResource(StackFrame *stackFrame)
{

   POOL_HEADER *pool;

   pool = remove_pool_nmi();
   if (pool && pool->Signature == POOLED_STACK_SIGNATURE)
   {
      CopyData((LONG *)stackFrame, (LONG *) &pool->TSS, sizeof(StackFrame));
      return (LONG) pool;
   }
   else
      return 0;
}

