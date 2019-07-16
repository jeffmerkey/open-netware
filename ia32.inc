

;***************************************************************************
;
;   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
;                            Reserved.
;
;   This program is an unpublished work of TRG, Inc. and contains
;   trade secrets and other proprietary information.  Unauthorized
;   use, copying, disclosure, or distribution of this file without the
;   consent of TRG, Inc. can subject the offender to severe criminal
;   and/or civil penalities.
;
;
;   AUTHOR   :  Jeff V. Merkey
;   FILE     :  IA32.INC
;   DESCRIP  :  IA32 Specific Processor Structures and Defines for MANOS v1.0
;   DATE     :  August 27, 1998
;
;
;***************************************************************************


TaskStateSegment struc
    tBackLink          dw     0
    tTSSReserved       dw     0
    tESP0              dd     0
    tSS0               dw     0
    tSS0res            dw     0
    tESP1              dd     0
    tSS1               dw     0
    tSS1res            dw     0
    tESP2              dd     0
    tSS2               dw     0
    tSS2res            dw     0
    tCR3               dd     0
    tEIP 	       dd     0
    tSystemFlags       dd     0
    tEAX       	       dd     0
    tECX               dd     0
    tEDX 	       dd     0
    tEBX     	       dd     0
    tESP     	       dd     0
    tEBP       	       dd     0
    tESI    	       dd     0
    tEDI     	       dd     0
    tES                dw     2 dup (0)
    tCS                dw     2 dup (0)
    tSS                dw     2 dup (0)
    tDS        	       dw     2 dup (0)
    tFS   	       dw     2 dup (0)
    tGS       	       dw     2 dup (0)
    tLDT       	       dw     2 dup (0)
    tIOPermissMap      dw     2 dup (0)
TaskStateSegment ends

GDTDescriptor struc
    Limit		dw	0FFFFh
    Base1		dw	0
    Base2		db	0
    GDTType		db	10010010b
    OtherType		db	11001111b
    Base3		db	0
GDTDescriptor ends

LDTDescriptor	struc
    LDTLimit		dw	0FFFFh
    LDTBase1		dw	0
    LDTBase2		db	0
    LDTGDTType		db	10000010b
    LDTOtherType	db	10001111b
    LDTBase3		db	0
LDTDescriptor	ends

IDTDescriptor	struc
    IDTLow	        dw	0
    IDTSegment		dw	8h
    IDTSkip		db	0
    IDTFlags		db	8Eh
    IDTHigh	        dw	0
IDTDescriptor	ends

TSSDescriptor	struc
    TSSLimit		dw	0080h
    TSSBase1		dw	0
    TSSBase2		db	0
    TSSType		db	10001001b
    TSSOtherType	db	00000000b
    TSSBase3		db	0
TSSDescriptor	ends

TSSGate	struc
    TSSRes1  		dw	0
    TSSSelector		dw	0
    TSSRes2 		db	0
    TSSFlags		db	10000101b
    TSSRes3 		dw	0
TSSGate	ends

REAL_MODE_REGS struc
    union
       struc
	  r_ax       dw   0
	  r_bx       dw   0
	  r_cx       dw   0
	  r_dx       dw   0
	  r_si       dw   0
	  r_di       dw   0
	  r_cflag    dw   0
	  r_flags    dw   0
       ends
       struc
	  r_al       db   0
	  r_ah       db   0
	  r_bl       db   0
	  r_bh       db   0
	  r_cl       db   0
	  r_ch       db   0
	  r_dl       db   0
	  r_dh       db   0
	  r_pad      dw   4 dup (0)
       ends
    ends
    r_es      dw   0
    r_cs      dw   0
    r_ss      dw   0
    r_ds      dw   0
REAL_MODE_REGS ends
