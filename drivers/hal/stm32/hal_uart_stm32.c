/**
 * @file    hal_uart_stm32.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   STM32 HAL-based UART backend with direct register IRQ handling
 *
 * @details
 * This module implements the drv_uart_hal_ops_t interface for STM32 devices,
 * providing a hardware abstraction layer between the generic UART driver
 * (drv_uart) and STM32 peripherals.
 *
 * The implementation uses STM32 HAL for initialization and blocking APIs,
 * while bypassing HAL interrupt handling in favor of direct register-level
 * ISR logic for improved determinism and control.
 *
 * Features:
 * - UART hardware initialization via board descriptor
 * - Blocking transmit and receive (HAL-based)
 * - Interrupt-driven TX using ring buffer
 * - Direct IRQ handling without HAL callback chain
 *
 * IRQ Design:
 * - Reads USART SR/CR1/CR3 registers directly
 * - Dispatches events through drv_irq subsystem
 * - Avoids HAL_UART_IRQHandler overhead
 *
 * IRQ Flow:
 * @code
 * USART IRQ →
 *   hal_uart_stm32_irq_handler()
 *     → RX   → drv_irq_dispatch_from_isr(UART_RX)
 *     → TX   → drv_irq_dispatch_from_isr(UART_TX_DONE)
 *     → ERR  → drv_irq_dispatch_from_isr(UART_ERROR)
 * @endcode
 *
 * @dependencies
 * board/mcu_config.h,
 * drivers/com/drv_uart.h,
 * drivers/com/hal/stm32/hal_uart_stm32.h,
 * drivers/drv_irq.h,
 * board/board_config.h
 *
 * @note
 * This file is part of FreeRTOS-OS Project.
 *
 * @license
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
 * You should have received a copy of the GNU General Public License 
 * along with FreeRTOS-OS. If not, see <https://www.gnu.org/licenses/>.
 */

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <drivers/drv_uart.h>
#include <drivers/hal/stm32/hal_uart_stm32.h>
#include <drivers/drv_irq.h>
#include <board/board_config.h>

/**
 * @brief Retrieve next byte from TX ring buffer.
 *
 * @param dev_id UART device ID
 * @param byte   Output byte
 * @return OS_ERR_NONE if byte available, otherwise error
 */
extern int32_t drv_uart_tx_get_next_byte(uint8_t dev_id, uint8_t *byte);

/* ────────────────────────────────────────────────────────────────────────── */
/* Internal Helpers                                                          */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Convert HAL status to OS error code.
 *
 * @param s HAL status
 * @return OS_ERR_NONE on success, OS_ERR_OP otherwise
 */
__SECTION_OS __USED
static int32_t _hal_to_os_err(HAL_StatusTypeDef s)
{
    return (s == HAL_OK) ? OS_ERR_NONE : OS_ERR_OP;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* HAL Operations                                                            */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Initialize UART hardware.
 *
 * @param h UART handle
 * @return OS error code
 *
 * @details
 * Configures GPIO, NVIC, and initializes UART via HAL.
 * Enables RXNE interrupt directly at register level.
 */
__SECTION_OS __USED
static int32_t stm32_uart_hw_init(drv_uart_handle_t *h)
{
    if (h == NULL || h->ops == NULL)
        return OS_ERR_OP;

    UART_HandleTypeDef      *huart = &h->hw.huart;
    const board_uart_desc_t *d     = board_find_uart(huart->Instance);
    if (d == NULL)
        return OS_ERR_OP;

    GPIO_InitTypeDef gpio = {
        .Mode      = d->tx_pin.mode,
        .Pull      = d->tx_pin.pull,
        .Speed     = d->tx_pin.speed,
        .Alternate = d->tx_pin.alternate,
    };

    gpio.Pin = d->tx_pin.pin;
    HAL_GPIO_Init(d->tx_pin.port, &gpio);

    gpio.Pin       = d->rx_pin.pin;
    gpio.Alternate = d->rx_pin.alternate;
    HAL_GPIO_Init(d->rx_pin.port, &gpio);

    drv_irq_enable((int32_t)d->irqn, d->irq_priority);

    huart->Init.BaudRate = h->baudrate;

    HAL_StatusTypeDef ret = HAL_UART_Init(huart);
    if (ret == HAL_OK)
    {
        h->initialized = 1;
        SET_BIT(huart->Instance->CR1, USART_CR1_RXNEIE);
    }

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/**
 * @brief Deinitialize UART hardware.
 *
 * @param h UART handle
 * @return OS error code
 */
__SECTION_OS __USED
static int32_t stm32_uart_hw_deinit(drv_uart_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    const board_uart_desc_t *d = board_find_uart(h->hw.huart.Instance);

    CLEAR_BIT(h->hw.huart.Instance->CR1, USART_CR1_RXNEIE);
    HAL_StatusTypeDef ret = HAL_UART_DeInit(&h->hw.huart);

    h->initialized = 0;
    h->last_err    = _hal_to_os_err(ret);

    if (d != NULL)
    {
        drv_irq_disable((int32_t)d->irqn);
        HAL_GPIO_DeInit(d->tx_pin.port, d->tx_pin.pin);
        HAL_GPIO_DeInit(d->rx_pin.port, d->rx_pin.pin);
    }

    return h->last_err;
}

/**
 * @brief Transmit data (blocking).
 */
__SECTION_OS __USED
static int32_t stm32_uart_transmit(drv_uart_handle_t *h,
                                   const uint8_t *data,
                                   uint16_t len,
                                   uint32_t timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret = HAL_UART_Transmit(&h->hw.huart,
                                              (uint8_t *)data,
                                              len,
                                              timeout_ms);
    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/**
 * @brief Receive data (blocking).
 */
__SECTION_OS __USED
static int32_t stm32_uart_receive(drv_uart_handle_t *h,
                                  uint8_t *data,
                                  uint16_t len,
                                  uint32_t timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret = HAL_UART_Receive(&h->hw.huart, data, len, timeout_ms);
    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/**
 * @brief Start TX interrupt-driven transmission.
 */
__SECTION_OS __USED
static void stm32_uart_tx_start(drv_uart_handle_t *h)
{
    if (h == NULL || !h->initialized)
        return;

    SET_BIT(h->hw.huart.Instance->CR1, USART_CR1_TXEIE);
}

/* ────────────────────────────────────────────────────────────────────────── */
/* Ops Table                                                                 */
/* ────────────────────────────────────────────────────────────────────────── */
static const drv_uart_hal_ops_t _stm32_uart_ops = {
    .hw_init   = stm32_uart_hw_init,
    .hw_deinit = stm32_uart_hw_deinit,
    .transmit  = stm32_uart_transmit,
    .receive   = stm32_uart_receive,
    .tx_start  = stm32_uart_tx_start,
};

/* ────────────────────────────────────────────────────────────────────────── */
/* Public API                                                                */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Get STM32 UART HAL operations.
 */
__SECTION_OS __USED
const drv_uart_hal_ops_t *hal_uart_stm32_get_ops(void)
{
    return &_stm32_uart_ops;
}

/**
 * @brief Configure UART parameters.
 */
__SECTION_OS __USED
void hal_uart_stm32_set_config(drv_uart_handle_t *h,
                               USART_TypeDef *instance,
                               uint32_t word_len,
                               uint32_t stop_bits,
                               uint32_t parity,
                               uint32_t mode)
{
    if (h == NULL || instance == NULL)
        return;

    UART_HandleTypeDef *huart = &h->hw.huart;

    huart->Instance          = instance;
    huart->Init.WordLength   = word_len;
    huart->Init.StopBits     = stop_bits;
    huart->Init.Parity       = parity;
    huart->Init.Mode         = mode;
    huart->Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart->Init.OverSampling = UART_OVERSAMPLING_16;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* IRQ Handler                                                               */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief USART IRQ handler (direct register-level implementation).
 *
 * @param instance USART peripheral instance
 *
 * @details
 * Handles RX, TX, and error interrupts without using HAL IRQ handlers.
 * Dispatches events through drv_irq subsystem.
 */
__SECTION_OS __USED
void hal_uart_stm32_irq_handler(USART_TypeDef *instance)
{
    for (uint8_t id = 0; id < NO_OF_UART; id++)
    {
        drv_uart_handle_t *h = drv_uart_get_handle(id);
        if (h == NULL || !h->initialized || h->hw.huart.Instance != instance)
            continue;

        uint32_t sr = READ_REG(instance->SR);
        uint32_t cr1 = READ_REG(instance->CR1);
        uint32_t cr3 = READ_REG(instance->CR3);
        uint32_t errorflags = sr & (USART_SR_PE | USART_SR_FE |
                                   USART_SR_ORE | USART_SR_NE);

        BaseType_t hpt = pdFALSE;

        if (errorflags == 0U)
        {
            if ((sr & USART_SR_RXNE) && (cr1 & USART_CR1_RXNEIE))
            {
                uint8_t byte = (uint8_t)(READ_REG(instance->DR) & 0xFFU);
                drv_irq_dispatch_from_isr(IRQ_ID_UART_RX(id), &byte, &hpt);
                portYIELD_FROM_ISR(hpt);
                return;
            }
        }

        if (errorflags && ((cr3 & USART_CR3_EIE) ||
                           (cr1 & (USART_CR1_RXNEIE | USART_CR1_PEIE))))
        {
            uint8_t byte = (uint8_t)(READ_REG(instance->DR) & 0xFFU);

            if (sr & USART_SR_RXNE)
                drv_irq_dispatch_from_isr(IRQ_ID_UART_RX(id), &byte, &hpt);

            drv_irq_dispatch_from_isr(IRQ_ID_UART_ERROR(id), &errorflags, &hpt);
            portYIELD_FROM_ISR(hpt);
            return;
        }

        if ((sr & USART_SR_TXE) && (cr1 & USART_CR1_TXEIE))
        {
            uint8_t byte;
            if (drv_uart_tx_get_next_byte(id, &byte) == OS_ERR_NONE)
            {
                WRITE_REG(instance->DR, byte);
            }
            else
            {
                CLEAR_BIT(instance->CR1, USART_CR1_TXEIE);
                drv_irq_dispatch_from_isr(IRQ_ID_UART_TX_DONE(id), NULL, &hpt);
            }
            portYIELD_FROM_ISR(hpt);
            return;
        }

        if ((sr & USART_SR_TC) && (cr1 & USART_CR1_TCIE))
        {
            CLEAR_BIT(instance->CR1, USART_CR1_TCIE);
            drv_irq_dispatch_from_isr(IRQ_ID_UART_TX_DONE(id), NULL, &hpt);
            portYIELD_FROM_ISR(hpt);
            return;
        }

        return;
    }
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */