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
#include "abp2.h"
#include "cc1101.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

//address offset in eeprom
typedef enum {
	EE_ADDR_SENSORID = 		0x00,
	EE_ADDR_Test = 			0x01
}ee_offset;

typedef enum {
	PRO_MODE_OPERATION = 	0x01,
	PRO_MODE_PROGRAMMING = 	0x02
} pro_mode;

typedef enum {
	PRO_CMD_SENSORID = 		0x01,
	PRO_CMD_TEST = 			0x02
} pro_cmd;

typedef enum {
	OP_MODE_STORAGE = 		0x01,
	OP_MODE_TRANSIENT = 	0x02,
	OP_MODE_STATIC =		0x03
} op_mode;

typedef struct {
    float y;            // filtered value
    float alpha;        // smaller value -> more smoothing
} ema;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define ABP2_I2C_ID		0x28
#define SENSOR_TYPE		0x01	//0x01 = tyre pressure | 0x02 = brake disk pressure (to be implemented)
#define EXPECTED_BYTES	5		//length, cmd, uint8 data, lqi, rssi

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

USART_HandleTypeDef husart1;

/* USER CODE BEGIN PV */

//===========Programmable variables========
uint8_t sensorid = 0x10;				//this is the default sensorid the current id is stored in eeprom
float trans_press_thresh1 = 0.97;		//move from storage into transient mode
float trans_press_thresh2 = 0.95;		//move from transient mode into storage mode
float trans_press_change = 0.02;		//minimum pressure change to move to transient mode

//==============Gereral variables==========
pro_mode programming_mode = PRO_MODE_OPERATION;
op_mode operational_mode = OP_MODE_STORAGE;
bool mode_changed = false;
ema ema_pressure = {0, 0.5};

//==============ABP2 Variables=============
abp2_t abp2;
float abp2_pressure = 0.0f;
float abp2_temperature = 0.0f;
abp2_status abp2status;
uint8_t abp2status_raw = 0;


//============CC1101 TX Variables==========
cc1101_t cc1101;
uint8_t tx_fifo[13] = {};
uint8_t tx_counter = 0;
cc1101_marc_state marcstate = CC1101_MARC_SLEEP;
bool Init_complete = false;
uint32_t tx_timer = 0;

//============CC1101 RX variables==========
uint8_t rx_bytes = 0x00;
uint8_t rx[EXPECTED_BYTES] = {0};
bool rx_received = false;

//=============Debug variables=============
uint8_t status_part = 0;
uint8_t status_ver = 0;
uint8_t reg_all_before[46] = {0};
uint8_t reg_all_after[46] = {0};
cc1101_tx_power cc1101_power_ramp[8] = {CC1101_TX_M30dB, CC1101_TX_M20dB, CC1101_TX_M15dB, CC1101_TX_M10dB, CC1101_TX_0dB, CC1101_TX_5dB, CC1101_TX_7dB, CC1101_TX_10dB};
uint8_t sensorid_1 = 0x00;
uint8_t sensorid_2 = 0x00;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_Init(void);
/* USER CODE BEGIN PFP */

static uint8_t EE_ReadU8(ee_offset offset);
static void EE_WriteU8(ee_offset offset, uint8_t val);
static void Write_Programming(pro_cmd cmd, uint8_t data);
static void Read_Programming_CMD(void);
static void Select_Mode(void);
static void Send_Data(void);
static void CreateTx_Frame(uint8_t *frame, uint8_t sensorid, cc1101_tx_power power, uint8_t counter, float pressure, float temperature, uint8_t status_raw);
static void Float_to_Bytearray(uint8_t p[4], float f);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (Init_complete)
	{
		if (GPIO_Pin == RX_TX_SW_IRQ_Pin)
		{
			Select_Mode();
		}

		else if (GPIO_Pin == RX_IRQ_Pin)
		{
			rx_received = true;
		}
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
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USART1_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */


  // ABP2 Init (Address 0x28)
  ABP2_Init(&abp2, &hi2c1, ABP2_I2C_ID);



  // CC1101 Init
  CC1101_Init(&cc1101, &hspi1, SPI1_CS_GPIO_Port, SPI1_CS_Pin);

  // CC1101 Debugging tests
  CC1101_read_status(&cc1101, CC1101_STATUS_PARTNUM, &status_part);
  CC1101_read_status(&cc1101, CC1101_STATUS_VERSION, &status_ver);

  // CC1101 Configuration
  CC1101_read_register_burst(&cc1101, CC1101_REG_IOCFG2, reg_all_before, 46);
  CC1101_Apply433_38k4(&cc1101);
  CC1101_read_register_burst(&cc1101, CC1101_REG_IOCFG2, reg_all_after, 46);

  // CC1101 Prepare for tx
  CC1101_PrepareTx(&cc1101);
  Init_complete = true;

  Select_Mode();		//init programming mode


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	if (mode_changed)		//change rx/tx state depending on current mode (programming vs operational)
	{						//changing the mode directly in IRQ function is not adviced (because CC1101 mode might get stuck)
	    if (HAL_GPIO_ReadPin(RX_TX_SW_IRQ_GPIO_Port, RX_TX_SW_IRQ_Pin) == GPIO_PIN_SET)
	    {
	    	CC1101_EnterRx(&cc1101);
	    }
	    else
	    {
	    	CC1101_PrepareTx(&cc1101);
	    }
	    mode_changed = false;
	}

	//execute tx/rx code
	if (programming_mode == PRO_MODE_OPERATION)			// Operation Mode read sensor data & send data
	{
		if (operational_mode == OP_MODE_STORAGE)
		{
			abp2status = ABP2_ReadAll(&abp2, &abp2_pressure, &abp2_temperature, &abp2status_raw);
			HAL_Delay(200);
		}

		else if (operational_mode == OP_MODE_TRANSIENT)
		{
			abp2status = ABP2_ReadAll(&abp2, &abp2_pressure, &abp2_temperature, &abp2status_raw);
			Send_Data();
		}

		else if (operational_mode == OP_MODE_STATIC)
		{
			abp2status = ABP2_ReadAll(&abp2, &abp2_pressure, &abp2_temperature, &abp2status_raw);
			Send_Data();
		}



	}
	else if ((programming_mode == PRO_MODE_PROGRAMMING) & rx_received)		//
	{
		//receive programming data and write to eeprom and variables
		Read_Programming_CMD();
		rx_received = false;
	}
	//Loop Indication & Delay

	HAL_GPIO_TogglePin(Debug_LED_GPIO_Port, Debug_LED_Pin);

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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_I2C1
                              |RCC_PERIPHCLK_USB;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00B07CB4;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  husart1.Instance = USART1;
  husart1.Init.BaudRate = 115200;
  husart1.Init.WordLength = USART_WORDLENGTH_8B;
  husart1.Init.StopBits = USART_STOPBITS_1;
  husart1.Init.Parity = USART_PARITY_NONE;
  husart1.Init.Mode = USART_MODE_TX_RX;
  husart1.Init.CLKPolarity = USART_POLARITY_LOW;
  husart1.Init.CLKPhase = USART_PHASE_1EDGE;
  husart1.Init.CLKLastBit = USART_LASTBIT_DISABLE;
  if (HAL_USART_Init(&husart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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

  /*Configure GPIO pin : RX_TX_SW_IRQ_Pin */
  GPIO_InitStruct.Pin = RX_TX_SW_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(RX_TX_SW_IRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI1_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Debug_LED_Pin */
  GPIO_InitStruct.Pin = Debug_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Debug_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ABP2_EOC_Pin */
  GPIO_InitStruct.Pin = ABP2_EOC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ABP2_EOC_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

static ema_set_alpha(ema *f, float dt_s, float tau_s)
{
	//set correct alpha for selected filter period
    f->alpha = 1.0f - expf(-dt_s / tau_s);
}

static float ema_push(ema *f, float val) {
    f->y += f->alpha * (val - f->y);
    return f->y;
}

static uint8_t EE_ReadU8(ee_offset offset)
{
	uint32_t addr = (uint32_t)(DATA_EEPROM_BASE + (uint32_t)offset);
    return *(__IO uint8_t*)addr;
}

static void EE_WriteU8(ee_offset offset, uint8_t val)
{
	uint32_t addr = (uint32_t)(DATA_EEPROM_BASE + (uint32_t)offset);
    HAL_FLASHEx_DATAEEPROM_Unlock();
    HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE, addr, val);
    HAL_FLASHEx_DATAEEPROM_Lock();
}

static void Write_Programming(pro_cmd cmd, uint8_t data)
{
	if (cmd == PRO_CMD_SENSORID)
	{
		sensorid = data;
		//EE_WriteU8(EE_ADDR_SENSORID, data);
	}
	else if (cmd == PRO_CMD_TEST)
	{
		//EE_WriteU8(EE_ADDR_Test, data);
	}
}

static void Read_Programming_CMD(void)
{
	//CC1101 RX Status
	CC1101_read_status(&cc1101, CC1101_STATUS_RXBYTES, &rx_bytes);
	CC1101_read_marcstate(&cc1101, &marcstate);


	if ((rx_bytes > 0) & (marcstate == CC1101_MARC_RX))
	{
		if (rx_bytes % EXPECTED_BYTES == 0)
		{
			for (int i = 0; i < (rx_bytes / EXPECTED_BYTES); i++)
			{
				//read all packets received (only if fifo is not read in between)
				CC1101_read_RX_FIFO(&cc1101, rx, rx_bytes);
				Write_Programming(rx[1], rx[2]);
			}
		}
		else if (rx_bytes > EXPECTED_BYTES)
		{
			CC1101_EnterRx(&cc1101);	//flushing fifo in RX state is not possible, so flush in idle state
		}
		else if (marcstate == CC1101_MARC_RXFIFO_OVERFLOW)
		{
			CC1101_Recover_RX_Overflow(&cc1101);		//goning to idle is only allowed via flushing rx fifo
		}
	}
}

static void Select_Mode(void)
{
    if (HAL_GPIO_ReadPin(RX_TX_SW_IRQ_GPIO_Port, RX_TX_SW_IRQ_Pin) == GPIO_PIN_SET)
    {
    	programming_mode = PRO_MODE_PROGRAMMING;
    }
    else
    {
    	programming_mode = PRO_MODE_OPERATION;
    }
		mode_changed = true;		//will be reset to false in main loop due to not changin cc1101 state in between main  methods
}

static void Send_Data(void)		//Variables are defined at start to be used for debugging
{
	//CC1101 Send with all power levels
	for(int i = 0; i < sizeof(cc1101_power_ramp); i++)
	{
		CC1101_Set_Tx_power(&cc1101, cc1101_power_ramp[i]);																		//ramp tx power to test rssi
		CreateTx_Frame(tx_fifo, cc1101_power_ramp[i], sensorid, tx_counter, abp2_pressure, abp2_temperature, abp2status_raw);		//sensorid and power reverse for testing best power
		CC1101_write_TX_FIFO(&cc1101, tx_fifo, sizeof(tx_fifo));
		tx_timer = CC1101_SendTx(&cc1101);																						//send time, depends on data rate
	}
	// Status read out
	tx_counter++;
	CC1101_read_marcstate(&cc1101, &marcstate);
}

static void CreateTx_Frame(uint8_t *frame, uint8_t sensorid, cc1101_tx_power power, uint8_t counter, float pressure, float temperature, uint8_t status_raw)
{
    uint8_t i = 1;
    frame[i++] = sensorid;
    frame[i++] = power;										//add send power
    frame[i++] = counter;                    				//add alive counter
    frame[i++] = status_raw;                   				//add sensor status
    Float_to_Bytearray(&frame[i], pressure);   i += 4;		//add pressure as uint32
    Float_to_Bytearray(&frame[i], temperature);i += 4;		//add temperature as uint32
    frame[0] = i-1;											//add frame length as first byte
}

static void Float_to_Bytearray(uint8_t p[4], float f)
{
    uint32_t u;
    //Float to U32
    memcpy(&u, &f, sizeof u);
    //U32 to Byte Array
    p[0] = (uint8_t)(u >> 24);
    p[1] = (uint8_t)(u >> 16);
    p[2] = (uint8_t)(u >>  8);
    p[3] = (uint8_t)(u >>  0);
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
