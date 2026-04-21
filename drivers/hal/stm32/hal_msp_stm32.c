/*
 * hal_msp_stm32.c — STM32 HAL MSP (MCU Support Package) callbacks
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * HAL_MspInit / HAL_xxx_MspInit / HAL_xxx_MspDeInit are callbacks invoked by
 * the STM32 HAL during peripheral initialisation and de-initialisation.  They
 * configure clocks and GPIO alternate functions by querying the board
 * descriptor tables (board_find_uart / board_find_iic / board_find_spi) so
 * this file contains zero board-specific constants.
 *
 * NVIC setup is routed through drv_irq_enable / drv_irq_disable so no
 * HAL_NVIC_* calls appear outside of drivers/drv_irq.c.
 */

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <device.h>
#include <board/board_config.h>
#include <drivers/drv_irq.h>
#include <drivers/drv_rcc.h>

/* ── Global MSP init ──────────────────────────────────────────────────────── */

void HAL_MspInit(void)
{
    drv_rcc_periph_clk_en(DRV_RCC_PERIPH_SYSCFG);
    drv_rcc_periph_clk_en(DRV_RCC_PERIPH_PWR);

    /* Cortex-M core exceptions — set priority only, no enable needed */
    drv_irq_set_priority((int32_t)SVCall_IRQn, 15);
    drv_irq_set_priority((int32_t)PendSV_IRQn, 15);
}

/* ── I2C MSP ──────────────────────────────────────────────────────────────── */

#ifdef HAL_I2C_MODULE_ENABLED
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    const board_iic_desc_t *d = board_find_iic(hi2c->Instance);
    if (d == NULL) return;

    d->periph_clk_enable();
    drv_rcc_gpio_clk_en(d->scl_pin.port);
    drv_rcc_gpio_clk_en(d->sda_pin.port);

    GPIO_InitTypeDef gpio = {
        .Mode      = d->scl_pin.mode,
        .Pull      = d->scl_pin.pull,
        .Speed     = d->scl_pin.speed,
        .Alternate = d->scl_pin.alternate,
    };

    gpio.Pin = d->scl_pin.pin;
    HAL_GPIO_Init(d->scl_pin.port, &gpio);

    gpio.Pin       = d->sda_pin.pin;
    gpio.Alternate = d->sda_pin.alternate;
    HAL_GPIO_Init(d->sda_pin.port, &gpio);

    drv_irq_enable((int32_t)d->ev_irqn, d->irq_priority);
    drv_irq_enable((int32_t)d->er_irqn, d->irq_priority);
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
    const board_iic_desc_t *d = board_find_iic(hi2c->Instance);
    if (d == NULL) return;

    drv_irq_disable((int32_t)d->ev_irqn);
    drv_irq_disable((int32_t)d->er_irqn);
    HAL_GPIO_DeInit(d->scl_pin.port, d->scl_pin.pin);
    HAL_GPIO_DeInit(d->sda_pin.port, d->sda_pin.pin);
}
#endif /* HAL_I2C_MODULE_ENABLED */

/* ── SPI MSP ──────────────────────────────────────────────────────────────── */

#ifdef HAL_SPI_MODULE_ENABLED
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    const board_spi_desc_t *d = board_find_spi(hspi->Instance);
    if (d == NULL) return;

    d->periph_clk_enable();
    drv_rcc_gpio_clk_en(d->sck_pin.port);
    drv_rcc_gpio_clk_en(d->miso_pin.port);
    drv_rcc_gpio_clk_en(d->mosi_pin.port);

    GPIO_InitTypeDef gpio = {
        .Mode      = d->sck_pin.mode,
        .Pull      = d->sck_pin.pull,
        .Speed     = d->sck_pin.speed,
        .Alternate = d->sck_pin.alternate,
    };

    gpio.Pin = d->sck_pin.pin;
    HAL_GPIO_Init(d->sck_pin.port, &gpio);

    gpio.Pin       = d->miso_pin.pin;
    gpio.Alternate = d->miso_pin.alternate;
    HAL_GPIO_Init(d->miso_pin.port, &gpio);

    gpio.Pin       = d->mosi_pin.pin;
    gpio.Alternate = d->mosi_pin.alternate;
    HAL_GPIO_Init(d->mosi_pin.port, &gpio);

    if (d->nss_pin.pin != 0)
    {
        drv_rcc_gpio_clk_en(d->nss_pin.port);
        gpio.Pin       = d->nss_pin.pin;
        gpio.Alternate = d->nss_pin.alternate;
        HAL_GPIO_Init(d->nss_pin.port, &gpio);
    }

    drv_irq_enable((int32_t)d->irqn, d->irq_priority);
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
    const board_spi_desc_t *d = board_find_spi(hspi->Instance);
    if (d == NULL) return;

    drv_irq_disable((int32_t)d->irqn);
    HAL_GPIO_DeInit(d->sck_pin.port,  d->sck_pin.pin);
    HAL_GPIO_DeInit(d->miso_pin.port, d->miso_pin.pin);
    HAL_GPIO_DeInit(d->mosi_pin.port, d->mosi_pin.pin);
    if (d->nss_pin.pin != 0)
        HAL_GPIO_DeInit(d->nss_pin.port, d->nss_pin.pin);
}
#endif /* HAL_SPI_MODULE_ENABLED */

/* ── UART MSP ─────────────────────────────────────────────────────────────── */

#ifdef HAL_UART_MODULE_ENABLED
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    const board_uart_desc_t *d = board_find_uart(huart->Instance);
    if (d == NULL) return;

    d->periph_clk_enable();
    drv_rcc_gpio_clk_en(d->tx_pin.port);
    drv_rcc_gpio_clk_en(d->rx_pin.port);

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
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    const board_uart_desc_t *d = board_find_uart(huart->Instance);
    if (d == NULL) return;

    drv_irq_disable((int32_t)d->irqn);
    HAL_GPIO_DeInit(d->tx_pin.port, d->tx_pin.pin);
    HAL_GPIO_DeInit(d->rx_pin.port, d->rx_pin.pin);
}
#endif /* HAL_UART_MODULE_ENABLED */

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
