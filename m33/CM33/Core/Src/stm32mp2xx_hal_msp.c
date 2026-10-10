/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32mp2xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "main.h"
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

  /* System interrupt init*/

  /* Peripheral interrupt init */
  /* RCC_WAKEUP_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(RCC_WAKEUP_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(RCC_WAKEUP_IRQn);

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}

/**
  * @brief I2C MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hi2c->Instance==I2C8)
  {
    /* USER CODE BEGIN I2C8_MspInit 0 */
    /* RIF (m33-examples config) delegates the GPIOZ bank clock (RCC resource
     * 101) and the I2C8 clock to CID2, so this core enables them directly.
     * ck_ker_i2c8 rate/parent is set by the A35 boot chain (OP-TEE DT). */
    __HAL_RCC_GPIOZ_CLK_ENABLE();
    __HAL_RCC_I2C8_CLK_ENABLE();
    /* USER CODE END I2C8_MspInit 0 */

    /**I2C8 GPIO Configuration
    PZ3     ------> I2C8_SDA
    PZ4     ------> I2C8_SCL
    */
    GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_I2C8;
    HAL_GPIO_Init(GPIOZ, &GPIO_InitStruct);

    /* USER CODE BEGIN I2C8_MspInit 1 */

    /* USER CODE END I2C8_MspInit 1 */

  }

}

/**
  * @brief I2C MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
  if(hi2c->Instance==I2C8)
  {
    /* USER CODE BEGIN I2C8_MspDeInit 0 */

    /* USER CODE END I2C8_MspDeInit 0 */

    /**I2C8 GPIO Configuration
    PZ3     ------> I2C8_SDA
    PZ4     ------> I2C8_SCL
    */
    HAL_GPIO_DeInit(GPIOZ, GPIO_PIN_3);

    HAL_GPIO_DeInit(GPIOZ, GPIO_PIN_4);

    /* USER CODE BEGIN I2C8_MspDeInit 1 */

    /* USER CODE END I2C8_MspDeInit 1 */
  }

}

/**
  * @brief UART MSP Initialization
  * This function configures the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(huart->Instance==USART6)
  {
    /* USER CODE BEGIN USART6_MspInit 0 */
    /* USART6 clock is CID2-delegated — enable it here. The GPIOF bank clock
     * (RCC resource 95) is NOT ours (semaphore whitelist = A35 only, a side
     * effect of reclaiming ethernet): do not call __HAL_RCC_GPIOF_CLK_ENABLE().
     * Linux keeps GPIOF clocked (ethernet pin PF1 lives in that bank), and
     * PF13/PF14 themselves are CID2 in the GPIO RIF, so pin config works. */
    __HAL_RCC_USART6_CLK_ENABLE();
    /* USER CODE END USART6_MspInit 0 */

    /**USART6 GPIO Configuration
    PF14     ------> USART6_RX
    PF13     ------> USART6_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Alternate = GPIO_AF3_USART6;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF3_USART6;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* USER CODE BEGIN USART6_MspInit 1 */

    /* USER CODE END USART6_MspInit 1 */

  }

}

/**
  * @brief UART MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if(huart->Instance==USART6)
  {
    /* USER CODE BEGIN USART6_MspDeInit 0 */

    /* USER CODE END USART6_MspDeInit 0 */

    /**USART6 GPIO Configuration
    PF14     ------> USART6_RX
    PF13     ------> USART6_TX
    */
    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_14|GPIO_PIN_13);

    /* USER CODE BEGIN USART6_MspDeInit 1 */

    /* USER CODE END USART6_MspDeInit 1 */
  }

}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
