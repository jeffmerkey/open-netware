
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  DEBMPS.C
*   DESCRIP  :  Multi-Processing Deugger MPS Support for MANOS v1.0
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
#include "types.h"
#include "emit.h"
#include "os.h"
#include "dos.h"
#include "tss.h"
#include "mps.h"
#include "hal.h"
#include "xcall.h"
#include "window.h"
#include "line.h"
#include "loader.h"
#include "menu.h"
#include "rlock.h"
#include "event.h"
#include "debug.h"
#include "dparse.h"

void debug_mps_ints(SCREEN *debugScreen)
{

    union mpcentry *entry;
    int num_entries, i, type, intr, r, lines;
    struct mpconfig *config;

    config = (struct mpconfig *) vendor_table;
    entry = config->entry;

    intr = 0;
    lines = num_procs + num_ioapics + num_buses;
    num_entries = config->hdr.num_entry;
    for (i=0; i < num_entries; i++)
    {
	type = entry->bytes[0];
	if ((type == MP_ET_I_INTR) || (type == MP_ET_L_INTR))
	{
	   if (!entry->i.intr_type)
	   {
	      printfScreenWithAttribute(debugScreen, LTCYAN,
			 (bus_info[entry->i.src_bus & 0xF].bus_type == 3)
			  ? "int(%2i)  pci_dev: %08X  bus: %02X  ioapic: %02X  line: %02X\n"
			  : "int(%2i)  bus_irq: %08X  bus: %02X  ioapic: %02X  line: %02X\n",
			  intr++,
			  entry->i.src_irq,
			  entry->i.src_bus,
			  entry->i.dest_apicid,
			  entry->i.dest_line);
	      if (lines++ > 20)
	      {
		 lines = 0;
		 Pause();
	      }
	   }
	}
	entry = (union mpcentry *)((LONG)entry + (LONG)mps_size[type]);
     }

}


LONG debug_mps_ioapics(SCREEN *debugScreen)
{

    register int i;

    for (i=0; i < num_ioapics; i++)
    {
       printfScreenWithAttribute(debugScreen, BRITEWHITE,
		     "io_apic addr: 0x%08X  id: %02X  lines: %02X (%s)\n",
		     io_apic_addr[i]->io_apic_adr,
		     io_apic_addr[i]->apic_id,
		     io_apic_nlines[i],
		     ((io_apic_addr[i]->apic_vers >> 4) & 0x01) ? "Embedded" : "82489DX");
    }
    return num_ioapics;

}

LONG debug_mps_procs(SCREEN *debugScreen)
{
     register int i;

     for (i=0; i < num_procs; i++)
	printfScreenWithAttribute(debugScreen, BRITEWHITE,
			"processor: %02X  apic: %02X  addr: 0x%08X  (%s)\n", i, proc_id[i]->apic_id,
			local_apic_address,
			((proc_id[i]->apic_vers >> 4) & 0x01) ? "Embedded" : "82489DX");
     return num_procs;

}


void debug_mps_buses(SCREEN *debugScreen)
{

    int num_entries, i, r, type;
    struct mpconfig *config;
    union mpcentry *entry;

    config = (struct mpconfig *) vendor_table;
    entry = config->entry;

    num_entries = config->hdr.num_entry;
    for (i=0; i < num_entries; i++)
    {
	type = entry->bytes[0];
	if (type == MP_ET_BUS)
	{
	   for (r=0; r < 18; r++)
	   {
	      if (!strncmp(entry->b.name, bus_strings[r], 6))
		 printfScreenWithAttribute(debugScreen, BRITEWHITE,
		       "bus (%s)  bus id: %02X\n", bus_display_strings[r], entry->b.bus_id);
	   }
	}
	entry = (union mpcentry *)((LONG)entry + (LONG)mps_size[type]);
     }

}

LONG debug_mps_table(SCREEN *debugScreen)
{

    if (mps_present && vendor_table)
    {
	printfScreenWithAttribute(debugScreen, BRITEWHITE, "Intel MPS version 1.%d, table type %d\n", mps_fp->rev, pcmp_fib1);
	return 0;
    }
    else if (mps_present && pcmp_fib1)
    {
	if (pcmp_fib1)
	   printfScreenWithAttribute(debugScreen, BRITEWHITE, "Intel MPS version 1.%d, default table %d\n", mps_fp->rev, pcmp_fib1);
	else
	   printfScreenWithAttribute(debugScreen, BRITEWHITE, "Intel MPS version 1.%d\n", mps_fp->rev);
	return 0;
    }
    else if (mps_present && !pcmp_fib1)
    {
	printfScreenWithAttribute(debugScreen, BRITEWHITE, "Use default table %d\n", pcmp_fib1);
	return 0;
    }
    else
    {
	printfScreenWithAttribute(debugScreen, BRITEWHITE, "System does not support Intel MPS\n");
	return -1;
    }

}

void displayMPSTables(SCREEN *debugScreen)
{

    if (!debug_mps_table(debugScreen))
    {
       debug_mps_buses(debugScreen);
       debug_mps_procs(debugScreen);
       debug_mps_ioapics(debugScreen);
       debug_mps_ints(debugScreen);
    }
    return;
}

