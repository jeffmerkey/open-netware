


/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  DOSFILE.H
*   DESCRIP  :  MSDOS Real Mode File System Interface for MANOS v1.0
*   DATE     :  January 11, 1998
*
*
***************************************************************************/

#include "types.h"

typedef struct _REGS {
    LONG flags;
    WORD es;
    WORD ds;
    LONG ebp;
    LONG edi;
    LONG esi;
    LONG edx;
    LONG ecx;
    LONG ebx;
    LONG eax;
    LONG res2[4];
} REGS;

// flags for DOSLseek

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

// attributes for DOSOpen

#define O_RDONLY	0x0000
#define O_WRONLY	0x0001
#define O_RDWR		0x0002
#define O_COMPAT        0x0000
#define O_DENYALL       0x0010
#define O_DENYWRITE     0x0020
#define O_DENYREAD      0x0030
#define O_DENYNONE      0x0040

//  attributes for DOSCreate

#define O_READWRITE     0x0000
#define O_READONLY      0x0001
#define O_HIDDEN        0x0002
#define O_SYSTEM        0x0004
#define O_VOLUME        0x0008
#define O_ARCHIVE       0x0020

extern long OutputText(BYTE *s);
extern LONG DOSClose(LONG fd);
extern LONG DOSOpen(BYTE *path, LONG oflag);
extern LONG DOSCreate(BYTE *path, LONG flags);
extern LONG DOSRead(LONG fd, void *buf, LONG size);
extern LONG DOSWrite(LONG fd, void *buf, LONG size);
extern LONG DOSDelete(BYTE *path);
extern LONG DOSLseek(LONG fd, LONG offset, LONG flags);
extern void DOSFlush(void);
extern LONG DOSFileSize(LONG fd);
extern LONG int86x(LONG, REGS *, REGS *);
