/**
 * @file        kernel_ipc.c
 * @brief       IPC primitive wrappers (semaphore / mutex / mailbox)
 * @ingroup     kernel_glue
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Kernel Glue
 * @info        Vendor-neutral IPC surface over FreeRTOS. Lets OS services and
 *              out-of-tree subsystems (e.g. the lwIP sys_arch port) block,
 *              signal and message-pass without pulling in FreeRTOS headers.
 * @dependency  FreeRTOS-Kernel (semphr, queue)
 *
 * @details
 * Handles are opaque void* pointers that alias the underlying FreeRTOS
 * SemaphoreHandle_t / QueueHandle_t. Timeouts are milliseconds; the
 * sentinel OS_WAIT_FOREVER maps to portMAX_DELAY (block indefinitely).
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

#include <os/kernel.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <queue.h>


/**
 * @brief Convert a millisecond timeout to FreeRTOS ticks.
 * @note  OS_WAIT_FOREVER becomes portMAX_DELAY (block forever). Every
 *        other value is rounded up by pdMS_TO_TICKS.
 */
static inline TickType_t _ms_to_ticks(uint32_t timeout_ms)
{
	if (timeout_ms == OS_WAIT_FOREVER)
	{
		return portMAX_DELAY;
	}

	return pdMS_TO_TICKS(timeout_ms);
}


/* ── Semaphores ─────────────────────────────────────────────────────────── */

/**
 * @brief Create a counting semaphore (max_count == 1 yields a binary sem).
 * @return Opaque handle, or NULL on allocation failure / bad argument.
 */
os_sem_t __SECTION_OS os_sem_create(uint32_t max_count, uint32_t initial_count)
{
	if ((max_count == 0) || (initial_count > max_count))
	{
		return NULL;
	}

	return (os_sem_t)xSemaphoreCreateCounting((UBaseType_t)max_count,
	                                           (UBaseType_t)initial_count);
}


/**
 * @brief Delete a semaphore and release its resources.
 */
void __SECTION_OS os_sem_delete(os_sem_t sem)
{
	if (sem != NULL)
	{
		vSemaphoreDelete((SemaphoreHandle_t)sem);
	}
}


/**
 * @brief Take (wait / decrement) a semaphore.
 * @return OS_ERR_NONE on success, OS_ERR_TIMEOUT on deadline,
 *         OS_ERR_NULL_PTR if the handle is NULL.
 */
int32_t __SECTION_OS os_sem_take(os_sem_t sem, uint32_t timeout_ms)
{
	if (sem == NULL)
	{
		return OS_ERR_NULL_PTR;
	}

	if (xSemaphoreTake((SemaphoreHandle_t)sem, _ms_to_ticks(timeout_ms)) == pdTRUE)
	{
		return OS_ERR_NONE;
	}

	return OS_ERR_TIMEOUT;
}


/**
 * @brief Give (signal / increment) a semaphore from thread context.
 * @return OS_ERR_NONE on success, OS_ERR_OP if the count is already at
 *         its maximum, OS_ERR_NULL_PTR if the handle is NULL.
 */
int32_t __SECTION_OS os_sem_give(os_sem_t sem)
{
	if (sem == NULL)
	{
		return OS_ERR_NULL_PTR;
	}

	if (xSemaphoreGive((SemaphoreHandle_t)sem) == pdTRUE)
	{
		return OS_ERR_NONE;
	}

	return OS_ERR_OP;
}


/**
 * @brief Give a semaphore from ISR context.
 * @param hp_task_woken Optional out-flag; set non-zero when a higher
 *                      priority task was unblocked and a context switch
 *                      should be requested before leaving the ISR.
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure,
 *         OS_ERR_NULL_PTR if the handle is NULL.
 */
int32_t __SECTION_OS os_sem_give_from_isr(os_sem_t sem, int32_t *hp_task_woken)
{
	if (sem == NULL)
	{
		return OS_ERR_NULL_PTR;
	}

	BaseType_t woken = pdFALSE;
	BaseType_t ret   = xSemaphoreGiveFromISR((SemaphoreHandle_t)sem, &woken);

	if (hp_task_woken != NULL)
	{
		*hp_task_woken = (int32_t)woken;
	}

	return (ret == pdTRUE) ? OS_ERR_NONE : OS_ERR_OP;
}


/* ── Mutexes ────────────────────────────────────────────────────────────── */

/**
 * @brief Create a priority-inheriting mutex.
 * @return Opaque handle, or NULL on allocation failure.
 */
os_mutex_t __SECTION_OS os_mutex_create(void)
{
	return (os_mutex_t)xSemaphoreCreateMutex();
}


/**
 * @brief Delete a mutex and release its resources.
 */
void __SECTION_OS os_mutex_delete(os_mutex_t mtx)
{
	if (mtx != NULL)
	{
		vSemaphoreDelete((SemaphoreHandle_t)mtx);
	}
}


/**
 * @brief Lock a mutex, blocking up to @p timeout_ms.
 * @return OS_ERR_NONE on success, OS_ERR_TIMEOUT on deadline,
 *         OS_ERR_NULL_PTR if the handle is NULL.
 */
int32_t __SECTION_OS os_mutex_lock(os_mutex_t mtx, uint32_t timeout_ms)
{
	if (mtx == NULL)
	{
		return OS_ERR_NULL_PTR;
	}

	if (xSemaphoreTake((SemaphoreHandle_t)mtx, _ms_to_ticks(timeout_ms)) == pdTRUE)
	{
		return OS_ERR_NONE;
	}

	return OS_ERR_TIMEOUT;
}


/**
 * @brief Unlock a mutex.
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure,
 *         OS_ERR_NULL_PTR if the handle is NULL.
 */
int32_t __SECTION_OS os_mutex_unlock(os_mutex_t mtx)
{
	if (mtx == NULL)
	{
		return OS_ERR_NULL_PTR;
	}

	if (xSemaphoreGive((SemaphoreHandle_t)mtx) == pdTRUE)
	{
		return OS_ERR_NONE;
	}

	return OS_ERR_OP;
}


/* ── Mailboxes (FIFO of void* pointers) ─────────────────────────────────── */

/**
 * @brief Create a mailbox holding up to @p size pointer-sized messages.
 * @return Opaque handle, or NULL on allocation failure / bad argument.
 */
os_mbox_t __SECTION_OS os_mbox_create(uint32_t size)
{
	if (size == 0)
	{
		return NULL;
	}

	return (os_mbox_t)xQueueCreate((UBaseType_t)size, sizeof(void *));
}


/**
 * @brief Delete a mailbox and release its resources.
 */
void __SECTION_OS os_mbox_delete(os_mbox_t mbox)
{
	if (mbox != NULL)
	{
		vQueueDelete((QueueHandle_t)mbox);
	}
}


/**
 * @brief Post a message, blocking up to @p timeout_ms if the mailbox is full.
 * @return OS_ERR_NONE on success, OS_ERR_TIMEOUT on deadline,
 *         OS_ERR_NULL_PTR if the handle is NULL.
 */
int32_t __SECTION_OS os_mbox_post(os_mbox_t mbox, void *msg, uint32_t timeout_ms)
{
	if (mbox == NULL)
	{
		return OS_ERR_NULL_PTR;
	}

	if (xQueueSend((QueueHandle_t)mbox, &msg, _ms_to_ticks(timeout_ms)) == pdTRUE)
	{
		return OS_ERR_NONE;
	}

	return OS_ERR_TIMEOUT;
}


/**
 * @brief Post a message without blocking.
 * @return OS_ERR_NONE on success, OS_ERR_OP if the mailbox is full,
 *         OS_ERR_NULL_PTR if the handle is NULL.
 */
int32_t __SECTION_OS os_mbox_trypost(os_mbox_t mbox, void *msg)
{
	if (mbox == NULL)
	{
		return OS_ERR_NULL_PTR;
	}

	if (xQueueSend((QueueHandle_t)mbox, &msg, 0) == pdTRUE)
	{
		return OS_ERR_NONE;
	}

	return OS_ERR_OP;
}


/**
 * @brief Post a message from ISR context without blocking.
 * @param hp_task_woken Optional out-flag; set non-zero when a higher
 *                      priority task was unblocked.
 * @return OS_ERR_NONE on success, OS_ERR_OP if the mailbox is full,
 *         OS_ERR_NULL_PTR if the handle is NULL.
 */
int32_t __SECTION_OS os_mbox_trypost_from_isr(os_mbox_t mbox, void *msg, int32_t *hp_task_woken)
{
	if (mbox == NULL)
	{
		return OS_ERR_NULL_PTR;
	}

	BaseType_t woken = pdFALSE;
	BaseType_t ret   = xQueueSendFromISR((QueueHandle_t)mbox, &msg, &woken);

	if (hp_task_woken != NULL)
	{
		*hp_task_woken = (int32_t)woken;
	}

	return (ret == pdTRUE) ? OS_ERR_NONE : OS_ERR_OP;
}


/**
 * @brief Fetch a message, blocking up to @p timeout_ms.
 * @param msg Out-parameter receiving the dequeued pointer.
 * @return OS_ERR_NONE on success, OS_ERR_TIMEOUT on deadline,
 *         OS_ERR_NULL_PTR if either pointer is NULL.
 */
int32_t __SECTION_OS os_mbox_fetch(os_mbox_t mbox, void **msg, uint32_t timeout_ms)
{
	if ((mbox == NULL) || (msg == NULL))
	{
		return OS_ERR_NULL_PTR;
	}

	if (xQueueReceive((QueueHandle_t)mbox, msg, _ms_to_ticks(timeout_ms)) == pdTRUE)
	{
		return OS_ERR_NONE;
	}

	return OS_ERR_TIMEOUT;
}


/**
 * @brief Fetch a message without blocking.
 * @param msg Out-parameter receiving the dequeued pointer.
 * @return OS_ERR_NONE on success, OS_ERR_OP if the mailbox is empty,
 *         OS_ERR_NULL_PTR if either pointer is NULL.
 */
int32_t __SECTION_OS os_mbox_tryfetch(os_mbox_t mbox, void **msg)
{
	if ((mbox == NULL) || (msg == NULL))
	{
		return OS_ERR_NULL_PTR;
	}

	if (xQueueReceive((QueueHandle_t)mbox, msg, 0) == pdTRUE)
	{
		return OS_ERR_NONE;
	}

	return OS_ERR_OP;
}
