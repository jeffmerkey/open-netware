
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  EXPRESS.C
*   DESCRIP  :  Expression Parser and Evaluation Routines for MANOS v1.0
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
#include "dos.h"
#include "tss.h"
#include "types.h"

extern LONG get_flags(void);
extern void set_flags(LONG);
extern void spin_lock(LONG *);
extern void spin_unlock(LONG *);

#define PROCESSOR_32        32
#define PROCESSOR_64        64

#define DEBUG_EXPRESS       0
#define DEBUG_BOOL          0
#define DEBUG_LOGICAL       0
#define DEBUG_BOOL_STACK    0

#define INVALID_EXPRESSION  0
#define NUMERIC_EXPRESSION  1
#define BOOLEAN_EXPRESSION  2

#define NUM_STACK_SIZE      50
#define CONTEXT_STACK_SIZE  100
#define BOOL_STACK_SIZE     50
#define LOGICAL_STACK_SIZE  50

#define NULL_TOKEN          0
#define NUMBER_TOKEN        1
#define MINUS_TOKEN         2
#define PLUS_TOKEN          3
#define MULTIPLY_TOKEN      4
#define DIVIDE_TOKEN        5
#define GREATER_TOKEN       6
#define LESS_TOKEN          7
#define XOR_TOKEN           8
#define AND_TOKEN           9
#define OR_TOKEN            10
#define NOT_TOKEN           11
#define NEG_TOKEN           12
#define EQUAL_TOKEN         13
#define LEFT_SHIFT_TOKEN    14
#define RIGHT_SHIFT_TOKEN   15
#define SPACE_TOKEN         16
#define FLAGS_TOKEN         17
#define EAX_TOKEN           18
#define EBX_TOKEN           19
#define ECX_TOKEN           20
#define EDX_TOKEN           21
#define ESI_TOKEN           22
#define EDI_TOKEN           23
#define EBP_TOKEN           24
#define ESP_TOKEN           25
#define CS_TOKEN            26
#define DS_TOKEN            27
#define ES_TOKEN            28
#define FS_TOKEN            29
#define GS_TOKEN            30
#define SS_TOKEN            31
#define DREF_OPEN_TOKEN     32
#define DREF_CLOSE_TOKEN    33
#define MOD_TOKEN           34
#define NUMBER_END          35
#define GREATER_EQUAL_TOKEN 36
#define LESS_EQUAL_TOKEN    37
#define EIP_TOKEN           38
#define ASSIGNMENT_TOKEN    39
#define DWORD_TOKEN         40
#define WORD_TOKEN          41
#define BYTE_TOKEN          42
#define LOGICAL_AND_TOKEN   43
#define LOGICAL_OR_TOKEN    44
#define CF_TOKEN            45
#define PF_TOKEN            46
#define AF_TOKEN            47
#define ZF_TOKEN            48
#define SF_TOKEN            49
#define IF_TOKEN            50
#define DF_TOKEN            51
#define OF_TOKEN            52
#define VM_TOKEN            53
#define AC_TOKEN            54
#define BB_TOKEN            55
#define EB_TOKEN            56
#define NOT_EQUAL_TOKEN     57

#define   CF_FLAG   0x00000001  // ????
#define   PF_FLAG   0x00000004
#define   AF_FLAG   0x00000010  // ????
#define   ZF_FLAG   0x00000040
#define   SF_FLAG   0x00000080
#define   TF_FLAG   0x00000100  // ss flag
#define   IF_FLAG   0x00000200
#define   DF_FLAG   0x00000400  // ????
#define   OF_FLAG   0x00000800
#define   NT_FLAG   0x00004000
#define   RF_FLAG   0x00010000  // resume flag
#define   VM_FLAG   0x00020000
#define   AC_FLAG   0x00040000  // ????
#define   VIF_FLAG  0x00080000
#define   VIP_FLAG  0x00100000
#define   ID_FLAG   0x00200000

#define   LONG_PTR          0
#define   WORD_PTR          1
#define   BYTE_PTR          2
#define   CLASS_DATA        1
#define   CLASS_ASSIGN      2
#define   CLASS_PARTITION   3
#define   CLASS_ARITHMETIC  4
#define   CLASS_BOOLEAN     5

BYTE *exprDescription[]={
     "INVALID",
     "NUMERIC",
     "BOOLEAN",
     "???????",
};

BYTE *parserDescription[]={
     "NULL_TOKEN",
     "NUMBER_TOKEN",
     "MINUS_TOKEN",
     "PLUS_TOKEN",
     "MULTIPLY_TOKEN",
     "DIVIDE_TOKEN",
     "GREATER_TOKEN",
     "LESS_TOKEN",
     "XOR_TOKEN",
     "AND_TOKEN",
     "OR_TOKEN",
     "NOT_TOKEN",
     "NEG_TOKEN",
     "EQUAL_TOKEN",
     "LEFT_SHIFT_TOKEN",
     "RIGHT_SHIFT_TOKEN",
     "SPACE_TOKEN",
     "FLAGS_TOKEN",
     "EAX_TOKEN",
     "EBX_TOKEN",
     "ECX_TOKEN",
     "EDX_TOKEN",
     "ESI_TOKEN",
     "EDI_TOKEN",
     "EBP_TOKEN",
     "ESP_TOKEN",
     "CS_TOKEN",
     "DS_TOKEN",
     "ES_TOKEN",
     "FS_TOKEN",
     "GS_TOKEN",
     "SS_TOKEN",
     "DREF_OPEN_TOKEN",
     "DREF_CLOSE_TOKEN",
     "MOD_TOKEN",
     "NUMBER_END",
     "GREATER_EQUAL_TOKEN",
     "LESS_EQUAL_TOKEN",
     "EIP_TOKEN",
     "ASSIGNMENT_TOKEN",
     "DWORD_TOKEN",
     "WORD_TOKEN",
     "BYTE_TOKEN",
     "LOGICAL_AND_TOKEN",
     "LOGICAL_OR_TOKEN",
     "CF_TOKEN",
     "PF_TOKEN",
     "AF_TOKEN",
     "ZF_TOKEN",
     "SF_TOKEN",
     "IF_TOKEN",
     "DF_TOKEN",
     "OF_TOKEN",
     "VM_TOKEN",
     "AC_TOKEN",
     "BEGIN_BRACKET",
     "END_BRACKET",
     "NOT_EQUAL_TOKEN"
};

BYTE TokenIndex[256];
BYTE TokenClass[256];
BYTE TokenType[256];
LONG TokenValue[256];
LONG TokenCount;

LONG numStack[NUM_STACK_SIZE];
LONG *sp;
LONG *tos;
LONG *bos;

LONG contextStack[CONTEXT_STACK_SIZE];
LONG *c_sp;
LONG *c_tos;
LONG *c_bos;

LONG booleanStack[BOOL_STACK_SIZE];
LONG *b_sp;
LONG *b_tos;
LONG *b_bos;

LONG logicalStack[LOGICAL_STACK_SIZE];
LONG *l_sp;
LONG *l_tos;
LONG *l_bos;

extern void CopyData(LONG *src, LONG *dest, LONG len);
extern void CopyDataB(LONG *src, LONG *dest, LONG len);
extern void SetData(LONG *dest, LONG value, LONG len);
extern void SetDataB(LONG *dest, LONG value, LONG len);
extern void SetDataD(LONG *dest, LONG value, LONG len);

extern long printfScreenWithAttribute(void *screen, LONG attr, char *format, ...);
extern long printfScreen(void *screen, char *format, ...);
extern long issymbol(long chr);

extern LONG GetValueFromSymbol(BYTE *symbolName);

LONG expressLock = 0;

LONG GetNumber(BYTE *p, BYTE **rp, LONG *opl)
{

    BYTE *op;
    LONG c = 0, decimal = 0, hex_found = 0;

    op = p;
    while (*p)
    {
       if (*p >= '0' && *p <= '9')
	  p++;
       else if (*p >= 'A' && *p <= 'F')
       {
	  hex_found = 1;
	  p++;
       }
       else if (*p >= 'a' && *p <= 'f')
       {
	  hex_found = 1;
	  p++;
       }
       else if (*p == 'R')
       {
	  decimal = 1;
	  p++;
       }
       else if (*p == 'r')
       {
	  decimal = 1;
	  p++;
       }
       else
	  break;
    }
    if (rp)
       *rp = p;
    if (opl)
       *opl = (LONG)((LONG)p - (LONG) op);

    p = op;
    if ((decimal) && (!hex_found))
    {
       // parse as decimal number;
       while (*p)
       {
	     if (*p >= '0' && *p <= '9')
		c = (c * 10) + (*p - '0');
	     else
		break;
	  p++;
       }
    }
    else
    {
       // parse as hex number;
       while (*p)
       {
	  if (*p >= '0' && *p <= '9')
	     c = (c << 4) | (*p - '0');
	  else if (*p >= 'A' && *p <= 'F')
	     c = (c << 4) | (*p - 'A' + 10);
	  else if (*p >= 'a' && *p <= 'f')
	     c = (c << 4) | (*p - 'a' + 10);
	  else
	     break;
	  p++;
       }
    }
    return (c);

}


BYTE *parseTokens(StackFrame *stackFrame, BYTE *p)
{

    register LONG i, value;
    BYTE symbol[100], *sym, *symt;
    BYTE *tmp, *op;
    LONG delta;

    op = p;
    TokenCount = 0;
    SetData((LONG *) &TokenValue[0], 0, 256);
    SetData((LONG *) &TokenType[0], 0, 256);
    SetData((LONG *) &TokenIndex[0], 0, 256);
    SetData((LONG *) &TokenClass[0], 0, 256);

    while (TokenCount < 200 && (LONG)p - (LONG)op < 200)
    {

       // symbols never start with a number

       if (isalpha(*p) || *p == '_' ||  *p == '@' || *p == '$')
       {
	  sym = p;
	  symt = symbol;
	  for (i=0; i < 100 && issymbol(*sym); i++)
	     *symt++ = *sym++;
	  *symt = '\0';
	  value = GetValueFromSymbol(symbol);
	  if (value != (LONG) -1)
	  {
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenValue[TokenCount] = value;
	     TokenType[TokenCount++] = NUMBER_TOKEN;
	     p = sym;   // bump the pointer past the symbol
	  }
       }

       if (stackFrame)
       {
	 switch (*p)
	 {

	  case '\0':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = NULL_TOKEN;
	     return (p);

	  case ']':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = DREF_CLOSE_TOKEN;
	     p++;
	     break;

	  case '(':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = BB_TOKEN;
	     p++;
	     break;

	  case ')':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = EB_TOKEN;
	     p++;
	     break;

	  case '+':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = PLUS_TOKEN;
	     p++;
	     break;

	  case '-':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = MINUS_TOKEN;
	     p++;
	     break;

	  case '*':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = MULTIPLY_TOKEN;
	     p++;
	     break;

	  case '/':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = DIVIDE_TOKEN;
	     p++;
	     break;

	  case '%':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = MOD_TOKEN;
	     p++;
	     break;

	  case '~':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = NEG_TOKEN;
	     p++;
	     break;

	  case '^':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = XOR_TOKEN;
	     p++;
	     break;

	  case '!':
	     p++;
	     if (*p == '=')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = NOT_EQUAL_TOKEN;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = NOT_TOKEN;
	     break;

	  case ' ':   // drop spaces on the floor
	     p++;
	     break;

	  //
	  //  These cases require special handling
	  //

	  case 'p':
	  case 'P':
	     p++;
	     if (*p == 'F' || *p == 'f')
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tSystemFlags & PF_FLAG;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = PF_TOKEN;
		p++;
		break;
	     }
	     break;

	  case 'z':
	  case 'Z':
	     p++;
	     if (*p == 'F' || *p == 'f')
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tSystemFlags & ZF_FLAG;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = ZF_TOKEN;
		p++;
		break;
	     }
	     break;

	  case 'i':
	  case 'I':
	     p++;
	     if (*p == 'F' || *p == 'f')
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tSystemFlags & IF_FLAG;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = IF_TOKEN;
		p++;
		break;
	     }
	     break;

	  case 'o':
	  case 'O':
	     p++;
	     if (*p == 'F' || *p == 'f')
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tSystemFlags & OF_FLAG;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = OF_TOKEN;
		p++;
		break;
	     }
	     break;

	  case 'v':
	  case 'V':
	     p++;
	     if (*p == 'M' || *p == 'm')
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tSystemFlags & VM_FLAG;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = VM_TOKEN;
		p++;
		break;
	     }
	     break;

	  case '0':
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	  case '8':
	  case '9':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenValue[TokenCount] = GetNumber(p, &p, &delta);
	     TokenType[TokenCount++] = NUMBER_TOKEN;
	     break;

	  case 'a':
	  case 'A':
	     tmp = p;
	     tmp++;
	     if ((*tmp == 'F' || *tmp == 'f') && (*(tmp + 1) == ' ' || *(tmp + 1) == '=' ))
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tSystemFlags & AF_FLAG;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = AF_TOKEN;
		p++;
		p++;
		break;
	     }
	     if ((*tmp == 'C' || *tmp == 'c') && (*(tmp + 1) == ' ' || *(tmp + 1) == '=' ))
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tSystemFlags & AC_FLAG;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = AC_TOKEN;
		p++;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenValue[TokenCount] = GetNumber(p, &p, &delta);
	     TokenType[TokenCount++] = NUMBER_TOKEN;
	     break;


	  case 'b':
	  case 'B':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenValue[TokenCount] = GetNumber(p, &p, &delta);
	     TokenType[TokenCount++] = NUMBER_TOKEN;
	     break;

	  case 'c':
	  case 'C':
	     tmp = p;
	     tmp++;
	     if (*tmp == 'S' || *tmp == 's')
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tCS;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = CS_TOKEN;
		p++;
		p++;
		break;
	     }
	     if ((*tmp == 'F' || *tmp == 'f') && (*(tmp + 1) == ' ' || *(tmp + 1) == '=' ))
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tSystemFlags & CF_FLAG;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = CF_TOKEN;
		p++;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenValue[TokenCount] = GetNumber(p, &p, &delta);
	     TokenType[TokenCount++] = NUMBER_TOKEN;
	     break;

	  case 'd':
	  case 'D':
	     tmp = p;
	     tmp++;
	     if (*tmp == 'S' || *tmp == 's')
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tDS;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = DS_TOKEN;
		p++;
		p++;
		break;
	     }
	     if ((*tmp == 'F' || *tmp == 'f') && (*(tmp + 1) == ' ' || *(tmp + 1) == '=' ))
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tSystemFlags & DF_FLAG;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = DF_TOKEN;
		p++;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenValue[TokenCount] = GetNumber(p, &p, &delta);
	     TokenType[TokenCount++] = NUMBER_TOKEN;
	     break;

	  case 'e':
	  case 'E':
	     tmp = p;
	     tmp++;
	     if (*tmp == 'A' || *tmp == 'a')
	     {
		tmp++;
		if (*tmp == 'X' || *tmp == 'x')
		{
		   if (stackFrame)
		      TokenValue[TokenCount] = stackFrame->tEAX;
		   TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		   TokenType[TokenCount++] = EAX_TOKEN;
		   p++;
		   p++;
		   p++;
		   break;
		}
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenValue[TokenCount] = GetNumber(p, &p, &delta);
		TokenType[TokenCount++] = NUMBER_TOKEN;
		break;
	     }
	     if (*tmp == 'B' || *tmp == 'b')
	     {
		tmp++;
		if (*tmp == 'X' || *tmp == 'x')
		{
		   if (stackFrame)
		      TokenValue[TokenCount] = stackFrame->tEBX;
		   TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		   TokenType[TokenCount++] = EBX_TOKEN;
		   p++;
		   p++;
		   p++;
		   break;
		}
		if (*tmp == 'P' || *tmp == 'p')
		{
		   if (stackFrame)
		      TokenValue[TokenCount] = stackFrame->tEBP;
		   TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		   TokenType[TokenCount++] = EBP_TOKEN;
		   p++;
		   p++;
		   p++;
		   break;
		}
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenValue[TokenCount] = GetNumber(p, &p, &delta);
		TokenType[TokenCount++] = NUMBER_TOKEN;
		break;
	     }
	     if (*tmp == 'C' || *tmp == 'c')
	     {
		tmp++;
		if (*tmp == 'X' || *tmp == 'x')
		{
		   if (stackFrame)
		      TokenValue[TokenCount] = stackFrame->tECX;
		   TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		   TokenType[TokenCount++] = ECX_TOKEN;
		   p++;
		   p++;
		   p++;
		   break;
		}
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenValue[TokenCount] = GetNumber(p, &p, &delta);
		TokenType[TokenCount++] = NUMBER_TOKEN;
		break;
	     }
	     if (*tmp == 'D' || *tmp == 'd')
	     {
		tmp++;
		if (*tmp == 'X' || *tmp == 'x')
		{
		   if (stackFrame)
		      TokenValue[TokenCount] = stackFrame->tEDX;
		   TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		   TokenType[TokenCount++] = EDX_TOKEN;
		   p++;
		   p++;
		   p++;
		   break;
		}
		if (*tmp == 'I' || *tmp == 'i')
		{
		   if (stackFrame)
		      TokenValue[TokenCount] = stackFrame->tEDI;
		   TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		   TokenType[TokenCount++] = EDI_TOKEN;
		   p++;
		   p++;
		   p++;
		   break;
		}
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenValue[TokenCount] = GetNumber(p, &p, &delta);
		TokenType[TokenCount++] = NUMBER_TOKEN;
		break;
	     }
	     if (*tmp == 'S' || *tmp == 's')
	     {
		tmp++;
		if (*tmp == 'P' || *tmp == 'p')
		{
		   if (stackFrame)
		      TokenValue[TokenCount] = stackFrame->tESP;
		   TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		   TokenType[TokenCount++] = ESP_TOKEN;
		   p++;
		   p++;
		   p++;
		   break;
		}
		if (*tmp == 'I' || *tmp == 'i')
		{
		   if (stackFrame)
		      TokenValue[TokenCount] = stackFrame->tESI;
		   TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		   TokenType[TokenCount++] = ESI_TOKEN;
		   p++;
		   p++;
		   p++;
		   break;
		}
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tES;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = ES_TOKEN;
		p++;
		p++;
		break;
	     }
	     if (*tmp == 'I' || *tmp == 'i')
	     {
		tmp++;
		if (*tmp == 'P' || *tmp == 'p')
		{
		   if (stackFrame)
		      TokenValue[TokenCount] = stackFrame->tEIP;
		   TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		   TokenType[TokenCount++] = EIP_TOKEN;
		   p++;
		   p++;
		   p++;
		   break;
		}
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenValue[TokenCount] = GetNumber(p, &p, &delta);
	     TokenType[TokenCount++] = NUMBER_TOKEN;
	     break;

	  case 'f':
	  case 'F':
	     tmp = p;
	     tmp++;
	     if (*tmp == 'S' || *tmp == 's')
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tFS;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = FS_TOKEN;
		p++;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenValue[TokenCount] = GetNumber(p, &p, &delta);
	     TokenType[TokenCount++] = NUMBER_TOKEN;
	     break;

	  case 'g':
	  case 'G':   // GS:
	     p++;
	     if (*p == 'S' || *p == 's')
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tGS;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = GS_TOKEN;
		p++;
		break;
	     }
	     break;

	  case 's':
	  case 'S':
	     p++;
	     if (*p == 'S' || *p == 's')
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tSS;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = SS_TOKEN;
		p++;
		break;
	     }
	     if (*p == 'F' || *p == 'f')
	     {
		if (stackFrame)
		   TokenValue[TokenCount] = stackFrame->tSystemFlags & SF_FLAG;
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = SF_TOKEN;
		p++;
		break;
	     }
	     break;

	  case '[':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = DREF_OPEN_TOKEN;
	     p++;
	     if (*p == 'D' || *p == 'd')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = DWORD_TOKEN;
		p++;
		break;
	     }
	     if (*p == 'W' || *p == 'w')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = WORD_TOKEN;
		p++;
		break;
	     }
	     if (*p == 'B' || *p == 'b')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = BYTE_TOKEN;
		p++;
		break;
	     }
	     break;

	  case '=':
	     p++;
	     if (*p == '=')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = EQUAL_TOKEN;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = ASSIGNMENT_TOKEN;
	     break;

	  case '<':
	     p++;
	     if (*p == '<')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = LEFT_SHIFT_TOKEN;
		p++;
		break;
	     }
	     if (*p == '=')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = LESS_EQUAL_TOKEN;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = LESS_TOKEN;
	     break;

	  case '>':
	     p++;
	     if (*p == '>')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = RIGHT_SHIFT_TOKEN;
		p++;
		break;
	     }
	     if (*p == '=')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = GREATER_EQUAL_TOKEN;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = GREATER_TOKEN;
	     break;

	  case '|':
	     p++;
	     if (*p == '|')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = LOGICAL_OR_TOKEN;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = OR_TOKEN;
	     break;

	  case '&':
	     p++;
	     if (*p == '&')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = LOGICAL_AND_TOKEN;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = AND_TOKEN;
	     break;

	  default: // if we get a default, drop the character on the floor
	     p++;
	     break;

	 }
       }
       else
       {
	 switch (*p)
	 {
	  case '\0':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = NULL_TOKEN;
	     return (p);

	  case ']':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = DREF_CLOSE_TOKEN;
	     p++;
	     break;

	  case '(':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = BB_TOKEN;
	     p++;
	     break;

	  case ')':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = EB_TOKEN;
	     p++;
	     break;

	  case '+':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = PLUS_TOKEN;
	     p++;
	     break;

	  case '-':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = MINUS_TOKEN;
	     p++;
	     break;

	  case '*':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = MULTIPLY_TOKEN;
	     p++;
	     break;

	  case '/':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = DIVIDE_TOKEN;
	     p++;
	     break;

	  case '%':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = MOD_TOKEN;
	     p++;
	     break;

	  case '~':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = NEG_TOKEN;
	     p++;
	     break;

	  case '^':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = XOR_TOKEN;
	     p++;
	     break;

	  case '!':
	     p++;
	     if (*p == '=')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = NOT_EQUAL_TOKEN;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = NOT_TOKEN;
	     break;

	  case ' ':   // drop spaces on the floor
	     p++;
	     break;

	  //
	  //  These cases require special handling
	  //

	  case '0':
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	  case '8':
	  case '9':
	  case 'a':
	  case 'A':
	  case 'b':
	  case 'B':
	  case 'c':
	  case 'C':
	  case 'd':
	  case 'D':
	  case 'e':
	  case 'E':
	  case 'f':
	  case 'F':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenValue[TokenCount] = GetNumber(p, &p, &delta);
	     TokenType[TokenCount++] = NUMBER_TOKEN;
	     break;

	  case '[':
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = DREF_OPEN_TOKEN;
	     p++;
	     if (*p == 'D' || *p == 'd')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = DWORD_TOKEN;
		p++;
		break;
	     }
	     if (*p == 'W' || *p == 'w')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = WORD_TOKEN;
		p++;
		break;
	     }
	     if (*p == 'B' || *p == 'b')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = BYTE_TOKEN;
		p++;
		break;
	     }
	     break;

	  case '=':
	     p++;
	     if (*p == '=')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = EQUAL_TOKEN;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = ASSIGNMENT_TOKEN;
	     break;

	  case '<':
	     p++;
	     if (*p == '<')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = LEFT_SHIFT_TOKEN;
		p++;
		break;
	     }
	     if (*p == '=')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = LESS_EQUAL_TOKEN;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = LESS_TOKEN;
	     break;

	  case '>':
	     p++;
	     if (*p == '>')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = RIGHT_SHIFT_TOKEN;
		p++;
		break;
	     }
	     if (*p == '=')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = GREATER_EQUAL_TOKEN;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = GREATER_TOKEN;
	     break;

	  case '|':
	     p++;
	     if (*p == '|')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = LOGICAL_OR_TOKEN;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = OR_TOKEN;
	     break;

	  case '&':
	     p++;
	     if (*p == '&')
	     {
		TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
		TokenType[TokenCount++] = LOGICAL_AND_TOKEN;
		p++;
		break;
	     }
	     TokenIndex[TokenCount] = (LONG) ((LONG) p - (LONG) op);
	     TokenType[TokenCount++] = AND_TOKEN;
	     break;

	  default: // if we get a default, drop the character on the floor
	     p++;
	     break;

	 }
       }
    }
    return p;

}

void displayExpressionHelp(SCREEN *screen)
{

       SetPauseMode(screen, screen->nLines - 3);

       printfScreenWithAttribute(screen, BRITEWHITE, "Arithmetic Operators\n");
       printfScreenWithAttribute(screen, LTCYAN, "+   add\n");
       printfScreenWithAttribute(screen, LTCYAN, "-   subtract\n");
       printfScreenWithAttribute(screen, LTCYAN, "*   multiply\n");
       printfScreenWithAttribute(screen, LTCYAN, "/   divide\n");
       printfScreenWithAttribute(screen, LTCYAN, "<<  bit shift left\n");
       printfScreenWithAttribute(screen, LTCYAN, ">>  bit shift right\n");
       printfScreenWithAttribute(screen, LTCYAN, "|   OR operator\n");
       printfScreenWithAttribute(screen, LTCYAN, "&   AND operator\n");
       printfScreenWithAttribute(screen, LTCYAN, "^   XOR operator\n");
       printfScreenWithAttribute(screen, LTCYAN, "~   NEG operator\n");
       printfScreenWithAttribute(screen, LTCYAN, "%%   MODULO operator\n");
       printfScreenWithAttribute(screen, BRITEWHITE, "\nExample 1:\n");
       printfScreenWithAttribute(screen, LTCYAN, "(0)> .e (100 + 100)\n");
       printfScreenWithAttribute(screen, LTCYAN, "(0)> result = 0x200 (512)\n");
       printfScreenWithAttribute(screen, BRITEWHITE, "Example 2:\n");
       printfScreenWithAttribute(screen, LTCYAN, "(0)> .e (1 << 20)\n");
       printfScreenWithAttribute(screen, LTCYAN, "(0)> result = 0x00100000 (1,024,000)\n");
       printfScreenWithAttribute(screen, BRITEWHITE, "Example 3:\n");
       printfScreenWithAttribute(screen, LTCYAN, "(0)> .e (FEF023 & 100F)\n");
       printfScreenWithAttribute(screen, LTCYAN, "(0)> result = 0x1003 (4099)\n\n");
       printfScreenWithAttribute(screen, BRITEWHITE, "\nBoolean Operators (Conditional Breakpoint)\n");
       printfScreenWithAttribute(screen, LTCYAN, "==      is equal to\n");
       printfScreenWithAttribute(screen, LTCYAN, "!=      is not equal to\n");
       printfScreenWithAttribute(screen, LTCYAN, "!<expr> is not\n");
       printfScreenWithAttribute(screen, LTCYAN, ">       is greater than\n");
       printfScreenWithAttribute(screen, LTCYAN, "<       is less than\n");
       printfScreenWithAttribute(screen, LTCYAN, ">=      is greater than or equal to\n");
       printfScreenWithAttribute(screen, LTCYAN, "<=      if less than or equal to\n");
       printfScreenWithAttribute(screen, LTCYAN, "||      logical OR operator\n");
       printfScreenWithAttribute(screen, LTCYAN, "&&      logical AND operator\n\n");
       printfScreenWithAttribute(screen, LTCYAN, "all breakpoint conditions must be enclosed in brackets () to\n");
       printfScreenWithAttribute(screen, LTCYAN, "evaluate correctly\n");
       printfScreenWithAttribute(screen, BRITEWHITE, "\nExample 1 (Execute Breakpoint):\n");
       printfScreenWithAttribute(screen, LTCYAN, "(0)> b 37000 (EAX == 20 && EBX <= 4000)\n");
       printfScreenWithAttribute(screen, LTCYAN, "breakpoint will activate if condition is true (returns 1)\n");
       printfScreenWithAttribute(screen, BRITEWHITE, "\nExample 4 (IO Breakpoint):\n");
       printfScreenWithAttribute(screen, LTCYAN, "(0)> bi 3D4 (!EBX && [d ESI+40] != 2000)\n");
       printfScreenWithAttribute(screen, LTCYAN, "breakpoint will activate if condition is true (returns 1)\n\n\n");
       printfScreenWithAttribute(screen, BRITEWHITE, "\nRegister Operators\n\n");
       printfScreenWithAttribute(screen, LTCYAN, "EAX, EBX, ECX, EDX        - general registers\n");
       printfScreenWithAttribute(screen, LTCYAN, "ESI, EDI, EBP, ESP        - pointer registers\n");
       printfScreenWithAttribute(screen, LTCYAN, "EIP, <symbol>             - instruction pointer or symbol\n");
       printfScreenWithAttribute(screen, LTCYAN, "CS, DS, ES, FS, GS, SS    - segment registers\n");
       printfScreenWithAttribute(screen, LTCYAN, "CF, PF, AF, ZF, SF, IF    - flags\n");
       printfScreenWithAttribute(screen, LTCYAN, "DF, OF, VM, AC\n");
       printfScreenWithAttribute(screen, LTCYAN, "=                         - set equal to\n");
       printfScreenWithAttribute(screen, BRITEWHITE, "\nExample 1:\n");
       printfScreenWithAttribute(screen, LTCYAN, "(0)> EAX = 0032700 \n");
       printfScreenWithAttribute(screen, LTCYAN, "EAX changed to 0x0032700\n");
       printfScreenWithAttribute(screen, BRITEWHITE, "\nExample 2:\n");
       printfScreenWithAttribute(screen, LTCYAN, "(0)> u thread_switch\n");
       printfScreenWithAttribute(screen, LTCYAN, "unassembles function thread_switch\n");
       printfScreenWithAttribute(screen, BRITEWHITE, "\nExample 3 (Dump):\n");
       printfScreenWithAttribute(screen, LTCYAN, "(0)> d EBP+ECX\n");
       printfScreenWithAttribute(screen, LTCYAN, "(dumps [d EBP + ECX])\n");
       printfScreenWithAttribute(screen, LTCYAN, "[addr] 00 00 00 01 02 04 07 ...\n");
       printfScreenWithAttribute(screen, LTCYAN, "[addr] 00 34 56 00 7A 01 00 ...\n");
       printfScreenWithAttribute(screen, BRITEWHITE, "\nBracket Operators\n\n");
       printfScreenWithAttribute(screen, LTCYAN, "(       begin expression bracket\n");
       printfScreenWithAttribute(screen, LTCYAN, ")       end expression bracket\n");
       printfScreenWithAttribute(screen, LTCYAN, "[       begin pointer\n");
       printfScreenWithAttribute(screen, LTCYAN, "]       end pointer\n");
       printfScreenWithAttribute(screen, LTCYAN, "d       DWORD reference\n");
       printfScreenWithAttribute(screen, LTCYAN, "w       WORD reference\n");
       printfScreenWithAttribute(screen, LTCYAN, "b       BYTE reference\n");
       printfScreenWithAttribute(screen, LTCYAN, "<num>r  parse number as decimal not hex flag (e.g. 512r == 200)\n");
       printfScreenWithAttribute(screen, BRITEWHITE, "\nExample 1 (dump):\n");
       printfScreenWithAttribute(screen, LTCYAN, "(0)> d [d EAX+100r] \n");
       printfScreenWithAttribute(screen, LTCYAN, "[eax + 100 (dec)] 00 00 00 01 02 04 07 00\n");
       printfScreenWithAttribute(screen, BRITEWHITE, "\nExample 2 (dump):\n");
       printfScreenWithAttribute(screen, LTCYAN, "(0)> d [w 003400] \n");
       printfScreenWithAttribute(screen, LTCYAN, "[addr (hex)] 00 22 00 01 02 04 07 ...\n");
       printfScreenWithAttribute(screen, LTCYAN, "[addr (hex)] 00 31 A1 00 6A 05 00 ...\n");
       printfScreenWithAttribute(screen, BRITEWHITE, "\nExample 3 (break):\n");
       printfScreenWithAttribute(screen, LTCYAN, "(0)> b = 7A000 (EAX + ECX == 30) && ([d B8000+50]  == 0x07)\n");
       printfScreenWithAttribute(screen, LTCYAN, "breakpoint will activate if condition is true (returns 1)\n\n");

       ClearPauseMode(screen);

       return;

}

LONG deref(LONG type, LONG value)
{

   LONG *pd;
   WORD *pw;
   BYTE *pb;

   switch (type)
   {
      case LONG_PTR:
	 pd = (LONG *) value;
#if (DEBUG_EXPRESS)
	 printfScreen(debugScreen, "dref (d): [%08X]-> %08X\n", pd, *pd);
#endif
	 return (LONG) *pd;

      case WORD_PTR:
	 pw = (WORD *) value;
#if (DEBUG_EXPRESS)
	 printfScreen(debugScreen, "dref (w): [%08X]-> %04X\n", pw, *pw);
#endif
	 return (WORD) *pw;

      case BYTE_PTR:
	 pb = (BYTE *) value;
#if (DEBUG_EXPRESS)
	 printfScreen(debugScreen, "dref (b): [%08X]-> %02X\n", pb, *pb);
#endif
	 return (BYTE) *pb;

      default:
#if (DEBUG_EXPRESS)
	 printfScreen(debugScreen, "dref (?): [0]-> 0\n");
#endif
	 return 0;
   }

}

LONG ExpressPush(LONG i)
{
     if (sp > bos)
     {
#if (DEBUG_EXPRESS)
	printfScreen(debugScreen, "push : <err>\n");
#endif
	return 0;
     }
     *sp = i;
#if (DEBUG_EXPRESS)
     printfScreen(debugScreen, "push : %08X (%d)\n", *sp, *sp);
#endif
     sp++;
     return 1;
}

LONG ExpressPop(void)
{
    sp--;
    if (sp < tos)
    {
       sp++;
#if (DEBUG_EXPRESS)
       printfScreen(debugScreen, "pop  : <err>\n");
#endif
       return 0;
    }
#if (DEBUG_EXPRESS)
    printfScreen(debugScreen, "pop  : %08X (%d)\n", *sp, *sp);
#endif
    return *sp;

}

LONG ContextPush(LONG i)
{
     if (c_sp > c_bos)
     {
#if (DEBUG_EXPRESS)
	printfScreen(debugScreen, "cpush: <err>\n");
#endif
	return 0;
     }
     *c_sp = i;
#if (DEBUG_EXPRESS)
     printfScreen(debugScreen, "cpush: %08X (%d)\n", *c_sp, *c_sp);
#endif
     c_sp++;
     return 1;
}

LONG ContextPop(void)
{
    c_sp--;
    if (c_sp < c_tos)
    {
       c_sp++;
#if (DEBUG_EXPRESS)
       printfScreen(debugScreen, "cpop : <err>\n");
#endif
       return 0;
    }
#if (DEBUG_EXPRESS)
    printfScreen(debugScreen, "cpop : %08X (%d)\n", *c_sp, *c_sp);
#endif
    return *c_sp;

}

LONG BooleanPush(LONG i)
{
     if (b_sp > b_bos)
     {
#if (DEBUG_BOOL_STACK)
	printfScreen(debugScreen, "bpush: <err>\n");
#endif
	return 0;
     }
     *b_sp = i;
#if (DEBUG_BOOL_STACK)
     printfScreen(debugScreen, "bpush: %08X (%d)\n", *b_sp, *b_sp);
#endif
     b_sp++;
     return 1;
}

LONG BooleanPop(void)
{
    b_sp--;
    if (b_sp < b_tos)
    {
       b_sp++;
#if (DEBUG_BOOL_STACK)
       printfScreen(debugScreen, "bpop : <err>\n");
#endif
       return 0;
    }
#if (DEBUG_BOOL_STACK)
    printfScreen(debugScreen, "bpop : %08X (%d)\n", *b_sp, *b_sp);
#endif
    return *b_sp;

}

LONG LogicalPush(LONG i)
{
     if (l_sp > l_bos)
     {
#if (DEBUG_LOGICAL_STACK)
	printfScreen(debugScreen, "lpush: <err>\n");
#endif
	return 0;
     }
     *l_sp = i;
#if (DEBUG_LOGICAL_STACK)
     printfScreen(debugScreen, "lpush: %08X (%d)\n", *l_sp, *l_sp);
#endif
     l_sp++;
     return 1;
}

LONG LogicalPop(void)
{
    l_sp--;
    if (l_sp < l_tos)
    {
       l_sp++;
#if (DEBUG_LOGICAL_STACK)
       printfScreen(debugScreen, "lpop : <err>\n");
#endif
       return 0;
    }
#if (DEBUG_LOGICAL_STACK)
    printfScreen(debugScreen, "lpop : %08X (%d)\n", *l_sp, *l_sp);
#endif
    return *l_sp;

}

void initNumericStacks(void)
{

    sp = numStack;
    tos = sp;
    bos = sp + NUM_STACK_SIZE - 1;

    c_sp = contextStack;
    c_tos = c_sp;
    c_bos = c_sp + CONTEXT_STACK_SIZE - 1;

    b_sp = booleanStack;
    b_tos = b_sp;
    b_bos = b_sp + BOOL_STACK_SIZE - 1;

    l_sp = logicalStack;
    l_tos = l_sp;
    l_bos = l_sp + LOGICAL_STACK_SIZE - 1;

}

LONG ProcessOperator(LONG oper)
{

    LONG a, b;

    b = ExpressPop();
    a = ExpressPop();
    switch(oper)
    {
       case NEG_TOKEN:
	  break;

       case LEFT_SHIFT_TOKEN:
	  ExpressPush(a << (b % PROCESSOR_32));  // mod (b) to base 32
	  break;

       case RIGHT_SHIFT_TOKEN:
	  ExpressPush(a >> (b % PROCESSOR_32));  // mob (b) to base 32
	  break;

       case PLUS_TOKEN:
	  ExpressPush(a + b);
	  break;

       case XOR_TOKEN:
	  ExpressPush(a ^ b);
	  break;

       case AND_TOKEN:
	  ExpressPush(a & b);
	  break;

       case MOD_TOKEN:
	  ExpressPush(a % b);
	  break;

       case OR_TOKEN:
	  ExpressPush(a | b);
	  break;

       case MINUS_TOKEN:
	  ExpressPush(a - b);
	  break;

       case MULTIPLY_TOKEN:
	  ExpressPush(a * b);
	  break;

       case DIVIDE_TOKEN:
	  if (b) // if divide by zero, drop value on the floor
	     ExpressPush(a / b);
	  else
	     ExpressPush(0);
	  break;

    }
    return 0;

}

LONG ProcessBoolean(LONG oper)
{

    LONG a, b;

    b = ExpressPop();
    a = ExpressPop();
    switch(oper)
    {
       case NOT_TOKEN:
	  ExpressPush(a == b); // we pushed an imaginary zero on the stack
	  break;               // this operation returns the boolean for (!x)

       case GREATER_TOKEN:
	  ExpressPush(a > b);
	  break;

       case LESS_TOKEN:
	  ExpressPush(a < b);
	  break;

       case GREATER_EQUAL_TOKEN:
	  ExpressPush(a >= b);
	  break;

       case LESS_EQUAL_TOKEN:
	  ExpressPush(a <= b);
	  break;

       case EQUAL_TOKEN:
	  ExpressPush(a == b);
	  break;

       case NOT_EQUAL_TOKEN:
	  ExpressPush(a != b);
	  break;
    }
    return 0;

}

LONG ProcessLogical(LONG oper)
{

    LONG a, b;

    b = ExpressPop();
    a = ExpressPop();
    switch(oper)
    {
       case LOGICAL_AND_TOKEN:
	  ExpressPush(a && b);
	  break;

       case LOGICAL_OR_TOKEN:
	  ExpressPush(a || b);
	  break;
    }
    return 0;

}

LONG ParseLogical(LONG logicalCount)
{

    register int i, r;
    LONG a, b;
    LONG c = 0, lastClass = 0, oper = 0;

    for (i=0; i < logicalCount; i++)
       ExpressPush(LogicalPop());

    for (i=0, r=0; i < (logicalCount / 2); i++)
    {
       a = ExpressPop();
       TokenType[r] = NUMBER_TOKEN;
       TokenValue[r++] = a;
       a = ExpressPop();
       TokenType[r] = a;  // get the operator type
       TokenValue[r++] = 0;
    }

    initNumericStacks();

#if (DEBUG_LOGICAL)
     printfScreen(debugScreen, "\n");
#endif
    for (i=0; i < logicalCount; i++)
    {
#if DEBUG_LOGICAL
       printfScreen(debugScreen, "token: %02X  value: %08X  type: %s\n", TokenType[i],
	      TokenValue[i], parserDescription[TokenType[i]]);
#endif
       switch (TokenType[i])
       {
	  case LOGICAL_AND_TOKEN:
	  case LOGICAL_OR_TOKEN:
	     if (lastClass != CLASS_BOOLEAN)
	     {
		lastClass = CLASS_BOOLEAN;
		oper = TokenType[i];
	     }
	     continue;

	  case NUMBER_TOKEN:
	     if (lastClass == CLASS_DATA)
	     {
		c = ExpressPop();
		return c;
	     }
	     lastClass = CLASS_DATA;
	     c = TokenValue[i];
	     ExpressPush(c);
	     if (oper)
		oper = ProcessLogical(oper);
	     continue;

	  case NULL_TOKEN:
	     c = ExpressPop();
	     return c;

	  default:
	     continue;
       }
    }
    return c;

}

LONG ParseBoolean(LONG booleanCount)
{

    register int i, r;
    LONG a, b, oper = 0;
    LONG c = 0, lastClass = 0, logicalCount = 0;

    for (i=0; i < booleanCount; i++)
       ExpressPush(BooleanPop());

    for (i=0, r=0; i < (booleanCount / 2); i++)
    {
       a = ExpressPop();
       TokenType[r] = NUMBER_TOKEN;
       TokenValue[r++] = a;
       a = ExpressPop();
       TokenType[r] = a;  // get the operator type
       TokenValue[r++] = 0;
    }

    initNumericStacks();

#if (DEBUG_BOOL)
     printfScreen(debugScreen, "\n");
#endif
    for (i=0; i < booleanCount; i++)
    {
#if DEBUG_BOOL
       printfScreen(debugScreen, "token: %02X  value: %08X  type: %s\n", TokenType[i],
	      TokenValue[i], parserDescription[TokenType[i]]);
#endif
       switch (TokenType[i])
       {
	  // partition operators
	  case LOGICAL_AND_TOKEN:
	  case LOGICAL_OR_TOKEN:
	     c = ExpressPop();
	     LogicalPush(c);
	     logicalCount++;
	     LogicalPush(TokenType[i]);
	     logicalCount++;
	     ExpressPush(c);
	     oper = 0;
	     lastClass = 0;
	     continue;

	  // boolean operators
	  case NOT_TOKEN:
	     if (lastClass != CLASS_BOOLEAN)
	     {
		ExpressPush(0);
		lastClass = CLASS_BOOLEAN;
		oper = TokenType[i];
	     }
	     continue;

	  case GREATER_TOKEN:
	  case LESS_TOKEN:
	  case GREATER_EQUAL_TOKEN:
	  case LESS_EQUAL_TOKEN:
	  case EQUAL_TOKEN:
	  case NOT_EQUAL_TOKEN:
	     if (lastClass != CLASS_BOOLEAN)
	     {
		lastClass = CLASS_BOOLEAN;
		oper = TokenType[i];
	     }
	     continue;

	  case NUMBER_TOKEN:
	     if (lastClass == CLASS_DATA)
	     {
		c = ExpressPop();
		if (logicalCount)
		{
		   LogicalPush(c);
		   logicalCount++;
		   LogicalPush(0); // push null token
		   logicalCount++;
		   c = ParseLogical(logicalCount);
		   return c;
		}
		return c;
	     }
	     lastClass = CLASS_DATA;
	     c = TokenValue[i];
	     ExpressPush(c);
	     if (oper)
		oper = ProcessBoolean(oper);
	     continue;

	  case NULL_TOKEN:
	     c = ExpressPop();
	     if (logicalCount)
	     {
		LogicalPush(c);
		logicalCount++;
		LogicalPush(0); // push null token
		logicalCount++;
		c = ParseLogical(logicalCount);
		return c;
	     }
	     return c;

	  default:
	     continue;
       }
    }
    return c;

}

LONG EvaluateExpression(StackFrame *stackFrame, BYTE **p, LONG *type)
{

     register LONG flags;
     register int i;
     LONG a, b, oper = 0, dref = 0, bracket = 0;
     LONG dref_type = 0, lastClass = 0, lastToken = 0;
     LONG neg_flag = 0, negative_flag = 0, c;
     LONG booleanCount = 0;

     flags = get_flags();
     spin_lock(&expressLock);

     if (type)
	*type = INVALID_EXPRESSION;
#if (DEBUG_BOOL)
     printfScreen(debugScreen, "\n");
#endif
#if (DEBUG_EXPRESS)
     printfScreen(debugScreen, "\np: %08X  %s\n", *p, *p);
#endif
     parseTokens(stackFrame, *p);
     if (TokenCount)
     {
	initNumericStacks();
	for (i=0; i < TokenCount; i++)
	{
#if (DEBUG_EXPRESS)
	   printfScreen(debugScreen, "token: %s  lastClass: %d\n", parserDescription[TokenType[i]], lastClass);
#endif
	   switch (TokenType[i])
	   {
	      case NOT_TOKEN:
		 if (lastClass != CLASS_DATA)
		 {
		    if (oper)
		       oper = ProcessOperator(oper);
		    c = ExpressPop();
		    BooleanPush(c);
		    booleanCount++;
		    BooleanPush(TokenType[i]);
		    booleanCount++;
		    dref_type = 0;
		    lastClass = 0;
		    neg_flag  = 0;
		    negative_flag = 0;
		 }
		 lastToken = NOT_TOKEN;
		 continue;

	      // boolean operators
	      case GREATER_TOKEN:
	      case LESS_TOKEN:
	      case GREATER_EQUAL_TOKEN:
	      case LESS_EQUAL_TOKEN:
	      case LOGICAL_AND_TOKEN:
	      case LOGICAL_OR_TOKEN:
	      case EQUAL_TOKEN:
	      case NOT_EQUAL_TOKEN:
		 if (oper)
		    oper = ProcessOperator(oper);
		 c = ExpressPop();
		 BooleanPush(c);
		 booleanCount++;
		 BooleanPush(TokenType[i]);
		 booleanCount++;
		 dref_type = 0;
		 lastClass = 0;
		 neg_flag  = 0;
		 negative_flag = 0;
		 lastToken = 0;
		 continue;

	      // partition operators
	      case DWORD_TOKEN:
		 if (dref)
		    dref_type = LONG_PTR;
		 lastToken = 0;
		 continue;

	      case WORD_TOKEN:
		 if (dref)
		    dref_type = WORD_PTR;
		 lastToken = 0;
		 continue;

	      case BYTE_TOKEN:
		 if (dref)
		    dref_type = BYTE_PTR;
		 lastToken = 0;
		 continue;

	      case DREF_OPEN_TOKEN:   // push state and nest for de-reference
		 if (lastClass == CLASS_DATA)
		 {
		    *p = (BYTE *)((LONG)*p + (LONG)TokenIndex[i]);
		    if (type)
		    {
		       if (booleanCount)
			  *type = BOOLEAN_EXPRESSION;
		       else
			  *type = NUMERIC_EXPRESSION;
		    }
		    c = ExpressPop();
		    if (booleanCount)
		    {
		       BooleanPush(c);
		       booleanCount++;
		       BooleanPush(0); // last operator is the null token
		       booleanCount++;
		       c = ParseBoolean(booleanCount);
#if (DEBUG_BOOL)
		       printfScreen(debugScreen, "be_N : (%d) = (%s)\n", c, c ? "TRUE" : "FALSE");
#endif
		       spin_unlock(&expressLock);
		       set_flags(flags);
		       return c;
		    }
#if (DEBUG_EXPRESS)
		    printfScreen(debugScreen, "ee_N : %08X (%d)\n", c, c);
#endif
		    spin_unlock(&expressLock);
		    set_flags(flags);
		    return c;
		 }
		 dref++;
		 ContextPush(dref_type);
		 ContextPush(oper);
		 ContextPush(lastClass);
		 ContextPush(neg_flag);
		 ContextPush(negative_flag);
		 dref_type = 0;
		 oper      = 0;
		 lastClass = 0;
		 neg_flag  = 0;
		 negative_flag = 0;
		 lastToken = 0;
		 continue;

	      case DREF_CLOSE_TOKEN:   // pop state, restore, and complete oper
		 if (!dref)
		    continue;
		 c = deref(dref_type, ExpressPop());
		 ExpressPush(c);
		 negative_flag  = ContextPop();
		 neg_flag  = ContextPop();
		 ContextPop();
		 oper      = ContextPop();
		 dref_type = ContextPop();
		 if (dref)
		    dref--;
		 lastClass = CLASS_DATA;

		 c = ExpressPop();
		 if (negative_flag)
		    c = 0 - c;
		 if (neg_flag)
		    c = ~c;
		 neg_flag = 0;
		 negative_flag = 0;
		 ExpressPush(c);

		 if (oper)
		    oper = ProcessOperator(oper);
		 lastToken = 0;
		 continue;

	      case BB_TOKEN:
		 if (lastClass == CLASS_DATA)
		 {
		    *p = (BYTE *)((LONG)*p + (LONG)TokenIndex[i]);
		    if (type)
		    {
		       if (booleanCount)
			  *type = BOOLEAN_EXPRESSION;
		       else
			  *type = NUMERIC_EXPRESSION;
		    }
		    c = ExpressPop();
		    if (booleanCount)
		    {
		       BooleanPush(c);
		       booleanCount++;
		       BooleanPush(0); // last operator is the null token
		       booleanCount++;
		       c = ParseBoolean(booleanCount);
#if (DEBUG_BOOL)
		       printfScreen(debugScreen, "be_N : (%d) = (%s)\n", c, c ? "TRUE" : "FALSE");
#endif
		       spin_unlock(&expressLock);
		       set_flags(flags);
		       return c;
		    }
#if (DEBUG_EXPRESS)
		    printfScreen(debugScreen, "ee_N : %08X (%d)\n", c, c);
#endif
		    spin_unlock(&expressLock);
		    set_flags(flags);
		    return c;
		 }
		 bracket++;
		 ContextPush(oper);
		 ContextPush(lastClass);
		 ContextPush(neg_flag);
		 ContextPush(negative_flag);
		 oper      = 0;
		 lastClass = 0;
		 neg_flag  = 0;
		 negative_flag = 0;
		 lastToken = 0;
		 continue;

	      case EB_TOKEN:
		 if (!bracket)
		    continue;
		 negative_flag  = ContextPop();
		 neg_flag  = ContextPop();
		 ContextPop();
		 oper      = ContextPop();
		 if (bracket)
		    bracket--;
		 lastClass = CLASS_DATA;
		 c = ExpressPop();
		 if (negative_flag)
		    c = 0 - c;
		 if (neg_flag)
		    c = ~c;
		 neg_flag = 0;
		 negative_flag = 0;
		 ExpressPush(c);
		 if (oper)
		    oper = ProcessOperator(oper);
		 lastToken = 0;
		 continue;

	      // arithmetic operators
	      case NEG_TOKEN:
		 neg_flag = 1;
		 lastToken = 0;
		 continue;

	      case MINUS_TOKEN:
		 if (lastClass == CLASS_ARITHMETIC)
		 {
		    lastToken = MINUS_TOKEN;
		    negative_flag = 1;
		    continue;
		 }
		 if (lastClass != CLASS_ARITHMETIC)
		 {
		    lastClass = CLASS_ARITHMETIC;
		    oper = TokenType[i];
		 }
		 lastToken = 0;
		 continue;

	      case PLUS_TOKEN:
	      case LEFT_SHIFT_TOKEN:
	      case RIGHT_SHIFT_TOKEN:
	      case XOR_TOKEN:
	      case AND_TOKEN:
	      case MOD_TOKEN:
	      case OR_TOKEN:
	      case MULTIPLY_TOKEN:
	      case DIVIDE_TOKEN:
		 if (lastClass != CLASS_ARITHMETIC)
		 {
		    lastClass = CLASS_ARITHMETIC;
		    oper = TokenType[i];
		 }
		 lastToken = 0;
		 continue;

	      // data operators
	      case CF_TOKEN:
	      case PF_TOKEN:
	      case AF_TOKEN:
	      case ZF_TOKEN:
	      case SF_TOKEN:
	      case IF_TOKEN:
	      case DF_TOKEN:
	      case OF_TOKEN:
	      case VM_TOKEN:
	      case AC_TOKEN:
	      case EIP_TOKEN:
	      case FLAGS_TOKEN:
	      case EAX_TOKEN:
	      case EBX_TOKEN:
	      case ECX_TOKEN:
	      case EDX_TOKEN:
	      case ESI_TOKEN:
	      case EDI_TOKEN:
	      case EBP_TOKEN:
	      case ESP_TOKEN:
	      case CS_TOKEN:
	      case DS_TOKEN:
	      case ES_TOKEN:
	      case FS_TOKEN:
	      case GS_TOKEN:
	      case SS_TOKEN:
	      case NUMBER_TOKEN:
		 if (lastClass == CLASS_DATA)
		 {
		    *p = (BYTE *)((LONG)*p + (LONG)TokenIndex[i]);
		    if (type)
		    {
		       if (booleanCount)
			  *type = BOOLEAN_EXPRESSION;
		       else
			  *type = NUMERIC_EXPRESSION;
		    }
		    c = ExpressPop();
		    if (booleanCount)
		    {
		       BooleanPush(c);
		       booleanCount++;
		       BooleanPush(0); // last operator is the null token
		       booleanCount++;
		       c = ParseBoolean(booleanCount);
#if (DEBUG_BOOL)
		       printfScreen(debugScreen, "be_N : (%d) = (%s)\n", c, c ? "TRUE" : "FALSE");
#endif
		       spin_unlock(&expressLock);
		       set_flags(flags);
		       return c;
		    }
#if (DEBUG_EXPRESS)
		    printfScreen(debugScreen, "ee_N : %08X (%d)\n", c, c);
#endif
		    spin_unlock(&expressLock);
		    set_flags(flags);
		    return c;
		 }
		 lastClass = CLASS_DATA;
		 c = TokenValue[i];
		 if (negative_flag)
		    c = 0 - c;
		 if (neg_flag)
		    c = ~TokenValue[i];
		 neg_flag = 0;
		 negative_flag = 0;
		 ExpressPush(c);
		 if (oper)
		    oper = ProcessOperator(oper);
		 lastToken = 0;
		 continue;

	      case NULL_TOKEN:
		 *p = (BYTE *)((LONG)*p + (LONG)TokenIndex[i]);
		 if (TokenCount > 1 && type)
		 {
		    if (booleanCount)
		       *type = BOOLEAN_EXPRESSION;
		    else
		       *type = NUMERIC_EXPRESSION;
		 }
		 c = ExpressPop();
		 if (booleanCount)
		 {
		    BooleanPush(c);
		    booleanCount++;
		    BooleanPush(0); // last operator is the null token
		    booleanCount++;
		    c = ParseBoolean(booleanCount);
#if (DEBUG_BOOL)
		    printfScreen(debugScreen, "be_N : (%d) = (%s)\n", c, c ? "TRUE" : "FALSE");
#endif
		    spin_unlock(&expressLock);
		    set_flags(flags);
		    return c;
		 }
#if (DEBUG_EXPRESS)
		 printfScreen(debugScreen, "ee_N : %08X (%d)\n", c, c);
#endif
		 spin_unlock(&expressLock);
		 set_flags(flags);
		 return c;

	      // assignment operators
	      case ASSIGNMENT_TOKEN:
		 lastToken = 0;
		 continue;

	      default:
		 lastToken = 0;
		 continue;
	   }
	}
     }
     if (type)
	*type = INVALID_EXPRESSION;

     if (lastToken) {};

     spin_unlock(&expressLock);
     set_flags(flags);
     return 0;

}

void EvaluateCommandExpression(SCREEN *screen, StackFrame *stackFrame, BYTE *p)
{

     BYTE *expr;
     LONG type;
     LONG c;

#if DEBUG_EXPRESS
     printfScreen(debugScreen, "expr: [%s]\n", p);
#endif
     expr = p;
     c = EvaluateExpression(stackFrame, &p, &type);
     if (type)
	printfScreenWithAttribute(screen, WHITE,
			    "expr: %s = 0x%08X (%dr) (%s) bool(%i) = %s\n",
			    expr,
			    c,
			    c,
			    exprDescription[type & 3],
			    (c) ? 1 : 0,
			    (c) ? "TRUE" : "FALSE");
     else
	displayExpressionHelp(screen);
     return;

}


