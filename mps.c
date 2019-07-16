

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
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  MPS.C
*   DESCRIP  :  Multi Processing Specification (MPS) for MANOS v1.0
*   DATE     :  November 2, 1997
*
*  This code is based on the standard MPS code distributed on Intel's
*  website.  The PCMP code used here is freely distributed by Intel
*  for use by systems developers.
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
#include "os.h"
#include "mps.h"
#include "event.h"

#define MPS_VERBOSE   0

// MPS 1.0 standard tables

BYTE mps_default_table_1[] = {
	'P','C','M','P',                        /* signature */
	0,0, 1, 0,              /* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,       /* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,     /* product id string */
	0,0,0,0,                /* oem table pointer */
	0,0, 2+1+1+16+2,0,              /* entry count */
	0x00,0x00,0xe0,0xfe,    /* address of local apic */
	0,0,0,0,                /* res. */

	MP_ET_PROC,     /* processor */
	0,              /* apic id == 0 */
	1,              /* apic type - 82489DX */
	1,              /* enable */
	0,0,0,0,        /* stepping, model, family, type */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_PROC,     /* processor */
	1,              /* apic id == 1 */
	1,              /* apic type - 82489DX */
	1,              /* enable */
	0,0,0,0,        /* stepping, model, family, type */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_BUS,      /* bus */
	0,              /* bus id */
	'I','S','A',' ',
	' ',' ',

	MP_ET_IOAPIC,   /* i/o apic */
	2,              /* apic id */
	1,              /* apic type - 82489DX (not used) */
	1,              /* enabled */
	0x00,0x00,0xc0,0xfe,    /* address of i/o apic */

	MP_ET_I_INTR,3,0,0,     /* PIC */
	0,0,0xff,0,             /* src(0,0) -> all i/o apics, line 0 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,1,2,1,                /* src(0,1) -> i/o apic with id=2, line 1 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,2,2,1,                /* src(0,2) -> i/o apic with id=2, line 2 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,3,2,3,                /* src(0,3) -> i/o apic with id=2, line 3 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,4,2,4,                /* src(0,4) -> i/o apic with id=2, line 4 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,5,2,5,                /* src(0,5) -> i/o apic with id=2, line 5 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,6,2,6,                /* src(0,6) -> i/o apic with id=2, line 6 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,7,2,7,                /* src(0,7) -> i/o apic with id=2, line 7 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,8,2,8,                /* src(0,8) -> i/o apic with id=2, line 8 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,9,2,9,                /* src(0,9) -> i/o apic with id=2, line 9 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,10,2,10,              /* src(0,10) -> i/o apic with id=2, line 10 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,11,2,11,              /* src(0,11) -> i/o apic with id=2, line 11 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,12,2,12,              /* src(0,12) -> i/o apic with id=2, line 12 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,13,2,12,              /* src(0,13) -> i/o apic with id=2, line 13 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,14,2,14,              /* src(0,14) -> i/o apic with id=2, line 14 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,15,2,15,              /* src(0,15) -> i/o apic with id=2, line 15 */

	MP_ET_L_INTR,3,0,0,     /* PIC */
	0,0,0xff,0,             /* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,     /* NMI */
	0,0,0xff,1,             /* src(0,0) -> all local apics, line 1 */
};

BYTE mps_default_table_2[] = {
	'P','C','M','P',                        /* signature */
	0,0, 1, 0,              /* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,       /* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,     /* product id string */
	0,0,0,0,                /* oem table pointer */
	0,0, 2+1+1+14+2,0,              /* entry count */
	0x00,0x00,0xe0,0xfe,    /* address of local apic */
	0,0,0,0,                /* res. */

	MP_ET_PROC,     /* processor */
	0,              /* apic id == 0 */
	1,              /* apic type - 82489DX */
	1,              /* enable */
	0,0,0,0,        /* stepping, model, family, type */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_PROC,     /* processor */
	1,              /* apic id == 1 */
	1,              /* apic type - 82489DX */
	1,              /* enable */
	0,0,0,0,        /* stepping, model, family, type */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_BUS,      /* bus */
	0,              /* bus id */
	'E','I','S','A',
	' ',' ',

	MP_ET_IOAPIC,   /* i/o apic */
	2,              /* apic id */
	1,              /* apic type - 82489DX (not used) */
	1,              /* enabled */
	0x00,0x00,0xc0,0xfe,    /* address of i/o apic */

	MP_ET_I_INTR,3,0,0,     /* PIC */
	0,0,0xff,0,             /* src(0,0) -> all i/o apics, line 0 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,1,2,1,                /* src(0,1) -> i/o apic with id=2, line 1 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,3,2,3,                /* src(0,3) -> i/o apic with id=2, line 3 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,4,2,4,                /* src(0,4) -> i/o apic with id=2, line 4 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,5,2,5,                /* src(0,5) -> i/o apic with id=2, line 5 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,6,2,6,                /* src(0,6) -> i/o apic with id=2, line 6 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,7,2,7,                /* src(0,7) -> i/o apic with id=2, line 7 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,8,2,8,                /* src(0,8) -> i/o apic with id=2, line 8 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,9,2,9,                /* src(0,9) -> i/o apic with id=2, line 9 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,10,2,10,              /* src(0,10) -> i/o apic with id=2, line 10 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,11,2,11,              /* src(0,11) -> i/o apic with id=2, line 11 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,12,2,12,              /* src(0,12) -> i/o apic with id=2, line 12 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,14,2,14,              /* src(0,14) -> i/o apic with id=2, line 14 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,15,2,15,              /* src(0,15) -> i/o apic with id=2, line 15 */

	MP_ET_L_INTR,3,0,0,     /* PIC */
	0,0,0xff,0,             /* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,     /* NMI */
	0,0,0xff,1,             /* src(0,0) -> all local apics, line 1 */
};

BYTE mps_default_table_3[] = {
	'P','C','M','P',                        /* signature */
	0,0, 1, 0,              /* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,       /* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,     /* product id string */
	0,0,0,0,                /* oem table pointer */
	0,0, 2+1+1+16+2,0,              /* entry count */
	0x00,0x00,0xe0,0xfe,    /* address of local apic */
	0,0,0,0,                /* res. */

	MP_ET_PROC,     /* processor */
	0,              /* apic id == 0 */
	1,              /* apic type - 82489DX */
	1,              /* enable */
	0,0,0,0,        /* stepping, model, family, type */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_PROC,     /* processor */
	1,              /* apic id == 1 */
	1,              /* apic type - 82489DX */
	1,              /* enable */
	0,0,0,0,        /* stepping, model, family, type */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_BUS,      /* bus */
	0,              /* bus id */
	'E','I','S','A',
	' ',' ',

	MP_ET_IOAPIC,   /* i/o apic */
	2,              /* apic id */
	1,              /* apic type - 82489DX (not used) */
	1,              /* enabled */
	0x00,0x00,0xc0,0xfe,    /* address of i/o apic */

	MP_ET_I_INTR,3,0,0,     /* PIC */
	0,0,0xff,0,             /* src(0,0) -> all i/o apics, line 0 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,1,2,1,                /* src(0,1) -> i/o apic with id=2, line 1 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,2,2,1,                /* src(0,2) -> i/o apic with id=2, line 2 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,3,2,3,                /* src(0,3) -> i/o apic with id=2, line 3 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,4,2,4,                /* src(0,4) -> i/o apic with id=2, line 4 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,5,2,5,                /* src(0,5) -> i/o apic with id=2, line 5 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,6,2,6,                /* src(0,6) -> i/o apic with id=2, line 6 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,7,2,7,                /* src(0,7) -> i/o apic with id=2, line 7 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,8,2,8,                /* src(0,8) -> i/o apic with id=2, line 8 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,9,2,9,                /* src(0,9) -> i/o apic with id=2, line 9 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,10,2,10,              /* src(0,10) -> i/o apic with id=2, line 10 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,11,2,11,              /* src(0,11) -> i/o apic with id=2, line 11 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,12,2,12,              /* src(0,12) -> i/o apic with id=2, line 12 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,13,2,12,              /* src(0,13) -> i/o apic with id=2, line 13 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,14,2,14,              /* src(0,14) -> i/o apic with id=2, line 14 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,15,2,15,              /* src(0,15) -> i/o apic with id=2, line 15 */

	MP_ET_L_INTR,3,0,0,     /* PIC */
	0,0,0xff,0,             /* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,     /* NMI */
	0,0,0xff,1,             /* src(0,0) -> all local apics, line 1 */
};

BYTE mps_default_table_4[] = {
	'P','C','M','P',                        /* signature */
	0,0, 1, 0,              /* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,       /* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,     /* product id string */
	0,0,0,0,                /* oem table pointer */
	0,0, 2+1+1+16+2,0,              /* entry count */
	0x00,0x00,0xe0,0xfe,    /* address of local apic */
	0,0,0,0,                /* res. */

	MP_ET_PROC,     /* processor */
	0,              /* apic id == 0 */
	1,              /* apic type - 82489DX */
	1,              /* enable */
	0,0,0,0,        /* stepping, model, family, type */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_PROC,     /* processor */
	1,              /* apic id == 1 */
	1,              /* apic type - 82489DX */
	1,              /* enable */
	0,0,0,0,        /* stepping, model, family, type */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_BUS,      /* bus */
	0,              /* bus id */
	'M','C','A',' ',
	' ',' ',

	MP_ET_IOAPIC,   /* i/o apic */
	2,              /* apic id */
	1,              /* apic type - 82489DX (not used) */
	1,              /* enabled */
	0x00,0x00,0xc0,0xfe,    /* address of i/o apic */

	MP_ET_I_INTR,3,0,0,     /* PIC */
	0,0,0xff,0,             /* src(0,0) -> all i/o apics, line 0 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,1,2,1,                /* src(0,1) -> i/o apic with id=2, line 1 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,2,2,1,                /* src(0,2) -> i/o apic with id=2, line 2 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,3,2,3,                /* src(0,3) -> i/o apic with id=2, line 3 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,4,2,4,                /* src(0,4) -> i/o apic with id=2, line 4 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,5,2,5,                /* src(0,5) -> i/o apic with id=2, line 5 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,6,2,6,                /* src(0,6) -> i/o apic with id=2, line 6 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,7,2,7,                /* src(0,7) -> i/o apic with id=2, line 7 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,8,2,8,                /* src(0,8) -> i/o apic with id=2, line 8 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,9,2,9,                /* src(0,9) -> i/o apic with id=2, line 9 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,10,2,10,              /* src(0,10) -> i/o apic with id=2, line 10 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,11,2,11,              /* src(0,11) -> i/o apic with id=2, line 11 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,12,2,12,              /* src(0,12) -> i/o apic with id=2, line 12 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,13,2,12,              /* src(0,13) -> i/o apic with id=2, line 13 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,14,2,14,              /* src(0,14) -> i/o apic with id=2, line 14 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,15,2,15,              /* src(0,15) -> i/o apic with id=2, line 15 */

	MP_ET_L_INTR,3,0,0,     /* PIC */
	0,0,0xff,0,             /* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,     /* NMI */
	0,0,0xff,1,             /* src(0,0) -> all local apics, line 1 */
};

BYTE mps_default_table_5[] = {
	'P','C','M','P',                        /* signature */
	0,0, 1, 0,              /* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,       /* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,     /* product id string */
	0,0,0,0,                /* oem table pointer */
	0,0, 2+2+1+16+2,0,              /* entry count */
	0x00,0x00,0xe0,0xfe,    /* address of local apic */
	0,0,0,0,                /* res. */

	MP_ET_PROC,     /* processor */
	0,              /* apic id == 0 */
	0x10,           /* apic type - Integrated */
	1,              /* enable */
	0,0,0,0,        /* stepping, model, family, type */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_PROC,     /* processor */
	1,              /* apic id == 1 */
	0x10,           /* apic type - Integrated */
	1,              /* enable */
	0,0x20,0,0,     /* stepping, model, family, type=CM */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_BUS,      /* bus */
	0,              /* bus id */
	'I','S','A',' ',
	' ',' ',

	MP_ET_BUS,      /* bus */
	0,              /* bus id */
	'P','C','I',' ',
	' ',' ',

	MP_ET_IOAPIC,   /* i/o apic */
	2,              /* apic id */
	0x10,           /* apic type - Integrated (not used) */
	1,              /* enabled */
	0x00,0x00,0xc0,0xfe,    /* address of i/o apic */

	MP_ET_I_INTR,3,0,0,     /* PIC */
	0,0,0xff,0,             /* src(0,0) -> all i/o apics, line 0 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,1,2,1,                /* src(0,1) -> i/o apic with id=2, line 1 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,2,2,1,                /* src(0,2) -> i/o apic with id=2, line 2 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,3,2,3,                /* src(0,3) -> i/o apic with id=2, line 3 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,4,2,4,                /* src(0,4) -> i/o apic with id=2, line 4 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,5,2,5,                /* src(0,5) -> i/o apic with id=2, line 5 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,6,2,6,                /* src(0,6) -> i/o apic with id=2, line 6 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,7,2,7,                /* src(0,7) -> i/o apic with id=2, line 7 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,8,2,8,                /* src(0,8) -> i/o apic with id=2, line 8 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,9,2,9,                /* src(0,9) -> i/o apic with id=2, line 9 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,10,2,10,              /* src(0,10) -> i/o apic with id=2, line 10 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,11,2,11,              /* src(0,11) -> i/o apic with id=2, line 11 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,12,2,12,              /* src(0,12) -> i/o apic with id=2, line 12 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,13,2,12,              /* src(0,13) -> i/o apic with id=2, line 13 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,14,2,14,              /* src(0,14) -> i/o apic with id=2, line 14 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,15,2,15,              /* src(0,15) -> i/o apic with id=2, line 15 */

	MP_ET_L_INTR,3,0,0,     /* PIC */
	0,0,0xff,0,             /* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,     /* NMI */
	0,0,0xff,1,             /* src(0,0) -> all local apics, line 1 */
};

BYTE mps_default_table_6[] = {
	'P','C','M','P',                        /* signature */
	0,0, 1, 0,              /* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,       /* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,     /* product id string */
	0,0,0,0,                /* oem table pointer */
	0,0, 2+2+1+16+2,0,              /* entry count */
	0x00,0x00,0xe0,0xfe,    /* address of local apic */
	0,0,0,0,                /* res. */

	MP_ET_PROC,     /* processor */
	0,              /* apic id == 0 */
	0x10,           /* apic type - Integrated */
	1,              /* enable */
	0,0,0,0,        /* stepping, model, family, type */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_PROC,     /* processor */
	1,              /* apic id == 1 */
	0x10,           /* apic type - Integrated */
	1,              /* enable */
	0,0x20,0,0,     /* stepping, model, family, type=CM */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_BUS,      /* bus */
	0,              /* bus id */
	'E','I','S','A',
	' ',' ',

	MP_ET_BUS,      /* bus */
	0,              /* bus id */
	'P','C','I',' ',
	' ',' ',

	MP_ET_IOAPIC,   /* i/o apic */
	2,              /* apic id */
	0x10,           /* apic type - Integrated (not used) */
	1,              /* enabled */
	0x00,0x00,0xc0,0xfe,    /* address of i/o apic */

	MP_ET_I_INTR,3,0,0,     /* PIC */
	0,0,0xff,0,             /* src(0,0) -> all i/o apics, line 0 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,1,2,1,                /* src(0,1) -> i/o apic with id=2, line 1 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,2,2,1,                /* src(0,2) -> i/o apic with id=2, line 2 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,3,2,3,                /* src(0,3) -> i/o apic with id=2, line 3 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,4,2,4,                /* src(0,4) -> i/o apic with id=2, line 4 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,5,2,5,                /* src(0,5) -> i/o apic with id=2, line 5 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,6,2,6,                /* src(0,6) -> i/o apic with id=2, line 6 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,7,2,7,                /* src(0,7) -> i/o apic with id=2, line 7 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,8,2,8,                /* src(0,8) -> i/o apic with id=2, line 8 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,9,2,9,                /* src(0,9) -> i/o apic with id=2, line 9 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,10,2,10,              /* src(0,10) -> i/o apic with id=2, line 10 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,11,2,11,              /* src(0,11) -> i/o apic with id=2, line 11 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,12,2,12,              /* src(0,12) -> i/o apic with id=2, line 12 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,13,2,12,              /* src(0,13) -> i/o apic with id=2, line 13 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,14,2,14,              /* src(0,14) -> i/o apic with id=2, line 14 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,15,2,15,              /* src(0,15) -> i/o apic with id=2, line 15 */

	MP_ET_L_INTR,3,0,0,     /* PIC */
	0,0,0xff,0,             /* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,     /* NMI */
	0,0,0xff,1,             /* src(0,0) -> all local apics, line 1 */
};

BYTE mps_default_table_7[] = {
	'P','C','M','P',                        /* signature */
	0,0, 1, 0,              /* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,       /* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,     /* product id string */
	0,0,0,0,                /* oem table pointer */
	0,0, 2+2+1+15+2,0,              /* entry count */
	0x00,0x00,0xe0,0xfe,    /* address of local apic */
	0,0,0,0,                /* res. */

	MP_ET_PROC,     /* processor */
	0,              /* apic id == 0 */
	0x10,           /* apic type - Integrated */
	1,              /* enable */
	0,0,0,0,        /* stepping, model, family, type */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_PROC,     /* processor */
	1,              /* apic id == 1 */
	0x10,           /* apic type - Integrated */
	1,              /* enable */
	0,0x20,0,0,     /* stepping, model, family, type */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_BUS,      /* bus */
	0,              /* bus id */
	'M','C','A',' ',
	' ',' ',

	MP_ET_BUS,      /* bus */
	0,              /* bus id */
	'P','C','I',' ',
	' ',' ',

	MP_ET_IOAPIC,   /* i/o apic */
	2,              /* apic id */
	0x10,           /* apic type - Integrated (not used) */
	1,              /* enabled */
	0x00,0x00,0xc0,0xfe,    /* address of i/o apic */

	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,1,2,1,                /* src(0,1) -> i/o apic with id=2, line 1 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,2,2,1,                /* src(0,2) -> i/o apic with id=2, line 2 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,3,2,3,                /* src(0,3) -> i/o apic with id=2, line 3 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,4,2,4,                /* src(0,4) -> i/o apic with id=2, line 4 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,5,2,5,                /* src(0,5) -> i/o apic with id=2, line 5 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,6,2,6,                /* src(0,6) -> i/o apic with id=2, line 6 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,7,2,7,                /* src(0,7) -> i/o apic with id=2, line 7 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,8,2,8,                /* src(0,8) -> i/o apic with id=2, line 8 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,9,2,9,                /* src(0,9) -> i/o apic with id=2, line 9 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,10,2,10,              /* src(0,10) -> i/o apic with id=2, line 10 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,11,2,11,              /* src(0,11) -> i/o apic with id=2, line 11 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,12,2,12,              /* src(0,12) -> i/o apic with id=2, line 12 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,13,2,12,              /* src(0,13) -> i/o apic with id=2, line 13 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,14,2,14,              /* src(0,14) -> i/o apic with id=2, line 14 */
	MP_ET_I_INTR,0,0,0,     /* INTR */
	0,15,2,15,              /* src(0,15) -> i/o apic with id=2, line 15 */

	MP_ET_L_INTR,3,0,0,     /* PIC */
	0,0,0xff,0,             /* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,     /* NMI */
	0,0,0xff,1,             /* src(0,0) -> all local apics, line 1 */
};

/*      NO_IO_APIC_OPTION       */
BYTE mps_default_table_8[] = {
	'P','C','M','P',                        /* signature */
	0,0, 1, 0,              /* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,       /* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,     /* product id string */
	0,0,0,0,                /* oem table pointer */
	0,0, 2+1+0+0+2,0,               /* entry count */
	0x00,0x00,0xe0,0xfe,    /* address of local apic */
	0,0,0,0,                /* res. */

	MP_ET_PROC,     /* processor */
	0,              /* apic id == 0 */
	0x10,           /* apic type - Integrated */
	1,              /* enable */
	0,0,0,0,        /* stepping, model, family, type */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_PROC,     /* processor */
	1,              /* apic id == 1 */
	0x10,           /* apic type - Integrated */
	1,              /* enable */
	0,0x20,0,0,     /* stepping, model, family, type=CM */
	0,0,0,0,        /* feature flags - not used */
	0,0,0,0, 0,0,0,0,       /* res. */

	MP_ET_BUS,      /* bus */
	0,              /* bus id */
	'I','S','A',' ',
	' ',' ',

	MP_ET_L_INTR,3,0,0,     /* PIC */
	0,0,0xff,0,             /* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,     /* NMI */
	0,0,0xff,1,             /* src(0,0) -> all local apics, line 1 */
};

LONG apic_defaults[] = {
	0,
	(LONG)mps_default_table_1,
	(LONG)mps_default_table_2,
	(LONG)mps_default_table_3,
	(LONG)mps_default_table_4,
	(LONG)mps_default_table_5,
	(LONG)mps_default_table_6,
	(LONG)mps_default_table_7,
	(LONG)mps_default_table_8,
	0
};

BYTE *bus_strings[]={
    "ISA   ",
    "EISA  ",
    "MCA   ",
    "PCI   ",
    "CBUS  ",
    "CBUSII",
    "FUTURE",
    "INTERN",
    "MBI   ",
    "MBII  ",
    "MPI   ",
    "MPSA  ",
    "NUBUS ",
    "PCMCIA",
    "TC    ",
    "VL    ",
    "VME   ",
    "XPRESS"
};

BYTE *bus_display_strings[]={
    "ISA",
    "EISA",
    "MCA",
    "PCI",
    "CBUS",
    "CBUSII",
    "FUTURE",
    "INTERN",
    "MBI",
    "MBII",
    "MPI",
    "MPSA",
    "NUBUS",
    "PCMCIA",
    "TC",
    "VL",
    "VME",
    "XPRESS"
};

BYTE vector_table[]={
   0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, // IRQ 0-7
   0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, // IRQ 8-15
   0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, // intr 16-23
   0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, // intr 24-31
   0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, // intr 32-39
   0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, // intr 40-47
   0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, // intr 48-55
   0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, // intr 56-63
   0xB2, 0xB3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // intr 64-65
};

LONG mps_size[]={
   sizeof(struct mpe_proc),
   sizeof(struct mpe_bus),
   sizeof(struct mpe_ioapic),
   sizeof(struct mpe_intr),
   sizeof(struct mpe_local)
};

struct intr_table int_table[]={
   -1, 0, 0, 0, 1, -1, 0, 0, 0, 1, -1, 0, 0, 0, 1, -1, 0, 0, 0, 1,
   -1, 0, 0, 0, 1, -1, 0, 0, 0, 1, -1, 0, 0, 0, 1, -1, 0, 0, 0, 1,
   -1, 0, 0, 0, 1, -1, 0, 0, 0, 1, -1, 0, 0, 0, 1, -1, 0, 0, 0, 1,
   -1, 0, 0, 0, 1, -1, 0, 0, 0, 1, -1, 0, 0, 0, 1, -1, 0, 0, 0, 1,

   -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0,
   -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0,
   -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0,
   -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0,

   -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0,
   -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0,
   -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0,
   -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0,

   -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0,
   -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0,
   -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0,
   -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0,

};

LONG elcr_flags;
LONG mps_present;
LONG pcmp_fib1;
LONG pcmp_fib2;
struct pcmp_fptr *mps_fp;
struct mpchdr *vendor_table;
LONG num_buses;
LONG num_procs;
LONG num_ioapics;

LONG ioapic_locks[MAX_IOAPICS] = { 0, 0, 0, 0, 0, 0, 0, 0 };
struct mpe_ioapic *io_apic_addr[MAX_IOAPICS];
struct io_apic_state_array io_apic_state[MAX_IOAPICS];
IOAPIC_IDS io_apic_ids[MAX_IOAPICS];
LONG io_apic_entry_num[MAX_IOAPICS];
LONG io_apic_nlines[MAX_IOAPICS];
struct bus_data bus_info[MAX_IOAPICS];
struct mpe_proc *proc_id[MAX_IOAPICS];
LONG local_apic_address;
LONG processor_mask;
LONG warm_reset_vector;
BYTE pic_assign[MAX_INTS];

extern char *strncpy(char *dst, const char *src, LONG n);

LONG mps_find_fp(LONG begin, LONG end)
{

     struct pcmp_fptr *fp, *endp;
     BYTE *cp, sum = 0;
     register int i;

     endp = (struct pcmp_fptr *) (begin + (end - begin + 1) - sizeof(*fp));
     for (fp = (struct pcmp_fptr *)begin; fp <= (struct pcmp_fptr *) endp; fp++)
     {
	if (fp->sig[0] == '_' && fp->sig[1] == 'M' &&
	    fp->sig[2] == 'P' && fp->sig[3] == '_')
	{
	   cp = (BYTE *) fp;
	   for (i=0; i < sizeof(*fp); i++)
	      sum += *cp++;

	   if (sum == 0)
	   {
	      mps_present = 1;
	      vendor_table = (struct mpchdr *) fp->paddr;
	      pcmp_fib1 = (LONG) fp->mp_byte[0];
	      pcmp_fib2 = (LONG) fp->mp_byte[1];
	      mps_fp = fp;
	      return (LONG) (1);
	   }
	}
     }
     return (LONG) (0);
}

LONG mps_locate(void)
{

    LONG *ebda_addr, *bmem_addr;
    LONG ebda_base, bmem_base;

    mps_present = 0;
    pcmp_fib1 = 0;
    pcmp_fib2 = 0;
    vendor_table = 0;
    mps_fp = 0;

    ebda_addr = (LONG *) EBDA_PTR;   // Extended BIOS Data Area
    ebda_base = *ebda_addr;
    ebda_base = (ebda_base << 4) & 0xFFFFF;
    if (ebda_base > 0x000A0000 || ebda_base < 0x0007F800)
       ebda_base = 0;

    bmem_addr = (LONG *) BMEM_PTR;   // Base Memory
    bmem_base = *bmem_addr;
    bmem_base = (bmem_base * 1024) & 0xFFFFF;
    if (bmem_base > 0x000A0000 || bmem_base < 0x0007F800)
       bmem_base = 0;

    if (ebda_base)
    {
       if (!mps_find_fp(ebda_base, ebda_base + 1023))
       {
	  if (!mps_find_fp(0xF0000, 0xFFFFF))
	  {
	     printf("System does not support Intel MPS\n");
	     return (LONG) (0);
	  }
       }
    }
    else
    {
       if (!bmem_base || !mps_find_fp(bmem_base, bmem_base + 1023))
       {
	  if(!mps_find_fp(0xF0000, 0xFFFFF))
	  {
	     printf("System does not support Intel MPS\n");
	     return (LONG) (0);
	  }
       }
    }

    if (mps_present)
    {
       if (vendor_table)
       {
	  printf("Intel MPS version 1.%d, table type %d\n", mps_fp->rev, pcmp_fib1);
	  return 1;
       }

       if (pcmp_fib1)
       {
	  vendor_table = (struct mpchdr *) apic_defaults[pcmp_fib1];
	  if (pcmp_fib1)
	     printf("Intel MPS version 1.%d, default table %d\n", mps_fp->rev, pcmp_fib1);
	  else
	     printf("Intel MPS version 1.%d\n", mps_fp->rev);
	  return 1;
       }

       if (!pcmp_fib1)
       {
	  mps_present = 1;
	  pcmp_fib1 = MP_DEF_TYPE;
	  pcmp_fib2 = MP_DEF_IMCR;
	  vendor_table = (struct mpchdr *) apic_defaults[MP_DEF_TYPE];
	  printf("Use default table %d\n", pcmp_fib1);
	  return 1;
       }
    }

    mps_present = 1;
    pcmp_fib1 = MP_DEF_TYPE;
    pcmp_fib2 = MP_DEF_IMCR;
    vendor_table = (struct mpchdr *) apic_defaults[MP_DEF_TYPE];
    printf("Use default table %d\n", pcmp_fib1);

    return 1;

}

void mps_ints(void)
{

    union mpcentry *entry;
    int num_entries, i, type, intr, r;
    struct mpconfig *config;

    config = (struct mpconfig *) vendor_table;
    entry = config->entry;

    num_entries = config->hdr.num_entry;
    for (i=0; i < num_entries; i++)
    {
	type = entry->bytes[0];
	if ((type == MP_ET_I_INTR) || (type == MP_ET_L_INTR))
	{
	   if (!entry->i.intr_type)
	   {
	      if (bus_info[entry->i.src_bus & 0xF].bus_type == 3)
	      {
		 intr = -1;
		 for (r=0; r < 64; r++)
		 {
		    if (!int_table[r].use)
		    {
		       int_table[r].use = 1;
		       intr = r;
		       break;
		    }
		 }
		 if (intr == -1)
		 {
		    printf("MPS Table error -- FATAL\n");
		    return;
		 }
		 int_table[intr].io_apicid = entry->i.dest_apicid;
		 int_table[intr].line = entry->i.dest_line;
		 int_table[intr].dev = entry->i.src_irq;
		 int_table[intr].bus = entry->i.src_bus;
	      }
	      else
	      {
		 intr = (entry->i.src_irq & 0xF);
		 int_table[intr].io_apicid = entry->i.dest_apicid;
		 int_table[intr].line = entry->i.dest_line;
		 int_table[intr].dev = entry->i.src_irq;
		 int_table[intr].bus = entry->i.src_bus;
	      }
	   }
	}
	entry = (union mpcentry *)((LONG)entry + (LONG)mps_size[type]);
     }

}

LONG mps_ioapics(void)
{

    register int num_entries, num, i, type;
    struct mpconfig *config;
    union mpcentry *entry;
    LONG *ioapic;

    config = (struct mpconfig *) vendor_table;
    entry = config->entry;

    num_entries = config->hdr.num_entry;
    num = 0;
    for (i=0; i < num_entries; i++)
    {
	type = entry->bytes[0];
	if (type >= MP_ET_IOAPIC)
	{
	   if (type != MP_ET_IOAPIC)
	      break;

	   if (entry->a.ioapic_flags & 1)
	   {
	      io_apic_addr[num] = (struct mpe_ioapic *) entry;
	      io_apic_ids[io_apic_addr[num]->apic_id & 0xF].address = io_apic_addr[num]->io_apic_adr;
	      io_apic_ids[io_apic_addr[num]->apic_id & 0xF].lnum = num;
	      num++;
	      if (num == 16)
		 break;
	   }
	}
	entry = (union mpcentry *) ((LONG)entry + (LONG)mps_size[type]);
    }
    if (!num)
	printf("no I/O apics found\n");

    for (i=0; i < num; i++)
    {
       // see if IOAPICs are 16 or 24 line
       ioapic = (LONG *) io_apic_addr[i]->io_apic_adr;

       // map ioapic address into page tables

       map_address((LONG)ioapic >> 12, (LONG)ioapic >> 12);
       map_address(((LONG)ioapic + 4096) >> 12, ((LONG)ioapic + 4096) >> 12);

       ioapic[APIC_IO_REG] = 1;
       io_apic_entry_num[i] = ioapic[APIC_IO_DATA];
       io_apic_entry_num[i] = (io_apic_entry_num[i] >> 16) & 0x000000FF;
       io_apic_nlines[i] = io_apic_entry_num[i];

#if MPS_VERBOSE
       printf("io_apic addr: 0x%08X  id: %02X  lines: %02X (%s)\n",
		     io_apic_addr[i]->io_apic_adr,
		     io_apic_addr[i]->apic_id,
		     io_apic_nlines[i],
		     ((io_apic_addr[i]->apic_vers >> 4) & 0x01) ? "Embedded" : "82489DX");
#endif

    }
    return (LONG) (num);

}

LONG mps_procs(void)
{

    int num_entries, num, i, type, id;
    struct mpconfig *config;
    union mpcentry *entry;

    config = (struct mpconfig *) vendor_table;
    entry = config->entry;

    num_entries = config->hdr.num_entry;
    local_apic_address = (LONG) config->hdr.loc_apic_adr;

    // map local apic address into page tables

    map_address(local_apic_address >> 12, local_apic_address >> 12);
    map_address(((LONG)local_apic_address + 4096) >> 12, ((LONG)local_apic_address + 4096) >> 12);

    num = 0;
    for (i=0; i < num_entries; i++)
    {
	type = entry->bytes[0];

	if (type != MP_ET_PROC)
	   break;

	if (entry->p.cpu_flags & 1)
	{
	   proc_id[num] = (struct mpe_proc *) entry;
	   num++;
	}
	entry = (union mpcentry *)((LONG)entry + (LONG)mps_size[type]);
     }

#if MPS_VERBOSE
     for (i=0; i < num; i++)
	printf("processor: %02X  apic: %02X  addr: 0x%08X  (%s)\n", i, proc_id[i]->apic_id,
			local_apic_address,
			((proc_id[i]->apic_vers >> 4) & 0x01) ? "Embedded" : "82489DX");
#endif

     return num;

}

void mps_buses(void)
{

    int num_entries, i, r, type;
    struct mpconfig *config;
    union mpcentry *entry;

    config = (struct mpconfig *) vendor_table;
    entry = config->entry;

    num_entries = config->hdr.num_entry;
    num_buses = 0;
    for (i=0; i < num_entries; i++)
    {
	type = entry->bytes[0];
	if (type == MP_ET_BUS)
	{
	   for (r=0; r < 18; r++)
	   {
	      if (!strncmp(entry->b.name, bus_strings[r], 6))
	      {
		 num_buses++;
		 bus_info[(entry->b.bus_id & 0xF)].bus_type = r;
		 bus_info[(entry->b.bus_id & 0xF)].bus_id = entry->b.bus_id;
#if MPS_VERBOSE
		 printf("bus (%s)  bus id: %02X\n", bus_display_strings[r], entry->b.bus_id);
#endif
	      }
	   }
	}
	entry = (union mpcentry *)((LONG)entry + (LONG)mps_size[type]);
     }

}

LONG MPSDetect(void)
{
     if (!mps_locate())
	return 0;

     mps_buses();
     num_procs = mps_procs();
     num_ioapics = mps_ioapics();
     mps_ints();

     RegisterEventNotification(EVENT_ENTER_REAL_MODE,
			       (void (*)(LONG))disable_ioapic_ints);

     RegisterEventNotification(EVENT_RETURN_REAL_MODE,
			       (void (*)(LONG))enable_ioapic_ints);

     return 1;

}

void apic_eoi(LONG intr)
{

    LONG *local, val;

    if (intr == 2)
       intr = 9;

    if (pic_assign[intr] || (intr < 16 && int_table[intr].line == (LONG) -1))
    {
       pic_eoi(intr);
    }
    else
    {
       val = 0;
       local = (LONG *) local_apic_address;
       local[APIC_EOI] = val;
    }
    return;

}

//
//  returns  0  -  command sent to other processor successfully
//          -3  -  command register busy
//          -4  -  type command error
//
//
//  type is one of the following:
//  0 - 32 bit APIC command
//  1 - 64 bit logical destination command (proc = processor_number)
//  2 - 64 bit physical destination command (proc = apic_id)

LONG apic_xcall(LONG proc, LONG command, LONG type)
{

    LONG *local;
    LONG val, i;
    register LONG flags;

    flags = get_flags();
    local = (LONG *) local_apic_address;
    val = APIC_VALUE_PENDING;
    switch (type)
    {
       case 0:
	  // if a command was still pending, then wait until clear
	  for (i=0; i < 0xFFFFF && local[APIC_ICMD] & val; i++) {};

	  if (i >= 0xFFFFF)
	     return (LONG) -3;

	  local[APIC_ICMD] = command;   // send the command
	  break;

       case 1:
	  // if a command was still pending, then wait until clear
	  for (i=0; i < 0xFFFFF && local[APIC_ICMD] & val; i++) {};

	  if (i >= 0xFFFFF)
	     return (LONG) -3;

	  local[APIC_ICMD2] = proc_id[proc]->apic_id << APIC_ID_SHIFT;
	  local[APIC_ICMD] = command;
	  break;

       case 2:
	  // if a command was still pending, then wait until clear
	  for (i=0; i < 0xFFFFF && local[APIC_ICMD] & val; i++) {};

	  if (i >= 0xFFFFF)
	     return (LONG) -3;

	  local[APIC_ICMD2] = proc << APIC_ID_SHIFT;
	  local[APIC_ICMD] = command;
	  break;

       default:
	  return -4;
    }

    set_flags(flags);
    return 0;

}

LONG apic_directed_nmi(LONG proc)
{
    register LONG retCode = 0;

    retCode |= apic_xcall(proc, 0x0000C400, 1);
    retCode |= apic_xcall(proc, 0x00008400, 1);

    return retCode;

}

LONG apic_init(LONG proc)
{

     LONG *local, *ioapic, val, vw;
     register int i, r;
     register LONG flags;

     flags = get_flags();
     local = (LONG *) local_apic_address;

     if (!proc)  // processor (0) is special case for system init
     {

	//  if the apic is masked for LINT0, then
	//  virtual wire mode 'A' should be programmed,
	//  else assume virtual wire mode 'B', and leave LINT0 and LINT1
	//  alone.  these entries are masked on processors other than (0)

	spin_lock(&ioapic_locks[0]);
	ioapic = (LONG *) io_apic_addr[0]->io_apic_adr;
	ioapic[APIC_IO_REG] = APIC_REG_RDT + 2 * 0;  // select INTIN_0 reg
	val = ioapic[APIC_IO_DATA];		 // get intr 0 vector info on ioapic 0
	spin_unlock(&ioapic_locks[0]);

	if ((val & APIC_VALUE_MASK) == APIC_VALUE_MASK)
	{
	   // program virtual wire mode 'A'

	   local[APIC_SPUR] = LU_UNIT_ENABLED;

	   val = local[APIC_LVT_I0];
	   val &= 0xFFFE58FF;
	   val |= 0x700;
	   local[APIC_LVT_I0] = val;  //  program LINT0 to service ExtINT (vw 'A')

	   val = local[APIC_LVT_I1];
	   val &= 0xFFFE58FF;
	   val |= 0xFFFF0400;
	   val &= ~APIC_VALUE_MASK;
	   local[APIC_LVT_I1] = val;  //  program LINT1 to service NMI requests

	   vw = 0;
	   printf("apic virtual wire mode '%c' selected\n", vw ? 'B' : 'A');
	}
	else
	{
	   // virtual wire mode 'B' pre-programmed
	   vw = 1;
	   printf("apic virtual wire mode %c selected\n", vw ? 'B' : 'A');
	}

	//  If IMCR is present, then switch to virtual wire mode 'A' or 'B'
	//  IMCR should be set after and not before APIC config.

	if (pcmp_fib2 & 0x80)
	{
	   outb(IMCR_ADDR, 0x70);
	   outb(IMCR_DATA, 0x01);
	}

	local[APIC_TASKPRI] = 0xFF;        // raise to highest priority (disable all interrupts)
	local[APIC_LVT_TIMER] |= APIC_VALUE_MASK;   // mask the timer
	local[APIC_DESTFMT] = LU_DEST_FORMAT_FLAT;  // select logical destination mode
	local[APIC_LDEST] = 1 << APIC_ID_SHIFT;       // set processor (0) to entry (0)
	local[APIC_SPUR] = LU_UNIT_ENABLED | 0x4F; // program the spurious vector

	apic_xcall(proc, DELIVER_INIT_REASSERT_ALL, 0); // tell the apic to re-sync

	for (r=0; r < num_ioapics; r++)
	{
	   spin_lock(&ioapic_locks[r]);
	   ioapic = (LONG *) io_apic_addr[r]->io_apic_adr;
	   for (i=0; i < io_apic_nlines[r] + 1; i++)
	   {
	      if (!r && i < 2)
		 continue;
	      ioapic[APIC_IO_REG] = APIC_REG_RDT + (2 * i);
	      val = APIC_VALUE_MASK;
	      ioapic[APIC_IO_DATA] |= val;
	   }
	   spin_unlock(&ioapic_locks[r]);
	}

	// clear any previous processor val values from the APIC_VALUE_LOPRI
	// ioapic registers

	for (r=0; r < num_ioapics; r++)
	{
	   spin_lock(&ioapic_locks[r]);
	   ioapic = (LONG *) io_apic_addr[r]->io_apic_adr;
	   for (i=0; i < io_apic_nlines[r] + 1; i++)
	   {
	      if (!r && i < 2)
		 continue;

	      ioapic[APIC_IO_REG] = APIC_REG_RDT2 + (2 * i);
	      ioapic[APIC_IO_DATA] = 0;
	   }
	   spin_unlock(&ioapic_locks[r]);
	}

	// EOI any pending interrupts left over from previous
	// shutdown/startup sequences
	apic_eoi(0);

	// reset ioapics configured for APIC_VALUE_LOPRI to current processor mask
	processor_mask |= 1 << (proc + APIC_ID_SHIFT);
	for (r=0; r < num_ioapics; r++)
	{
	   spin_lock(&ioapic_locks[r]);
	   ioapic = (LONG *) io_apic_addr[r]->io_apic_adr;
	   for (i=0; i < io_apic_nlines[r] + 1; i++)
	   {
	      if (!r && i < 2)
		 continue;
	      ioapic[APIC_IO_REG] = APIC_REG_RDT2 + (2 * i);
	      ioapic[APIC_IO_DATA] |= processor_mask;
	   }
	   spin_unlock(&ioapic_locks[r]);
	}
	local[APIC_TASKPRI] = 0;  // set lowest priority (enable all interrupts)
	set_flags(flags);
	return 0;

     }

     // if we got here, we are assumed on a processor other than (0)

     local[APIC_LVT_TIMER] |= (LONG) APIC_VALUE_MASK;    // mask the timer
     local[APIC_LVT_I0] |= (LONG) APIC_VALUE_MASK;       // mask LINT0
     local[APIC_LVT_I1] |= (LONG) APIC_VALUE_MASK;       // mask LINT1
     local[APIC_DESTFMT] = (LONG) LU_DEST_FORMAT_FLAT;  // select logical destination mode
     local[APIC_LDEST] =  1 << (proc + APIC_ID_SHIFT);     // set processor entry to (P# << p + APIC_ID_SHIFT)
     local[APIC_SPUR] = LU_UNIT_ENABLED | 0x4F; // set the spurious vector

     apic_xcall(proc, DELIVER_INIT_REASSERT_ALL, 0); // re-sync with other apics


     // reset ioapics configured for APIC_VALUE_LOPRI to current processor mask
     processor_mask |= 1 << (proc + APIC_ID_SHIFT);
     for (r=0; r < num_ioapics; r++)
     {
	spin_lock(&ioapic_locks[r]);
	ioapic = (LONG *) io_apic_addr[r]->io_apic_adr;
	for (i=0; i < io_apic_nlines[r] + 1; i++)
	{
	   if (!r && i < 2)
	      continue;
	   ioapic[APIC_IO_REG] = APIC_REG_RDT2 + (2 * i);
	   ioapic[APIC_IO_DATA] |= processor_mask;
	}
	spin_unlock(&ioapic_locks[r]);
     }
     local[APIC_TASKPRI] = 0;  // set lowest priority (enable all interrupts)

     set_flags(flags);

     return 0;

}

void apic_close(LONG proc)
{

     register int i, r;
     LONG val;
     LONG *local, *ioapic;
     register LONG flags;

     flags = get_flags();

     local = (LONG *) local_apic_address;
     local[APIC_TASKPRI] = 0xFF;  // disable all interrupts
     local[APIC_ICOUNT] = 0;      // clear timer count register

     val = APIC_VALUE_MASK;              // mask the local timer
     local[APIC_LVT_TIMER] |= val;

     processor_mask &= ~(1 << (proc + APIC_ID_SHIFT));  // remove the processor
     for (r = 0; r < num_ioapics; r++)
     {
	spin_lock(&ioapic_locks[r]);
	ioapic = (LONG *) io_apic_addr[r]->io_apic_adr;
	for (i=0; i < io_apic_nlines[r] + 1; i++)
	{
	   if (!r && i < 2)
	      continue;
	   ioapic[APIC_IO_REG] = APIC_REG_RDT + 2 * i;
	   if (ioapic[APIC_IO_DATA] & DELIVER_LOW_PRIORITY)
	   {
	      ioapic[APIC_IO_REG] = APIC_REG_RDT2 + 2 * i;
	      val = ~processor_mask;
	      ioapic[APIC_IO_DATA] &= val;
	   }
	   // if we are on processor (0) then mask as well
	   if (!proc)
	   {
	      ioapic[APIC_IO_REG] = APIC_REG_RDT + (2 * i);
	      val = APIC_VALUE_MASK;
	      ioapic[APIC_IO_DATA] |= val;
	   }
	}
	spin_unlock(&ioapic_locks[r]);
     }

     // lower the task prority (enable interrupts)
     local[APIC_TASKPRI] = 0;

     // allow pending interrupts to occur
     sti();
     for (i=0; i < 1000; i++) i=i;

     // raise the task priority to highest level (disable interrupts)
     cli();
     local[APIC_TASKPRI] = 0xFF;

     set_flags(flags);

     return;

}

void write_boot_vector(LONG addr)
{

    register int i;
    BYTE *dest, *src;
    struct rm_addr
    {
      WORD offset;
      WORD segment;
    } rm;
    register LONG flags;

    flags = get_flags();

    src = (BYTE *) CPQ_RESET_VECT;  // save warm boot vector
    dest = (BYTE *) &warm_reset_vector;
    for (i=0; i < 4; i++)
       *dest++ = *src++;

    rm.offset = addr & 0xF;
    rm.segment = (addr >> 4) & 0xFFFF;
    dest = (BYTE *) CPQ_RESET_VECT;  // warm boot vector address
    src = (BYTE *) &rm;

    for (i=0; i < 4; i++)
       *dest++ = *src++;

    // setup warm boot vector in CMOS
    outb(CMOS_ADDR, CMOSCTL);
    outb(CMOS_DATA, CMOSWARM);

    set_flags(flags);

    return;

}

void apic_timer_start(void)
{

    LONG *local;
    LONG vector;
    register LONG flags;

    flags = get_flags();

    local = (LONG *) local_apic_address;
    local[APIC_TASKPRI] = 0;
    local[APIC_ICOUNT] = (APIC_CLKNUM * 8 * 2); // 50 ms intervals
    vector = TIMER_VECTOR;
    vector &= 0xFFFF;
    vector |= 0x60000;
    vector &= ~APIC_VALUE_MASK;
    local[APIC_LVT_TIMER] = vector;

    set_flags(flags);
    return;

}

void apic_timer_stop(void)
{

    LONG *local;
    LONG val;
    register LONG flags;

    flags = get_flags();
    local = (LONG *) local_apic_address;
    local[APIC_ICOUNT] = 0;         // zero count register
    val = APIC_VALUE_MASK;
    local[APIC_LVT_TIMER] |= val;   // mask timer

    set_flags(flags);
    return;

}

void disable_ioapic_ints(void)
{

    register int i, r;
    LONG val;
    LONG *ioapic;
    LONG *local;
    register LONG flags;

    flags = get_flags();
    local = (LONG *) local_apic_address;
    local[APIC_TASKPRI] = 0xFF;

    for (r=0; r < num_ioapics; r++)
    {
       spin_lock(&ioapic_locks[r]);
       ioapic = (LONG *) io_apic_addr[r]->io_apic_adr;
       for (i=0; i < io_apic_nlines[r]+1; i++)
       {
	   if (!r && i < 2)
	      continue;

	   ioapic[APIC_IO_REG] = APIC_REG_RDT + (2 * i);
	   val = APIC_VALUE_MASK;
	   ioapic[APIC_IO_DATA] |= val;
       }
       spin_unlock(&ioapic_locks[r]);
    }
    set_flags(flags);
    return;

}

void enable_ioapic_ints(void)
{

    register int i, r;
    LONG val;
    LONG *ioapic;
    LONG *local;
     register LONG flags;

    flags = get_flags();
    for (r=0; r < num_ioapics; r++)
    {
       spin_lock(&ioapic_locks[r]);
       ioapic = (LONG *) io_apic_addr[r]->io_apic_adr;
       for (i=0; i < io_apic_nlines[r]+1; i++)
       {
	  if (!r && i < 2)
	      continue;

	  if (io_apic_state[r].line_state[i])
	  {
	     ioapic[APIC_IO_REG] = APIC_REG_RDT + (2 * i);
	     val = ioapic[APIC_IO_DATA];
	     val &= ~APIC_VALUE_MASK;
	     ioapic[APIC_IO_DATA] = val;
	  }
       }
       spin_unlock(&ioapic_locks[r]);
    }
    local = (LONG *) local_apic_address;
    local[APIC_TASKPRI] = 0;
    set_flags(flags);

    return;

}

LONG apic_activate_processor(LONG proc, LONG addr)
{

    LONG i;

    // if the processor does not exist, then return
    if (!proc_id[proc])
       return 0;

    // delay loops are required to prevent bus hang
    for (i=0; i < 0xFFFFF; i++) { i = i; };

    // note: addr must be on a page boundry for startup IPI.  this is
    // not a requirement for assert/deassert calls for 82489DX devices
    write_boot_vector(addr);

    // if the APIC is 82489DX, then use ASSERT/DEASSERT
    // otherwise, use the startup IPI command

    if (!((proc_id[proc]->apic_vers) & 0xF0))
    {
       apic_xcall(proc_id[proc]->apic_id, APIC_VALUE_RESET | APIC_VALUE_LEVEL | APIC_VALUE_ASSERT, 2);
       for (i=0; i < 0xFFFFF; i++)
	  { i = i; };
       apic_xcall(proc_id[proc]->apic_id, APIC_VALUE_RESET | APIC_VALUE_LEVEL | APIC_VALUE_DEASSERT, 2);
       for (i=0; i < 0xFFFFF; i++)
	  { i = i; };
    }
    else
    {
       apic_xcall(proc_id[proc]->apic_id, APIC_VALUE_RESET | APIC_VALUE_LEVEL | APIC_VALUE_ASSERT, 2);
       for (i=0; i < 0xFFFFF; i++)
	  { i = i; };
       apic_xcall(proc_id[proc]->apic_id, APIC_VALUE_RESET | APIC_VALUE_LEVEL | APIC_VALUE_DEASSERT, 2);
       for (i=0; i < 0xFFFFF; i++)
	  { i = i; };
       apic_xcall(proc_id[proc]->apic_id, APIC_VALUE_STARTUP | APIC_VALUE_EDGE | ((addr >> 12) & 0xFF), 2);
       for (i=0; i < 0xFFFFF; i++)
	  { i = i; };
       apic_xcall(proc_id[proc]->apic_id, APIC_VALUE_STARTUP | APIC_VALUE_EDGE | ((addr >> 12) & 0xFF), 2);
       for (i=0; i < 0xFFFFF; i++)
	  { i = i; };
    }
    return 0;

}

void dump_int_table(SCREEN *screen)
{

    register int i;

    SetPauseMode(screen, screen->nLines - 3);

    printfScreenWithAttribute(screen, BRITEWHITE, "mps interrupt table at 0x%08X\n", &int_table[0]);
    for (i=0; i < 64; i++)
    {
       printfScreenWithAttribute(screen, LTCYAN,
			   "(%02i): dev: %08X  bus: %02X ioapic: %02X  vect: %02X  line: %02X  use: %02X\n",
			   i,
			   int_table[i].dev,
			   int_table[i].bus,
			   int_table[i].io_apicid,
			   vector_table[i],
			   int_table[i].line,
			   int_table[i].use);
    }

    ClearPauseMode(screen);

}

void dump_ioapic(SCREEN *screen, LONG num)
{

     LONG *p;
     LONG i, val;

     p = (LONG *) io_apic_addr[num]->io_apic_adr;
     printfScreenWithAttribute(screen, BRITEWHITE, "io_apic registers [0x%08X]\n", p);
     for (i = 0; i <= 0x2F; i++)
     {
	if ((i & 3) == 0)
	   printfScreenWithAttribute(screen, LTCYAN, "%08X: ", i);
	*p = i;
	val = p[4];
	printfScreenWithAttribute(screen, LTCYAN, "%08X ", val);
	if ((i & 3) == 3)
	   printfScreenWithAttribute(screen, LTCYAN, "\n");
     }

}

void dump_local_apic(SCREEN *screen)
{

    LONG *p;
    LONG i, val;

    p = (LONG *) local_apic_address;
    printfScreenWithAttribute(screen, BRITEWHITE, "local apic registers [0x%08X]\n", p);
    printfScreenWithAttribute(screen, LTCYAN, "apic_id      : %08X\n", p[APIC_ID]);
    printfScreenWithAttribute(screen, LTCYAN, "apic_vers    : %08X\n", p[APIC_VERS]);
    printfScreenWithAttribute(screen, LTCYAN, "apic_taskpri : %08X\n", p[APIC_TASKPRI]);
    printfScreenWithAttribute(screen, LTCYAN, "apic_ldest   : %08X\n", p[APIC_LDEST]);
    printfScreenWithAttribute(screen, LTCYAN, "apic_destfmt : %08X\n", p[APIC_DESTFMT]);
    printfScreenWithAttribute(screen, LTCYAN, "apic_spur    : %08X\n", p[APIC_SPUR]);
    printfScreenWithAttribute(screen, LTCYAN, "apic_irr0    : %08X\n", p[APIC_IRR0]);
    printfScreenWithAttribute(screen, LTCYAN, "apic_icmd    : %08X\n", p[APIC_ICMD]);
    printfScreenWithAttribute(screen, LTCYAN, "apic_icmd2   : %08X\n", p[APIC_ICMD2]);
    printfScreenWithAttribute(screen, LTCYAN, "apic_ltimer  : %08X\n", p[APIC_LVT_TIMER]);
    printfScreenWithAttribute(screen, LTCYAN, "apic_lvt_i0  : %08X\n", p[APIC_LVT_I0]);
    printfScreenWithAttribute(screen, LTCYAN, "apic_lvt_i1  : %08X\n", p[APIC_LVT_I1]);
    printfScreenWithAttribute(screen, LTCYAN, "apic_icount  : %08X\n", p[APIC_ICOUNT]);
    printfScreenWithAttribute(screen, LTCYAN, "apic_ccount  : %08X\n\n", p[APIC_CCOUNT]);
    for (i = 0; i <= 0x3F; i++)
    {
       if ((i & 3) == 0)
	  printfScreenWithAttribute(screen, LTCYAN, "%08X: ", i);
       val = p[i * 4];
       printfScreenWithAttribute(screen, LTCYAN, "%08X ", val);
       if ((i & 3) == 3)
	  printfScreenWithAttribute(screen, LTCYAN, "\n");
    }

}

void dump_remote_apic(SCREEN *screen, LONG proc)
{

    LONG *p;
    LONG i, val;

    p = (LONG *) local_apic_address;
    printfScreenWithAttribute(screen, BRITEWHITE, "remote apic registers processor(%d) [0x%08X]\n", proc, p);
    for (i=0; i <= 0x3F; i++)
    {
       if ((i & 3) == 0)
	  printfScreenWithAttribute(screen, LTCYAN, "%08X: ", i);

       apic_xcall(proc, i | 0x00000B00, 1);
       while ((p[4 * 0x30] & ICR_RR_STATUS_MASK) == ICR_RR_IN_PROGRESS)
	  { p = p; };

       if ((p[4 * 0x30] & ICR_RR_STATUS_MASK) == ICR_RR_VALID)
	  val = p[0xc * 4];
       else
	  val = 0xDEADBEEF;

       printfScreenWithAttribute(screen, LTCYAN, "%08X ", val);
       if ((i & 3) == 3)
	  printfScreenWithAttribute(screen, LTCYAN, "\n");
    }

}

LONG get_apic_id(void)
{
    register int i;
    LONG *local, val;

    local = (LONG *) local_apic_address;
    val = local[APIC_LDEST];
    val >>= APIC_ID_SHIFT;
    for (i=0; i < num_procs; i++)
       if ((val >> i) & 1)
	  return (i);
    return -1;
}

void apic_mask_timer(void)
{
    LONG *local, val;
     register LONG flags;

    flags = get_flags();

    local = (LONG *) local_apic_address;
    val = local[APIC_LVT_TIMER];
    val |= APIC_VALUE_MASK;
    local[APIC_LVT_TIMER] = val;

    set_flags(flags);

    return;

}

void apic_unmask_timer(void)
{
    LONG *local, val;
     register LONG flags;

    flags = get_flags();

    local = (LONG *) local_apic_address;
    val = local[APIC_LVT_TIMER];
    val &= ~APIC_VALUE_MASK;
    local[APIC_LVT_TIMER] = val;

    set_flags(flags);

    return;

}

void configure_eisa_el(LONG intr, LONG mode)
{

    unsigned int cascade, low_int, high_int;

    if (intr == 0 || intr == 1 || intr == 2 || intr == 8 || intr == 13)
	  mode = ELCR_EDGE;

    cascade = inb(PIC2_ELCR_PORT);
    low_int = inb(PIC1_ELCR_PORT);
    high_int = (cascade << 8) | low_int;
    high_int &= ~(1 << intr);
    high_int |= (mode << intr);

    outb(PIC1_ELCR_PORT, (high_int & 0xFF));
    outb(PIC2_ELCR_PORT, ((high_int >> 8) & 0xFF));

    elcr_flags |= high_int;

    return;

}

//
//   0 - use PIC on P0
//   1 - bind to processor
//   2 - set to APIC_VALUE_LOPRI

LONG apic_set_int(LONG intr, LONG proc, LONG mode, LONG share)
{

     LONG *ioapic;
     LONG val, mval = 0, vector;
     LONG line, lnum;
     WORD elcr_reg;
     register LONG flags;

     if (intr == 2)
	intr = 9;

     if (share)
     {
	mval = APIC_VALUE_LEVEL;
	configure_eisa_el(intr, ELCR_LEVEL);
     }
     else
	configure_eisa_el(intr, ELCR_EDGE);

     if (intr > 15)
	mval = APIC_VALUE_LEVEL;

     ioapic = (LONG *) io_apic_ids[int_table[intr].io_apicid].address;
     lnum = io_apic_ids[int_table[intr].io_apicid].lnum;
     line = int_table[intr].line;
     if (line == (LONG) -1)   // if not redir entry, return error
	return -1;

     flags = get_flags();
     switch (mode)
     {
	case 0:
	   if (intr <= 15)
	      unmask_pic(intr);
	   pic_assign[intr] = 1;
	   break;

	case 1:
	   vector = vector_table[intr];
	   spin_lock(&ioapic_locks[lnum]);
	   io_apic_state[lnum].line_state[line] = TRUE;
	   ioapic[APIC_IO_REG] = APIC_REG_RDT2 + 2 * line;
	   ioapic[APIC_IO_DATA] = 1 << (proc + APIC_ID_SHIFT);
	   ioapic[APIC_IO_REG] = APIC_REG_RDT + 2 * line;
	   ioapic[APIC_IO_DATA] = (vector | APIC_VALUE_LDEST | APIC_VALUE_FIXED | mval);
	   spin_unlock(&ioapic_locks[lnum]);
	   if (intr <= 15)
	   {
	      if (elcr_flags & (1 << intr))
	      {
		 elcr_reg = inw(EISA_POLARITY_REG);
		 elcr_reg ^= (1 << line);
		 outw(EISA_POLARITY_REG, elcr_reg);
	      }
	   }
	   mask_pic(intr);
	   break;

	case 2:
	   vector = vector_table[intr];
	   spin_lock(&ioapic_locks[lnum]);
	   io_apic_state[lnum].line_state[line] = TRUE;
	   ioapic[APIC_IO_REG] = APIC_REG_RDT2 + 2 * line;
	   ioapic[APIC_IO_DATA] = processor_mask;
	   ioapic[APIC_IO_REG] = APIC_REG_RDT + 2 * line;
	   ioapic[APIC_IO_DATA] = (vector | APIC_VALUE_LDEST | APIC_VALUE_LOPRI | mval);
	   spin_unlock(&ioapic_locks[lnum]);
	   if (intr <= 15)
	   {
	      if (elcr_flags & (1 << intr))
	      {
		 elcr_reg = inw(EISA_POLARITY_REG);
		 elcr_reg ^= (1 << line);
		 outw(EISA_POLARITY_REG, elcr_reg);
	      }
	   }
	   mask_pic(intr);
	   break;

	default:
	   spin_lock(&ioapic_locks[lnum]);
	   io_apic_state[lnum].line_state[line] = 0;
	   ioapic[APIC_IO_REG] = APIC_REG_RDT2 + 2 * line;
	   ioapic[APIC_IO_DATA] = APIC_VALUE_MASK;
	   spin_unlock(&ioapic_locks[lnum]);
	   if (intr <= 15)
	   {
	      if (elcr_flags & (1 << intr))
	      {
		 elcr_reg = inw(EISA_POLARITY_REG);
		 elcr_reg ^= (1 << line);
		 outw(EISA_POLARITY_REG, elcr_reg);
	      }
	   }
	   mask_pic(intr);
	   break;

     }
     set_flags(flags);
     return 0;

}

LONG apic_clear_int(LONG intr)
{

     LONG *ioapic, line, lnum;
     register LONG flags;

     flags = get_flags();

     if (intr <= 15)
	mask_pic(intr);

     pic_assign[intr] = 0;

     ioapic = (LONG *) io_apic_ids[int_table[intr].io_apicid].address;
     lnum = io_apic_ids[int_table[intr].io_apicid].lnum;
     line = int_table[intr].line;

     spin_lock(&ioapic_locks[lnum]);
     ioapic[APIC_IO_REG] = APIC_REG_RDT + 2 * line;
     ioapic[APIC_IO_DATA] = APIC_VALUE_MASK;
     spin_unlock(&ioapic_locks[lnum]);
     set_flags(flags);

     return 0;
}

LONG apic_mask_int(LONG intr)
{
     LONG *ioapic, line, val, lnum;
     register LONG flags;

     flags = get_flags();

     if (intr <= 15)
	mask_pic(intr);

     ioapic = (LONG *) io_apic_ids[int_table[intr].io_apicid].address;
     lnum = io_apic_ids[int_table[intr].io_apicid].lnum;
     line = int_table[intr].line;

     spin_lock(&ioapic_locks[lnum]);
     ioapic[APIC_IO_REG] = APIC_REG_RDT + 2 * line;
     val = ioapic[APIC_IO_DATA];
     val |= APIC_VALUE_MASK;
     ioapic[APIC_IO_DATA] = val;
     spin_unlock(&ioapic_locks[lnum]);
     set_flags(flags);

     return 0;
}

LONG apic_unmask_int(LONG intr)
{
     LONG *ioapic, line, val, lnum;
     register LONG flags;

     flags = get_flags();

     if (intr <= 15)
	mask_pic(intr);

     ioapic = (LONG *) io_apic_ids[int_table[intr].io_apicid].address;
     lnum = io_apic_ids[int_table[intr].io_apicid].lnum;
     line = int_table[intr].line;

     spin_lock(&ioapic_locks[lnum]);
     ioapic[APIC_IO_REG] = APIC_REG_RDT + 2 * line;
     val = ioapic[APIC_IO_DATA];
     val &= ~APIC_VALUE_MASK;
     ioapic[APIC_IO_DATA] = val;
     spin_unlock(&ioapic_locks[lnum]);
     set_flags(flags);

     return 0;
}

void program_8254(void)
{

      // program 8254 Timer Chip to 1/18 second interval

      outb(0x43, 0x24);
      outb(0x40, 0);
      outb(0x40, 0);

      return;

}
