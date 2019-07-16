


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
*   FILE     :  EMIT.H
*   DESCRIP  :  Inline Macros for MANOS v1.0
*   DATE     :  November 9, 1997
*
*
***************************************************************************/

#define cli() __emit__((char)(0xFA))
#define sti()  __emit__((char)(0xFB))
#define INVLPG() __emit__ ((char)(0x0F), (char)(0x01), (char)(0x07))
#define BREAK()  __emit__((char)(0xCC))
#define HALT()  __emit__((char)(0xF4))
#define LOCK(p) __emit__((char)(0xF0) p)

#define bswapEAX() __emit__ ((char)(0x0F), (char)(0xC8))
#define bswapECX() __emit__ ((char)(0x0F), (char)(0xC9))
#define bswapEDX() __emit__ ((char)(0x0F), (char)(0xCA))
#define bswapEBX() __emit__ ((char)(0x0F), (char)(0xCB))
#define bswapESP() __emit__ ((char)(0x0F), (char)(0xCC))
#define bswapEBP() __emit__ ((char)(0x0F), (char)(0xCD))
#define bswapESI() __emit__ ((char)(0x0F), (char)(0xCE))
#define bswapEDI() __emit__ ((char)(0x0F), (char)(0xCF))


