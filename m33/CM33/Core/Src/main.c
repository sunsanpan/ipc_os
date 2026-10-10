/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "openamp.h"
#include "copro_sync.h"
#include "sensor_app.h"
#include <stdio.h>   /* boot-path trace via printf -> trace0 (see __LOG_TRACE_IO_) */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c8;

UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
IPCC_HandleTypeDef hipcc1; /* name required by CoproSync */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C8_Init(void);
static void MX_USART6_UART_Init(void);
/* USER CODE BEGIN PFP */
static void MX_IPCC_Init(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* Production boot via remoteproc has no debugger attached and must not
   * stall in the generated "wait for ST-LINK" loop below, which CubeMX
   * resets to debug=1 on every regeneration. */
  #undef DEBUG
  /* USER CODE END 1 */
  #if defined(DEBUG)
    volatile uint32_t debug = 1;
    while(debug);   // waiting for ST-LINK attachment. User can change "debug" to 0 to continue the execution.
  #endif

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* Unbuffered so every char reaches system_log_buf immediately (a fault mid
   * line still leaves its trail in trace0). */
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("M33: boot, HAL ok\r\n");
  /* USER CODE END Init */

    /* Configure the system clock */
  if(IS_DEVELOPER_BOOT_MODE())
  {
    SystemClock_Config();
  }
  else
  {
   SystemCoreClockUpdate();
  }

  /* USER CODE BEGIN SysInit */
  printf("M33: clock cfg ok (boot mode=%s)\r\n",
         IS_DEVELOPER_BOOT_MODE() ? "developer" : "production");
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C8_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
  /* Mailbox for the RPMsg kicks */
  MX_IPCC_Init();
  printf("M33: ipcc ok\r\n");

  if (!IS_DEVELOPER_BOOT_MODE())
  {
    /* Production boot: firmware loaded by Linux remoteproc — register the
     * shutdown handshake and pick up the clock tree set by the A35. */
    CoproSync_Init();
    SystemCoreClockUpdate();
  }

  /* OpenAMP: M33 is the rpmsg remote, Linux the master */
  printf("M33: openamp init (waiting for host DRIVER_OK)...\r\n");
  {
    int oa = MX_OPENAMP_Init(RPMSG_REMOTE, NULL);
    printf("M33: openamp init done, ret=%d\r\n", oa);
  }

  /* BMP280 + GY-271 sampling, VIRT_UART endpoint (Linux: /dev/ttyRPMSG0) */
  SensorApp_Init(&hi2c8);
  printf("M33: sensor init done, entering main loop\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    OPENAMP_check_for_message();
    SensorApp_Loop();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* Sparse heartbeat: proves the loop is alive without flooding (and
     * wrapping) the small trace buffer. On success the boot lines scroll off
     * and you see these; on a boot failure the loop is never reached, so the
     * boot lines stay put in trace0. */
    {
      static uint32_t last_hb = 0;
      uint32_t now = HAL_GetTick();
      if (now - last_hb >= 5000U)
      {
        last_hb = now;
        printf("M33: alive t=%lums\r\n", (unsigned long)now);
      }
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
}

/**
  * @brief I2C8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C8_Init(void)
{

  /* USER CODE BEGIN I2C8_Init 0 */

  /* USER CODE END I2C8_Init 0 */

  /* USER CODE BEGIN I2C8_Init 1 */

  /* USER CODE END I2C8_Init 1 */
  hi2c8.Instance = I2C8;
  hi2c8.Init.Timing = 0x10707DBC;
  hi2c8.Init.OwnAddress1 = 0;
  hi2c8.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c8.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c8.Init.OwnAddress2 = 0;
  hi2c8.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c8.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c8.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c8) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c8, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c8, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C8_Init 2 */

  /* USER CODE END I2C8_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart6.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart6, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart6, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
  * @brief IPCC Initialization Function
  */
static void MX_IPCC_Init(void)
{
  hipcc1.Instance = IPCC1;
  if (HAL_IPCC_Init(&hipcc1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_NVIC_SetPriority(IPCC1_RX_IRQn, DEFAULT_IRQ_PRIO, 0);
  HAL_NVIC_EnableIRQ(IPCC1_RX_IRQn);
}

/**
  * @brief Linux asked us to shut down (remoteproc stop): release the rpmsg
  *        endpoint and vrings, then acknowledge so the A35 can power us off.
  */
void CoproSync_ShutdownCb(IPCC_HandleTypeDef *hipcc, uint32_t ChannelIndex, IPCC_CHANNELDirTypeDef ChannelDir)
{
  SensorApp_DeInit();
  OPENAMP_DeInit();
  HAL_IPCC_NotifyCPU(hipcc, ChannelIndex, IPCC_CHANNEL_DIR_RX);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  printf("!! Error_Handler: HAL failure (soft error, not a bus fault)\r\n");
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
