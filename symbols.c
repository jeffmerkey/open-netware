

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  SYMBOLS.C
*   DESCRIP  :  Symbol Library for MANOS v1.0
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
#include "symbols.h"
#include "malloc.h"
#include "free.h"

LONG SymbolListLock = 0;
SYMBOL *SymbolListHead = 0;
SYMBOL *SymbolListTail = 0;
SYMBOL *SymbolNameHead = 0;
SYMBOL *SymbolNameTail = 0;
SYMBOL *SymbolAddressHead = 0;
SYMBOL *SymbolAddressTail = 0;

SYMBOL *CreatePublicSymbol(MODULE_HANDLE *module,
			   BYTE *SymbolName,
			   LONG SymbolAddress,
			   LONG Type,
			   LONG SymbolFormat,
			   void *SymbolLink,
			   LONG SymbolType,
			   LONG InternalType)
{
    register LONG retCode;
    SYMBOL *symbol;

    symbol = kmalloc(sizeof(SYMBOL) * 2);
    if (symbol)
    {
       SetData((LONG *)symbol, 0, sizeof(SYMBOL));
       symbol->symbolName = SymbolName;
       symbol->symbolNameLength = strlen(SymbolName);
       symbol->symbolAddress = SymbolAddress;
       symbol->symbolFormat = SymbolFormat;
       symbol->Type = Type;
       symbol->symbolType = SymbolType;
       symbol->symbolLink = SymbolLink;
       symbol->ModuleContext = module;
       symbol->symbolSignature = SYMBOL_SIGNATURE;
       symbol->InternalType = InternalType;

       retCode = AddPublicSymbol(symbol);
       if (retCode)
       {
	  kfree(symbol);
	  return 0;
       }
       else
	  return symbol;
    }
    return 0;

}

SYMBOL *InitCreatePublicSymbol(MODULE_HANDLE *module,
			       BYTE *SymbolName,
			       LONG SymbolAddress,
			       LONG Type,
			       LONG SymbolFormat,
			       void *SymbolLink,
			       LONG SymbolType,
			       LONG InternalType)
{
    extern void *InitMalloc(LONG size);
    register LONG retCode;
    SYMBOL *symbol;

    symbol = InitMalloc(sizeof(SYMBOL));
    if (symbol)
    {
       SetData((LONG *)symbol, 0, sizeof(SYMBOL));
       symbol->symbolName = SymbolName;
       symbol->symbolNameLength = strlen(SymbolName);
       symbol->symbolAddress = SymbolAddress;
       symbol->symbolFormat = SymbolFormat;
       symbol->Type = Type;
       symbol->symbolType = SymbolType;
       symbol->symbolLink = SymbolLink;
       symbol->ModuleContext = module;
       symbol->symbolSignature = SYMBOL_SIGNATURE;
       symbol->InternalType = InternalType;

       retCode = AddPublicSymbol(symbol);
       if (retCode)
	  return 0;
       return symbol;
    }
    return 0;

}

SYMBOL *insertSymbol(SYMBOL *i, SYMBOL *top)
{
    SYMBOL *old, *p;

    if (!SymbolListTail)
    {
       i->next = i->prior = NULL;
       SymbolListTail = i;
       return i;
    }
    p = top;
    old = NULL;
    while (p)
    {
       if (strcmp(p->symbolName, i->symbolName) < 0)
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
    SymbolListTail = i;
    return SymbolListHead;

}

SYMBOL *insertSymbolName(SYMBOL *i, SYMBOL *top)
{
    SYMBOL *old, *p;

    if (!SymbolNameTail)
    {
       i->nameNext = i->namePrior = NULL;
       SymbolNameTail = i;
       return i;
    }
    p = top;
    old = NULL;
    while (p)
    {
       if (strcmp(p->symbolName, i->symbolName) < 0)
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
    SymbolNameTail = i;
    return SymbolNameHead;

}

SYMBOL *insertSymbolAddress(SYMBOL *i, SYMBOL *top)
{
    SYMBOL *old, *p;

    if (!SymbolAddressTail)
    {
       i->addressNext = i->addressPrior = NULL;
       SymbolAddressTail = i;
       return i;
    }
    p = top;
    old = NULL;
    while (p)
    {
       if (p->symbolAddress < i->symbolAddress)
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
    SymbolAddressTail = i;
    return SymbolAddressHead;

}

LONG AddPublicSymbol(SYMBOL *NewSymbol)
{
    register MODULE_HANDLE *module = NewSymbol->ModuleContext;
    register SYMBOL *Symbol;
    register LONG flags;

    if (module->ModuleSignature != MODULE_SIGNATURE)
       return -3;

    if (NewSymbol->symbolSignature != SYMBOL_SIGNATURE)
       return -1;

    flags = get_flags();
    spin_lock(&SymbolListLock);

    Symbol = SymbolListHead;
    while (Symbol)
    {
       if (Symbol == NewSymbol)
       {
	  spin_unlock(&SymbolListLock);  // already exists
	  set_flags(flags);
	  return 1;
       }
       Symbol = Symbol->next;
    }
    SymbolListHead = insertSymbol(NewSymbol, SymbolListHead);
    SymbolNameHead = insertSymbolName(NewSymbol, SymbolNameHead);
    SymbolAddressHead = insertSymbolAddress(NewSymbol, SymbolAddressHead);

    spin_unlock(&SymbolListLock);
    set_flags(flags);
    return 0;
}

LONG RemovePublicSymbol(SYMBOL *symbol)
{
    register SYMBOL *searchSymbol;
    register LONG flags;

    if (symbol->symbolSignature != SYMBOL_SIGNATURE)
       return -1;

    flags = get_flags();
    spin_lock(&SymbolListLock);

    searchSymbol = SymbolListHead;
    while (searchSymbol)
    {
       if (searchSymbol == symbol)   // found, remove from list
       {
	  if (SymbolListHead == symbol)
	  {
	     SymbolListHead = (void *) symbol->next;
	     if (SymbolListHead)
		SymbolListHead->prior = NULL;
	     else
		SymbolListTail = NULL;
	  }
	  else
	  {
	     symbol->prior->next = symbol->next;
	     if (symbol != SymbolListTail)
		symbol->next->prior = symbol->prior;
	     else
		SymbolListTail = symbol->prior;
	  }
	  symbol->next = symbol->prior = 0;
	  break;
       }
       searchSymbol = searchSymbol->next;
    }

    searchSymbol = SymbolNameHead;
    while (searchSymbol)
    {
       if (searchSymbol == symbol)   // found, remove from list
       {
	  if (SymbolNameHead == symbol)
	  {
	     SymbolNameHead = (void *) symbol->nameNext;
	     if (SymbolNameHead)
		SymbolNameHead->namePrior = NULL;
	     else
		SymbolNameTail = NULL;
	  }
	  else
	  {
	     symbol->namePrior->nameNext = symbol->nameNext;
	     if (symbol != SymbolNameTail)
		symbol->nameNext->namePrior = symbol->namePrior;
	     else
		SymbolNameTail = symbol->namePrior;
	  }
	  symbol->nameNext = symbol->namePrior = 0;
	  break;
       }
       searchSymbol = searchSymbol->nameNext;
    }

    searchSymbol = SymbolAddressHead;
    while (searchSymbol)
    {
       if (searchSymbol == symbol)   // found, remove from list
       {
	  if (SymbolAddressHead == symbol)
	  {
	     SymbolAddressHead = (void *) symbol->addressNext;
	     if (SymbolAddressHead)
		SymbolAddressHead->addressPrior = NULL;
	     else
		SymbolAddressTail = NULL;
	  }
	  else
	  {
	     symbol->addressPrior->addressNext = symbol->addressNext;
	     if (symbol != SymbolAddressTail)
		symbol->addressNext->addressPrior = symbol->addressPrior;
	     else
		SymbolAddressTail = symbol->addressPrior;
	  }
	  symbol->addressNext = symbol->addressPrior = 0;
	  break;
       }
       searchSymbol = searchSymbol->addressNext;
    }

    spin_unlock(&SymbolListLock);
    set_flags(flags);
    return 0;
}

void DumpOSSymbolTable(SCREEN *screen)
{
    register LONG flags;
    register SYMBOL *symbol;
    MODULE_HANDLE *module;
    extern BYTE *GetSymbolDescription(LONG type);

    SetPauseMode(screen, screen->nLines - 3);
    flags = get_flags();
    spin_lock(&SymbolListLock);

    symbol = SymbolNameHead;
    while (symbol)
    {
       module = symbol->ModuleContext;
       if (printfScreenWithAttribute(screen,
				     LTGREEN,
				     "%08X %s %s (%s)\n",
				     symbol->symbolAddress,
				     GetSymbolDescription(symbol->Type),
				     symbol->symbolName,
				     module->ModuleShortName))
	  break;
       symbol = symbol->nameNext;
    }
    spin_lock(&SymbolListLock);
    set_flags(flags);
    ClearPauseMode(screen);
    return;

}

void DumpOSSymbolTableMatch(SCREEN *screen, BYTE *match)
{
    register LONG flags;
    register SYMBOL *symbol;
    MODULE_HANDLE *module;
    extern BYTE *GetSymbolDescription(LONG type);

    SetPauseMode(screen, screen->nLines - 3);

    printfScreenWithAttribute(screen,
			      LTGREEN,
			      "Symbols That Match '%s'\n",
			      match);

    flags = get_flags();
    spin_lock(&SymbolListLock);

    symbol = SymbolNameHead;
    while (symbol)
    {
       module = symbol->ModuleContext;
       if (!strnicmp(match, symbol->symbolName, strlen(match)))
       {
	  if (printfScreenWithAttribute(screen,
					LTGREEN,
					"%08X %s %s (%s)\n",
					symbol->symbolAddress,
					GetSymbolDescription(symbol->Type),
					symbol->symbolName,
					module->ModuleShortName))
	     break;
       }
       symbol = symbol->nameNext;
    }
    spin_lock(&SymbolListLock);
    set_flags(flags);
    ClearPauseMode(screen);
    return;
}

void RemoveModuleSymbols(MODULE_HANDLE *module)
{
    register SYMBOL *symbolList, *symbol;
    register LONG flags;
    extern BYTE *GetSymbolDescription(LONG type);

    flags = get_flags();
    spin_lock(&SymbolListLock);
    symbol = SymbolListHead;
    while (symbol)
    {
       symbolList = symbol->next;
       if (symbol->ModuleContext == module)
       {
	  if (SymbolListHead == symbol)
	  {
	     SymbolListHead = (void *) symbol->next;
	     if (SymbolListHead)
		SymbolListHead->prior = NULL;
	     else
		SymbolListTail = NULL;
	  }
	  else
	  {
	     symbol->prior->next = symbol->next;
	     if (symbol != SymbolListTail)
		symbol->next->prior = symbol->prior;
	     else
		SymbolListTail = symbol->prior;
	  }
	  symbol->next = symbol->prior = 0;

	  if (SymbolNameHead == symbol)
	  {
	     SymbolNameHead = (void *) symbol->nameNext;
	     if (SymbolNameHead)
		SymbolNameHead->namePrior = NULL;
	     else
		SymbolNameTail = NULL;
	  }
	  else
	  {
	     symbol->namePrior->nameNext = symbol->nameNext;
	     if (symbol != SymbolNameTail)
		symbol->nameNext->namePrior = symbol->namePrior;
	     else
		SymbolNameTail = symbol->namePrior;
	  }
	  symbol->nameNext = symbol->namePrior = 0;

	  if (SymbolAddressHead == symbol)
	  {
	     SymbolAddressHead = (void *) symbol->addressNext;
	     if (SymbolAddressHead)
		SymbolAddressHead->addressPrior = NULL;
	     else
		SymbolAddressTail = NULL;
	  }
	  else
	  {
	     symbol->addressPrior->addressNext = symbol->addressNext;
	     if (symbol != SymbolAddressTail)
		symbol->addressNext->addressPrior = symbol->addressPrior;
	     else
		SymbolAddressTail = symbol->addressPrior;
	  }
	  symbol->addressNext = symbol->addressPrior = 0;

	  kfree(symbol);
       }
       symbol = symbolList;
    }
    spin_unlock(&SymbolListLock);
    set_flags(flags);
    return;

}

LONG GetValueFromSymbol(BYTE *symbolName)
{
    register flags, len = strlen(symbolName);
    SYMBOL *symbol;

    flags = get_flags();
    spin_lock(&SymbolListLock);

    symbol = SymbolNameHead;
    while (symbol)
    {
       if (symbol->symbolNameLength == len)
       {
	  if (!strcmp(symbol->symbolName, symbolName))
	  {
	     spin_unlock(&SymbolListLock);
	     set_flags(flags);
	     return symbol->symbolAddress;
	  }
       }
       symbol = symbol->nameNext;
    }

    spin_unlock(&SymbolListLock);
    set_flags(flags);
    return -1;
}

BYTE *GetSymbolFromValue(LONG value)
{
    register flags;
    SYMBOL *symbol;

    flags = get_flags();
    spin_lock(&SymbolListLock);

    symbol = SymbolAddressHead;
    while (symbol)
    {
       if (symbol->symbolAddress == value)
       {
	  spin_unlock(&SymbolListLock);
	  set_flags(flags);
	  return symbol->symbolName;
       }
       symbol = symbol->addressNext;
    }
    spin_unlock(&SymbolListLock);
    set_flags(flags);
    return 0;
}


void DisplayClosestSymbols(SCREEN *screen, LONG address)
{

    register flags;
    register SYMBOL *currentSymbol = 0;
    register SYMBOL *priorSymbol = 0;
    register SYMBOL *nextSymbol;

    flags = get_flags();
    spin_lock(&SymbolListLock);

    nextSymbol = SymbolAddressHead;
    while (nextSymbol)
    {
       if ((nextSymbol->InternalType == SYMBOL_PROCEDURE ||
	    nextSymbol->InternalType == SYMBOL_DATA ||
	    nextSymbol->InternalType == SYMBOL_ENTRY) &&
	    nextSymbol->symbolAddress > address)
       {
	     if (priorSymbol)
		printfScreenWithAttribute(screen, LTGREEN,
				    "Previous (%08X %c %08X) %s\n",
				    address,
				    priorSymbol->symbolAddress < address
				    ? '-' : '+',
				    priorSymbol->symbolAddress < address
				    ? address - priorSymbol->symbolAddress
				    : priorSymbol->symbolAddress - address,
				    priorSymbol->symbolName);

	     if (currentSymbol)
		printfScreenWithAttribute(screen, LTGREEN,
				    "Current  (%08X %c %08X) %s\n",
				    address,
				    currentSymbol->symbolAddress < address
				    ? '-' : '+',
				    currentSymbol->symbolAddress < address
				    ? address - currentSymbol->symbolAddress
				    : currentSymbol->symbolAddress - address,
				    currentSymbol->symbolName);

	     if (nextSymbol)
		printfScreenWithAttribute(screen, LTGREEN,
				    "Next     (%08X %c %08X) %s\n",
				    address,
				    nextSymbol->symbolAddress < address
				    ? '-' : '+',
				    nextSymbol->symbolAddress < address
				    ? address - nextSymbol->symbolAddress
				    : nextSymbol->symbolAddress - address,
				    nextSymbol->symbolName);

	  spin_unlock(&SymbolListLock);
	  set_flags(flags);
	  return;
       }
       if (currentSymbol && currentSymbol->InternalType == SYMBOL_PROCEDURE ||
	   currentSymbol->InternalType == SYMBOL_DATA)
	  priorSymbol = currentSymbol;

       if (nextSymbol && nextSymbol->InternalType == SYMBOL_PROCEDURE ||
	   nextSymbol->InternalType == SYMBOL_DATA)
	  currentSymbol = nextSymbol;

       nextSymbol = nextSymbol->addressNext;
    }
    spin_unlock(&SymbolListLock);
    set_flags(flags);
    return;

}

void DisplayClosestSymbol(SCREEN *screen, LONG address)
{

    register flags;
    register SYMBOL *symbol, *lastSymbol;

    flags = get_flags();
    spin_lock(&SymbolListLock);

    symbol = SymbolAddressHead;
    while (symbol)
    {
       if ((symbol->InternalType == SYMBOL_PROCEDURE ||
	    symbol->InternalType == SYMBOL_DATA) &&
	    symbol->symbolAddress > address)
       {
	  if (lastSymbol &&
	     (lastSymbol->InternalType == SYMBOL_PROCEDURE ||
	      lastSymbol->InternalType == SYMBOL_DATA      ||
	      lastSymbol->InternalType == SYMBOL_ENTRY))
	     printfScreenWithAttribute(screen,
				      (lastSymbol->InternalType == SYMBOL_PROCEDURE ||
				       lastSymbol->InternalType == SYMBOL_ENTRY)
				       ? LTGREEN
				       : BRITEWHITE,
				       "%08X (%s %c %08X)",
				       lastSymbol->symbolAddress,
				       lastSymbol->symbolName,
				       lastSymbol->symbolAddress > address
				       ? '-' : '+',
				       lastSymbol->symbolAddress < address
				       ? address - lastSymbol->symbolAddress
				       : lastSymbol->symbolAddress - address);
	  spin_unlock(&SymbolListLock);
	  set_flags(flags);
	  return;
       }
       if (symbol->InternalType == SYMBOL_PROCEDURE || symbol->InternalType == SYMBOL_DATA)
	  lastSymbol = symbol;
       symbol = symbol->addressNext;
    }
    spin_unlock(&SymbolListLock);
    set_flags(flags);
    return;

}

