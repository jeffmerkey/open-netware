

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
*   FILE     :  UNI.H
*   DESCRIP  :  Unicode support for MANOS v1.0
*   DATE     :  November 2, 1997
*
*
***************************************************************************/

#include "types.h"

struct unicode_value {
   BYTE uni1;
   BYTE uni2;
};

extern BYTE FAT_UNI2CODE[64];
extern BYTE FAT_CODE2UNI[256];
extern BYTE UNI_PAGE_00[256];
extern BYTE UNI_PAGE_25[256];
extern BYTE *FAT_UNI2ASCII_PAGE[256];
extern BYTE FAT_A2ALIAS[256];
extern struct unicode_value FAT_A2UNI[256];