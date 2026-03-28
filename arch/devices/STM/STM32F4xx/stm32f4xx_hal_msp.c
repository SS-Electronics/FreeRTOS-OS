/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32f4xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "device.h"
#include <board/board_config.h>
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{

  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* System interrupt init*/
  /* SVCall_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SVCall_IRQn, 15, 0);
  /* PendSV_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}

/**
  * @brief I2C MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
#ifdef HAL_I2C_MODULE_ENABLED
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
    const board_iic_desc_t *d = board_find_iic(hi2c->Instance);
    if (d == NULL) return;

    d->periph_clk_enable();
    board_gpio_clk_enable(d->scl_pin.port);
    board_gpio_clk_enable(d->sda_pin.port);

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

    HAL_NVIC_SetPriority(d->ev_irqn, d->irq_priority, 0);
    HAL_NVIC_EnableIRQ(d->ev_irqn);
    HAL_NVIC_SetPriority(d->er_irqn, d->irq_priority, 0);
    HAL_NVIC_EnableIRQ(d->er_irqn);
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
    const board_iic_desc_t *d = board_find_iic(hi2c->Instance);
    if (d == NULL) return;

    HAL_NVIC_DisableIRQ(d->ev_irqn);
    HAL_NVIC_DisableIRQ(d->er_irqn);
    HAL_GPIO_DeInit(d->scl_pin.port, d->scl_pin.pin);
    HAL_GPIO_DeInit(d->sda_pin.port, d->sda_pin.pin);
}
#endif /* HAL_I2C_MODULE_ENABLED */

/**
  * @brief SPI MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hspi: SPI handle pointer
  * @retval None
  */
#ifdef HAL_SPI_MODULE_ENABLED
void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
    const board_spi_desc_t *d = board_find_spi(hspi->Instance);
    if (d == NULL) return;

    d->periph_clk_enable();
    board_gpio_clk_enable(d->sck_pin.port);
    board_gpio_clk_enable(d->miso_pin.port);
    board_gpio_clk_enable(d->mosi_pin.port);

    GPIO_InitTypeDef gpio = {
        .Mode      = d->sck_pin.mode,
        .Pull      = d->sck_pin.pull,
        .Speed     = d->sck_pin.speed,
        .Alternate = d->sck_pin.alternate,
    };

    gpio.Pin = d->sck_pin.pin;
    HAL_GPIO_Init(d->sck_pin.port, &gpio);

    gpio.Pin = d->miso_pin.pin;
    gpio.Alternate = d->miso_pin.alternate;
    HAL_GPIO_Init(d->miso_pin.port, &gpio);

    gpio.Pin = d->mosi_pin.pin;
    gpio.Alternate = d->mosi_pin.alternate;
    HAL_GPIO_Init(d->mosi_pin.port, &gpio);

    if (d->nss_pin.pin != 0) {
        board_gpio_clk_enable(d->nss_pin.port);
        gpio.Pin = d->nss_pin.pin;
        gpio.Alternate = d->nss_pin.alternate;
        HAL_GPIO_Init(d->nss_pin.port, &gpio);
    }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
    const board_spi_desc_t *d = board_find_spi(hspi->Instance);
    if (d == NULL) return;

    HAL_GPIO_DeInit(d->sck_pin.port,  d->sck_pin.pin);
    HAL_GPIO_DeInit(d->miso_pin.port, d->miso_pin.pin);
    HAL_GPIO_DeInit(d->mosi_pin.port, d->mosi_pin.pin);
    if (d->nss_pin.pin != 0)
        HAL_GPIO_DeInit(d->nss_pin.port, d->nss_pin.pin);
}
#endif /* HAL_SPI_MODULE_ENABLED */

/**
  * @brief UART MSP Initialization
  * This function configures the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
#ifdef HAL_UART_MODULE_ENABLED
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
    const board_uart_desc_t *d = board_find_uart(huart->Instance);
    if (d == NULL) return;

    d->periph_clk_enable();
    board_gpio_clk_enable(d->tx_pin.port);
    board_gpio_clk_enable(d->rx_pin.port);

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

    HAL_NVIC_SetPriority(d->irqn, d->irq_priority, 0);
    HAL_NVIC_EnableIRQ(d->irqn);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
    const board_uart_desc_t *d = board_find_uart(huart->Instance);
    if (d == NULL) return;

    HAL_NVIC_DisableIRQ(d->irqn);
    HAL_GPIO_DeInit(d->tx_pin.port, d->tx_pin.pin);
    HAL_GPIO_DeInit(d->rx_pin.port, d->rx_pin.pin);
}
#endif /* HAL_UART_MODULE_ENABLED */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
