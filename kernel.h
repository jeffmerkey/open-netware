

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  KERNEL.H
*   DESCRIP  :  Multi-Processing Kernel for MANOS v1.0
*   DATE     :  December 10, 1997
*
*
***************************************************************************/

#include "types.h"
#include "version.h"

#define KERNEL_DEBUG       0
#define RWLOCK_TRACE       1

#define LWP_INACTIVE       0
#define LWP_ACTIVE         1
#define LWP_RUNNING        2

#define USER_MEMORY        0x00000000
#define KERNEL_MEMORY      0xFEEDBEEF
#define PROCESS_SIGNATURE  0xBEEFBEEF

#define SEMA_SIGNATURE     0xC0EDBEEF
#define LSEMA_SIGNATURE    0xC0EDBEFF
#define MUTEX_SIGNATURE    0xC1EDBEEF
#define LMUTEX_SIGNATURE   0xC1EDBEFF
#define RWLOCK_SIGNATURE   0xC2EDBEEF
#define LRWLOCK_SIGNATURE  0xC2EDBEFF

#define STACK_END          0xDEADBEEF

#define IDLE_THREAD       1
#define KERNEL_THREAD     2

#define PS_INIT           0
#define PS_ACTIVE         1
#define PS_SLEEP          2
#define PS_SYNC           3
#define PS_LWP            4
#define PS_HALT           5
#define PS_SHUTDOWN       6
#define PS_SWAPPED        7

#define PROCESS_DELAYED               0x000000C1
#define PROCESS_AWAKENED              0x000000C2
#define PROCESS_BLOCKED_KEYBOARD      0x000000C3
#define PROCESS_BLOCKED_SYNC          0x000000C4
#define PROCESS_READER_BLOCKED_SYNC   0x000000C5
#define PROCESS_WRITER_BLOCKED_SYNC   0x000000C6

#if (IA32_OPCODE)

//  128 bytes total size numeric register context

typedef struct _NPXREG {
  WORD sig0;        //  10 bytes total size this structure
  WORD sig1;
  WORD sig2;
  WORD sig3;
  WORD exponent:15;
  WORD sign:1;
} NUMERIC_REGISTER_CONTEXT;

typedef struct _NPX {
  LONG control;
  LONG status;
  LONG tag;
  LONG eip;
  LONG cs;
  LONG dataptr;
  LONG datasel;
  NUMERIC_REGISTER_CONTEXT reg[8];    // 80 bytes
  LONG pad[5];
} NUMERIC_FRAME;

//  128 bytes total size register context

typedef struct _CONTEXT_FRAME {
    WORD cBackLink;
    WORD cTSSReserved;
    LONG cESP0;
    WORD cSS0;
    WORD cSS0res;
    LONG cESP1;
    WORD cSS1;
    WORD cSS1res;
    LONG cESP2;
    WORD cSS2;
    WORD cSS2res;
    LONG cCR3;
    LONG cEIP;
    LONG cSystemFlags;
    LONG cEAX;
    LONG cECX;
    LONG cEDX;
    LONG cEBX;
    LONG cESP;
    LONG cEBP;
    LONG cESI;
    LONG cEDI;
    LONG cES;
    LONG cCS;
    LONG cSS;
    LONG cDS;
    LONG cFS;
    LONG cGS;
    LONG cLDT;
    LONG cIOPermissMap;
    LONG pad[6];
} CONTEXT_FRAME;

#endif

// NOTE: these structures (SEMA, PROCESS) are also contained in kernel.inc
// If you change these structures, then you must change them there
// as well

typedef struct _SEMA {
   struct _SEMA *next;
   struct _SEMA *prior;
   struct _SEMA *kernelNext;
   struct _SEMA *kernelPrior;
   long sema_value;
   void *sema_head;
   void *sema_tail;
   LONG sema_mutex;
   LONG sema_waiters;
   LONG sema_signature;
   LONG timerID;
} sema_t;

// general purpose sync structure

typedef struct _SYNC {
   struct _SYNC *next;
   struct _SYNC *prior;
   struct _SYNC *kernelNext;
   struct _SYNC *kernelPrior;
   LONG value;
   LONG mutex;
   LONG waiters;
   LONG signature;
   LONG owner;
   LONG owner_count;
} sync_t;

// general purpose sync structure

typedef struct _SYNC_Q {
   struct _SYNC_Q *next;
   struct _SYNC_Q *prior;
   struct _SYNC_Q *kernelNext;
   struct _SYNC_Q *kernelPrior;
   LONG value;
   void *head;
   void *tail;
   LONG mutex;
   LONG waiters;
   LONG signature;
   LONG owner;
   LONG owner_count;
} sync_q_t;

// general purpose sync structure

#if RWLOCK_TRACE

#define READER_LOCK        0x11
#define READER_UNLOCK      0x21
#define READER_SLEEP       0x31
#define READER_WAKE        0x41
#define WRITER_LOCK        0x51
#define WRITER_UNLOCK      0x61
#define WRITER_SLEEP       0x71
#define WRITER_WAKE        0x81
#define WRITER_TO_READER   0x91
#define READER_TO_WRITER   0xA1

#define RREAD_LOCK         0xB1
#define RREAD_TRY_LOCK     0xC1
#define RREAD_UNLOCK       0xD1
#define WWRITE_LOCK        0xE1
#define WWRITE_TRY_LOCK    0xF1
#define WWRITE_UNLOCK      0x1F
#define WWRITER_TO_READER  0x2F
#define RREADER_TO_WRITER  0x3F


#endif

typedef struct _RW_Q {
   struct _RW_Q *next;
   struct _RW_Q *prior;
   struct _RW_Q *kernelNext;
   struct _RW_Q *kernelPrior;
   LONG readers;
   LONG writers;
   void *head;
   void *tail;
   LONG mutex;
   LONG waiters;
   LONG signature;
   LONG owner;
   LONG owner_count;
   LONG flags;

#if RWLOCK_TRACE

   LONG RWProcess[256];
   LONG RWTarget[256];
   LONG RWOp[256];
   LONG RWTime[256];
   LONG RWType[256];
   LONG RWIndex;

#endif

} rw_q_t;

typedef sync_t    spin_t;
typedef sync_q_t  mutex_t;
typedef sync_q_t  rmutex_t;
typedef rw_q_t    rwlock_t;

typedef struct _PROCESS_CONTROL {
   struct _PROCESS_CONTROL *next;
   struct _PROCESS_CONTROL *prior;
   struct _PROCESS_CONTROL *kernelNext;
   struct _PROCESS_CONTROL *kernelPrior;
   struct _PROCESS_CONTROL *syncNext;
   struct _PROCESS_CONTROL *syncPrior;
   LONG     processID;
   LONG     typeID;
   LONG     *stackPointer;
   LONG     *stackLimit;
   LONG     *stackEnd;
   LONG     processSignature;
   LONG     refCount;
   LONG     threadMutex;
   LONG     processorBinding;
   LONG     realTimePriority;
   LONG     inheritedPriority;
   LONG     setMemberPriority;
   BYTE     processName[64];
   LONG     (*startAddress)(struct _PROCESS_CONTROL *);
   void     *startParm;
   LONG     utilizationCount;
   LONG     threadState;
   LONG     threadType;
   LONG     threadFlags;
   LONG     codeSegment;
   LONG     threadDataArea[10];
   LONG     lastProcessor;
   LONG     resourceFlag;
   LONG     threadSemaphore;
   LONG     threadUtilization;

   //  NOTE:  this section mirrors a timer control block
   //  if you change the timer comtrol block structure
   //  in timer.h, then you must change this structure
   //  as well and in context.386.

   void     *delayNext;
   void     *delayPrior;
   LONG     delaySignature;
   LONG     delayWaitTime;
   LONG     delayCurrTime;
   void     *delayParameter;
   void     (*delayFunction)(void *);
   LONG     delayFlags;
   LONG     delayStamp;

   LONG     keyBuffer;
   LONG     typeAheadCount;
   LONG     threadSubUtil;
   LONG     threadPreempt;
   LONG     bindswFlags;

   CONTEXT_FRAME    ProcessContext;
   NUMERIC_FRAME    NumericContext;

   void     *ModuleContext;
   void     *AddressSpace;
   sema_t   thread_sema;
   void     *syncObject;
   LONG     syncState;
   LONG     syncFlag;

} PROCESS;

typedef struct _LIGHT_WEIGHT_PROCESS {
   struct _LIGHT_WEIGHT_PROCESS *next;
   struct _LIGHT_WEIGHT_PROCESS *prior;
   LONG (*procedureAddress)(void *);
   LONG lwpState;
   LONG lwpDelay;
   LONG lwpFlags;
   void *lwpArgument;
} LWP;

typedef struct _PROCESSOR_QUEUE {
   PROCESS *idle_process;
   PROCESS *running_process;
   PROCESS *local_queue_head;
   PROCESS *local_queue_tail;
   PROCESS *bind_queue_head;
   PROCESS *bind_queue_tail;
   LONG bind_queue_mutex;
   LWP *lwp_queue_head;
   LWP *lwp_queue_tail;
   LWP *lwp_bind_queue_head;
   LWP *lwp_bind_queue_tail;
   LONG lwp_bind_queue_mutex;
   LWP *soft_int_head;
   LWP *soft_int_tail;
   LONG soft_int_mutex;
   LWP *fast_lwp_head;
   LWP *fast_lwp_tail;
   LONG fast_lwp_mutex;
   PROCESS *lwp_worker_queue_head;
   PROCESS *lwp_worker_queue_tail;
   LONG local_queue_count;
   LONG bind_queue_count;
   LONG lwp_queue_count;
   LONG lwpExecCount;
   LONG idleCount;
   LONG utilizationCount;
   LONG sampleCount;
   LONG processorUtilization;
   LONG eligibleCount;
   LONG activeCount;
   LONG lwpProcessCounter;
   LONG sleepDisallowed;
   LONG processorNumber;
   PROCESS *bindsw_queue_head;
   PROCESS *bindsw_queue_tail;
   LONG bindsw_queue_mutex;
   void *ModuleContext;
   void *AddressSpace;
} PROCESSOR_SET;

extern PROCESSOR_SET *ptable[MAX_PROCESSORS];
extern PROCESSOR_SET processorSet[MAX_PROCESSORS];
extern PROCESS idle_p[MAX_PROCESSORS];
extern PROCESS *dispatch_queue_head;
extern PROCESS *dispatch_queue_tail;
extern LONG dispatch_queue_mutex;
extern LWP *lwp_dispatch_queue_head;
extern LWP *lwp_dispatch_queue_tail;
extern LONG lwp_dispatch_queue_mutex;
extern LONG dispatch_queue_count;
extern LONG lwp_dispatch_queue_count;

extern PROCESS *get_dispatch(void);
extern void put_dispatch(PROCESS *p);
PROCESS *get_rundown(void);
void put_rundown(PROCESS *p);
extern PROCESS *get_local(LONG proc);
extern void put_local(PROCESS *p, LONG proc);
extern PROCESS *get_bind(LONG proc);
extern void put_bind(PROCESS *p, LONG proc);
extern void put_bind_pset(PROCESS *p, PROCESSOR_SET *pset);
extern PROCESS *get_bindsw(LONG proc);
extern void put_bindsw(PROCESS *p, LONG proc);
extern LWP *get_lwp_bind(LONG proc);
extern void put_lwp_bind(LWP *p, LONG proc);
extern LWP *get_dispatch_lwp(void);
extern void put_dispatch_lwp(LWP *p);
extern LWP *get_local_lwp(LONG proc);
extern void put_local_lwp(LWP *p, LONG proc);
extern void thread_switch(void);
extern void thread_switch_target(PROCESS *p);
extern PROCESS *get_running_process(void);
extern void idle_loop(LONG p);

extern switch_process_context(PROCESS *p, PROCESS *t);
extern preempt_restore(PROCESS *target);

PROCESS *get_lwp_thread(LONG proc);
void put_lwp_thread(PROCESS *p, LONG proc);

extern LONG close_processor(LONG p);
extern void processor_init(LONG p);
extern LONG activate_processor(LONG p);
extern void processor_exit(LONG p);

extern PROCESS *commandP;
extern LONG LWPCount;
extern void LWPProcess(LONG proc);

extern void EnterDebugger(void);

extern PROCESS *get_free_process(void);
extern void put_free_process(PROCESS *p);
extern void remove_process(PROCESS *p);
extern void insert_process(PROCESS *p);
extern void threadCleanup(PROCESS *p);
extern void threadStart(PROCESS *p);
extern PROCESS *createThread(BYTE *name, LONG (*start)(), LONG stackSize, void *parm, LONG bind);
extern PROCESS *createThreadContext(BYTE *name, LONG (*start)(), LONG stackSize, void *parm, LONG bind);
extern PROCESS *sleepThread(void);
extern LONG rescheduleThread(PROCESS *p);
extern LONG killThread(PROCESS *p);
extern LONG delayThread(LONG delay);

extern LONG scheduleLocalLWP(LWP *lwp);
extern LONG cancelLocalLWP(LWP *lwp);
extern LONG scheduleBoundLWP(LWP *lwp, LONG proc);
extern LONG cancelBoundLWP(LWP *lwp);
extern LONG scheduleLWP(LWP *lwp);
extern LONG cancelLWP(LWP *lwp);
extern LONG scheduleFastLWP(LWP *lwp);
extern LONG cancelFastLWP(LWP *lwp);
extern LONG scheduleSoftInt(LWP *lwp);
extern LONG cancelSoftInt(LWP *lwp);

extern PROCESS *systemThreadHead;
extern PROCESS *systemThreadTail;
extern LONG systemThreadmutex;

extern PROCESS *rundown_queue_head;
extern PROCESS *rundown_queue_tail;
extern LONG rundown_queue_mutex;

extern LONG active_processors;
extern LONG initial_stack_array[MAX_PROCESSORS];
extern LONG initial_stack_size_array[MAX_PROCESSORS];

extern LONG systemUtilization;
extern LONG processorUtilization[MAX_PROCESSORS];
extern LONG processorCountArray[MAX_PROCESSORS];
extern LONG threadCountArray[MAX_PROCESSORS];
extern LONG idleCountArray[MAX_PROCESSORS];

extern LONG IDLE_COUNT;
extern LONG SAMPLE_INTERVAL;
extern LONG DELTA;
extern LONG LWP_PROCESS_COUNT;

extern LONG DisablePreemption(void);
extern LONG EnablePreemption(void);
extern void panic(BYTE *msg);

extern LWP *get_fast_lwp_chain(void);
extern LWP *get_fast_lwp(void);
extern void put_fast_lwp(LWP *p);
extern LWP *get_local_fast_lwp_chain(LONG proc);
extern LWP *get_local_fast_lwp(LONG proc);
extern void put_local_fast_lwp(LWP *p, LONG proc);
extern LWP *get_local_soft_int_chain(LONG proc);
extern LWP *get_local_soft_int(LONG proc);
extern void put_local_soft_int(LWP *p, LONG proc);
extern LWP *get_soft_int_chain(void);
extern LWP *get_soft_int(void);
extern void put_soft_int(LWP *p);
extern LONG StartSleepNotAllowed(void);
extern LONG EndSleepNotAllowed(void);
extern LONG thread_switch_processor(LONG);
extern LONG thread_switch_and_bind(LONG);

// sema prototypes

extern sema_t *sema_init(sema_t *sema, LONG value);
extern sema_t *sema_alloc(LONG value);
extern LONG sema_free(sema_t *sema);
extern LONG sema_wait(sema_t *sema);
extern LONG sema_post(sema_t *sema);
extern LONG sema_timed_wait(sema_t *sema, LONG delay);
extern LONG sema_timed_post(sema_t *sema);
extern LONG sema_release(sema_t *sema);
extern PROCESS *get_sema_process(sema_t *sema);
extern void put_sema_process(sema_t *sema, PROCESS *p);

// mutex prototypes

extern mutex_t *mutex_init(mutex_t *mutex, LONG value);
extern mutex_t *mutex_alloc(LONG value);
extern LONG mutex_free(mutex_t *mutex);
extern LONG mutex_lock(mutex_t *mutex);
extern LONG mutex_unlock(mutex_t *mutex);
extern LONG mutex_release(mutex_t *mutex);
extern PROCESS *get_mutex_process(mutex_t *mutex);
extern void put_mutex_process(mutex_t *mutex, PROCESS *p);

// rmutex prototypes

extern rmutex_t *rmutex_init(rmutex_t *rmutex, LONG value);
extern rmutex_t *rmutex_alloc(LONG value);
extern LONG rmutex_free(rmutex_t *rmutex);
extern LONG rmutex_lock(rmutex_t *rmutex);
extern LONG rmutex_unlock(rmutex_t *rmutex);
extern LONG rmutex_release(rmutex_t *rmutex);
extern PROCESS *get_rmutex_process(rmutex_t *rmutex);
extern void put_rmutex_process(rmutex_t *rmutex, PROCESS *p);

// rwlock prototypes

extern rwlock_t *rwlock_init(rwlock_t *rwlock);
extern rwlock_t *rwlock_alloc(void);
extern LONG rwlock_free(rwlock_t *rwlock);
extern LONG rwlock_read_lock(rwlock_t *rwlock);
extern LONG rwlock_write_lock(rwlock_t *rwlock);
extern LONG rwlock_read_try_lock(rwlock_t *rwlock);
extern LONG rwlock_write_try_lock(rwlock_t *rwlock);
extern LONG rwlock_read_unlock(rwlock_t *rwlock);
extern LONG rwlock_write_unlock(rwlock_t *rwlock);
extern LONG rwlock_reader_to_writer(rwlock_t *rwlock);
extern LONG rwlock_reader_to_writer_unordered(rwlock_t *rwlock);
extern LONG rwlock_writer_to_reader(rwlock_t *rwlock);
extern PROCESS *get_rwlock_process(rwlock_t *rwlock);
extern void put_rwlock_process(rwlock_t *rwlock, PROCESS *p);
extern LONG rwlock_release(rwlock_t *rwlock);



