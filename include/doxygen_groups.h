/**
 * @file        doxygen_groups.h
 * @brief       Master Doxygen group hierarchy for FreeRTOS-OS
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Documentation
 * @info        This file is read by Doxygen only — it does not contribute
 *              any code.  Every first-party source file in FreeRTOS-OS
 *              declares an @ingroup pointing into one of the leaves
 *              defined below, so the generated HTML / man pages have a
 *              stable navigation tree.
 *
 *              Hierarchy:
 *                  os                      (root — everything is here)
 *                  ├── arch                (CPU architectures & MCU vendors)
 *                  ├── boot                (Reset → main → scheduler)
 *                  ├── mm                  (Heap, list, DMA pool)
 *                  ├── ipc                 (Queues, ring buffer, global vars)
 *                  ├── irq                 (Linux-style irq_desc, NVIC chip)
 *                  ├── log                 (Structured ring-buffer logger)
 *                  ├── safety              (IWDG, safe-state, fault handler)
 *                  ├── services            (Mgmt threads: UART/I2C/SPI/...)
 *                  ├── shell               (FreeRTOS-Plus-CLI integration)
 *                  ├── drivers             (Vendor-agnostic driver layer)
 *                  │   ├── hal_stm32       (STM32 HAL backend)
 *                  │   └── hal_infineon    (Infineon XMC backend stubs)
 *                  ├── drv_app             (Driver-application helpers)
 *                  ├── drv_ext_chips       (External chip drivers)
 *                  ├── config              (Kconfig, autoconf, board cfg)
 *                  └── public_api          (Public OS API surface)
 * @dependency  None — header is documentation-only.
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

/**
 * @defgroup    os              FreeRTOS-OS
 * @brief       Linux-inspired OS layer atop FreeRTOS for ARM Cortex-M.
 *
 * FreeRTOS-OS is a thin OS layer built above the FreeRTOS kernel.  It
 * adds:
 *   - a Linux-style driver model (`drv_*` + `hal_*` vtables);
 *   - service threads for UART/I2C/SPI/GPIO/ADC;
 *   - an interactive shell on UART_APP;
 *   - a medical-grade safety stack (IWDG + SW watchdog + safe-state);
 *   - generators that turn `board/*.xml` into BSP, IRQ tables, and
 *     STM32 HAL configuration.
 *
 * Everything in the project hangs off this top-level group.
 */

/**
 * @defgroup    arch            Architecture & MCU Support
 * @ingroup     os
 * @brief       CPU architectures (`arch/arm/`) and MCU vendor packages
 *              (`arch/devices/<vendor>/<family>/<part>/`).
 *
 * Each `(vendor, family, part)` triple ships:
 *   - a startup file (`startup_*.c|.s`);
 *   - a linker script (`*_FLASH.ld`);
 *   - a CMSIS device header (`<part>.h`);
 *   - optional system-init (`system_*.c`).
 *
 * The dispatching glue lives in @c arch/Makefile and @c arch/Kconfig.
 */

/** @defgroup boot             Boot & Initialisation       @ingroup os */
/** @defgroup mm               Memory Management           @ingroup os */
/** @defgroup ipc              Inter-Process Communication @ingroup os */
/** @defgroup irq              Interrupt Subsystem         @ingroup os */
/** @defgroup log              Structured Logging          @ingroup os */
/** @defgroup safety           Medical-Grade Safety        @ingroup os */
/** @defgroup services         Service Threads             @ingroup os */
/** @defgroup shell            Interactive Shell           @ingroup os */
/** @defgroup drivers          Driver Layer                @ingroup os */
/** @defgroup hal_stm32        STM32 HAL Backend           @ingroup drivers */
/** @defgroup hal_infineon     Infineon HAL Backend        @ingroup drivers */
/** @defgroup drv_app          Driver-Application Helpers  @ingroup drivers */
/** @defgroup drv_ext_chips    External Chip Drivers       @ingroup drivers */
/** @defgroup config           Configuration & Definitions @ingroup os */
/** @defgroup public_api       Public OS API               @ingroup os */
/** @defgroup kernel_glue      Kernel Glue (libc shims)    @ingroup os */
