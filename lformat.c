

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
*   FILE     :  FORMAT.C
*   DESCRIP  :  Formatted Output for MANOS v1.0
*   DATE     :  November 2, 1997
*
*
***************************************************************************/

#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "kernel.h"
#include "keyboard.h"
#include "screen.h"
#include "types.h"

extern long OutputText(BYTE *s);

#define LEFT_JUSTIFY   0x00000001
#define RIGHT_JUSTIFY  0x00000002
#define ZERO_FILL      0x00000004
#define COMMA_JUSTIFY  0x00000008
#define SIGN_FLAG      0x00000010

/*******************************************
*  isalpha, isdigit, isalnum, isspace,
*  isgraph, isctrl, islower, isupper,
*  ispunct, isxdigit, toupper, tolower
*********************************************/

/************************************************
* Determine if a character is alphabetic
*************************************************/
long isalpha(long chr)
{
   return  (((chr >= 'A') && (chr <= 'Z')) ||
		 ((chr >= 'a') && (chr <= 'z')));
}

long issymbol(long chr)
{

   return (isalnum(chr) || chr == '_' && chr != ' ');

}

/*********************************************
* Determine if a character is a numeric digit
**********************************************/

long isdigit(long chr)
{
   return  ((chr >= '0') && (chr <= '9'));
}

/*********************************************
* Determine if a character is a alphanumeric
**********************************************/

long isalnum(long chr)
{
   return  (((chr >= 'A') && (chr <= 'Z')) ||
		 ((chr >= 'a') && (chr <= 'z')) ||
		 ((chr >= '0') && (chr <= '9')));
}

/*********************************************
* Determine if a character is a whitespace
* (space, ff, lf, cr, tab, vtab)
**********************************************/

long isspace(long chr)
{
   return  ((chr == ' ')  ||
		 (chr == '\f') ||
		 (chr == '\r') ||
		 (chr == '\n') ||
		 (chr == '\t') ||
		 (chr == 0x0B));
}

/***************************************************
* Determine if a character is printable ASCII
* (eg, char graphics)
****************************************************/
long isascii(long chr)
{
   return (chr >= 0x21 && chr <= 0x7e);
}


/***************************************************
* Determine if a character is printable ASCII
* (eg, char graphics)
****************************************************/
long isprint(long chr)
{
   return (chr >= 0x21 && chr <= 0x7e);
}

/***************************************************
* Determine if a character is printable ASCII
* (eg, char graphics)
****************************************************/
long isgraph(long chr)
{
   return (chr >= 0x21 && chr <= 0x7e);
}


/*********************************************
* Determine if a character is control char
**********************************************/
long isctrl(long chr)
{
   return ((chr >= 0) && (chr <= 0x1f));
}

/*********************************************
* Determine if a character is lower case
**********************************************/
long islower(long chr)
{
   return ((chr >= 'a') && (chr <= 'z'));
}

/*********************************************
* Determine if a character is UPPER CASE
**********************************************/
long isupper(long chr)
{
   return ((chr >= 'A') && (chr <= 'Z'));
}

/***************************************************
* Determine if a character is punctuation
* (any printing char except space, letter, or digit)
****************************************************/
long ispunct(long chr)
{
   return  ((chr >= 0x21 && chr <= 0x2f) ||  	/*  ! to /  */
			 (chr >= 0x3a && chr <= 0x40) ||	/*  : to @  */
			 (chr >= 0x7a && chr <= 0x7e)); 	/*  { to ~  */
}

/***************************************************
* Determine if char is a hex digit (0-9, a-f, A-F)
****************************************************/
long isxdigit(long chr)
{
   return  (chr >= '0' && chr <= '9') ||
			(chr >= 'a' && chr <= 'f') ||
			(chr >= 'A' && chr <= 'F');
}

/************************************************
* Make char UPPER CASE
*************************************************/
long toupper(long chr)
{
   if ((chr >= 'a') && (chr <= 'z'))
	return (chr - 0x20);
   return (chr);
}

/************************************************
* Make char lower case
*************************************************/
long tolower(long chr)
{
   if ((chr >= 'A') && (chr <= 'Z'))
	return (chr + 0x20);
   return (chr);
}

long max(long value1, long value2)
{
   return (value1 > value2 ? value1 : value2);
}

long min(long value1, long value2)
{
   return (value1 < value2 ? value1 : value2);
}

/*************************************************************
 This does the actual parsing of the format and also moves to
 the next arg(s) in the list from the passed in arg pointer.
 The number of chars written is returned (not incl \0).
**************************************************************/

LONG atox(BYTE *p)
{
    LONG c = 0;

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
    return (c);
}

LONG atol(BYTE *p)
{
    LONG c = 0;

    while (*p)
    {
	  if (*p >= '0' && *p <= '9')
	     c = (c * 10) + (*p - '0');
	  else
	     break;
       p++;
    }
    return (c);
}


long strlen(char *str)
{

    BYTE *s;

    if (str == 0)
       return 0;

    for (s = str; *s; ++s);
       return s-str;

}

char *strupr(char *_s)
{
    char *rv = _s;

    while (*_s)
    {
       *_s = toupper(*_s);
       _s++;
    }
    return rv;

}

char *strcat(char *s, const char *append)
{

    char *save = s;

    for (; *s; ++s);

    while (*append)
       *s++ = *append++;
    *s = 0;

    return save;
}

char *strlwr(char *_s)
{
    char *rv = _s;

    while (*_s)
    {
       *_s = tolower(*_s);
       _s++;
    }
    return rv;
}


int strnicmp(const char *s1, const char *s2, LONG n)
{

    if (n == 0)
       return 0;
    do
    {
       if (tolower(*s1) != tolower(*s2++))
	  return (int) tolower(*s1) - (int) tolower(*--s2);

       if (*s1++ == 0)
	  break;
    } while (--n != 0);

    return 0;
}

int stricmp(const char *s1, const char *s2)
{

    while (tolower(*s1) == tolower(*s2))
    {
       if (*s1 == 0)
	  return 0;
       s1++;
       s2++;
    }
    return (int) tolower(*s1) - (int) tolower(*s2);

}

long strncmp(const char *s1, const char *s2, LONG n)
{

    if (n == 0)
       return 0;
    do
    {
       if (*s1 != *s2++)
	  return *(unsigned char *) s1 - *(unsigned char *)--s2;

       if (*s1++ == 0)
	  break;

    } while (--n != 0);

    return 0;

}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 == *s2)
    {
       if (*s1 == 0)
	  return 0;
       s1++;
       s2++;
    }
    return *(unsigned const char *) s1 - *(unsigned const char *)(s2);

}

char *strcpy(char *to, const char *from)
{

    char *save = to;

    while (*from)
       *to++ = *from++;
    *to = 0;

    return save;

}

char *strncat(char *dst, const char *src, LONG n)
{

    if (n != 0)
    {
       char *d = dst;
       const char *s = src;

       while (*d != 0)
	  d++;
       do
       {
	  if ((*d = *s++) == 0)
	     break;
	  d++;
       } while (--n != 0);
       *d = 0;
    }
    return dst;

}

char *strncpy(char *dst, const char *src, LONG n)
{

    if (n != 0)
    {
       char *d = dst;
       const char *s = src;

       do
       {
	  if ((*d++ = *s++) == 0)
	  {
	     while (--n != 0)
		*d++ = 0;
	     break;
	  }
       } while (--n != 0);
    }
    return dst;

}

long strcspn(char *s1, char *s2)
{

    char *p, *spanp;
    char c, sc;

    for (p = s1;;)
    {
       c = *p++;
       spanp = s2;
       do
       {
	  if ((sc = *spanp++) == c)
	     return (long) (long) p - 1 - (long)s1;
       } while (sc != 0);
    }

}

char *strchr(char *s, LONG c)
{

     char cc = (char ) c;

     while (*s)
     {
       if (*s == cc)
	  return s;
       s++;
    }
    if (cc == 0)
       return s;

    return 0;

}

char *strrchr(char *s, LONG c)
{

    char cc = (char) c;
    char *sp = 0;

    while (*s)
    {
       if (*s == cc)
	  sp = s;
       s++;
    }
    if (cc == 0)
       sp = s;

    return sp;

}


void printNumber(LONG n, LONG radix, LONG width, BYTE *buffer, LONG *ndx, LONG flags)
{

     register LONG tmp = n, pos;
     register int i, field = 0;
     BYTE *digits = "0123456789ABCDEF";
     register BYTE ch, field_end = 0;

     if (!buffer || !ndx || !radix)
	return;

     // get actual field length

     do
     {
	tmp /= radix;
	field++;
     } while (tmp);


     // parse the number and convert via radix division


     if (!width)
     {
	if (flags & COMMA_JUSTIFY)
	   field = field + (field / 4);
	i = *ndx;
	pos = 0;
	do
	{
	  if (flags & COMMA_JUSTIFY && (pos + 1) >= 4 && (pos + 1) % 4 == 0)
	  {
	     buffer[i + (field - 1)] = ',';
	  }
	  else
	  {
	     buffer[i + (field - 1)] = digits[n % radix];
	     n /= radix;
	  }
	  *ndx += 1;
	  field--;
	  pos++;
	} while (n);
     }
     else
     {
	if (flags & COMMA_JUSTIFY)
	   width = width + (width / 4);
	i = *ndx;
	pos = 0;
	do
	{
	   if (flags & ZERO_FILL)
	   {
	      if (flags & COMMA_JUSTIFY && (pos + 1) >= 4 && (pos + 1) % 4 == 0)
	      {
		 buffer[i + (width - 1)] = ',';
	      }
	      else
	      {
		 buffer[i + (width - 1)] = digits[n % radix];
		 n /= radix;
	      }
	      *ndx += 1;
	      width--;
	      pos++;
	   }
	   else
	   {
	      if (flags & COMMA_JUSTIFY && (pos + 1) >= 4 && (pos + 1) % 4 == 0)
	      {
		 buffer[i + (width - 1)] = ',';
	      }
	      else
	      {
		 if (!field_end)
		    ch = digits[n % radix];
		 else
		    ch = ' ';
		 buffer[i + (width - 1)] = ch;
		 n /= radix;
		 if (!n)
		    field_end = 1;
	      }
	      *ndx += 1;
	      width--;
	      pos++;
	   }
	} while (width);
     }

}

void field(char **format, LONG *width, LONG *precision, LONG *flags)
{

     char *ptr;

     ptr = *format;

    *flags = 0;
    while (*ptr == '-' || *ptr == '+' || *ptr == '0' || *ptr == ',')
    {
       if (*ptr == '-')
	  *flags |= LEFT_JUSTIFY;
       if (*ptr == '+')
	  *flags |= RIGHT_JUSTIFY;
       if (*ptr == ',')
	  *flags |= COMMA_JUSTIFY;
       if (*ptr == '0')
	  *flags |= ZERO_FILL;
       ptr++;
    }

    *width = 0;
    while (*ptr >= '0' && *ptr <= '9')
    {
       *width = (*width * 10) + (*ptr - '0');
       ++ptr;
    }

    *precision = 0;

    if (*ptr == '.')
    {
       ++ptr;
       while (*ptr >= '0' && *ptr <= '9')
       {
	  *precision = (*precision * 10) + (*ptr - '0');
	  ++ptr;
       }
    }

    if (*ptr == 'l' || *ptr == 'L')
       ++ptr;

    *format = ptr;

}

LONG _fmt(void *format, void *parms, char *buffer)
{
	char *fmt;
	char *s;
	va_list *adx = (va_list *)parms;
	int arg;
	int c;
	LONG width, precision, flags, length, count;
	LONG ndx = 0;

	fmt = (char *)format;

	while ((c = *fmt++) != '\0')
	{
	   if (c != '%')
	   {
	      buffer[ndx] = c;
	      ndx += 1;
	      continue;
	   }
	   field(&fmt, &width, &precision, &flags);
	   switch(c = *fmt++)
	   {

	      case 'i':
	      case 'd':
		 arg = va_arg(*adx, int);
		 if (arg < 0)
		 {
		    buffer[ndx] = '-';
		    ndx += 1;
		    arg = - arg;
		 }
		 else if (flags & 2)
		 {
		    buffer[ndx] = '+';
		    ndx += 1;
		 }
		 printNumber(arg, 10, width, buffer, &ndx, flags);
		 break;

	      case 'o':
		 arg = va_arg(*adx, int);
		 printNumber(arg, 8, width, buffer, &ndx, flags);
		 break;

	      case 'u':
		 arg = va_arg(*adx, int);
		 printNumber(arg, 10, width, buffer, &ndx, flags);
		 break;

	      case 'x':
	      case 'X':
		 arg = va_arg(*adx, int);
		 printNumber(arg, 16, width, buffer, &ndx, flags);
		 break;

	      case 'z':
	      case 'Z':
		 arg = va_arg(*adx, int);
		 if (arg < 0)
		 {
		    buffer[ndx] = '-';
		    ndx += 1;
		    arg = -arg;
		 }
		 else
		 {
		    buffer[ndx] = '+';
		    ndx += 1;
		 }
		 printNumber(arg, 16, width, buffer, &ndx, flags);
		 break;

	      case 'w':
	      case 'W':
		 arg = va_arg(*adx, int);
		 if (arg < 0)
		 {
		    buffer[ndx] = '-';
		    ndx += 1;
		    arg = -arg;
		 }
		 printNumber(arg, 16, width, buffer, &ndx, flags);
		 break;

	      case 'c':
		 arg = va_arg(*adx, int);
		 buffer[ndx] = arg;
		 ndx += 1;
		 break;

	      case 'S':
		 s = va_arg(*adx, char *);
		 length = *s++;
		 for (count = length; count > 0; count--)
		 {
		    buffer[ndx] = *s++;
		    ndx += 1;
		 }
		 break;

	      case 's':
		 s = va_arg(*adx, char *);
		 if (width)
		    length = width;
		 else
		    length = strlen(s);
		 for (count = length; count > 0; count--)
		 {
		    buffer[ndx] = *s++;
		    ndx += 1;
		 }
		 break;

	      default:
		 buffer[ndx] = c;
		 ndx += 1;
		 break;
	   }
	}
	if (buffer != 0)
	   buffer[ndx] = 0;

	return (ndx);
}

long printf(char *format, ...)
{
	va_list parms;
	long retCode;
	BYTE buffer[512];

	va_start(parms, format);
	_fmt(format, &parms, (char *)buffer);
	retCode = OutputText(&buffer[0]);
	va_end(parms);

	return retCode;
}

long sprintf(char *buffer, char *format, ...)
{
	va_list parms;
	LONG retval;

	va_start(parms, format);
	retval = _fmt(format, &parms, (char *)buffer);
	va_end(parms);

	return retval;
}

long vsprintf(void *buffer, char *format, va_list parms)
{
	LONG retval;

	retval = _fmt(format, &parms, (char *)buffer);
	return retval;
}



