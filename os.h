
/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  OS.H
*   DESCRIP  :  Operating System externs for MANOS v1.0
*   DATE     :  December 10, 1997
*
*
***************************************************************************/

#include "types.h"
#include "emit.h"

extern void spin_lock(LONG *mutex);
extern LONG spin_try_lock(LONG *mutex);
extern LONG spin_try_with_bus_lock(LONG *mutex);
extern void spin_unlock(LONG *mutex);
extern LONG atomic_xchg(LONG *lock, LONG value);

extern void *malloc(LONG  size);
extern void free(void *ptr);

extern LONG get_processor_id(void);
extern void set_processor_id(LONG);
extern LONG get_flags(void);

extern void InitializeKernel(void);
extern void CommandStartup(void);
extern void ExitOS(void);

extern LONG set_interrupt(LONG intr, LONG (*isr)(LONG), LONG share, LONG mode, LONG proc);
extern LONG clear_interrupt(LONG intr, LONG (*isr)(LONG));
extern void mask_interrupt(LONG intr);
extern void unmask_interrupt(LONG intr);
extern void eoi_interrupt(LONG intr);
extern void interrupt_entry(LONG intr);
extern void init_interrupts(void);
extern void restore_interrupts(void);

extern void outb(LONG, LONG);
extern void outw(LONG, LONG);
extern void outws(LONG, void *, LONG);
extern void outd(LONG, LONG);
extern BYTE inb(LONG);
extern WORD inw(LONG);
extern WORD inws(LONG, void *, LONG);
extern LONG ind(LONG);

extern void MicroDelay(LONG);
extern LONG DMASetUp(void *PhyMem, LONG size, LONG channel, LONG type, LONG mode);

extern LONG SystemTickCounter;
extern LONG SystemDTSCTimeCounter;
extern LONG SystemDTSCTimeBase;
extern LONG SystemCurrentTime;
extern LONG ReadDTSC(LONG *base, LONG *counter);
extern void GetSystemDTSC(LONG *base, LONG *counter);

extern void GetCMOSDate(LONG *);
extern void GetCMOSTime(LONG *);
extern LONG ReadCMOS(LONG);
extern LONG GetCurrentTime(void);

extern void CopyData(LONG *src, LONG *dest, LONG len);
extern void CopyDataB(LONG *src, LONG *dest, LONG len);
extern void CopyDataD(LONG *src, LONG *dest, LONG len);
extern void SetData(LONG *dest, LONG value, LONG len);
extern void SetDataB(LONG *dest, LONG value, LONG len);
extern void SetDataD(LONG *dest, LONG value, LONG len);
extern LONG CompareData(LONG *src, LONG *dest, LONG len);
extern LONG CompareDataB(LONG *src, LONG *dest, LONG bytes);
extern LONG CompareDataW(LONG *src, LONG *dest, LONG words);
extern LONG CompareDataD(LONG *src, LONG *dest, LONG dwords);

extern long printfScreenWithAttribute(void *screen, LONG attr, char *format, ...);
extern long printfScreen(void *screen, char *format, ...);
extern void EvaluateCommandExpression(void *screen, void *stackFrame, BYTE *p);
extern LONG EvaluateExpression(void *stackFrame, BYTE **p, LONG *type);

extern long isalpha(long chr);
extern long isdigit(long chr);
extern long isalnum(long chr);
extern long issymbols(long chr);
extern long isspace(long chr);
extern long isascii(long chr);
extern long isprint(long chr);
extern long isgraph(long chr);
extern long isctrl(long chr);
extern long islower(long chr);
extern long isupper(long chr);
extern long ispunct(long chr);
extern long isxdigit(long chr);
extern long toupper(long chr);
extern long tolower(long chr);
extern long max(long value1, long value2);
extern long min(long value1, long value2);

extern LONG TLBShootDown(void);
extern LONG VMFlushTLB(void);
extern void FlushTLB(void);
extern void LoadPDE(LONG);
extern void EnablePaging(void);
extern void map_address(LONG physical, LONG logical);

extern LONG get_flags(void);
extern void set_flags(LONG);

