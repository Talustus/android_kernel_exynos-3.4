/*
 *
 * (C) COPYRIGHT 2011-2012 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */



/**
 * @file mali_kbase_defs.h
 *
 * Defintions (types, defines, etcs) common to Kbase. They are placed here to
 * allow the hierarchy of header files to work.
 */

#ifndef _KBASE_DEFS_H_
#define _KBASE_DEFS_H_

#define KBASE_DRV_NAME  "mali"

#include <kbase/mali_kbase_config.h>
#include <kbase/mali_base_hwconfig.h>
#include <osk/mali_osk.h>

#if defined(CONFIG_KDS) && MALI_LICENSE_IS_GPL
#define MALI_USE_KDS
#endif

#ifdef MALI_USE_KDS
#include <linux/kds.h>
#endif

#ifdef CONFIG_SYNC
#include <linux/sync.h>
#endif

/** Enable SW tracing when set */
#ifdef CONFIG_MALI_T6XX_ENABLE_TRACE
#define KBASE_TRACE_ENABLE 1
#endif

#ifndef KBASE_TRACE_ENABLE
#if MALI_DEBUG
#define KBASE_TRACE_ENABLE 1
#else
#define KBASE_TRACE_ENABLE 0
#endif /*MALI_DEBUG*/
#endif /*KBASE_TRACE_ENABLE*/

/* Maximum number of outstanding atoms per kbase context
 * this is set for security reasons to prevent a malicious app
 * from hanging the driver */
#define MAX_KCTX_OUTSTANDING_ATOMS (1ul << 6)

/** Dump Job slot trace on error (only active if KBASE_TRACE_ENABLE != 0) */
#define KBASE_TRACE_DUMP_ON_JOB_SLOT_ERROR 1

/**
 * Number of milliseconds before resetting the GPU when a job cannot be "zapped" from the hardware.
 * Note that the time is actually ZAP_TIMEOUT+SOFT_STOP_RESET_TIMEOUT between the context zap starting and the GPU
 * actually being reset to give other contexts time for their jobs to be soft-stopped and removed from the hardware
 * before resetting.
 */
#define ZAP_TIMEOUT             1000

/**
 * Prevent soft-stops from occuring in scheduling situations
 *
 * This is not due to HW issues, but when scheduling is desired to be more predictable.
 *
 * Therefore, soft stop may still be disabled due to HW issues.
 *
 * @note Soft stop will still be used for non-scheduling purposes e.g. when terminating a context.
 *
 * @note if not in use, define this value to 0 instead of \#undef'ing it
 */
#define KBASE_DISABLE_SCHEDULING_SOFT_STOPS 0

/**
 * Prevent hard-stops from occuring in scheduling situations
 *
 * This is not due to HW issues, but when scheduling is desired to be more predictable.
 *
 * @note Hard stop will still be used for non-scheduling purposes e.g. when terminating a context.
 *
 * @note if not in use, define this value to 0 instead of \#undef'ing it
 */
#define KBASE_DISABLE_SCHEDULING_HARD_STOPS 0

/* Forward declarations+defintions */
typedef struct kbase_context kbase_context;
typedef struct kbase_jd_atom kbasep_jd_atom;
typedef struct kbase_device kbase_device;

/**
 * The maximum number of Job Slots to support in the Hardware.
 *
 * You can optimize this down if your target devices will only ever support a
 * small number of job slots.
 */
#define BASE_JM_MAX_NR_SLOTS        16

/**
 * The maximum number of Address Spaces to support in the Hardware.
 *
 * You can optimize this down if your target devices will only ever support a
 * small number of Address Spaces
 */
#define BASE_MAX_NR_AS              16

#ifndef UINTPTR_MAX

/**
 * @brief Maximum value representable by type uintptr_t
 */
#if CSTD_CPU_32BIT
#define UINTPTR_MAX U32_MAX
#elif CSTD_CPU_64BIT
#define UINTPTR_MAX U64_MAX
#endif /* CSTD_CPU_64BIT */

#endif /* !defined(UINTPTR_MAX) */

/* mmu */
#define ENTRY_IS_ATE        1ULL
#define ENTRY_IS_INVAL      2ULL
#define ENTRY_IS_PTE        3ULL

#define MIDGARD_MMU_VA_BITS 48

#define ENTRY_ATTR_BITS (7ULL << 2) /* bits 4:2 */
#define ENTRY_RD_BIT (1ULL << 6)
#define ENTRY_WR_BIT (1ULL << 7)
#define ENTRY_SHARE_BITS (3ULL << 8) /* bits 9:8 */
#define ENTRY_ACCESS_BIT (1ULL << 10)
#define ENTRY_NX_BIT (1ULL << 54)

#define ENTRY_FLAGS_MASK (ENTRY_ATTR_BITS | ENTRY_RD_BIT | ENTRY_WR_BIT | ENTRY_SHARE_BITS | ENTRY_ACCESS_BIT | ENTRY_NX_BIT)

#if MIDGARD_MMU_VA_BITS > 39
#define MIDGARD_MMU_TOPLEVEL    0
#else
#define MIDGARD_MMU_TOPLEVEL    1
#endif

#define GROWABLE_FLAGS_REQUIRED (KBASE_REG_PF_GROW | KBASE_REG_ZONE_TMEM)
#define GROWABLE_FLAGS_MASK     (GROWABLE_FLAGS_REQUIRED | KBASE_REG_FREE)

/** setting in kbase_context::as_nr that indicates it's invalid */
#define KBASEP_AS_NR_INVALID     (-1)

#define KBASE_LOCK_REGION_MAX_SIZE (63)
#define KBASE_LOCK_REGION_MIN_SIZE (11)

#define KBASE_TRACE_SIZE_LOG2 8 /* 256 entries */
#define KBASE_TRACE_SIZE (1 << KBASE_TRACE_SIZE_LOG2)
#define KBASE_TRACE_MASK ((1 << KBASE_TRACE_SIZE_LOG2)-1)

#include "mali_kbase_js_defs.h"

typedef struct kbase_event {
	osk_dlist_item      entry;
	const void          *data;
	base_jd_event_code  event_code;
} kbase_event;


/* Hijack the event entry field to link the struct with the different
 * queues... */
typedef struct kbase_jd_bag {
	kbase_event event;
	u64         core_restriction;
	size_t      offset;
	u32         nr_atoms;
	/** Set when the bag has a power management reference. This is used to ensure that the GPU is
	 * not turned off after a soft-job has read the GPU counters until the bag has completed */
	mali_bool8  has_pm_ctx_reference;
} kbase_jd_bag;

/**
 * @brief States to model state machine processed by kbasep_js_job_check_ref_cores(), which
 * handles retaining cores for power management and affinity management.
 *
 * The state @ref KBASE_ATOM_COREREF_STATE_RECHECK_AFFINITY prevents an attack
 * where lots of atoms could be submitted before powerup, and each has an
 * affinity chosen that causes other atoms to have an affinity
 * violation. Whilst the affinity was not causing violations at the time it
 * was chosen, it could cause violations thereafter. For example, 1000 jobs
 * could have had their affinity chosen during the powerup time, so any of
 * those 1000 jobs could cause an affinity violation later on.
 *
 * The attack would otherwise occur because other atoms/contexts have to wait for:
 * -# the currently running atoms (which are causing the violation) to
 * finish
 * -# and, the atoms that had their affinity chosen during powerup to
 * finish. These are run preferrentially because they don't cause a
 * violation, but instead continue to cause the violation in others.
 * -# or, the attacker is scheduled out (which might not happen for just 2
 * contexts)
 *
 * By re-choosing the affinity (which is designed to avoid violations at the
 * time it's chosen), we break condition (2) of the wait, which minimizes the
 * problem to just waiting for current jobs to finish (which can be bounded if
 * the Job Scheduling Policy has a timer).
 */
typedef enum
{
	/** Starting state: No affinity chosen, and cores must be requested. kbase_jd_atom::affinity==0 */
	KBASE_ATOM_COREREF_STATE_NO_CORES_REQUESTED,
	/** Cores requested, but waiting for them to be powered. Requested cores given by kbase_jd_atom::affinity */
	KBASE_ATOM_COREREF_STATE_WAITING_FOR_REQUESTED_CORES,
	/** Cores given by kbase_jd_atom::affinity are powered, but affinity might be out-of-date, so must recheck */
	KBASE_ATOM_COREREF_STATE_RECHECK_AFFINITY,
	/** Cores given by kbase_jd_atom::affinity are powered, and affinity is up-to-date, but must check for violations */
	KBASE_ATOM_COREREF_STATE_CHECK_AFFINITY_VIOLATIONS,
	/** Cores are powered, kbase_jd_atom::affinity up-to-date, no affinity violations: atom can be submitted to HW */
	KBASE_ATOM_COREREF_STATE_READY

} kbase_atom_coreref_state;

typedef struct kbase_jd_atom {
	kbase_event     event;
	osk_workq_work  work;
	kbasep_js_tick  start_timestamp;
	base_jd_atom    *user_atom;
	kbase_jd_bag    *bag;
	kbase_context   *kctx;
	base_jd_dep     pre_dep;
	base_jd_dep     post_dep;
	u32             nr_syncsets;
	u32             nr_extres;
	u32             device_nr;
	u64             affinity;
	u64             jc;
	kbase_atom_coreref_state   coreref_state;
#ifdef MALI_USE_KDS
	struct kds_resource_set *  kds_rset;
	mali_bool                  kds_dep_satisfied;
#endif
#ifdef CONFIG_SYNC
	struct sync_fence           *fence;
	struct sync_fence_waiter    sync_waiter;
#endif

	base_jd_core_req    core_req;       /**< core requirements */

	kbasep_js_policy_job_info sched_info;
	/** Job Slot to retry submitting to if submission from IRQ handler failed
	 *
	 * NOTE: see if this can be unified into the another member e.g. the event */
	int             retry_submit_on_slot;
	/* atom priority scaled to nice range with +20 offset 0..39 */
	int             nice_prio;

	int             poking; /* BASE_HW_ISSUE_8316 */
	/*this pointer needed to keep external resources not corruptible by userspace*/
	base_external_resource *resources;
} kbase_jd_atom;

/*
 * Theory of operations:
 *
 * - sem is an array of 256 bits, each bit being a semaphore
 * for a 1-1 job dependency:
 * Initially set to 0 (passing)
 * Incremented when a post_dep is queued
 * Decremented when a post_dep is completed
 * pre_dep is satisfied when value is 0
 * sem #0 is hardwired to 0 (always passing).
 *
 * - queue is an array of atoms, one per semaphore.
 * When a pre_dep is not satisfied, the atom is added to both
 * queues it depends on (except for queue 0 which is never used).
 * Each time a post_dep is signal, the corresponding bit is cleared,
 * the atoms removed from the queue, and the corresponding pre_dep
 * is cleared. The atom can be run when pre_dep[0] == pre_dep[1] == 0.
 */

#define KBASE_JD_DEP_QUEUE_SIZE 256

typedef struct kbase_jd_dep_queue {
	kbase_jd_atom *queue[KBASE_JD_DEP_QUEUE_SIZE];
	u32            sem[BASEP_JD_SEM_ARRAY_SIZE];
} kbase_jd_dep_queue;

typedef struct kbase_jd_context {
	osk_mutex           lock;
	kbasep_js_kctx_info sched_info;
	kbase_jd_dep_queue  dep_queue;
	base_jd_atom        *pool;
	size_t              pool_size;

	/** Tracks all job-dispatch jobs.  This includes those not tracked by
	 * the scheduler: 'not ready to run' and 'dependency-only' jobs. */
	u32                 job_nr;

	/** Waitq that reflects whether there are no jobs (including SW-only
	 * dependency jobs). This is set when no jobs are present on the ctx,
	 * and clear when there are jobs.
	 *
	 * @note: Job Dispatcher knows about more jobs than the Job Scheduler:
	 * the Job Scheduler is unaware of jobs that are blocked on dependencies,
	 * and SW-only dependency jobs.
	 *
	 * This waitq can be waited upon to find out when the context jobs are all
	 * done/cancelled (including those that might've been blocked on
	 * dependencies) - and so, whether it can be terminated. However, it should
	 * only be terminated once it is neither present in the policy-queue (see
	 * kbasep_js_policy_try_evict_ctx() ) nor the run-pool (see
	 * kbasep_js_kctx_info::ctx::is_scheduled_waitq).
	 *
	 * Since the waitq is only set under kbase_jd_context::lock,
	 * the waiter should also briefly obtain and drop kbase_jd_context::lock to
	 * guarentee that the setter has completed its work on the kbase_context */
	osk_waitq           zero_jobs_waitq;
	osk_workq           job_done_wq;
	osk_spinlock_irq    tb_lock;
	u32                 *tb;
	size_t              tb_wrap_offset;

#ifdef MALI_USE_KDS
	struct kds_callback kds_cb;
#endif
} kbase_jd_context;

typedef struct kbase_jm_slot
{
	/* The number of slots must be a power of two */
#define BASE_JM_SUBMIT_SLOTS        16
#define BASE_JM_SUBMIT_SLOTS_MASK   (BASE_JM_SUBMIT_SLOTS - 1)

	kbase_jd_atom    *submitted[BASE_JM_SUBMIT_SLOTS];

	u8               submitted_head;
	u8               submitted_nr;

} kbase_jm_slot;

typedef enum kbase_midgard_type
{
	KBASE_MALI_T6XM,
	KBASE_MALI_T6F1,
	KBASE_MALI_T601,
	KBASE_MALI_T604,
	KBASE_MALI_T608,

	KBASE_MALI_COUNT
} kbase_midgard_type;

#define KBASE_FEATURE_HAS_MODEL_PMU             (1U << 0)
#define KBASE_FEATURE_NEEDS_REG_DELAY           (1U << 1)
#define KBASE_FEATURE_HAS_16BIT_PC              (1U << 2)
#define KBASE_FEATURE_LACKS_RESET_INT           (1U << 3)
#define KBASE_FEATURE_DELAYED_PERF_WRITE_STATUS (1U << 4)

typedef struct kbase_device_info
{
	kbase_midgard_type  dev_type;
	u32                 features;
} kbase_device_info;

/**
 * Important: Our code makes assumptions that a kbase_as structure is always at
 * kbase_device->as[number]. This is used to recover the containing
 * kbase_device from a kbase_as structure.
 *
 * Therefore, kbase_as structures must not be allocated anywhere else.
 */
typedef struct kbase_as
{
	int               number;

	osk_workq         pf_wq;
	osk_workq_work    work_pagefault;
	osk_workq_work    work_busfault;
	mali_addr64       fault_addr;
	osk_mutex         transaction_mutex;

	/* BASE_HW_ISSUE_8316  */
	osk_workq         poke_wq;
	osk_workq_work    poke_work;
	osk_atomic        poke_refcount;
	osk_timer         poke_timer;
} kbase_as;

/* tracking of memory usage */
typedef struct kbasep_mem_usage
{
	u32        max_pages;
	osk_atomic cur_pages;
} kbasep_mem_usage;

/**
 * @brief Specifies order in which physical allocators are selected.
 *
 * Enumeration lists different orders in which physical allocators are selected on memory allocation.
 *
 */
typedef enum kbase_phys_allocator_order
{
	ALLOCATOR_ORDER_CONFIG,                 /* Select allocators in order they appeared in the configuration file */
	ALLOCATOR_ORDER_GPU_PERFORMANCE,        /* Select allocators in order from fastest to slowest on the GPU */
	ALLOCATOR_ORDER_CPU_PERFORMANCE,        /* Select allocators in order from fastest to slowest on the CPU */
	ALLOCATOR_ORDER_CPU_GPU_PERFORMANCE,    /* Select allocators in order from fastest to slowest on the CPU and GPU */

	ALLOCATOR_ORDER_COUNT
} kbase_phys_allocator_order;


/* A simple structure to keep a sorted list of
 * osk_phy_allocator pointers.
 * Used by the iterator object
 */
typedef struct kbase_phys_allocator_array
{
	/* the allocators */
	osk_phy_allocator * allocs;
	osk_phy_allocator ** sorted_allocs[ALLOCATOR_ORDER_COUNT];
	/* number of allocators */
	unsigned int count;

#if MALI_DEBUG
	mali_bool it_bound;
#endif /* MALI_DEBUG */
} kbase_phys_allocator_array;

/**
 * Instrumentation State Machine States:
 * DISABLED    - requires instrumentation to be enabled
 * IDLE        - state machine is active and ready for a command.
 * DUMPING     - hardware is currently dumping a frame.
 * POSTCLEANING- hardware is currently cleaning and invalidating caches.
 * PRECLEANING - same as POSTCLEANING, except on completion, state machine will transiton to CLEANED instead of IDLE.
 * CLEANED     - cache clean completed, waiting for Instrumentation setup.
 * ERROR       - an error has occured during DUMPING (page fault).
 */

typedef enum
{
	KBASE_INSTR_STATE_DISABLED = 0,
	KBASE_INSTR_STATE_IDLE,
	KBASE_INSTR_STATE_DUMPING,
	KBASE_INSTR_STATE_CLEANED,
	KBASE_INSTR_STATE_PRECLEANING,
	KBASE_INSTR_STATE_POSTCLEANING,
	KBASE_INSTR_STATE_RESETTING,
	KBASE_INSTR_STATE_FAULT

} kbase_instr_state;


typedef struct kbasep_mem_device
{
#if MALI_USE_UMP == 1
	u32                        ump_device_id;            /* Which UMP device this GPU should be mapped to.
	                                                        Read-only, copied from platform configuration on startup.*/
#endif /* MALI_USE_UMP == 1 */

	u32                        per_process_memory_limit; /* How much memory (in bytes) a single process can access.
	                                                        Read-only, copied from platform configuration on startup. */
	kbasep_mem_usage           usage;                    /* Tracks usage of OS shared memory. Initialized with platform
	                                                        configuration data, updated when OS memory is allocated/freed.*/
	kbase_phys_allocator_array allocators;               /* List of available physical memory allocators */
} kbasep_mem_device;


#define KBASE_TRACE_CODE( X ) KBASE_TRACE_CODE_ ## X

typedef enum
{
	/* IMPORTANT: USE OF SPECIAL #INCLUDE OF NON-STANDARD HEADER FILE
	 * THIS MUST BE USED AT THE START OF THE ENUM */
#define KBASE_TRACE_CODE_MAKE_CODE( X ) KBASE_TRACE_CODE( X )
#include "mali_kbase_trace_defs.h"
#undef  KBASE_TRACE_CODE_MAKE_CODE
	/* Comma on its own, to extend the list */
	,
	/* Must be the last in the enum */
	KBASE_TRACE_CODE_COUNT
} kbase_trace_code;

#define KBASE_TRACE_FLAG_REFCOUNT (((u8)1) << 0)
#define KBASE_TRACE_FLAG_JOBSLOT  (((u8)1) << 1)

typedef struct kbase_trace
{
	osk_timeval timestamp;
	u32   thread_id;
	u32   cpu;
	void *ctx;
	void *uatom;
	u64   gpu_addr;
	u32   info_val;
	u8    code;
	u8    jobslot;
	u8    refcount;
	u8    flags;
} kbase_trace;

struct kbase_device {
	const kbase_device_info *dev_info;
	/** Spinlock protecting access to all elements of jm_slots */
	osk_spinlock_irq        jm_slots_lock;
	kbase_jm_slot           jm_slots[BASE_JM_MAX_NR_SLOTS];
	s8                      slot_submit_count_irq[BASE_JM_MAX_NR_SLOTS];
	kbase_os_device         osdev;
	kbase_pm_device_data    pm;
	kbasep_js_device_data   js_data;
	kbasep_mem_device       memdev;

	kbase_as                as[BASE_MAX_NR_AS];

	osk_phy_allocator       mmu_fault_allocator;
	osk_phy_addr            mmu_fault_pages[4];
	osk_spinlock_irq        mmu_mask_change;

	kbase_gpu_props         gpu_props;

	/** List of SW workarounds for HW issues */
	unsigned long           hw_issues_mask[(BASE_HW_ISSUE_END + OSK_BITS_PER_LONG - 1)/OSK_BITS_PER_LONG];

	/* Cached present bitmaps - these are the same as the corresponding hardware registers */
	u64                     shader_present_bitmap;
	u64                     tiler_present_bitmap;
	u64                     l2_present_bitmap;
	u64                     l3_present_bitmap;

	/* Bitmaps of cores that are currently in use (running jobs).
	 * These should be kept up to date by the job scheduler.
	 *
	 * pm.power_change_lock should be held when accessing these members.
	 *
	 * kbase_pm_check_transitions should be called when bits are cleared to
	 * update the power management system and allow transitions to occur. */
	u64                     shader_inuse_bitmap;
	u64                     tiler_inuse_bitmap;

	/* Refcount for cores in use */
	u32                     shader_inuse_cnt[64];
	u32                     tiler_inuse_cnt[64];

	/* Bitmaps of cores the JS needs for jobs ready to run */
	u64                     shader_needed_bitmap;
	u64                     tiler_needed_bitmap;

	/* Refcount for cores needed */
	u32                      shader_needed_cnt[64];
	u32                      tiler_needed_cnt[64];

	/* Refcount for tracking users of the l2 cache, e.g. when using hardware counter instrumentation. */
	u32                      l2_users_count;

	/* Bitmaps of cores that are currently available (powered up and the power policy is happy for jobs to be
	 * submitted to these cores. These are updated by the power management code. The job scheduler should avoid
	 * submitting new jobs to any cores that are not marked as available.
	 *
	 * pm.power_change_lock should be held when accessing these members.
	 */
	u64                     shader_available_bitmap;
	u64                     tiler_available_bitmap;

	s8                      nr_hw_address_spaces;     /**< Number of address spaces in the GPU (constant after driver initialisation) */
	s8                      nr_user_address_spaces;   /**< Number of address spaces available to user contexts */

	/* Structure used for instrumentation and HW counters dumping */
	struct {
		/* The lock should be used when accessing any of the following members */
		osk_spinlock_irq    lock;

		kbase_context      *kctx;
		u64                 addr;
		osk_waitq           waitqueue;
		kbase_instr_state   state;
	} hwcnt;

	/* Set when we're about to reset the GPU */
	osk_atomic              reset_gpu;
#define KBASE_RESET_GPU_NOT_PENDING     0 /* The GPU reset isn't pending */
#define KBASE_RESET_GPU_PREPARED        1 /* kbase_prepare_to_reset_gpu has been called */
#define KBASE_RESET_GPU_COMMITTED       2 /* kbase_reset_gpu has been called - the reset will now definitely happen
                                           * within the timeout period */
#define KBASE_RESET_GPU_HAPPENING       3 /* The GPU reset process is currently occuring (timeout has expired or
                                           * kbasep_try_reset_gpu_early was called) */

	/* Work queue and work item for performing the reset in */
	osk_workq               reset_workq;
	osk_workq_work          reset_work;
	/* Signalled when reset_gpu==KBASE_RESET_GPU_NOT_PENDING */
	osk_waitq               reset_waitq;
	osk_timer               reset_timer;

	/*value to be written to the irq_throttle register each time an irq is served */
	osk_atomic irq_throttle_cycles;

	const kbase_attribute   *config_attributes;

	/* >> BASE_HW_ISSUE_8401 >> */
#define KBASE_8401_WORKAROUND_COMPUTEJOB_COUNT 3
	kbase_context           *workaround_kctx;
	osk_virt_addr           workaround_compute_job_va[KBASE_8401_WORKAROUND_COMPUTEJOB_COUNT];
	osk_phy_addr            workaround_compute_job_pa[KBASE_8401_WORKAROUND_COMPUTEJOB_COUNT];
	/* << BASE_HW_ISSUE_8401 << */

#if KBASE_TRACE_ENABLE != 0
	osk_spinlock_irq        trace_lock;
	u16                     trace_first_out;
	u16                     trace_next_in;
	kbase_trace            *trace_rbuf;
#endif

#if MALI_CUSTOMER_RELEASE == 0
	/* This is used to override the current job scheduler values for
	 * KBASE_CONFIG_ATTR_JS_STOP_STOP_TICKS_SS
	 * KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_SS
	 * KBASE_CONFIG_ATTR_JS_HARD_STOP_TICKS_NSS
	 * KBASE_CONFIG_ATTR_JS_RESET_TICKS_SS
	 * KBASE_CONFIG_ATTR_JS_RESET_TICKS_NSS.
	 *
	 * These values are set via the js_timeouts sysfs file.
	 */
	u32                     js_soft_stop_ticks;
	u32                     js_hard_stop_ticks_ss;
	u32                     js_hard_stop_ticks_nss;
	u32                     js_reset_ticks_ss;
	u32                     js_reset_ticks_nss;
#endif
	/* Platform specific private data to be accessed by mali_kbase_config_xxx.c only */
	void                    *platform_context;
#ifdef CONFIG_MALI_T6XX_RT_PM
	struct delayed_work runtime_pm_workqueue;
#endif
};

struct kbase_context
{
	kbase_device            *kbdev;
	osk_phy_allocator       pgd_allocator;
	osk_phy_addr            pgd;
	osk_dlist               event_list;
	osk_mutex               event_mutex;
	mali_bool               event_closed;

	osk_atomic              setup_complete;
	osk_atomic              setup_in_progress;

	u64                     *mmu_teardown_pages;

	osk_mutex               reg_lock; /* To be converted to a rwlock? */
#if MALI_KBASEP_REGION_RBTREE
	struct rb_root			reg_rbtree; /* Red-Black tree of GPU regions (live regions) */
#else
	osk_dlist               reg_list;   /* Ordered list of GPU regions (all regions) */
#endif

	kbase_os_context        osctx;
	kbase_jd_context        jctx;
	kbasep_mem_usage        usage;
	ukk_session             ukk_session;
	u32                     nr_outstanding_atoms;
	osk_waitq               complete_outstanding_waitq; /*if there are too many outstanding atoms
	                                                     *per context we wait on this waitqueue
	                                                     *to be signaled before submitting more jobs
	                                                     */

	osk_dlist               waiting_soft_jobs;

	/** This is effectively part of the Run Pool, because it only has a valid
	 * setting (!=KBASEP_AS_NR_INVALID) whilst the context is scheduled in
	 *
	 * The kbasep_js_device_data::runpool_irq::lock must be held whilst accessing
	 * this.
	 *
	 * If the context relating to this as_nr is required, you must use
	 * kbasep_js_runpool_retain_ctx() to ensure that the context doesn't disappear
	 * whilst you're using it. Alternatively, just hold the kbasep_js_device_data::runpool_irq::lock
	 * to ensure the context doesn't disappear (but this has restrictions on what other locks
	 * you can take whilst doing this) */
	int                     as_nr;

	/* NOTE:
	 *
	 * Flags are in jctx.sched_info.ctx.flags
	 * Mutable flags *must* be accessed under jctx.sched_info.ctx.jsctx_mutex
	 *
	 * All other flags must be added there */
};

typedef enum kbase_reg_access_type
{
	REG_READ,
	REG_WRITE
} kbase_reg_access_type;


typedef enum kbase_share_attr_bits
{
	/* (1ULL << 8) bit is reserved */
	SHARE_BOTH_BITS  = (2ULL << 8), /* inner and outer shareable coherency */
	SHARE_INNER_BITS = (3ULL << 8)  /* inner shareable coherency */
} kbase_share_attr_bits;


#endif /* _KBASE_DEFS_H_ */

