/*
 * hal_iic_infineon.h — Infineon XMC series baremetal I2C ops declaration
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef DRIVERS_HAL_INFINEON_HAL_IIC_INFINEON_H_
#define DRIVERS_HAL_INFINEON_HAL_IIC_INFINEON_H_

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)

#include <drivers/com/drv_iic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Return pointer to the Infineon I2C HAL ops table. */
const drv_iic_hal_ops_t *hal_iic_infineon_get_ops(void);

/**
 * @brief  Set the hardware context for an Infineon I2C channel.
 *
 * @param  h             Generic I2C handle.
 * @param  channel_base  USIC I2C channel base address.
 */
void hal_iic_infineon_set_config(drv_iic_handle_t *h,
                                 uint32_t          channel_base);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON */

#endif /* DRIVERS_HAL_INFINEON_HAL_IIC_INFINEON_H_ */
