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
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum { PH_FLOAT, PH_LOW, PH_PWM } PhaseRole;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* Steps to lead the rotor by; this also sets the direction of rotation.
   Try 1 or 2 for one direction, 4 or 5 for the other. 0 and 3 won't spin. */
#define COMMUTATION_OFFSET 1
/* PWM compare used while running, out of Period = 5312 (~9.4%). Start gentle. */
#define RUN_DUTY 500
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */
/* ############# PRIVATE VARIABLES ################# */
volatile uint8_t hall_state = 0; // Stores the state of the hall sensors, volatile because it can be changed outside of the main program flow
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);

/* USER CODE BEGIN PFP */
static void set_phase(uint32_t channel, PhaseRole role, uint16_t duty);
void drive_step(uint8_t step, uint16_t duty);
static uint8_t read_hall_state(void);
static void commutate(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* Retarget printf() to the ARM semihosting channel so output shows up in the
   OpenOCD/GDB console. Overrides the weak _write() in syscalls.c.
   NOTE: this requires a debugger attached with semihosting enabled
   ("monitor arm semihosting enable" in OpenOCD). Running the board
   stand-alone (no debugger) will hang on the first printf. */
int _write(int file, char *ptr, int len)
{
  (void)file;
  for (int i = 0; i < len; i++)
  {
    char c = ptr[i];
    register int r0 asm("r0") = 0x03; /* SYS_WRITEC: write one char */
    register char *r1 asm("r1") = &c;
    asm volatile("bkpt 0xAB" : : "r"(r0), "r"(r1) : "memory");
  }
  return len;
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
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  setvbuf(stdout, NULL, _IONBF, 0); /* unbuffered: chars come out immediately */
  printf("Hall debug ready - spin the motor by hand...\r\n");

  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */



    for (uint8_t step = 0; step < 6; step++)
  {
      drive_step(step, 400);        /* ~7.5% duty: enough to align, gentle */
      HAL_Delay(1500);              /* let the rotor settle */

      uint8_t h1 = HAL_GPIO_ReadPin(HALL_1_GPIO_Port, HALL_1_Pin);
      uint8_t h2 = HAL_GPIO_ReadPin(HALL_2_GPIO_Port, HALL_2_Pin);
      uint8_t h3 = HAL_GPIO_ReadPin(HALL_3_GPIO_Port, HALL_3_Pin);
      hall_state = (h3 << 2) | (h2 << 1) | h1;

      printf("step %u -> hall_state %u\r\n", step, hall_state);
  }
  drive_step(0xFF, 0);              /* hits your default: case -> all float */
  printf("calibration done\r\n");
  while (1) { HAL_Delay(100); }     /* park here */


  HAL_Delay(1);

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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED1;
  htim1.Init.Period = 5312;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 1;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 127;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pins : HALL_1_Pin HALL_2_Pin HALL_3_Pin */
  GPIO_InitStruct.Pin = HALL_1_Pin|HALL_2_Pin|HALL_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

static const uint8_t step_for_hall[8] = {
  [6] = 0,
  [4] = 1,
  [5] = 2,
  [1] = 3,
  [3] = 4,
  [2] = 5,
  [0] = 0xFF,
  [7] = 0xFF
};
static uint8_t read_hall_state(void){
    uint8_t h1 = HAL_GPIO_ReadPin(HALL_1_GPIO_Port, HALL_1_Pin);
    uint8_t h2 = HAL_GPIO_ReadPin(HALL_2_GPIO_Port, HALL_2_Pin);
    uint8_t h3 = HAL_GPIO_ReadPin(HALL_3_GPIO_Port, HALL_3_Pin);
    return (h3 << 2) | (h2 << 1) | h1;
}
static void set_phase(uint32_t channel, PhaseRole role, uint16_t duty)
{
  uint32_t ccer_enable_bits;
  uint16_t compare_value;

  switch (channel)
  {
  case TIM_CHANNEL_1:
    ccer_enable_bits = TIM_CCER_CC1E | TIM_CCER_CC1NE;
    break;

  case TIM_CHANNEL_2:
  ccer_enable_bits = TIM_CCER_CC2E | TIM_CCER_CC2NE;
    break;

  case TIM_CHANNEL_3:
  ccer_enable_bits = TIM_CCER_CC3E | TIM_CCER_CC3NE;
    break;

  default:
    return;
  }


  if (role == PH_FLOAT){
    TIM1->CCER &= ~ccer_enable_bits;
    return;
  }


  if (role == PH_PWM)
  {
      compare_value = duty;   /* PWM role: high side switches at the requested duty */
  }
  else
  {
      compare_value = 0;      /* must be PH_LOW (returned early for PH_FLOAT),
                                 and compare = 0 keeps the low side on all period */
  }


  TIM1->CCER |= ccer_enable_bits; // ORs CCER with our bits that we want to enable for whatever phase we are in
  __HAL_TIM_SET_COMPARE(&htim1, channel, compare_value);

}

/*
 *   step | PWM | LOW | FLOAT
 *    0   |  A  |  B  |  C
 *    1   |  A  |  C  |  B
 *    2   |  B  |  C  |  A
 *    3   |  B  |  A  |  C
 *    4   |  C  |  A  |  B
 *    5   |  C  |  B  |  A
 */
void drive_step(uint8_t step, uint16_t duty)
{
  switch (step)
      {
      case 0: /* PWM=A, LOW=B, FLOAT=C */
        set_phase(TIM_CHANNEL_1, PH_PWM,   duty);
        set_phase(TIM_CHANNEL_2, PH_LOW,   duty);
        set_phase(TIM_CHANNEL_3, PH_FLOAT, duty);
        break;

      case 1: /* PWM=A, LOW=C, FLOAT=B */
        set_phase(TIM_CHANNEL_1, PH_PWM,   duty);
        set_phase(TIM_CHANNEL_2, PH_FLOAT, duty);
        set_phase(TIM_CHANNEL_3, PH_LOW,   duty);
        break;

      case 2:
        set_phase(TIM_CHANNEL_1, PH_FLOAT, duty);
        set_phase(TIM_CHANNEL_2, PH_PWM, duty);
        set_phase(TIM_CHANNEL_3, PH_LOW, duty);
        break;

      case 3:
        set_phase(TIM_CHANNEL_1, PH_LOW, duty);
        set_phase(TIM_CHANNEL_2, PH_PWM, duty);
        set_phase(TIM_CHANNEL_3, PH_FLOAT, duty);
        break;

      case 4:
        set_phase(TIM_CHANNEL_1, PH_LOW, duty);
        set_phase(TIM_CHANNEL_2, PH_FLOAT, duty);
        set_phase(TIM_CHANNEL_3, PH_PWM, duty);
        break;

      case 5:
        set_phase(TIM_CHANNEL_1, PH_FLOAT, duty);
        set_phase(TIM_CHANNEL_2, PH_LOW, duty);
        set_phase(TIM_CHANNEL_3, PH_PWM, duty);
        break;

      default: /* invalid step: safest thing is everything off */
          set_phase(TIM_CHANNEL_1, PH_FLOAT, 0);
          set_phase(TIM_CHANNEL_2, PH_FLOAT, 0);
          set_phase(TIM_CHANNEL_3, PH_FLOAT, 0);
          break;
      }
  }
    

static void commutate(void){
  uint8_t state = read_hall_state();
  uint8_t hold = step_for_hall[state];
  uint8_t step;

  if (hold == 0xFF){
    drive_step(0xFF, 0);
    return;
  }

  step = (hold + COMMUTATION_OFFSET) % 6;

  drive_step(step, RUN_DUTY);

  hall_state = state;

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
