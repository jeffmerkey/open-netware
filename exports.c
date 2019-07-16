

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  EXPORTS.C
*   DESCRIP  :  Exports Library for MANOS v1.0
*   DATE     :  August 22, 1998
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
#include "bus.h"
#include "peexe.h"
#include "exports.h"
#include "malloc.h"
#include "free.h"

LONG exportIDs = 1;
LONG NumberOfExports = 0;

LONG ExportListLock = 0;
EXPORT *ExportListHead = 0;
EXPORT *ExportListTail = 0;
EXPORT *ExportNameHead = 0;
EXPORT *ExportNameTail = 0;
EXPORT *ExportAddressHead = 0;
EXPORT *ExportAddressTail = 0;

EXPORT *insertExport(EXPORT *i, EXPORT *top)
{
    EXPORT *old, *p;

    if (!ExportListTail)
    {
       i->next = i->prior = NULL;
       ExportListTail = i;
       return i;
    }
    p = top;
    old = NULL;
    while (p)
    {
       if (strcmp(p->ExportName, i->ExportName) < 0)
       {
	  old = p;
	  p = p->next;
       }
       else
       {
	  if (p->prior)
	  {
	     p->prior->next = i;
	     i->next = p;
	     i->prior = p->prior;
	     p->prior = i;
	     return top;
	  }
	  i->next = p;
	  i->prior = NULL;
	  p->prior = i;
	  return i;
       }
    }
    old->next = i;
    i->next = NULL;
    i->prior = old;
    ExportListTail = i;
    return ExportListHead;

}

EXPORT *insertExportName(EXPORT *i, EXPORT *top)
{
    EXPORT *old, *p;

    if (!ExportNameTail)
    {
       i->nameNext = i->namePrior = NULL;
       ExportNameTail = i;
       return i;
    }
    p = top;
    old = NULL;
    while (p)
    {
       if (strcmp(p->ExportName, i->ExportName) < 0)
       {
	  old = p;
	  p = p->nameNext;
       }
       else
       {
	  if (p->namePrior)
	  {
	     p->namePrior->nameNext = i;
	     i->nameNext = p;
	     i->namePrior = p->namePrior;
	     p->namePrior = i;
	     return top;
	  }
	  i->nameNext = p;
	  i->namePrior = NULL;
	  p->namePrior = i;
	  return i;
       }
    }
    old->nameNext = i;
    i->nameNext = NULL;
    i->namePrior = old;
    ExportNameTail = i;
    return ExportNameHead;

}

EXPORT *insertExportAddress(EXPORT *i, EXPORT *top)
{
    EXPORT *old, *p;

    if (!ExportAddressTail)
    {
       i->addressNext = i->addressPrior = NULL;
       ExportAddressTail = i;
       return i;
    }
    p = top;
    old = NULL;
    while (p)
    {
       if (p->ExportJumpAddress < i->ExportJumpAddress)
       {
	  old = p;
	  p = p->addressNext;
       }
       else
       {
	  if (p->addressPrior)
	  {
	     p->addressPrior->addressNext = i;
	     i->addressNext = p;
	     i->addressPrior = p->addressPrior;
	     p->addressPrior = i;
	     return top;
	  }
	  i->addressNext = p;
	  i->addressPrior = NULL;
	  p->addressPrior = i;
	  return i;
       }
    }
    old->addressNext = i;
    i->addressNext = NULL;
    i->addressPrior = old;
    ExportAddressTail = i;
    return ExportAddressHead;

}

LONG AddPublicExport(EXPORT *NewExport)
{
    register EXPORT *Export;
    register LONG flags;

    if (NewExport->module->ModuleSignature != MODULE_SIGNATURE)
       return -3;

    flags = get_flags();
    spin_lock(&ExportListLock);

    ExportListHead = insertExport(NewExport, ExportListHead);
    ExportNameHead = insertExportName(NewExport, ExportNameHead);
    ExportAddressHead = insertExportAddress(NewExport, ExportAddressHead);
    NumberOfExports++;

    spin_unlock(&ExportListLock);

    EventNotify(EVENT_EXPORT_SYMBOL, (LONG) NewExport);

    set_flags(flags);
    return 0;
}

LONG RemovePublicExport(EXPORT *export)
{
    register EXPORT *searchExport;
    register LONG flags;

    flags = get_flags();
    spin_lock(&ExportListLock);

    searchExport = ExportListHead;
    while (searchExport)
    {
       if (searchExport == export)   // found, remove from list
       {
	  if (ExportListHead == export)
	  {
	     ExportListHead = (void *) export->next;
	     if (ExportListHead)
		ExportListHead->prior = NULL;
	     else
		ExportListTail = NULL;
	  }
	  else
	  {
	     export->prior->next = export->next;
	     if (export != ExportListTail)
		export->next->prior = export->prior;
	     else
		ExportListTail = export->prior;
	  }
	  export->next = export->prior = 0;
	  break;
       }
       searchExport = searchExport->next;
    }

    searchExport = ExportNameHead;
    while (searchExport)
    {
       if (searchExport == export)   // found, remove from list
       {
	  if (ExportNameHead == export)
	  {
	     ExportNameHead = (void *) export->nameNext;
	     if (ExportNameHead)
		ExportNameHead->namePrior = NULL;
	     else
		ExportNameTail = NULL;
	  }
	  else
	  {
	     export->namePrior->nameNext = export->nameNext;
	     if (export != ExportNameTail)
		export->nameNext->namePrior = export->namePrior;
	     else
		ExportNameTail = export->namePrior;
	  }
	  export->nameNext = export->namePrior = 0;
	  break;
       }
       searchExport = searchExport->nameNext;
    }

    searchExport = ExportAddressHead;
    while (searchExport)
    {
       if (searchExport == export)   // found, remove from list
       {
	  if (ExportAddressHead == export)
	  {
	     ExportAddressHead = (void *) export->addressNext;
	     if (ExportAddressHead)
		ExportAddressHead->addressPrior = NULL;
	     else
		ExportAddressTail = NULL;
	  }
	  else
	  {
	     export->addressPrior->addressNext = export->addressNext;
	     if (export != ExportAddressTail)
		export->addressNext->addressPrior = export->addressPrior;
	     else
		ExportAddressTail = export->addressPrior;
	  }
	  export->addressNext = export->addressPrior = 0;
	  break;
       }
       searchExport = searchExport->addressNext;
    }
    if (NumberOfExports)
       NumberOfExports--;
    spin_unlock(&ExportListLock);

    EventNotify(EVENT_REMOVE_EXPORT, (LONG) export);

    set_flags(flags);
    return 0;
}


void InitializeExportList(MODULE_HANDLE *module)
{
    extern LONG ExportRVA;
    extern LONG CodeRVA;
    register LONG j;
    register LONG *OrdinalTable;
    register WORD *ordinals;
    register LONG *name;
    register LONG Ordinal;
    register PIMAGE_EXPORT_DIRECTORY exportDir;
    register EXPORT *export;

    // post exported functions

    if (module->ExportDirectory)
    {
	exportDir = (PIMAGE_EXPORT_DIRECTORY) module->ExportDirectory;
	OrdinalTable = (LONG *)((LONG)exportDir->AddressOfFunctions - ExportRVA + (LONG)exportDir);
	ordinals = (WORD *)((LONG)exportDir->AddressOfNameOrdinals - ExportRVA + (LONG)exportDir);
	name = (LONG *)((LONG)exportDir->AddressOfNames - ExportRVA + (LONG)exportDir);
	for (j=0; j < exportDir->NumberOfNames; j++)
	{
	   export = (EXPORT *) kmalloc(sizeof(EXPORT));
	   if (!export)
	      break;

	   SetData((LONG *)export, 0, sizeof(EXPORT));
	   export->ExportPDE = 0;
	   export->ExportName = (BYTE *) (*name - ExportRVA + (LONG)exportDir);
	   export->ExportNameLength = strlen(export->ExportName);
	   Ordinal = export->ExportOrdinal = (*ordinals + exportDir->Base);
	   export->ExportJumpAddress = (OrdinalTable[Ordinal - 1] - CodeRVA + CodeSegment);
	   export->ExportType = EXPORT_PROC;
	   export->SymbolFormat = FORMAT_BORLAND;
	   export->ExportID = exportIDs++;
	   export->module = module;

	   AddPublicExport(export);

	   name++;
	   ordinals++;
	}
    }

}

void DisplayExports(SCREEN *screen, LONG attr)
{
     register LONG flags;
     EXPORT *export;

     SetPauseMode(screen, screen->nLines - 3);

     flags = get_flags();
     spin_lock(&ExportListLock);
     export = ExportNameHead;
     while (export)
     {
	if (printfScreenWithAttribute(screen, attr, "%s|%08X  %04X  %s\n",
				      export->module->ModuleShortName,
				      export->ExportJumpAddress,
				      export->ExportOrdinal,
				      export->ExportName))
	   break;
	export = export->nameNext;
     }
     spin_unlock(&ExportListLock);
     set_flags(flags);

     ClearPauseMode(screen);
     return;
}

void DisplayExportsMatch(SCREEN *screen, LONG attr, BYTE *match)
{
     register LONG flags;
     EXPORT *export;

     SetPauseMode(screen, screen->nLines - 3);
     printfScreenWithAttribute(screen, attr, "exports matching '%s'\n", match);

     flags = get_flags();
     spin_lock(&ExportListLock);
     export = ExportNameHead;
     while (export)
     {
	if (!strnicmp(match, export->ExportName, strlen(match)))
	{
	   if (printfScreenWithAttribute(screen, attr, "%s|%08X  %04X  %s\n",
					 export->module->ModuleShortName,
					 export->ExportJumpAddress,
					 export->ExportOrdinal,
					 export->ExportName))
	      break;
	}
	export = export->nameNext;
     }
     spin_unlock(&ExportListLock);
     set_flags(flags);

     ClearPauseMode(screen);
     return;

}

LONG ImportExportedAddress(BYTE *name, BYTE *moduleName,
			   MODULE_HANDLE **module, EXPORT **exportRecord)
{
    register LONG flags;
    register LONG moduleNameLength = strlen(moduleName);
    register LONG exportNameLength = strlen(name);
    EXPORT *export, *exportList;

    flags = get_flags();
    spin_lock(&ExportListLock);
    export = ExportNameHead;
    while (export)
    {
       exportList = export->nameNext;
       if (export->ExportNameLength == exportNameLength
	   && !strcmp(export->ExportName, name)
	   && export->module->ModuleShortNameLength == moduleNameLength
	   && !stricmp(export->module->ModuleShortName, moduleName)
	   && export->ExportType == EXPORT_PROC)
       {
	  if (module)
	     *module = (MODULE_HANDLE *) export->module;
	  if (exportRecord)
	     *exportRecord = export;
	  spin_unlock(&ExportListLock);
	  set_flags(flags);
	  return (LONG) (export->ExportJumpAddress);
       }
       export = exportList;
    }
    spin_unlock(&ExportListLock);
    set_flags(flags);
    return (LONG) -1;

}

LONG GetGlobalExportOrdinal(BYTE *name, BYTE *moduleName)
{
    register LONG flags;
    register LONG moduleNameLength = strlen(moduleName);
    register LONG exportNameLength = strlen(name);
    EXPORT *export, *exportList;

    flags = get_flags();
    spin_lock(&ExportListLock);
    export = ExportNameHead;
    while (export)
    {
       exportList = export->next;
       if (export->ExportNameLength == exportNameLength
	   && !strcmp(export->ExportName, name)
	   && export->module->ModuleShortNameLength == moduleNameLength
	   && !stricmp(export->module->ModuleShortName, moduleName))
       {
	  spin_unlock(&ExportListLock);
	  set_flags(flags);
	  return (LONG) (export->ExportOrdinal);
       }
       export = exportList;
    }
    spin_unlock(&ExportListLock);
    set_flags(flags);
    return (LONG) -1;
}

LONG RegisterModuleExport(MODULE_HANDLE *module,
			  BYTE *name,
			  LONG address,
			  LONG Ordinal,
			  LONG type)
{

    MODULE_HANDLE *newModule;
    register LONG retCode;
    EXPORT *export, *newExport;

    // see if it already exists

    retCode = ImportExportedAddress(name, module->ModuleShortName,
				    &newModule, &newExport);
    if (retCode != (LONG) -1)
       return -2;

    export = (EXPORT *) kmalloc(sizeof(EXPORT));
    if (!export)
       return (-1);

    SetData((LONG *)export, 0, sizeof(EXPORT));
    export->ExportPDE = 0;
    export->ExportName = name;
    export->ExportNameLength = strlen(name);
    export->ExportOrdinal = Ordinal;
    export->ExportJumpAddress = address;
    export->ExportType = type;
    export->SymbolFormat = module->DebugSymbolType;
    export->ExportID = exportIDs++;
    export->module = module;

    AddPublicExport(export);

    return (LONG) 0;

}

void RemoveModuleExports(MODULE_HANDLE *module)
{
    register EXPORT *exportList, *export;
    register LONG flags;

    flags = get_flags();
    spin_lock(&ExportListLock);
    export = ExportListHead;
    while (export)
    {
       exportList = export->next;
       if (export->module == module)
       {
	  if (ExportListHead == export)
	  {
	     ExportListHead = (void *) export->next;
	     if (ExportListHead)
		ExportListHead->prior = NULL;
	     else
		ExportListTail = NULL;
	  }
	  else
	  {
	     export->prior->next = export->next;
	     if (export != ExportListTail)
		export->next->prior = export->prior;
	     else
		ExportListTail = export->prior;
	  }
	  export->next = export->prior = 0;

	  if (ExportNameHead == export)
	  {
	     ExportNameHead = (void *) export->nameNext;
	     if (ExportNameHead)
		ExportNameHead->namePrior = NULL;
	     else
		ExportNameTail = NULL;
	  }
	  else
	  {
	     export->namePrior->nameNext = export->nameNext;
	     if (export != ExportNameTail)
		export->nameNext->namePrior = export->namePrior;
	     else
		ExportNameTail = export->namePrior;
	  }
	  export->nameNext = export->namePrior = 0;

	  if (ExportAddressHead == export)
	  {
	     ExportAddressHead = (void *) export->addressNext;
	     if (ExportAddressHead)
		ExportAddressHead->addressPrior = NULL;
	     else
		ExportAddressTail = NULL;
	  }
	  else
	  {
	     export->addressPrior->addressNext = export->addressNext;
	     if (export != ExportAddressTail)
		export->addressNext->addressPrior = export->addressPrior;
	     else
		ExportAddressTail = export->addressPrior;
	  }
	  export->addressNext = export->addressPrior = 0;

	  if (NumberOfExports)
	     NumberOfExports--;

	  EventNotify(EVENT_REMOVE_EXPORT, (LONG) export);

	  kfree(export);
       }
       export = exportList;
    }
    spin_unlock(&ExportListLock);
    set_flags(flags);
    return;

}



