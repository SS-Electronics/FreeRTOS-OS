/**
 * @file        sys_arch.c
 * @brief       lwIP OS abstraction layer over the FreeRTOS-OS kernel API
 * @ingroup     net_port
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Network Port
 * @info        Implements the lwIP sys_arch contract (threaded mode, NO_SYS=0)
 *              entirely on top of the vendor-neutral os_*() primitives from
 *              kernel.h. No FreeRTOS headers are pulled in here, so the TCP/IP
 *              stack stays decoupled from the RTOS vendor.
 * @dependency  os/kernel.h, lwIP (sys.h)
 *
 * @details
 * Mapping
 * ───────
 *   sys_sem_t    -> os_sem_t   (binary or counting semaphore)
 *   sys_mutex_t  -> os_mutex_t (priority-inheriting mutex)
 *   sys_mbox_t   -> os_mbox_t  (FIFO of void* messages)
 *   sys_thread_t -> thread_id  (os_thread_create return)
 *   sys_prot_t   -> PRIMASK snapshot (lightweight critical section)
 *
 * lwIP timeout convention: a timeout of 0 ms means "block forever", which we
 * translate to OS_WAIT_FOREVER. Wait functions return the elapsed time in ms,
 * or SYS_ARCH_TIMEOUT when the deadline passes.
 *
 * @copyright
 * This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with FreeRTOS-OS. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/err.h"

#include <os/kernel.h>


/* ── Lightweight critical section (PRIMASK save/restore) ─────────────────── */
/* Cortex-M intrinsics expressed as inline asm to keep this layer free of any
 * vendor CMSIS/FreeRTOS include. The masked region is only ever a few
 * instructions (lwIP stat / memory bookkeeping). */

static inline uint32_t _irq_save(void)
{
	uint32_t primask;
	__asm volatile ("mrs %0, primask" : "=r" (primask));
	__asm volatile ("cpsid i" ::: "memory");
	return primask;
}

static inline void _irq_restore(uint32_t primask)
{
	__asm volatile ("msr primask, %0" :: "r" (primask) : "memory");
}


/* ── Module init ────────────────────────────────────────────────────────── */

/** @brief Nothing to set up: the kernel primitives are self-contained. */
void sys_init(void)
{
}


/* ── Time base ──────────────────────────────────────────────────────────── */

/** @brief Monotonic millisecond clock for lwIP timers. */
u32_t sys_now(void)
{
	return (u32_t)os_uptime_ms();
}


/* ── Critical section ───────────────────────────────────────────────────── */

sys_prot_t sys_arch_protect(void)
{
	return (sys_prot_t)_irq_save();
}

void sys_arch_unprotect(sys_prot_t pval)
{
	_irq_restore((uint32_t)pval);
}


/* ── Semaphores ─────────────────────────────────────────────────────────── */

/**
 * @brief Create a counting semaphore with initial value @p count.
 * @note  lwIP only ever uses counts of 0 or 1 here, but a counting
 *        semaphore with a generous ceiling covers both safely.
 */
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
	if (sem == NULL)
	{
		return ERR_ARG;
	}

	*sem = os_sem_create(1, (uint32_t)count);

	return (*sem != NULL) ? ERR_OK : ERR_MEM;
}

void sys_sem_free(sys_sem_t *sem)
{
	if ((sem != NULL) && (*sem != NULL))
	{
		os_sem_delete(*sem);
		*sem = NULL;
	}
}

void sys_sem_signal(sys_sem_t *sem)
{
	if ((sem != NULL) && (*sem != NULL))
	{
		(void)os_sem_give(*sem);
	}
}

/**
 * @brief Wait on a semaphore for up to @p timeout ms (0 == block forever).
 * @return Elapsed milliseconds, or SYS_ARCH_TIMEOUT on deadline.
 */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	if ((sem == NULL) || (*sem == NULL))
	{
		return SYS_ARCH_TIMEOUT;
	}

	uint32_t start  = os_uptime_ms();
	uint32_t wait   = (timeout == 0) ? OS_WAIT_FOREVER : timeout;

	if (os_sem_take(*sem, wait) == OS_ERR_NONE)
	{
		return (u32_t)(os_uptime_ms() - start);
	}

	return SYS_ARCH_TIMEOUT;
}

/* cppcheck-suppress constParameterPointer ; signature fixed by lwIP sys.h */
int sys_sem_valid(sys_sem_t *sem)
{
	return (sem != NULL) && (*sem != NULL);
}

void sys_sem_set_invalid(sys_sem_t *sem)
{
	if (sem != NULL)
	{
		*sem = NULL;
	}
}


/* ── Mutexes ────────────────────────────────────────────────────────────── */

err_t sys_mutex_new(sys_mutex_t *mutex)
{
	if (mutex == NULL)
	{
		return ERR_ARG;
	}

	*mutex = os_mutex_create();

	return (*mutex != NULL) ? ERR_OK : ERR_MEM;
}

void sys_mutex_lock(sys_mutex_t *mutex)
{
	if ((mutex != NULL) && (*mutex != NULL))
	{
		(void)os_mutex_lock(*mutex, OS_WAIT_FOREVER);
	}
}

void sys_mutex_unlock(sys_mutex_t *mutex)
{
	if ((mutex != NULL) && (*mutex != NULL))
	{
		(void)os_mutex_unlock(*mutex);
	}
}

void sys_mutex_free(sys_mutex_t *mutex)
{
	if ((mutex != NULL) && (*mutex != NULL))
	{
		os_mutex_delete(*mutex);
		*mutex = NULL;
	}
}

/* cppcheck-suppress constParameterPointer ; signature fixed by lwIP sys.h */
int sys_mutex_valid(sys_mutex_t *mutex)
{
	return (mutex != NULL) && (*mutex != NULL);
}

void sys_mutex_set_invalid(sys_mutex_t *mutex)
{
	if (mutex != NULL)
	{
		*mutex = NULL;
	}
}


/* ── Mailboxes ──────────────────────────────────────────────────────────── */

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	if (mbox == NULL)
	{
		return ERR_ARG;
	}

	*mbox = os_mbox_create((uint32_t)size);

	return (*mbox != NULL) ? ERR_OK : ERR_MEM;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
	if ((mbox != NULL) && (*mbox != NULL))
	{
		os_mbox_delete(*mbox);
		*mbox = NULL;
	}
}

/** @brief Post a message, blocking until space is available. */
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
	if ((mbox != NULL) && (*mbox != NULL))
	{
		(void)os_mbox_post(*mbox, msg, OS_WAIT_FOREVER);
	}
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
	if ((mbox == NULL) || (*mbox == NULL))
	{
		return ERR_ARG;
	}

	return (os_mbox_trypost(*mbox, msg) == OS_ERR_NONE) ? ERR_OK : ERR_MEM;
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
	if ((mbox == NULL) || (*mbox == NULL))
	{
		return ERR_ARG;
	}

	/* hp_task_woken is unused: the FreeRTOS-OS net stack uses a polled RX
	 * thread, never an ISR. The caller decides whether to yield. */
	int32_t ret = os_mbox_trypost_from_isr(*mbox, msg, NULL);

	return (ret == OS_ERR_NONE) ? ERR_OK : ERR_MEM;
}

/**
 * @brief Fetch a message, waiting up to @p timeout ms (0 == block forever).
 * @return Elapsed milliseconds, or SYS_ARCH_TIMEOUT on deadline.
 */
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
	if ((mbox == NULL) || (*mbox == NULL))
	{
		return SYS_ARCH_TIMEOUT;
	}

	void    *dummy;
	void   **target = (msg != NULL) ? msg : &dummy;
	uint32_t start  = os_uptime_ms();
	uint32_t wait   = (timeout == 0) ? OS_WAIT_FOREVER : timeout;

	if (os_mbox_fetch(*mbox, target, wait) == OS_ERR_NONE)
	{
		return (u32_t)(os_uptime_ms() - start);
	}

	return SYS_ARCH_TIMEOUT;
}

/** @brief Non-blocking fetch. @return 0 on success, SYS_MBOX_EMPTY if empty. */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	if ((mbox == NULL) || (*mbox == NULL))
	{
		return SYS_ARCH_TIMEOUT;
	}

	void  *dummy;
	void **target = (msg != NULL) ? msg : &dummy;

	if (os_mbox_tryfetch(*mbox, target) == OS_ERR_NONE)
	{
		return 0;
	}

	return SYS_MBOX_EMPTY;
}

/* cppcheck-suppress constParameterPointer ; signature fixed by lwIP sys.h */
int sys_mbox_valid(sys_mbox_t *mbox)
{
	return (mbox != NULL) && (*mbox != NULL);
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	if (mbox != NULL)
	{
		*mbox = NULL;
	}
}


/* ── Threads ────────────────────────────────────────────────────────────── */

/**
 * @brief Spawn an lwIP worker thread (e.g. the tcpip_thread).
 * @param name      Debug name.
 * @param thread    Entry function (void(*)(void*)).
 * @param arg       Opaque argument.
 * @param stacksize Stack depth in words (lwipopts supplies word counts).
 * @param prio      Scheduling priority forwarded to os_thread_create().
 * @return          The kernel thread_id, or 0 on failure.
 */
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread,
                            void *arg, int stacksize, int prio)
{
	int32_t id = os_thread_create((thread_func_t)thread, name,
	                              (uint32_t)stacksize, (uint32_t)prio, arg);

	return (sys_thread_t)((id > 0) ? id : 0);
}
