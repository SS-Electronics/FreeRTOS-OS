/*
 * hal_iic_stm32.h — STM32 HAL I2C ops table declaration
 *
 * This file is part of FreeRTOS-OS Project.
 */

#ifndef DRIVERS_HAL_STM32_HAL_IIC_STM32_H_
#define DRIVERS_HAL_STM32_HAL_IIC_STM32_H_

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM) && defined(HAL_I2C_MODULE_ENABLED)

#include <drivers/com/drv_iic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Return pointer to the STM32 I2C HAL ops table. */
const drv_iic_hal_ops_t *hal_iic_stm32_get_ops(void);

/**
 * @brief  Populate the hardware context in a generic I2C handle.
 *
 * @param  h          Generic I2C handle.
 * @param  instance   I2C peripheral instance (I2C1, I2C2, …).
 * @param  addr_mode  I2C_ADDRESSINGMODE_7BIT / I2C_ADDRESSINGMODE_10BIT
 * @param  dual_addr  I2C_DUALADDRESS_DISABLE / I2C_DUALADDRESS_ENABLE
 */
void hal_iic_stm32_set_config(drv_iic_handle_t *h,
                              I2C_TypeDef      *instance,
                              uint32_t          addr_mode,
                              uint32_t          dual_addr);

/**
 * @brief  Called from I2Cx_EV_IRQHandler to dispatch HAL_I2C_EV_IRQHandler
 *         to whichever generic handle owns @p instance.
 */
void hal_iic_stm32_ev_irq_handler(I2C_TypeDef *instance);

/**
 * @brief  Called from I2Cx_ER_IRQHandler to dispatch HAL_I2C_ER_IRQHandler
 *         to whichever generic handle owns @p instance.
 */
void hal_iic_stm32_er_irq_handler(I2C_TypeDef *instance);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM && HAL_I2C_MODULE_ENABLED */

#endif /* DRIVERS_HAL_STM32_HAL_IIC_STM32_H_ */
