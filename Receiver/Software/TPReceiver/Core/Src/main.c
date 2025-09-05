/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cc1101.h"
#include "sensor_map.h"
#include "type_conv.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum {
	PRO_CMD_SENSOR_ID = 				0x01,
	PRO_CMD_SENSOR_TYPE = 				0x02,
	PRO_CMD_STORAGE_SAMPLE = 			0x10,
	PRO_CMD_TRANS_SAMPLE = 				0x11,
	PRO_CMD_TRANS_TX = 					0x12,
	PRO_CMD_STATIC_SAMPLE = 			0x13,
	PRO_CMD_STATIC_TX = 				0x14,
	PRO_CMD_PRESS_THRESH1 = 			0x20,
	PRO_CMD_PRESS_THRESH2 = 			0x21,
	PRO_CMD_PRESS_THRESH_CHANGE = 		0x22,
	PRO_CMD_READ_ALL =					0x50
} pro_cmd_t;

typedef struct {
	uint8_t sensorid;						//this is the default sensorid the current id is stored in eeprom
	uint8_t sensortype;						//0x01 = tyre pressure | 0x02 = brake disk pressure (to be implemented)
	uint32_t storage_time_sample;			//sample rate tx (ms) in transient mode
	uint32_t trans_time_tx;					//sample rate tx (ms) in transient mode
	uint32_t trans_time_sample;				//sample rate abp2 read (ms) in transient mode
	uint32_t static_time_tx;				//sample rate tx (ms) in transient mode
	uint32_t static_time_sample;			//sample rate abp2 read (ms) in transient mode
	float trans_press_thresh1;				//move from storage into transient mode
	float trans_press_thresh2;				//move from transient mode into storage mode
	float trans_press_change;				//minimum pressure change to move to transient mode
	int16_t rssi;
}eeprom_conf_t;



/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define EXPECTED_BYTES_DATA		16	//bytes send (incl. length) +2 (rssi and lqi)
#define EXPECTED_BYTES_EEPROM	38	//eeprom config (incl. length) +2 (rssi and lqi)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim21;

/* USER CODE BEGIN PV */

cc1101_t cc1101;

cc1101_marc_state marcstate = CC1101_MARC_SLEEP;
uint8_t rx_bytes = 0xFF;
uint8_t rx[50] = {0};		//must be big enough to fit all bytes
bool new_data = false;

sensor_data sensor_received;
sensor_map_t sensor_map[SENSORS_MAX];
eeprom_conf_t eeprom_conf = {0};

bool Init_complete = false;

//===========Debug variables==============
uint8_t debug_status_part = 0;
uint8_t debug_status_ver = 0;

uint8_t  debug_reg_all_before[46] = {0};
uint8_t  debug_reg_all_after[46] = {0};

uint8_t tx_fifo[6] = {0x05, PRO_CMD_READ_ALL, 0x01, 0x02, 0x03, 0x04};
uint8_t tx_counter = 0x01;
uint8_t pktstatus = 0x00;
bool tx_send = false;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM21_Init(void);
/* USER CODE BEGIN PFP */

static void ReadRx(void);
static void ParseRx_DataFrame(uint8_t *frame, sensor_data *val);
static void ParseRx_EEPROMFrame(uint8_t *frame, eeprom_conf_t *val);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (Init_complete)
	{
		//CC1101 complete package received
		new_data = true;
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim21)
    {
        tx_send = true;
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USB_DEVICE_Init();
  MX_TIM21_Init();
  /* USER CODE BEGIN 2 */


  // CC1101 Init
  CC1101_Init(&cc1101, &hspi1, SPI1_CS_GPIO_Port, SPI1_CS_Pin);

  // CC1101 Debugging tests
  CC1101_read_status(&cc1101, CC1101_STATUS_PARTNUM, &debug_status_part);
  CC1101_read_status(&cc1101, CC1101_STATUS_VERSION, &debug_status_ver);

  // CC1101 configuration
  CC1101_read_register_burst(&cc1101, CC1101_REG_IOCFG2, debug_reg_all_before, 46);
  CC1101_Apply433_38k4(&cc1101);
  CC1101_read_register_burst(&cc1101, CC1101_REG_IOCFG2, debug_reg_all_after, 46);

  CC1101_Set_Rx_threshold(&cc1101, CC1101_RX_18dB);

  // CC1101 Start Rx
  //CC1101_EnterRx(&cc1101);

  //programming mode test
  //CC1101_PrepareTx(&cc1101);
  CC1101_EnterRx(&cc1101);


  CC1101_Set_Tx_power(&cc1101, CC1101_TX_10dB);
  Init_complete = true;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	//Toggle LED when TX is send or RX is recived

	if (new_data)	//Triggered by GDO IRQ
	{
		ReadRx();
		HAL_GPIO_TogglePin(Debug_LED_GPIO_Port, Debug_LED_Pin);
		new_data = false;
	}

	if (tx_send)	//Triggered by Timer --> move to UDP cmd in the future
	{
		//Prepare TX FIFO and prepare for TX
		tx_fifo[2] = tx_counter;
		tx_counter++;
		CC1101_PrepareTx(&cc1101);

		//Write FIFO and send FIFO
		CC1101_write_TX_FIFO(&cc1101, tx_fifo, sizeof(tx_fifo));
		CC1101_SendTx(&cc1101);

		//Return back to RX so data can be received
		CC1101_EnterRx(&cc1101);
		CC1101_read_marcstate(&cc1101, &marcstate);

		HAL_GPIO_TogglePin(Debug_LED_GPIO_Port, Debug_LED_Pin);
	}

	//HAL_Delay(5);		//Might not be required
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_6;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_1);
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM21 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM21_Init(void)
{

  /* USER CODE BEGIN TIM21_Init 0 */

  /* USER CODE END TIM21_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM21_Init 1 */

  /* USER CODE END TIM21_Init 1 */
  htim21.Instance = TIM21;
  htim21.Init.Prescaler = 32000-1;
  htim21.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim21.Init.Period = 2000-1;
  htim21.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim21.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim21) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim21, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim21) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim21, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim21, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM21_Init 2 */

  /* USER CODE END TIM21_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Debug_LED_GPIO_Port, Debug_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : RX_IRQ_Pin */
  GPIO_InitStruct.Pin = RX_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(RX_IRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI1_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Debug_LED_Pin */
  GPIO_InitStruct.Pin = Debug_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Debug_LED_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

static void ReadRx(void)
{
	//CC1101 RX Status
	CC1101_read_status(&cc1101, CC1101_STATUS_RXBYTES, &rx_bytes);
	CC1101_read_marcstate(&cc1101, &marcstate);
	CC1101_read_status(&cc1101, CC1101_STATUS_PKTSTATUS, &pktstatus);

	if ((rx_bytes == EXPECTED_BYTES_DATA) && (marcstate == CC1101_MARC_RX))
	{
		if (rx_bytes == EXPECTED_BYTES_DATA)
		{
			//read all packets received (only if fifo is not read in between)
			CC1101_read_RX_FIFO(&cc1101, rx, rx_bytes);
			ParseRx_DataFrame(rx, &sensor_received);
			sensors_set_sensor(sensor_map, sensor_received);
		}
		else if (rx_bytes)
		{
			CC1101_read_RX_FIFO(&cc1101, rx, rx_bytes);
			ParseRx_EEPROMFrame(rx, &eeprom_conf);
		}
		else if (marcstate == CC1101_MARC_RXFIFO_OVERFLOW)
		{
			CC1101_Recover_RX_Overflow(&cc1101);		//goning to idle is only allowed via flushing rx fifo
		}
		else
		{
			CC1101_EnterRx(&cc1101);	//flushing fifo in RX state is not possible, so flush in idle state
		}
	}
}

static void ParseRx_DataFrame(uint8_t *frame, sensor_data *val)
{
	//skip byte 1, since frame length is no data
    uint8_t i = 1;
    val->sensor_id = frame[i++];									//get sensorid
    val->sensor_type = frame[i++];									//get sensortype
    val->power = frame[i++];										//get tx power stage
    val->counter = frame[i++];                    					//get alive counter
    val->abp2_status_raw = frame[i++];								//get sensor status

    val->abp2_pressure = Bytearray_to_Float(&frame[i]);				i += 4;		//get pressure as uint32
    val->abp2_temperature = Bytearray_to_Float(&frame[i]);			i += 4;		//get temperature as uint32

    val->rssi = CC1101_Convert_RSSI(frame[i++]);
}

static void ParseRx_EEPROMFrame(uint8_t *frame, eeprom_conf_t *val)
{
	//skip byte 1, since frame length is no data
    uint8_t i = 1;
    val->sensorid = frame[i++];
    val->sensortype = frame[i++];

    val->storage_time_sample = Bytearray_to_UINT32(&frame[i]);		i += 4;
    val->trans_time_tx = Bytearray_to_UINT32(&frame[i]);			i += 4;
    val->trans_time_sample = Bytearray_to_UINT32(&frame[i]);		i += 4;
    val->static_time_tx = Bytearray_to_UINT32(&frame[i]);			i += 4;
    val->static_time_sample = Bytearray_to_UINT32(&frame[i]);		i += 4;

    val->trans_press_thresh1 = Bytearray_to_Float(&frame[i]);		i += 4;
    val->trans_press_thresh2 = Bytearray_to_Float(&frame[i]);		i += 4;
    val->trans_press_change = Bytearray_to_Float(&frame[i]);		i += 4;

    val->rssi = CC1101_Convert_RSSI(frame[i++]);
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
