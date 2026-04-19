/*
 * hal_spi_stm32.c — STM32 HAL SPI ops implementation
 *
 * This file is part of FreeRTOS-OS Project.
 */

#include <config/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM) && defined(HAL_SPI_MODULE_ENABLED)

#include <drivers/drv_handle.h>
#include <drivers/com/hal/stm32/hal_spi_stm32.h>

static int32_t _hal_to_os_err(HAL_StatusTypeDef s)
{
    return (s == HAL_OK) ? OS_ERR_NONE : OS_ERR_OP;
}

/* ── HAL ops implementations ──────────────────────────────────────────── */

static int32_t stm32_spi_hw_init(drv_spi_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret = HAL_SPI_Init(&h->hw.hspi);
    if (ret == HAL_OK)
        h->initialized = 1;

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_spi_hw_deinit(drv_spi_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret = HAL_SPI_DeInit(&h->hw.hspi);
    h->initialized = 0;
    h->last_err    = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_spi_transmit(drv_spi_handle_t *h,
                                   const uint8_t    *data,
                                   uint16_t          len,
                                   uint32_t          timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret = HAL_SPI_Transmit(&h->hw.hspi,
                                              (uint8_t *)data,
                                              len,
                                              timeout_ms);
    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_spi_receive(drv_spi_handle_t *h,
                                  uint8_t          *data,
                                  uint16_t          len,
                                  uint32_t          timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret = HAL_SPI_Receive(&h->hw.hspi, data, len, timeout_ms);
    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_spi_transfer(drv_spi_handle_t *h,
                                   const uint8_t    *tx,
                                   uint8_t          *rx,
                                   uint16_t          len,
                                   uint32_t          timeout_ms)
{
    if (h == NULL || !h->initialized || tx == NULL || rx == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(&h->hw.hspi,
                                                     (uint8_t *)tx,
                                                     rx,
                                                     len,
                                                     timeout_ms);
    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/* ── Static ops table ─────────────────────────────────────────────────── */

static const drv_spi_hal_ops_t _stm32_spi_ops = {
    .hw_init   = stm32_spi_hw_init,
    .hw_deinit = stm32_spi_hw_deinit,
    .transmit  = stm32_spi_transmit,
    .receive   = stm32_spi_receive,
    .transfer  = stm32_spi_transfer,
};

/* ── Public API ───────────────────────────────────────────────────────── */

const drv_spi_hal_ops_t *hal_spi_stm32_get_ops(void)
{
    return &_stm32_spi_ops;
}

void hal_spi_stm32_set_config(drv_spi_handle_t *h,
                               SPI_TypeDef      *instance,
                               uint32_t          mode,
                               uint32_t          direction,
                               uint32_t          data_size,
                               uint32_t          clk_polarity,
                               uint32_t          clk_phase,
                               uint32_t          nss,
                               uint32_t          baud_prescaler,
                               uint32_t          bit_order)
{
    if (h == NULL || instance == NULL)
        return;

    SPI_HandleTypeDef *hspi = &h->hw.hspi;

    hspi->Instance               = instance;
    hspi->Init.Mode              = mode;
    hspi->Init.Direction         = direction;
    hspi->Init.DataSize          = data_size;
    hspi->Init.CLKPolarity       = clk_polarity;
    hspi->Init.CLKPhase          = clk_phase;
    hspi->Init.NSS               = nss;
    hspi->Init.BaudRatePrescaler = baud_prescaler;
    hspi->Init.FirstBit          = bit_order;
    hspi->Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi->Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi->Init.CRCPolynomial     = 10;
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM && HAL_SPI_MODULE_ENABLED */
