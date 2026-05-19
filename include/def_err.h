/**
 * @file        def_err.h
 * @brief       def err
 * @ingroup     config
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Definitions
 * @info        Project-wide attribute / compiler / standard-type macros and error code enums.
 * @dependency  Compiler intrinsics, CMSIS
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

#ifndef __DEF_ERR__
#define __DEF_ERR__

/* ── Generic return codes ─────────────────────────────────────────────────── */
#define OS_ERR_NONE          0    /* success */
#define OS_ERR_OP           -1    /* generic operation failure */
#define OS_ERR_MEM_OF       -2    /* memory overflow */
#define OS_ERR_MEM_OP       -3    /* memory operation failure */
#define OS_ERR_NULL_PTR     -4    /* null pointer argument */
#define OS_ERR_TIMEOUT      -5    /* operation timed out */
#define OS_ERR_INVALID_ARG  -6    /* invalid argument value */
#define OS_ERR_HW_FAULT     -7    /* hardware peripheral fault */
#define OS_ERR_WATCHDOG     -8    /* watchdog kick missed */
#define OS_ERR_STACK_OVF    -9    /* stack overflow detected */
#define OS_ERR_MALLOC_FAIL  -10   /* heap allocation failure */
#define OS_ERR_BOOT_FAIL    -11   /* fatal failure during boot sequence */
#define OS_ERR_SAFE_STATE   -12   /* system entered safe state */
#define OS_ERR_CRC          -13   /* data integrity check (CRC) failed */
#define OS_ERR_NOT_INIT     -14   /* module not yet initialised */

/* Legacy aliases */
#define ERROR_NONE          OS_ERR_NONE
#define ERROR_OP            OS_ERR_OP




#endif