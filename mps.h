

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
*   FILE     :  MPS.H
*   DESCRIP  :  Multi Processing Specification (MPS) for MANOS v1.0
*   DATE     :  January 6, 1998
*
***************************************************************************/

#include "types.h"
#include "emit.h"


#define  _82489APIC_MASK            0x000000F0
#define	 APIC_IO_REG	            0x00000000
#define  APIC_IO_DATA	            0x00000004

// APIC registers are 128 bit aligned.  accesses are offset * 4

#define  APIC_TASKPRI	            (4 * 0x00000008)
#define  APIC_ID		    (4 * 0x00000002)
#define  APIC_VERS                  (4 * 0x00000003)
#define  APIC_LDEST	            (4 * 0x0000000D)
#define  APIC_EOI 	            (4 * 0x0000000B)
#define  APIC_DESTFMT	            (4 * 0x0000000E)
#define  APIC_SPUR		    (4 * 0x0000000F)
#define  APIC_IRR0		    (4 * 0x00000020)
#define  APIC_ICMD		    (4 * 0x00000030)
#define  APIC_ICMD2	            (4 * 0x00000031)
#define  APIC_LVT_TIMER	            (4 * 0x00000032)
#define  APIC_LVT_I0	            (4 * 0x00000035)
#define  APIC_LVT_I1	            (4 * 0x00000036)
#define  APIC_ICOUNT	            (4 * 0x00000038)
#define  APIC_CCOUNT	            (4 * 0x00000039)

// APIc command values

#define  APIC_REG_ID		    0x00000000
#define  APIC_REG_RDT		    0x00000010
#define  APIC_REG_RDT2	            0x00000011
#define  APIC_VALUE_MASK	    0x00010000
#define  APIC_VALUE_TOALL	    0x7FFFFFFF
#define  APIC_LOGDEST(c)	    (0x40000000 >> (c))
#define  APIC_VALUE_IM_OFF	    0x80000000

#define  APIC_VALUE_FIXED	    0x00000000
#define  APIC_VALUE_LOPRI	    0x00000100
#define  APIC_VALUE_NMI		    0x00000400

#define  APIC_VALUE_RESET	    0x00000500
#define  APIC_VALUE_STARTUP	    0x00000600
#define  APIC_VALUE_EXTINT	    0x00000700
#define  APIC_VALUE_PENDING	    0x00001000
#define  APIC_VALUE_PDEST	    0x00000000
#define  APIC_VALUE_LDEST	    0x00000800
#define  APIC_VALUE_POLOW	    0x00002000
#define  APIC_VALUE_POHIGH	    0x00000000
#define  APIC_VALUE_ASSERT	    0x00004000
#define  APIC_VALUE_DEASSERT	    0x00000000
#define  APIC_VALUE_EDGE	    0x00000000
#define  APIC_VALUE_LEVEL	    0x00008000
#define  APIC_VALUE_XTOSELF	    0x00040000
#define  APIC_VALUE_XTOALL	    0x00080000

// APIC timer init values

#define  HERTZ                      100
#define  NANOSECOND_PULSE_RATE      90
#define  APIC_CLKNUM                ((1000000000/NANOSECOND_PULSE_RATE)/HERTZ)
#define	 APIC_ID_MASK               0x0F000000
#define	 APIC_ID_SHIFT              24
#define  TIMER_VECTOR               0x00000028

// IOAPIC interrupt delivery modes

#define	 DELIVERY_MODE_MASK         0x00000700
#define	 DELIVER_FIXED              0x00000000
#define	 DELIVER_LOW_PRIORITY       0x00000100
#define	 DELIVER_SMI                0x00000200
#define	 DELIVER_REMOTE_READ        0x00000300
#define	 DELIVER_NMI                0x00000400
#define	 DELIVER_INIT               0x00000500
#define	 DELIVER_INIT_REASSERT_ALL  0x00088500
#define	 DELIVER_INIT_REASSERT_SELF 0x00048500
#define	 DELIVER_RESET_HOLD_ALL     0x0008C500
#define	 DELIVER_EXTINT             0x00000700

// APIC addressing mode values

#define	 PHYSICAL_DESTINATION       0x00000000
#define	 LOGICAL_DESTINATION        0x00000800
#define	 DELIVERY_PENDING           0x00001000
#define	 ACTIVE_LOW                 0x00002000
#define	 REMOTE_IRR                 0x00004000
#define	 LEVEL_TRIGGERED            0x00008000
#define	 INTERRUPT_MASKED           0x00010000
#define	 INTERRUPT_UNMASKED         0x00000000
#define	 PERIODIC_TIMER             0x00020000

#define	 TIMER_BASE_CLOCK           0x00000000
#define	 TIMER_BASE_TMBASE          0x00040000
#define	 TIMER_BASE_DIVIDER         0x00080000

#define	 ICR_LEVEL_ASSERTED         0x00004000
#define	 ICR_RR_STATUS_MASK         0x00030000
#define	 ICR_RR_INVALID             0x00000000
#define	 ICR_RR_IN_PROGRESS         0x00010000
#define	 ICR_RR_VALID               0x00020000
#define	 ICR_SHORTHAND_MASK         0x000C0000
#define	 ICR_USE_DEST_FIELD         0x00000000
#define	 ICR_SELF                   0x00040000
#define	 ICR_ALL_INCL_SELF          0x00080000
#define	 ICR_ALL_EXCL_SELF          0x000C0000

#define	 LU_DEST_FORMAT_MASK        0xF0000000
#define	 LU_DEST_FORMAT_FLAT        0xFFFFFFFF
#define	 LU_UNIT_ENABLED            0x00000100

//
//  8259 PIC registers
//

#define  MAX_PICS     3
#define  PIC_0     0x20
#define  PIC_1     0xA0
#define  PIC_2     0x30
#define  MASK_0    0x21
#define  MASK_1    0xA1
#define  MASK_2    0x31
#define  PIC_EOI   0x20
#define  MAX_INTS      64
#define  CHAIN_LENGTH  16

//
//  EISA polarity registers
//

#define  EISA_POLARITY_REG          0x00000C0E
#define	 PIC1_ELCR_PORT             0x000004D0
#define	 PIC2_ELCR_PORT             0x000004D1
#define  ELCR_EDGE                  0
#define  ELCR_LEVEL                 1

/* Definitions for Intel PC+MP Platform Specification */

/* Misc. */
#define EBDA_BASE	0x0009FC00    /* base of EBDA default location 639k	*/
#define EBDA_PTR	0x0000040E    /* pointer to base of EBDA segment 	*/
#define BMEM_PTR	0x00000413    /* pointer to installed base memory in kbyte*/
#define CPQ_RESET_VECT  0x00000467    /* reset vector location */

/* PC+MP Interrupt Mode Control Registers */

#define IMCR_ADDR	0x22
#define IMCR_DATA	0x23
#define CMOSCTL         0x8F    /* CMOS warm-boot addr                */
#define CMOSWARM        0x0A    /* CMOS warm-boot flag                */
#define CMOS_ADDR       0x70
#define CMOS_DATA       0x71

#define MP_IMCRP	0x80	/* IMCR present */
#define MP_IMCRA	0x0	/* IMCR absent */
#define MP_DEF_TYPE	6	/* default table to use */
#define MP_DEF_IMCR 	MP_IMCRA/* default IMCRP to use */

/* types of entries (stored in bytes[0]) */
#define MP_ET_PROC	0	/* processor */
#define MP_ET_BUS	1	/* bus */
#define MP_ET_IOAPIC	2	/* i/o apic */
#define MP_ET_I_INTR	3	/* interrupt assignment -> i/o apic */
#define MP_ET_L_INTR	4	/* interrupt assignment -> local apic */

/* flag values for intr */
#define MP_INTR_POVAL	1
#define MP_INTR_POLOW	2
#define MP_INTR_ELVAL	4
#define MP_INTR_ELLEVEL 8

#define MAX_BUSES       16
#define MAX_IOAPICS     16

struct pcmp_fptr {	/* PC+MP floating pointer structure */
   BYTE sig[4];		/* signature "_MP_"	*/
   LONG *paddr;		/* physical address pointer to MP table */
   BYTE len;
   BYTE rev;		/* table length in 16 byte; revision #	*/
   BYTE checksum;	/* checksum				*/
   BYTE mp_byte[5];     /* MP feature byte 1: default system type */
};			/* MP feature byte 2: bit 7: IMCRP	*/

struct mpchdr {
   BYTE sig[4];	  	/* signature "MPAT" */
   WORD tbl_len;	/* length of table */
   BYTE spec_rev;	/* MP+AT specification revision no. */
   BYTE checksum;
   BYTE oem_id[8];
   BYTE product_id[12];
   LONG *oem_ptr;	        /* pointer to oem table (optional) */
   WORD oem_len;		/* length of above table */
   WORD num_entry;	        /* number of 'entry's to follow */
   LONG loc_apic_adr;	        /* local apic physical address */
   LONG reserved;
};

struct mpe_proc {
   BYTE entry_type;
   BYTE apic_id;
   BYTE apic_vers;
   BYTE cpu_flags;
   LONG cpu_signature;
   LONG features;
   LONG reserved[2];
};

struct mpe_bus {
   BYTE entry_type;
   BYTE bus_id;
   BYTE name[6];
};

struct mpe_ioapic {
   BYTE entry_type;
   BYTE apic_id;
   BYTE apic_vers;
   BYTE ioapic_flags;
   LONG io_apic_adr;
};

struct mpe_intr {
   BYTE entry_type;
   BYTE intr_type;
   WORD intr_flags;
   BYTE src_bus;
   BYTE src_irq;
   BYTE dest_apicid;
   BYTE dest_line;
};

struct mpe_local {
   BYTE entry_type;
   BYTE intr_type;
   WORD intr_flags;
   BYTE src_bus;
   BYTE src_irq;
   BYTE dest_apicid;
   BYTE dest_line;
};

union mpcentry {
   BYTE bytes[20];
   struct mpe_proc p;
   struct mpe_bus b;
   struct mpe_ioapic a;
   struct mpe_intr i;
   struct mpe_local l;
};

struct mpconfig {
   struct mpchdr hdr;
   union mpcentry entry[1];
};


struct bus_data {
   BYTE bus_id;
   BYTE bus_type;
};

struct io_apic_state_array {
   BYTE line_state[24];
};

struct intr_table {
   LONG line;
   LONG io_apicid;
   LONG dev;
   LONG bus;
   LONG use;
};

typedef struct _IOAPIC_IDS {
   LONG address;
   LONG lnum;
} IOAPIC_IDS;


//
//  Intel mps apic external data
//

// MPS 1.0 standard tables

extern LONG mps_size[];
extern BYTE mps_default_table_1[];
extern BYTE mps_default_table_2[];
extern BYTE mps_default_table_3[];
extern BYTE mps_default_table_4[];
extern BYTE mps_default_table_5[];
extern BYTE mps_default_table_6[];
extern BYTE mps_default_table_7[];
extern BYTE mps_default_table_8[];
extern LONG apic_defaults[];
extern BYTE *bus_strings[];
extern BYTE *bus_display_strings[];
extern BYTE vector_table[];
extern struct intr_table int_table[];
extern BYTE irq_control[];
extern BYTE irq_mask[];
extern BYTE mask_value[];
extern LONG elcr_flags;
extern LONG mps_present;
extern LONG pcmp_fib1;
extern LONG pcmp_fib2;
extern struct pcmp_fptr *mps_fp;
extern struct mpchdr *vendor_table;
extern LONG num_buses;
extern LONG num_procs;
extern LONG num_ioapics;
extern struct mpe_ioapic *io_apic_addr[];
extern struct io_apic_state_array io_apic_state[];
extern IOAPIC_IDS io_apic_ids[];
extern LONG io_apic_entry_num[];
extern LONG io_apic_nlines[];
extern struct bus_data bus_info[];
extern struct mpe_proc *proc_id[];
extern LONG local_apic_address;
extern LONG processor_mask;
extern LONG warm_reset_vector;

//
//  Intel mps apic functions
//

extern LONG mps_find_fp(LONG begin, LONG end);
extern LONG mps_locate(void);
extern void mps_ints(void);
extern LONG mps_ioapics(void);
extern LONG mps_procs(void);
extern void mps_buses(void);
extern LONG MPSDetect(void);
extern void apic_eoi(LONG intr);
extern LONG apic_xcall(LONG proc, LONG command, LONG type);
extern LONG apic_init(LONG proc);
extern void apic_close(LONG proc);
extern void write_boot_vector(LONG addr);
extern void apic_timer_start(void);
extern void apic_timer_stop(void);
extern void disable_ioapic_ints(void);
extern void enable_ioapic_ints(void);
extern LONG apic_activate_processor(LONG proc, LONG addr);
extern void dump_int_table(SCREEN *);
extern void dump_ioapic(SCREEN *, LONG num);
extern void dump_local_apic(SCREEN *);
extern void dump_remote_apic(SCREEN *, LONG proc);
extern LONG get_apic_id(void);
extern void apic_mask_timer(void);
extern void apic_unmask_timer(void);
extern void configure_eisa_el(LONG intr, LONG mode);
extern LONG apic_set_int(LONG intr, LONG proc, LONG mode, LONG share);
extern LONG apic_clear_int(LONG intr);
extern LONG apic_mask_int(LONG intr);
extern LONG apic_unmask_int(LONG intr);
extern LONG apic_directed_nmi(LONG proc);
extern void program_8254(void);

//
//  internal 8259 PIC HAL section
//

extern void unmask_pic(LONG intr);
extern void mask_pic(LONG intr);
extern void mask_pic_timer(void);
extern void unmask_pic_timer(void);
extern void mask_system(void);
extern void unmask_system(void);
extern LONG pic_set_int(LONG intr, LONG proc, LONG mode, LONG share);
extern LONG pic_clear_int(LONG intr);
extern void pic_eoi(LONG intr);
