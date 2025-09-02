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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "math.h"
#include "abp2.h"
#include "cc1101.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

//address offset in eeprom
typedef enum {
	// U8 Values
	EE_ADDR_SENSOR_ID = 			0x00,
	EE_ADDR_SENSOR_TYPE = 			0x01,
	// U32 Values
	EE_ADDR_STORAGE_SAMPLE =		0x10,
	EE_ADDR_TRANS_SAMPLE =			0x14,
	EE_ADDR_TRANS_TX =				0x18,
	EE_ADDR_STATIC_SAMPLE =			0x1B,
	EE_ADDR_STATIC_TX =				0x1F,
	// float
	EE_ADDR_THRESH1 =				0x30,
	EE_ADDR_THRESH2 =				0x34,
	EE_ADDR_CHANGE =				0x38
}ee_offset_t;

typedef enum {
	PRO_MODE_OPERATION = 		0x01,
	PRO_MODE_PROGRAMMING = 		0x02
} pro_mode_t;

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
	PRO_CMD_PRESS_TRESH_CHANGE = 		0x22
} pro_cmd_t;


typedef enum {
	OP_MODE_STORAGE = 			0x01,
	OP_MODE_TRANSIENT = 		0x02,
	OP_MODE_STATIC =			0x03
} op_mode_t;

typedef struct {
    float y;            // filtered value
    float alpha;        // smaller value -> more smoothing
} ema_t;


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define ABP2_I2C_ID		0x28
#define EXPECTED_BYTES	5		// (cmd string) length, cmd, uint8 data, lqi, rssi

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim21;
TIM_HandleTypeDef htim22;

/* USER CODE BEGIN PV */

//===========Programmable variables========
uint8_t sensorid = 0x10;				//this is the default sensorid the current id is stored in eeprom
uint8_t sensortype = 0x01;				//0x01 = tyre pressure | 0x02 = brake disk pressure (to be implemented)
float trans_press_thresh1 = 1.1;		//move from storage into transient mode
float trans_press_thresh2 = 1.0;		//move from transient mode into storage mode
float trans_press_change = 0.0;			//minimum pressure change to move to transient mode
uint32_t storage_time_sample = 5000;	//sample rate tx (ms) in transient mode
uint32_t trans_time_tx = 1000;			//sample rate tx (ms) in transient mode
uint32_t trans_time_sample = 100;		//sample rate abp2 read (ms) in transient mode
uint32_t static_time_tx = 10000;		//sample rate tx (ms) in transient mode
uint32_t static_time_sample = 100;		//sample rate abp2 read (ms) in transient mode

//==============Gereral variables==========
pro_mode_t programming_mode = PRO_MODE_OPERATION;
op_mode_t operational_mode = OP_MODE_TRANSIENT;
op_mode_t operational_mode_old = OP_MODE_TRANSIENT;
bool pro_mode_changed = false;
bool op_mode_changed = false;
ema_t ema_pressure = {0, 0.5};
ema_t ema_temperature = {20, 0.5};

//==============ABP2 Variables=============
abp2_t abp2;
float abp2_pressure = 0.0f;
float abp2_pressure_change = 0.0f;
float abp2_temperature = 0.0f;
abp2_status abp2status;
uint8_t abp2status_raw = 0;
bool abp2_sample_ready = false;
uint32_t abp2_sample_timer = 100;


//============CC1101 TX Variables==========
cc1101_t cc1101;
uint8_t tx_fifo[13] = {};
uint8_t tx_counter = 0;
cc1101_marc_state marcstate = CC1101_MARC_SLEEP;
bool Init_complete = false;
uint32_t tx_duration = 0;
bool tx_ready = false;
uint32_t tx_timer = 1000;


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

uint32_t debug_timer_tick = 0;
uint32_t debug_time_ms = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_RTC_Init(void);
static void MX_TIM21_Init(void);
static void MX_TIM22_Init(void);
/* USER CODE BEGIN PFP */

static uint32_t EE_ADDR(uint32_t off);
static uint8_t  EE_ReadU8 (uint32_t off);
static uint32_t EE_ReadU32(uint32_t off);
static float EE_ReadF32(uint32_t off);
static void EE_WriteU8(uint32_t off, uint8_t v);
static void EE_WriteU32(uint32_t off, uint32_t v);
static void EE_WriteF32(uint32_t off, float f);
static void Settings_LoadOrInit(bool firstcall);
static void Write_Programming(pro_cmd_t cmd, uint8_t data);

static float ema_push(ema_t *f, float val);
static void ema_set_alpha(ema_t *f, float dt_s, float tau_s);

static void SetTimer(TIM_HandleTypeDef *htim, uint32_t timer_ms);
static void DeepSleep(uint32_t timer_s);

static void Send_Data(float pressure, float temperature);
static void Read_Programming_CMD(void);

static void Select_Mode(void);
static void CreateTx_DataFrame(uint8_t *frame, uint8_t sensorid, uint8_t sensor_type, cc1101_tx_power power, uint8_t counter, float pressure, float temperature, uint8_t status_raw);
static void CreateTx_EEPROMFrame(uint8_t *frame);

static void Float_to_Bytearray(uint8_t p[4], float f);
static void UINT32_to_Bytearray(uint8_t out[4], uint32_t u);
static uint32_t Float_to_UINT32(float f);

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
			//rx_received = true;
		}
	}
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
	abp2_sample_ready = true;      // nur Flag setzen
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim21) {
        abp2_sample_ready = true;      // nur Flag setzen
    }
    else if (htim == &htim22)
    {
    	tx_ready = true;
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
  MX_RTC_Init();
  MX_TIM21_Init();
  MX_TIM22_Init();
  /* USER CODE BEGIN 2 */

  Settings_LoadOrInit(true);		//true write first time to eeprom, flase read from eeprom

  //Start Timers
  HAL_TIM_Base_Start_IT(&htim21);
  HAL_TIM_Base_Start_IT(&htim22);

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
	//==================================Programming Mode========================================
	if (pro_mode_changed)	//change rx/tx state depending on current mode (programming vs operational)
	{						//changing the mode directly in IRQ function is not adviced (because CC1101 mode might get stuck)
	    if (HAL_GPIO_ReadPin(RX_TX_SW_IRQ_GPIO_Port, RX_TX_SW_IRQ_Pin) == GPIO_PIN_SET)
	    {
	    	CC1101_EnterRx(&cc1101);
	    }
	    else
	    {
	    	CC1101_PrepareTx(&cc1101);
	    }
	    pro_mode_changed = false;
	}


	if (programming_mode == PRO_MODE_OPERATION)
	{


		//=================================Operating Mode============================================
		op_mode_changed = (operational_mode_old != operational_mode);
		operational_mode_old = operational_mode;
		abp2_pressure_change = 0;//fabs(abp2_pressure - ema_pressure.y);

		if (op_mode_changed)		// Only execute once if mode changes
		{
			switch (operational_mode)
			{
			case OP_MODE_STORAGE:
				ema_set_alpha(&ema_pressure, storage_time_sample, storage_time_sample);		//set alpha
				break;

			case OP_MODE_TRANSIENT:
				ema_set_alpha(&ema_pressure, trans_time_sample, trans_time_tx);				//set alpha
				break;

			case OP_MODE_STATIC:
				ema_set_alpha(&ema_pressure, static_time_sample, static_time_tx);			//set alpha
				break;
			}
		}


		switch (operational_mode)		// Execute perodically (induced by timer set above)
		{
		case OP_MODE_STORAGE:			// Operation Mode read sensor data & send data

			if (true)		// abp2_sample_ready
			{
				abp2status = ABP2_ReadAll(&abp2, &abp2_pressure, &abp2_temperature, &abp2status_raw);
				ema_push(&ema_pressure, abp2_pressure);
				//DeepSleep(5);
				HAL_Delay(2000);	//Remove with stop mode, and wake up with
				debug_timer_tick = __HAL_TIM_GET_COUNTER(&htim21);
			}

			// Condition change
			if (abp2_pressure > trans_press_thresh1)
			{
				if (abp2_pressure_change > trans_press_change)
				{
					operational_mode = OP_MODE_TRANSIENT;
				}
				else
				{
					operational_mode = OP_MODE_STATIC;
				}
			}
			break;

		case OP_MODE_TRANSIENT:

			if (abp2_sample_ready)
			{
				abp2status = ABP2_ReadAll(&abp2, &abp2_pressure, &abp2_temperature, &abp2status_raw);
				ema_push(&ema_pressure, abp2_pressure);
				abp2_sample_ready = false;
				//start new timer
			}

			if (tx_ready)
			{
				CC1101_PrepareTx(&cc1101);
				Send_Data(ema_pressure.y, ema_temperature.y);
				CC1101_EnterSleep(&cc1101);
				tx_ready = false;
			}

			//Condition change
			if (abp2_pressure_change > trans_press_change)
			{
				operational_mode = OP_MODE_TRANSIENT;
			}
			else if (abp2_pressure < trans_press_thresh2)
			{
				operational_mode = OP_MODE_STORAGE;
			}
			break;


		case OP_MODE_STATIC:

			if (abp2_sample_ready)
			{
				abp2status = ABP2_ReadAll(&abp2, &abp2_pressure, &abp2_temperature, &abp2status_raw);
				abp2_sample_ready = false;
			}

			if (tx_ready)
			{
				Send_Data(ema_pressure.y, ema_temperature.y);
				tx_ready = false;
			}

			//Condition change
			if (abp2_pressure_change > trans_press_change)
			{
				operational_mode = OP_MODE_TRANSIENT;
			}
			else if (abp2_pressure < trans_press_thresh2)
			{
				operational_mode = OP_MODE_STORAGE;
			}
			break;
		}
	}

	else if ((programming_mode == PRO_MODE_PROGRAMMING) & rx_received)		//
	{
		//receive programming data and write to eeprom and variables
		Read_Programming_CMD();
		rx_received = false;
	}
	//Loop Indication & Delay
	HAL_Delay(200);
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_RTC;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_1);
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
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 288;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the WakeUp
  */
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

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
  htim21.Init.Period = 1000-1;
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
  * @brief TIM22 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM22_Init(void)
{

  /* USER CODE BEGIN TIM22_Init 0 */

  /* USER CODE END TIM22_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM22_Init 1 */

  /* USER CODE END TIM22_Init 1 */
  htim22.Instance = TIM22;
  htim22.Init.Prescaler = 32000-1;
  htim22.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim22.Init.Period = 1000-1;
  htim22.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim22.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim22) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim22, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim22) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim22, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim22, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM22_Init 2 */

  /* USER CODE END TIM22_Init 2 */

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

//=============================== EEPROM ==================================

static uint32_t EE_ADDR(uint32_t off)
{
	return (0x08080000UL + off);
}

// --- READ ---
static uint8_t  EE_ReadU8 (uint32_t off)
{
	return *(__IO uint8_t*) EE_ADDR(off);
}

static uint32_t EE_ReadU32(uint32_t off)
{
	return *(__IO uint32_t*)EE_ADDR(off);
}

static float EE_ReadF32(uint32_t off)
{
    uint32_t u = EE_ReadU32(off);
    float f;
    memcpy(&f, &u, sizeof f);     // Bitwise
    return f;
}

// --- WRITE ---
static void EE_WriteU8(uint32_t off, uint8_t v)
{
    if (EE_ReadU8(off) == v) return;
    HAL_FLASHEx_DATAEEPROM_Unlock();
    HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE, EE_ADDR(off), v);
    HAL_FLASHEx_DATAEEPROM_Lock();
}

static void EE_WriteU32(uint32_t off, uint32_t v)
{
    if (EE_ReadU32(off) == v) return;
    HAL_FLASHEx_DATAEEPROM_Unlock();
    HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, EE_ADDR(off), v);
    HAL_FLASHEx_DATAEEPROM_Lock();
}

static void EE_WriteF32(uint32_t off, float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof u);     // Float-Bits unverändert
    EE_WriteU32(off, u);
}

static void Settings_LoadOrInit(bool firstcall)
{
    if (firstcall)
    {
        EE_WriteU8(EE_ADDR_SENSOR_ID, sensorid);
        EE_WriteU8(EE_ADDR_SENSOR_TYPE, sensortype);

        EE_WriteU32(EE_ADDR_STORAGE_SAMPLE, storage_time_sample);
        EE_WriteU32(EE_ADDR_TRANS_TX, trans_time_tx);
        EE_WriteU32(EE_ADDR_TRANS_SAMPLE, trans_time_sample);
        EE_WriteU32(EE_ADDR_STATIC_TX, static_time_tx);
        EE_WriteU32(EE_ADDR_STATIC_SAMPLE, static_time_sample);

        EE_WriteF32(EE_ADDR_THRESH1, trans_press_thresh1);
        EE_WriteF32(EE_ADDR_THRESH2, trans_press_thresh2);
        EE_WriteF32(EE_ADDR_CHANGE, trans_press_change);
    }
    else
    {
        sensorid            = EE_ReadU8(EE_ADDR_SENSOR_ID);
        sensortype          = EE_ReadU8(EE_ADDR_SENSOR_TYPE);

        storage_time_sample = EE_ReadU32(EE_ADDR_STORAGE_SAMPLE);
        trans_time_tx       = EE_ReadU32(EE_ADDR_TRANS_TX);
        trans_time_sample   = EE_ReadU32(EE_ADDR_TRANS_SAMPLE);
        static_time_tx      = EE_ReadU32(EE_ADDR_STATIC_TX);
        static_time_sample  = EE_ReadU32(EE_ADDR_STATIC_SAMPLE);

        trans_press_thresh1 = EE_ReadF32(EE_ADDR_THRESH1);
        trans_press_thresh2 = EE_ReadF32(EE_ADDR_THRESH2);
        trans_press_change  = EE_ReadF32(EE_ADDR_CHANGE);
    }
}

static void Write_Programming(pro_cmd_t cmd, uint8_t data)
{
	switch (cmd)
	{
	case PRO_CMD_SENSOR_ID:
		sensorid = data;
		//EE_WriteU8(EE_ADDR_SENSORID, data);

	case PRO_CMD_SENSOR_TYPE:
		sensortype = data;
		//EE_WriteU8(EE_ADDR_..., data);
	}
}

//================================= EMA ====================================

static void ema_set_alpha(ema_t *f, float dt_s, float tau_s)
{
	//set correct alpha for selected filter period
    f->alpha = 1.0f - expf(-dt_s / tau_s);
}

static float ema_push(ema_t *f, float val) {
    f->y += f->alpha * (val - f->y);
    return f->y;
}


//================================= TIMER ====================================

static void SetTimer(TIM_HandleTypeDef *htim, uint32_t timer_ms)
{
	timer_ms = timer_ms - 1;


    __HAL_TIM_DISABLE(htim);                       				// Stop Timer
    __HAL_TIM_SET_AUTORELOAD(htim, timer_ms);          			// Set Timer period
    __HAL_TIM_SET_COUNTER(htim, 0);               				// zero counter
    __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);  				// remove update flag
    HAL_TIM_GenerateEvent(htim, TIM_EVENTSOURCE_UPDATE); 		// latch psc
    __HAL_TIM_ENABLE(htim);                        				// Restart Timer
}

static void DeepSleep(uint32_t timer_s)
{
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, timer_s, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);
    HAL_SuspendTick();

    HAL_PWREx_EnableUltraLowPower();
    HAL_PWREx_EnableFastWakeUp();
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    SystemClock_Config();
    HAL_ResumeTick();
}

//================================= TX ====================================

static void Send_Data(float pressure, float temperature)		//Variables are defined at start to be used for debugging
{
	//CC1101 Send with all power levels
	for(int i = 0; i < sizeof(cc1101_power_ramp); i++)
	{
		CC1101_Set_Tx_power(&cc1101, cc1101_power_ramp[i]);																						//ramp tx power to test rssi
		CreateTx_DataFrame(tx_fifo, cc1101_power_ramp[i], sensortype, sensorid, tx_counter, pressure, temperature, abp2status_raw);					//sensorid and power reverse for testing best power
		CC1101_write_TX_FIFO(&cc1101, tx_fifo, sizeof(tx_fifo));
		tx_duration = CC1101_SendTx(&cc1101);																									//send time, depends on data rate
	}
	// Status read out
	tx_counter++;
	CC1101_read_marcstate(&cc1101, &marcstate);
}
//================================= RX ====================================

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

//================================= IRQ ====================================

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
		pro_mode_changed = true;		//will be reset to false in main loop due to not changin cc1101 state in between main  methods
}


//================================= FIFO ====================================

static void CreateTx_DataFrame(uint8_t *frame, uint8_t sensorid, uint8_t sensor_type, cc1101_tx_power power, uint8_t counter, float pressure, float temperature, uint8_t status_raw)
{
    uint8_t i = 1;

    frame[i++] = sensorid;
    frame[i++] = sensor_type;
    frame[i++] = power;										//add send power
    frame[i++] = counter;                    				//add alive counter
    frame[i++] = status_raw;  								//add sensor status

    Float_to_Bytearray(&frame[i], pressure);   i += 4;		//add pressure as uint32
    Float_to_Bytearray(&frame[i], temperature);i += 4;		//add temperature as uint32

    frame[0] = i-1;											//add frame length as first byte
}

static void CreateTx_EEPROMFrame(uint8_t *frame)
{
    uint8_t i = 1;

    frame[i++] = sensorid;
    frame[i++] = sensortype;

    UINT32_to_Bytearray(&frame[i], storage_time_sample);		i += 4;
    UINT32_to_Bytearray(&frame[i], trans_time_tx);				i += 4;
    UINT32_to_Bytearray(&frame[i], trans_time_sample);			i += 4;
    UINT32_to_Bytearray(&frame[i], static_time_tx);				i += 4;
    UINT32_to_Bytearray(&frame[i], static_time_sample);			i += 4;


    Float_to_Bytearray(&frame[i], trans_press_thresh1);   		i += 4;
    Float_to_Bytearray(&frame[i], trans_press_thresh2);			i += 4;
    Float_to_Bytearray(&frame[i], trans_press_change);			i += 4;

    frame[0] = i-1;											//add frame length as first byte
}


//============================== DATA CONVERSION =================================

static void Float_to_Bytearray(uint8_t p[4], float f)
{
    uint32_t u = Float_to_UINT32(f);
}

static void UINT32_to_Bytearray(uint8_t out[4], uint32_t u)
{
    out[0] = (uint8_t)(u >> 24);
    out[1] = (uint8_t)(u >> 16);
    out[2] = (uint8_t)(u >>  8);
    out[3] = (uint8_t)(u >>  0);
}

static uint32_t Float_to_UINT32(float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof u);   // Bitwise
    return u;
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
