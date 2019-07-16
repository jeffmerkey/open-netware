
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  FILEIO.C
*   DESCRIP  :  Streams File IO support for MANOS v1.0
*   DATE     :  June 26, 1998
*
***************************************************************************/

#include "version.h"
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
#include "timer.h"
#include "peexe.h"
#include "dosfile.h"
#include "malloc.h"
#include "free.h"
#include "trees.h"
#include "ifs.h"

#define   DUP_ERROR	226

static LONG error_num = 0;

int fflush(FILE *fp)
{
   IFSFlushCache((LONG)fp);
   return 0;
}

FILE *fopen(char *name, char *mode)
{

    LONG handle, fappend = 0, fcreate = 0, fdiscard = 0, lmode;

    if (*mode=='r')
       lmode = 0;
    else
    if (*mode=='w')
    {
       lmode = 1;
       fcreate = 1;
       fdiscard = 1;
    }
    else
    if (*mode=='a')
    {
       fappend = 1;
       fcreate = 1;
       lmode = 1;
    }
    else
       return (0);

    /* see if they want to update also */

    if (mode[1] == '+' || mode[2] == '+')
       lmode = 1;

    if (fdiscard || fcreate)
    {
       handle = IFSCreateFile(name, 0);
       if (handle)
	  IFSCloseFile(handle);
    }

    if (lmode) {};

    handle = IFSOpenFile(name, O_RDWR);
    if (handle)
    {
       if (fappend)
	  IFSFileSeek(handle, 0, SEEK_END);
    }

//    printf("fopen  handle: %d mode: %d discard: %d create: %d append: %d\n",
//	    handle, lmode, fdiscard, fcreate, fappend);

    return (FILE *) (handle);

}

long *fclose(FILE *stream)
{
    long *retCode;

    retCode = (long *)(IFSCloseFile((LONG)stream));

//    printf("fclose  ret: %d\n", retCode ? -1 : 0);

    if (retCode)
       return (long *) -1;
    else
       return (long *) 0;

}

long ftell(FILE *stream)
{
    LONG retCode;

    retCode = IFSFileSeek((LONG)stream, 0, SEEK_CUR);

//    printf("ftell ret: %d\n", retCode);

    return (retCode);

}

void rewind(FILE *stream)
{
    register LONG retCode;

    retCode = IFSFileSeek((LONG)stream, 0, SEEK_SET);
    if (retCode) {};

//    printf("rewind %d\n", retCode);

}

long fseek(FILE *stream, long offset, long origin)
{
    LONG retCode;

    retCode = IFSFileSeek((LONG)stream, offset, origin);

//    printf("fseek ret: %d : %d/%d\n", retCode != (LONG) -1 ? 0 : 1,
//	    origin, offset);

    if (retCode != (LONG) -1)
       return 0;
    else
       return 1;
}

long fread(char *ptr, long size, long nobj, FILE *stream)
{
    register LONG i, retCode;

    for (i=0; i < nobj; i++)
    {
       retCode = IFSReadFile((LONG)stream, &ptr[i * size], size);
       if (!retCode)
	  break;
    }

//    printf("fread %d/%d  ret: %d\n", size, nobj, i);

    return (i);

}

long fwrite(char *ptr, long size, long nobj, FILE *stream)
{
    register LONG i, retCode;

    for (i=0; i < nobj; i++)
    {
       retCode = IFSWriteFile((LONG)stream, &ptr[i * size], size);
       if (!retCode)
	  break;
    }
//    printf("fwrite %d/%d  ret: %d\n", size, nobj, i);

    return (i);

}


long rename(char *oldname, char *newname)
{
    if (oldname) {};
    if (newname) {};

    // -1 error 0 success
    return (0);

}

long remove(char *name)
{
     register LONG retCode;

     // -1 error 0 success
     retCode = IFSDeleteFile(name);
//     printf("remove  ret: %d\n", retCode);
     return (retCode);
}

char fgetc(FILE *stream)
{

     LONG retCode;
     BYTE ch[1], chl;

     retCode = IFSReadFile((LONG)stream, &ch[0], 1);
     if (!retCode)
	return EOF;
     else
	chl = ch[0];
     return (chl);

}

char *fgets(char *s, long n, FILE *stream)
{

    char  ch;
    char  *ss, c;

    ss = s;
    while (n > 1)
    {
       ch = fgetc(stream);
       if (ch == EOF)
       {
	  *s = 0;
	  return (0);
       }
       else
       {
	  c = ch;
	  *s = c;
	  n--;
	  if (c == 0x0A)
	     n = 0;
	  s++;
       }
    }
    *s = 0;

    return (ss);

}

long fputc(long c, FILE *stream)
{

    LONG retCode, ch;

    retCode = IFSWriteFile((LONG)stream, (BYTE *)&c, 1);
    if (!retCode)
       ch = EOF;
    else
       ch = c;
    return (ch);

}

long fputs(const char *s, FILE *stream)
{

    long retCode, i, ch;

    ch = EOF;
    i = strlen((char *)s);
    if (i)
    {
       retCode = IFSWriteFile((LONG)stream, (BYTE *)s, i);
       if (!retCode)
	  ch = EOF;
       else
	  ch = 1;
    }
    return (ch);

}





