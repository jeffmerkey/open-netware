
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
*   FILE     :  SYMBOLS.H
*   DESCRIP  :  Symbol Library for MANOS v1.0
*   DATE     :  August 22, 1998
*
*
***************************************************************************/

#include "types.h"

#define SYMBOL_SIGNATURE 0xFFEEDDCC

#define SYMBOL_PROCEDURE    1
#define SYMBOL_DATA         2
#define SYMBOL_CONTROL      4
#define SYMBOL_ENTRY        8

typedef struct _FORWARDER {
   struct _FORWARDER *next;
   struct _FORWARDER *prior;
   BYTE *symbolName;
   LONG symbolNameLength;
   LONG Type;
   LONG symbolType;
   LONG InternalType;
   LONG symbolFormat;
   void *symbolLink;
   LONG symbolAddress;
} FORWARDER;

typedef struct _SYMBOL {
   struct _SYMBOL *next;
   struct _SYMBOL *prior;
   struct _SYMBOL *addressNext;    // address forward search link
   struct _SYMBOL *addressPrior;   // address previous search link
   struct _SYMBOL *nameNext;       // name forward search link
   struct _SYMBOL *namePrior;      // name previous search link
   BYTE *symbolName;
   LONG symbolNameLength;
   LONG Type;
   void *ModuleContext;
   LONG symbolType;
   LONG symbolFormat;
   void *symbolLink;
   LONG symbolAddress;
   LONG symbolSignature;
   LONG ForwarderListLock;
   FORWARDER *ForwarderListHead;
   FORWARDER *ForwarderListTail;
   LONG InternalType;
} SYMBOL;

extern SYMBOL *CreatePublicSymbol(MODULE_HANDLE *module,
			   BYTE *SymbolName,
			   LONG SymbolAddress,
			   LONG Type,
			   LONG SymbolFormat,
			   void *SymbolLink,
			   LONG SymbolType,
			   LONG InternalType);

extern SYMBOL *InitCreatePublicSymbol(MODULE_HANDLE *module,
			   BYTE *SymbolName,
			   LONG SymbolAddress,
			   LONG Type,
			   LONG SymbolFormat,
			   void *SymbolLink,
			   LONG SymbolType,
			   LONG InternalType);

extern SYMBOL *insertSymbol(SYMBOL *i, SYMBOL *top);
extern SYMBOL *insertSymbolName(SYMBOL *i, SYMBOL *top);
extern SYMBOL *insertSymbolAddress(SYMBOL *i, SYMBOL *top);
extern LONG AddPublicSymbol(SYMBOL *NewSymbol);
extern LONG RemovePublicSymbol(SYMBOL *NewSymbol);
extern void RemoveModuleSymbols(MODULE_HANDLE *module);
