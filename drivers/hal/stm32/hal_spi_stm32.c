/*
 * hal_spi_stm32.c — STM32 HAL SPI ops implementation
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Provides both blocking ops (transmit/receive/transfer) and interrupt-driven
 * ops (transmit_it/receive_it/transfer_it).  Completion events are dispatched
 * directly from hal_spi_stm32_irq_handler() by inspecting HAL_SPI_GetState()
 * before and after calling the HAL handler — no HAL weak callbacks are used.
 */

#include <board/mcu_config.h>
#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM) && defined(HAL_SPI_MODULE_ENABLED)

#include <drivers/drv_handle.h>
#include <drivers/com/hal/stm32/hal_spi_stm32.h>
#include <drivers/drv_irq.h>

static int32_t _hal_to_os_err(HAL_StatusTypeDef s)
{
    return (s == HAL_OK) ? OS_ERR_NONE : OS_ERR_OP;
}

/* ── HAL ops implementations ──────────────────────────────────────────────── */

static int32_t stm32_spi_hw_init(drv_spi_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    SPI_HandleTypeDef      *hspi = &h->hw.hspi;
    const board_spi_desc_t *d    = board_find_spi(hspi->Instance);
    if (d == NULL)
        return OS_ERR_OP;

    /* GPIO and NVIC setup — owned here so HAL_SPI_MspInit (called internally
     * by HAL_SPI_Init below) is a no-op and init ownership stays in this fn. */
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
        gpio.Pin       = d->nss_pin.pin;
        gpio.Alternate = d->nss_pin.alternate;
        HAL_GPIO_Init(d->nss_pin.port, &gpio);
    }

    drv_irq_enable((int32_t)d->irqn, d->irq_priority);

    HAL_StatusTypeDef ret = HAL_SPI_Init(hspi);
    if (ret == HAL_OK)
        h->initialized = 1;

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_spi_hw_deinit(drv_spi_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    const board_spi_desc_t *d = board_find_spi(h->hw.hspi.Instance);

    HAL_StatusTypeDef ret = HAL_SPI_DeInit(&h->hw.hspi);
    h->initialized = 0;
    h->last_err    = _hal_to_os_err(ret);

    if (d != NULL)
    {
        drv_irq_disable((int32_t)d->irqn);
        HAL_GPIO_DeInit(d->sck_pin.port,  d->sck_pin.pin);
        HAL_GPIO_DeInit(d->miso_pin.port, d->miso_pin.pin);
        HAL_GPIO_DeInit(d->mosi_pin.port, d->mosi_pin.pin);
        if (d->nss_pin.pin != 0)
            HAL_GPIO_DeInit(d->nss_pin.port, d->nss_pin.pin);
    }

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
                                              (uint8_t *)data, len, timeout_ms);
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
                                                     (uint8_t *)tx, rx, len,
                                                     timeout_ms);
    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/* ── IT ops ───────────────────────────────────────────────────────────────── */

static int32_t stm32_spi_transmit_it(drv_spi_handle_t *h,
                                      const uint8_t    *data,
                                      uint16_t          len)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret = HAL_SPI_Transmit_IT(&h->hw.hspi, (uint8_t *)data, len);
    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_spi_receive_it(drv_spi_handle_t *h,
                                     uint8_t          *data,
                                     uint16_t          len)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret = HAL_SPI_Receive_IT(&h->hw.hspi, data, len);
    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_spi_transfer_it(drv_spi_handle_t *h,
                                      const uint8_t    *tx,
                                      uint8_t          *rx,
                                      uint16_t          len)
{
    if (h == NULL || !h->initialized || tx == NULL || rx == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive_IT(&h->hw.hspi,
                                                        (uint8_t *)tx, rx, len);
    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/* ── Static ops table ─────────────────────────────────────────────────────── */

static const drv_spi_hal_ops_t _stm32_spi_ops = {
    .hw_init     = stm32_spi_hw_init,
    .hw_deinit   = stm32_spi_hw_deinit,
    .transmit    = stm32_spi_transmit,
    .receive     = stm32_spi_receive,
    .transfer    = stm32_spi_transfer,
    .transmit_it = stm32_spi_transmit_it,
    .receive_it  = stm32_spi_receive_it,
    .transfer_it = stm32_spi_transfer_it,
};

/* ── Public API ───────────────────────────────────────────────────────────── */

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

/* ── IRQ dispatch ─────────────────────────────────────────────────────────── */

/*
 * hal_spi_stm32_irq_handler — SPI IRQ, dispatches completion/error events.
 *
 * Captures HAL state before and after calling HAL_SPI_IRQHandler.  When the
 * state returns to READY the transfer is done; if an error was recorded the
 * error path takes priority, otherwise the previous state selects the IRQ ID:
 *   BUSY_TX    → IRQ_ID_SPI_TX_DONE
 *   BUSY_RX    → IRQ_ID_SPI_RX_DONE
 *   BUSY_TX_RX → IRQ_ID_SPI_TXRX_DONE
 * No HAL weak callbacks (TxCplt / RxCplt / TxRxCplt / ErrorCallback) needed.
 */
void hal_spi_stm32_irq_handler(SPI_TypeDef *instance)
{
    for (uint8_t id = 0; id < NO_OF_SPI; id++)
    {
        drv_spi_handle_t *h = drv_spi_get_handle(id);
        if (h == NULL || !h->initialized || h->hw.hspi.Instance != instance)
            continue;

        HAL_SPI_StateTypeDef prev = HAL_SPI_GetState(&h->hw.hspi);
        HAL_SPI_IRQHandler(&h->hw.hspi);

        if (HAL_SPI_GetState(&h->hw.hspi) != HAL_SPI_STATE_READY)
            return; /* transfer still in progress — multi-interrupt op */

        BaseType_t hpt  = pdFALSE;
        uint32_t   err  = HAL_SPI_GetError(&h->hw.hspi);

        if (err != HAL_SPI_ERROR_NONE)
        {
            h->last_err = OS_ERR_OP;
            drv_irq_dispatch_from_isr(IRQ_ID_SPI_ERROR(id), &err, &hpt);
        }
        else
        {
            h->last_err = OS_ERR_NONE;
            if (prev == HAL_SPI_STATE_BUSY_TX)
                drv_irq_dispatch_from_isr(IRQ_ID_SPI_TX_DONE(id), NULL, &hpt);
            else if (prev == HAL_SPI_STATE_BUSY_RX)
                drv_irq_dispatch_from_isr(IRQ_ID_SPI_RX_DONE(id), NULL, &hpt);
            else if (prev == HAL_SPI_STATE_BUSY_TX_RX)
                drv_irq_dispatch_from_isr(IRQ_ID_SPI_TXRX_DONE(id), NULL, &hpt);
        }
        portYIELD_FROM_ISR(hpt);
        return;
    }
}

/* ── SPI MSP / completion stubs ──────────────────────────────────────────── */

/* GPIO and NVIC are owned by stm32_spi_hw_init/deinit; MspInit/DeInit are
 * no-ops so HAL_SPI_Init/DeInit do not double-execute setup.
 * Completion and error callbacks are handled in the IRQ handler above;
 * these stubs silence the HAL weak-symbol override. */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)          { (void)hspi; }
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)        { (void)hspi; }
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)   { (void)hspi; }
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)   { (void)hspi; }
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) { (void)hspi; }
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)    { (void)hspi; }

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM && HAL_SPI_MODULE_ENABLED */
