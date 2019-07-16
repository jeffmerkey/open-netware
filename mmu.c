/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Darren Major
*   FILE     :  MMU.C
*   DESCRIP  :  Page System Manager for MANOS v1.0
*   DATE     :  February 5, 1997
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
#include "dos.h"
#include "tss.h"
#include "os.h"
#include "mps.h"
#include "hal.h"
#include "peexe.h"
#include "window.h"
#include "timer.h"
#include "globals.h"

#define PAGE_ADDRESS_EXT      0x00000040
#define PAGE_SIZE_EXT         0x00000008

extern	uint32		*system_mat_table;

LONG OSPAETable;
LONG OSPageTable;
LONG OSPageTableSize;
LONG OSPageTableEnd;
LONG OSPDE;
LONG DebuggerPAETable;
LONG DebuggerPageTable;
LONG DebuggerPageTableSize;
LONG DebuggerPageTableEnd;
LONG DebuggerPDE;
LONG DefaultPageSize = 4096;
LONG PhysicalPTETable;
LONG ExtraTables;
LONG	page_fault_success  = 0;
LONG	page_fault_failure = 0;
LONG	page_fault_address;
LONG	page_fault_move_counter = 0;
LONG	page_fault_move_repeater = 0;
LONG	page_fault_alloc_error = 0;
LONG	page_fault_other_counter = 0;
LONG	page_fault_post_alloc_counter = 0;

LONG	fault_test_nest_count = 0;
LONG	fault_test_unwind_count = 0;
LONG	page_fault_mutex = 0;
LONG 	page_fault_addresses[4];

uint32	fault_wakeup_skip = 0;
uint32	fault_wakeup_count = 100;
uint32	fault_wakeup_ids[64];


LONG	empty_page_pte;
LONG	*default_debugger_pte;
LONG	default_debugger_address;
LONG    *os_shaddow_pde;
LONG    *debugger_shaddow_pde;
LONG	*os_pte_table;
LONG	*debugger_pte_table;
LONG	logical_pte_table_size;

void EnableSystemPaging(void)
{
    LoadPDE(OSPDE);
    EnablePaging();
}

LONG PageFaultHandler(
	LONG FaultAddress,
	LONG running_process_id,
	LONG processor_id,
	LONG nesting_count)
{
	uint32	p_index;
	uint32	*pde;
	uint32	*pte;
	uint32	index;
	uint32	logical_page;
	uint32	ccode;
	book_node	*book;
	chapter_node	*chapter;

	spin_lock(&page_fault_mutex);
vmdbg	if (nesting_count > 1)
vmdbg		panic("page fault reentrancy detected");
vmdbg	if (processor_id > 4)
vmdbg		panic("invalid processor id passed to page fault handler");
	pde = (uint32 *) OSPDE;
	if ((pte = (uint32 *) pde[FaultAddress >> 22]) != 0)
	{
		if (((uint32) pte & PAGE_SIZE_BIT) == 0)
		{
			pte = (uint32 *) ((uint32) pte & 0xFFFFF000);
			index = (FaultAddress >> 12) & 0x000003FF;
			if (pte[index] != 0)
			{
				if ((pte[index] & PAGING_BEING_MOVED_BITS) == PAGING_BEING_MOVED_BITS)
				{
					page_fault_move_counter ++;
					while ((pte[index] & PAGING_BEING_MOVED_BITS) == PAGING_BEING_MOVED_BITS)
					{
						page_fault_move_repeater ++;
						FlushTLB();
					}
					spin_unlock(&page_fault_mutex);
					return(0);
				}
				page_fault_other_counter++;
				FlushTLB();
				spin_unlock(&page_fault_mutex);
				return(0);
			}
		}
	}

	LAS_get_current(&book);
	logical_page = FaultAddress & 0xFFFFF000;

	if (new_mat_lookup_ccb(&chapter, (uint32 *) logical_page) == 0)
	{
		if (chapter_map_page(chapter, (uint32 *) logical_page) == 0)
		{
			page_fault_success ++;
			spin_unlock(&page_fault_mutex);
			return(0);
		}
		page_fault_alloc_error ++;
		mpt_wait_for_page(FaultAddress, running_process_id);
		spin_unlock(&page_fault_mutex);
		return(-4);
	}
	page_fault_failure ++;
	spin_unlock(&page_fault_mutex);
	return(1);
}


uint32	page_fault_wake_up(void)
{
	uint32	i;
       delayThread(200);
       fault_wakeup_count = 0;
	while (1)
	{
		for (i=0; i<fault_wakeup_count; i++)
		{
			rescheduleThread((PROCESS *) fault_wakeup_ids[i]);
			fault_wakeup_ids[i] = 0;

		}
		fault_wakeup_count = 0;
	       delayThread(1);
	}
//	return(0);
}

void map_address(LONG physical_page_index, LONG	logical_page_index)
{

	LONG	*pde;
	LONG	*pte;
	LONG	*lptr;
	LONG	i;

//	printf("map address %08x to %08x \n", physical_page_index, logical_page_index);

//
//	add to os table
//
	pde = (LONG *) OSPageTable;
	if (DefaultPageSize == 0x00200000)
	{
	   pde += logical_page_index >> 9;
	   pde += logical_page_index >> 9;
	}
	else
	{
	   pde += logical_page_index >> 10;
	}

	if ((pde[0] == 0) || (pde[0] & PAGE_SIZE_BIT))
	{
	   pte = (LONG *) ExtraTables;
	   ExtraTables += LOGICAL_PAGE_SIZE;

	   for (lptr = pte, i=0; i<1024; i++, lptr ++)
	   {
	      if (DefaultPageSize == 0x00200000)
	      {
		 lptr[0] = (pde[0] == 0) ? 0 : (pde[0] & 0xFFE00000) + (i << 11) | USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
		 lptr[1] = 0;
		 lptr++;
		 i++;
	      }
	      else
	      {
		 lptr[0] = (pde[0] == 0) ? 0 : (pde[0] & 0xFFC00000) + (i << 12) | USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	      }
	   }
	   pde[0] = (LONG) pte | USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	   if (DefaultPageSize == 0x00200000)
	      pde[1] = 0;
	}
	pte = (LONG *) (pde[0] & 0xFFFFFC00);

	if (DefaultPageSize == 0x00200000)
	{
	   pte += logical_page_index & 0x000001FF;
	   pte += logical_page_index & 0x000001FF;
	   pte[0] = (physical_page_index << 12) | USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	   pte[1] = 0;
	}
	else
	{
	   pte += logical_page_index & 0x000003FF;
	   pte[0]  = (physical_page_index << 12) | USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	}

//
//	add to debugger table
//

	pde = (LONG *) DebuggerPageTable;
	if (DefaultPageSize == 0x00200000)
	{
	   pde += logical_page_index >> 9;
	   pde += logical_page_index >> 9;
	}
	else
	{
	   pde += logical_page_index >> 10;
	}

	if ((pde[0] == 0) || (pde[0] & PAGE_SIZE_BIT) || ((pde[0] & 0xFFFFF000) == (uint32) default_debugger_pte))
	{
	   pte = (LONG *) ExtraTables;
	   ExtraTables += LOGICAL_PAGE_SIZE;

	   for (lptr = pte, i=0; i < 1024; i++, lptr++)
	   {
	      if (DefaultPageSize == 0x00200000)
	      {
		 lptr[0] = (pde[0] == 0) ? 0 : (pde[0] & 0xFFE00000) + (i << 11) | USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
		 lptr[1] = 0;
		 lptr++;
		 i++;
	      }
	      else
	      {
		 lptr[0] = (pde[0] == 0) ? 0 : (pde[0] & 0xFFC00000) + (i << 12) | USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	      }
	   }
	   pde[0] = (LONG) pte | USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	   if (DefaultPageSize == 0x00200000)
	      pde[1] = 0;
	}
	else
	   pte = (LONG *) (pde[0] & 0xFFFFFC00);

	if (DefaultPageSize == 0x00200000)
	{
	   pte += logical_page_index & 0x000001FF;
	   pte += logical_page_index & 0x000001FF;
	   pte[0] = (physical_page_index << 12) | USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	   pte[1] = 0;
	}
	else
	{
	   pte += logical_page_index & 0x000001FF;
	   pte[0]  = (physical_page_index << 12) | USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	}

}

uint32	PDE_SIZE;
uint32	PTE_PAGE_COUNT;

void SetSystemPageSize(DOS_TABLE *dos, LONG *mem_start, LONG *mem_length)
{
	uint32	base_address;
	base_address = *mem_start;

      PDE_SIZE = PHYSICAL_PAGE_SIZE;
      PTE_PAGE_COUNT = (((PHYSICAL_EQUALS_LOGICAL_BOUNDARY / PHYSICAL_PAGE_SIZE) + (PHYSICAL_CHAPTER_PAGE_SIZE - 1)) >> PHYSICAL_CHAPTER_PAGE_SHIFT) << PHYSICAL_PAGE_SHIFT;
    if (dos->FEATURE_FLAGS & PAGE_ADDRESS_EXT)
    {
//       DefaultPageSize = 0x200000;
//       PDE_SIZE = 4096 * 4;
//       PTE_PAGE_COUNT *= 2;
       DefaultPageSize = 0x400000;
    }
    else if (dos->FEATURE_FLAGS & PAGE_SIZE_EXT)
    {
       DefaultPageSize = 0x400000;
    }
    else
    {
       DefaultPageSize = 4096;
	}

    OSPAETable = base_address;
    base_address += PHYSICAL_PAGE_SIZE * 2;
    OSPageTable = base_address;
    base_address += PDE_SIZE * 2;
    OSPDE = (DefaultPageSize == 0x00200000) ?  OSPAETable : OSPageTable;

    DebuggerPAETable = base_address;
    base_address += PHYSICAL_PAGE_SIZE * 2;
    DebuggerPageTable = base_address;
    base_address += PDE_SIZE * 2;
    DebuggerPDE = (DefaultPageSize == 0x00200000) ?  DebuggerPAETable : DebuggerPageTable;

    PhysicalPTETable = base_address;
    base_address += PTE_PAGE_COUNT * 2;
    ExtraTables = base_address;
    base_address += PHYSICAL_PAGE_SIZE * 8;

    os_shaddow_pde = (uint32 *) base_address;
    base_address += PDE_SIZE * 2;
    debugger_shaddow_pde = (uint32 *) base_address;
    base_address += PDE_SIZE * 2;
    default_debugger_pte = (uint32 *) base_address;
    base_address += PHYSICAL_PAGE_SIZE * 2;
    system_mat_table = (uint32 *) base_address;
    base_address += PHYSICAL_PAGE_SIZE * 2;
    empty_page_pte = base_address;
    base_address += PHYSICAL_PAGE_SIZE * 2;

    logical_pte_table_size = LOGICAL_CHAPTER_SIZE * 2 * 2;

    default_debugger_address = ((*mem_start + *mem_length + 4096) & 0xFFFFF000) + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
    SetDataD((uint32 *) *mem_start, 0, (base_address - (*mem_start)) / 4);
    SetDataD((uint32 *) empty_page_pte, EMPTY_PAGE_STAMP, PHYSICAL_PAGE_SIZE / 4);
    SetDataD((uint32 *) default_debugger_pte, default_debugger_address , PHYSICAL_PAGE_SIZE / 4);
    *mem_length -= base_address - (*mem_start);
    *mem_start = base_address;
}


void InitializeBasePageTables(void)
{

    LONG 	*pde, *local_pte;
	LONG	*os_pde, *debugger_pde, *sos_pde, *sdebugger_pde;
	register LONG i, j;

//    if (SystemMemorySize != 0)
//	;
    //  initialize page tables

    pde = (LONG *) OSPAETable;
    pde[0] = OSPageTable + (4096 * 0);
    pde[1] = 0;
    pde[2] = OSPageTable + (4096 * 1);
    pde[3] = 0;
    pde[4] = OSPageTable + (4096 * 2);
    pde[5] = 0;
    pde[6] = OSPageTable + (4096 * 3);
    pde[7] = 0;

    pde = (LONG *) DebuggerPAETable;
    pde[0] = DebuggerPageTable + (4096 * 0);
    pde[1] = 0;
    pde[2] = DebuggerPageTable + (4096 * 1);
    pde[3] = 0;
    pde[4] = DebuggerPageTable + (4096 * 2);
    pde[5] = 0;
    pde[6] = DebuggerPageTable + (4096 * 3);
    pde[7] = 0;

    os_pde = (uint32 *) OSPDE;
    debugger_pde = (uint32 *) DebuggerPDE;
    sos_pde = (uint32 *) os_shaddow_pde;
    sdebugger_pde = (uint32 *) debugger_shaddow_pde;
    local_pte = (uint32 *) PhysicalPTETable;
    for (i=0; i<PHYSICAL_EQUALS_LOGICAL_BOUNDARY; os_pde ++, debugger_pde ++, sos_pde ++, sdebugger_pde ++)
    {
	 os_pde[0] = (uint32) local_pte + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	 debugger_pde[0] = (uint32) local_pte + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	 sos_pde[0] = (uint32) local_pte + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	 sdebugger_pde[0] = (uint32) local_pte + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	 for (j=0; j < 1024; j++, local_pte++)
	 {
		 local_pte[0] = (i + (j << 12)) + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	 }
	 i+= 0x400000;
    }

	for (; i > 0; os_pde ++, debugger_pde ++, sos_pde ++, sdebugger_pde ++)
	{
	 os_pde[0] = 0;
	 debugger_pde[0] = i + PAGE_SIZE_BIT | USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	 sos_pde[0] = (uint32) empty_page_pte + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	 sdebugger_pde[0] = (uint32) empty_page_pte + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	 i+= 0x400000;
	}

	pde = (uint32 *) OSPageTable;
	pde += PHYSICAL_EQUALS_LOGICAL_BOUNDARY >> LOGICAL_CHAPTER_SHIFT;
	pde[0] = (uint32) os_shaddow_pde + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	pde[1] = (uint32) debugger_shaddow_pde + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;

	pde = (uint32 *) DebuggerPageTable;
	pde += PHYSICAL_EQUALS_LOGICAL_BOUNDARY >> LOGICAL_CHAPTER_SHIFT;
	pde[0] = (uint32) os_shaddow_pde + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
	pde[1] = (uint32) debugger_shaddow_pde + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;

	os_pte_table = (uint32 *) PHYSICAL_EQUALS_LOGICAL_BOUNDARY;
	debugger_pte_table = (uint32 *) (PHYSICAL_EQUALS_LOGICAL_BOUNDARY + LOGICAL_CHAPTER_SIZE);
}


LONG	MMU_validate_page(
	LONG	logical_page)
{
	uint32			ccode;
	chapter_node	*chapter;

	ccode = mat_validate_ccb(&chapter, (uint32 *) logical_page);
	if (ccode == 0)
	{
		return (chapter_validate_page(chapter, (uint32 *) logical_page));
	}
	if (ccode == (uint32) MAT_RESERVED_CHAPTER)
		return(0);
	return(-1);
}


void	MMU_convert_flags(
	LONG	*new_flags,
	LONG	flags)
{
	*new_flags = USER_PAGE_BIT;
	*new_flags |= (flags & MMU_FLAGS_NOT_PRESENT_BIT) ? 0 : PRESENT_BIT;
	*new_flags |= (flags & MMU_FLAGS_READ_ONLY_BIT) ? 0 : READ_WRITE_PAGE_BIT;
	*new_flags |= (flags & MMU_FLAGS_MOVING_BIT) ? PAGING_BEING_MOVED_BITS : 0;
}

void	MMU_unconvert_flags(
	LONG	*old_flags,
	LONG	flags)
{
	*old_flags = 0;
	*old_flags |= (flags & PRESENT_BIT) ? 0: MMU_FLAGS_NOT_PRESENT_BIT;
	*old_flags |= (flags & READ_WRITE_PAGE_BIT) ? 0 : MMU_FLAGS_READ_ONLY_BIT;
	*old_flags |= (flags & PAGE_ACCESSED_BIT) ? MMU_FLAGS_ACCESSED_BIT : 0;
	*old_flags |= (flags & PAGE_DIRTY_BIT) ? MMU_FLAGS_DIRTY_BIT : 0;
	*old_flags |= (flags & PAGING_BEING_MOVED_BITS) ? MMU_FLAGS_MOVING_BIT : 0;
}

LONG	MMU_raw_page_map(
	LONG	lpn,
	LONG	ppn,
	LONG	flags)
{
	uint32		*os_pte;
	uint32		*debugger_pte;

	uint32		*os_pde;
	uint32		*sos_pde;
	uint32		*debugger_pde;
	uint32		*sdebugger_pde;

	quad_entry	*p_entry;
	uint32		chapter_number;
	uint32		chapter_index;
	uint32		new_flags;
	uint32		os_ppn;
	uint32		debugger_ppn;
	uint32		ccode;

	chapter_number = lpn >> PHYSICAL_CHAPTER_PAGE_SHIFT;
	chapter_index = lpn & PHYSICAL_CHAPTER_PAGE_UNMASK;
	MMU_convert_flags(&new_flags, flags);

	os_pte = &os_pte_table[(chapter_number << PHYSICAL_PAGE_SHIFT) / 4];
	if (os_pte[chapter_index] == 0)
	{
		debugger_pte = &debugger_pte_table[(chapter_number << PHYSICAL_PAGE_SHIFT) / 4];
		os_pte[chapter_index] = (ppn << PHYSICAL_PAGE_SHIFT) + new_flags;
		debugger_pte[chapter_index] = (ppn << PHYSICAL_PAGE_SHIFT) + new_flags;
		return(0);
	}
	if (os_pte[chapter_index] == EMPTY_PAGE_STAMP)
	{
		mpt_system_alloc_page(&os_ppn, MPT_PTE_PAGE, MPT_PTE_PAGE);
		mpt_system_alloc_page(&debugger_ppn, MPT_PTE_PAGE, MPT_PTE_PAGE);
		os_pde = (uint32 *) OSPDE;
		sos_pde = os_shaddow_pde;
		debugger_pde = (uint32 *) DebuggerPDE;
		sdebugger_pde = debugger_shaddow_pde;

		sos_pde[chapter_number] = (os_ppn << PHYSICAL_PAGE_SHIFT) + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
		sdebugger_pde[chapter_number] = (debugger_ppn << PHYSICAL_PAGE_SHIFT) + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
		os_pte = &os_pte_table[(chapter_number << PHYSICAL_PAGE_SHIFT) / 4];
		debugger_pte = &debugger_pte_table[(chapter_number << PHYSICAL_PAGE_SHIFT) / 4];

		MMU_invalidate_page((uint32) os_pte >> PHYSICAL_PAGE_SHIFT);
		MMU_invalidate_page((uint32) debugger_pte >> PHYSICAL_PAGE_SHIFT);

		SetDataD(os_pte, 0, PHYSICAL_PAGE_SIZE / 4);
		SetDataD(debugger_pte, default_debugger_address, PHYSICAL_PAGE_SIZE / 4);

		os_pde[chapter_number] =  (os_ppn << PHYSICAL_PAGE_SHIFT) + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;
		debugger_pde[chapter_number] =  (debugger_ppn << PHYSICAL_PAGE_SHIFT) + USER_PAGE_BIT | READ_WRITE_PAGE_BIT | PRESENT_BIT;

		os_pte[chapter_index] = (ppn << PHYSICAL_PAGE_SHIFT) + new_flags;
		debugger_pte[chapter_index] = (ppn << PHYSICAL_PAGE_SHIFT) + new_flags;

		ccode = VMFlushTLB();
vmdbg		if (ccode)
vmdbg		   panic("flush TLB returned an error");
		return(0);
	}
vmdbg	panic("attempting to map already mapped page");
	return(-1);


}
LONG	MMU_page_map(
	LONG	lpn,
	LONG	ppn,
	LONG	flags)
{
	uint32		*os_pte;
	uint32		*debugger_pte;

	uint32		*os_pde;
	uint32		*sos_pde;
	uint32		*debugger_pde;
	uint32		*sdebugger_pde;

	quad_entry	*p_entry;
	uint32		chapter_number;
	uint32		chapter_index;
	uint32		new_flags;
	uint32		os_ppn;
	uint32		debugger_ppn;
	uint32		ccode;
	chapter_number = lpn >> PHYSICAL_CHAPTER_PAGE_SHIFT;
	chapter_index = lpn & PHYSICAL_CHAPTER_PAGE_UNMASK;
	MMU_convert_flags(&new_flags, flags);

	cli();
	os_pte = &os_pte_table[(chapter_number << PHYSICAL_PAGE_SHIFT) / 4];
	if (os_pte[chapter_index] == 0)
	{
		debugger_pte = &debugger_pte_table[(chapter_number << PHYSICAL_PAGE_SHIFT) / 4];
		os_pte[chapter_index] = (ppn << PHYSICAL_PAGE_SHIFT) + new_flags;
		debugger_pte[chapter_index] = (ppn << PHYSICAL_PAGE_SHIFT) + new_flags;
		sti();
		return(0);
	}
	spin_lock(&page_fault_mutex);
	ccode = MMU_raw_page_map(lpn, ppn, flags);
	spin_unlock(&page_fault_mutex);
	sti();
	return(ccode);
}

LONG	MMU_page_unmap(
	LONG	lpn,
	LONG	*ppn,
	LONG	*flags)
{
	uint32		*os_pte;
	uint32		*debugger_pte;

	uint32		chapter_number;
	uint32		chapter_index;

	chapter_number = lpn >> PHYSICAL_CHAPTER_PAGE_SHIFT;
	chapter_index = lpn & PHYSICAL_CHAPTER_PAGE_UNMASK;
	os_pte = &os_pte_table[(chapter_number << PHYSICAL_PAGE_SHIFT) / 4];
	cli();
	if ((os_pte[chapter_index] != 0) && (os_pte[chapter_index] != EMPTY_PAGE_STAMP))
	{
		debugger_pte = &debugger_pte_table[(chapter_number << PHYSICAL_PAGE_SHIFT) / 4];
		if (ppn != 0)
			*ppn = os_pte[chapter_index] & 0xFFFFF000;
		if (flags != 0)
			MMU_unconvert_flags(flags, os_pte[chapter_index] & 0x00000FFF);
		os_pte[chapter_index] = 0;
		debugger_pte[chapter_index] = default_debugger_address;
		sti();
		return(0);
	}
	if (ppn != 0)
		*ppn = 0;
	if (flags != 0)
		*flags = 0;
	sti();
	return(-1);
}

LONG	MMU_page_get_info(
	LONG	lpn,
	LONG	*ppn,
	LONG	*flags)
{
	uint32		*os_pte;
	uint32		*debugger_pte;

	uint32		chapter_number;
	uint32		chapter_index;

	chapter_number = lpn >> PHYSICAL_CHAPTER_PAGE_SHIFT;
	chapter_index = lpn & PHYSICAL_CHAPTER_PAGE_UNMASK;
	os_pte = &os_pte_table[(chapter_number << PHYSICAL_PAGE_SHIFT) / 4];
	if ((os_pte[chapter_index] != 0) && (os_pte[chapter_index] != EMPTY_PAGE_STAMP))
	{
		*ppn = os_pte[chapter_index] & 0xFFFFF000;
		MMU_unconvert_flags(flags, os_pte[chapter_index] & 0x00000FFF);
		return(0);
	}
vmdbg	panic("unmapped page number passed to MMU_page_get_info");
	return(-1);
}

LONG	MMU_page_set_info(
	LONG	lpn,
	LONG	ppn,
	LONG	flags)
{
	uint32		*os_pte;
	uint32		*debugger_pte;

	uint32		chapter_number;
	uint32		chapter_index;
	uint32		new_flags;

	chapter_number = lpn >> PHYSICAL_CHAPTER_PAGE_SHIFT;
	chapter_index = lpn & PHYSICAL_CHAPTER_PAGE_UNMASK;
	MMU_convert_flags(&new_flags, flags);

	os_pte = &os_pte_table[(chapter_number << PHYSICAL_PAGE_SHIFT) / 4];
	if ((os_pte[chapter_index] != 0) && (os_pte[chapter_index] != EMPTY_PAGE_STAMP))
	{
		if (ppn == 0)
			ppn = os_pte[chapter_index] & 0xFFFFF000;
		os_pte[chapter_index] = ppn + new_flags;
		return(0);
	}
vmdbg	panic("unmapped page number passed to MMU_page_set_info");
	return(-1);
}


LONG	MMU_invalidate_page(
	LONG	lpn)
{
	VMFlushTLB();
	return(lpn);
}


LONG	MMU_TLB_flush()
{
	VMFlushTLB();
	return(0);

}

LONG	MMU_TLB_shoot_down()
{
	VMFlushTLB();
	return(0);

}


