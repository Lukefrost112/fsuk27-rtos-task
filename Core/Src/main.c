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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "event_groups.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

// Enum to store where the data came from (shared, counter)
typedef enum { SRC_SHARED, SRC_COUNTER } source_t;

// a struct holding one uart message (where it came from and the value of thein
// message)
typedef struct {
  source_t source;
  uint8_t value;
} uart_msg_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// TODO comment: what event this bit means and why bit 0
#define BUTTON_PRESSED_BIT (1 << 0)
// TODO comment: why 50 ticks (bounce ~1-10 ms vs a real human press)
#define DEBOUNCE_TICKS 70

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for IncreaseVar */
osThreadId_t IncreaseVarHandle;
const osThreadAttr_t IncreaseVar_attributes = {
    .name = "IncreaseVar",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for DecreaseVar */
osThreadId_t DecreaseVarHandle;
const osThreadAttr_t DecreaseVar_attributes = {
    .name = "DecreaseVar",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for CalcSwitch1 */
osThreadId_t CalcSwitch1Handle;
const osThreadAttr_t CalcSwitch1_attributes = {
    .name = "CalcSwitch1",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for UARTData */
osMessageQueueId_t UARTDataHandle;
const osMessageQueueAttr_t UARTData_attributes = {.name = "UARTData"};
/* Definitions for sharedMutex */
osMutexId_t sharedMutexHandle;
const osMutexAttr_t sharedMutex_attributes = {.name = "sharedMutex"};
/* USER CODE BEGIN PV */

// Global protected "shared variable"
// TODO comment: range 100-200; who changes it; protected by sharedMutex
uint8_t shared = 100;

/* USER CODE END PV */

EventGroupHandle_t buttonEvents = NULL;
// TODO comment: why volatile (changed in the ISR, read in a task)
volatile uint8_t counter = 0;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void *argument);
void StartIncreaseVar(void *argument);
void StartDecreaseVar(void *argument);
void StartCalcSwitch1(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of sharedMutex */
  sharedMutexHandle = osMutexNew(&sharedMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of UARTData */
  UARTDataHandle =
      osMessageQueueNew(5, sizeof(uart_msg_t), &UARTData_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle =
      osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of IncreaseVar */
  IncreaseVarHandle =
      osThreadNew(StartIncreaseVar, NULL, &IncreaseVar_attributes);

  /* creation of DecreaseVar */
  DecreaseVarHandle =
      osThreadNew(StartDecreaseVar, NULL, &DecreaseVar_attributes);

  /* creation of CalcSwitch1 */
  CalcSwitch1Handle =
      osThreadNew(StartCalcSwitch1, NULL, &CalcSwitch1_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  // TODO comment: what this creates [REF: freertos.org page for
  // xEventGroupCreate] + why the NULL check
  buttonEvents = xEventGroupCreate();
  if (buttonEvents == NULL)
    Error_Handler();
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
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
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : Button_Pin */
  GPIO_InitStruct.Pin = Button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Button_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE; // FreeRTOS fills this in
  // TODO comment: what static does here and why lastPress needs it
  static TickType_t lastPress = 0;
  TickType_t now = xTaskGetTickCountFromISR();

  if (GPIO_Pin == Button_Pin && buttonEvents != NULL) {

    // TODO comment: why 'now - lastPress' survives the tick wrap
    if (now - lastPress >= DEBOUNCE_TICKS) {
      counter++;

      // TODO comment: your interpretation of 'reset when it reaches 9' (values
      // 0..8)
      if (counter >= 9) {
        counter = 0;
      }

      lastPress = now;

      // TODO comment: why the FromISR version + what xHigherPriorityTaskWoken
      // is [REF: freertos.org xEventGroupSetBitsFromISR]
      xEventGroupSetBitsFromISR(buttonEvents, BUTTON_PRESSED_BIT,
                                &xHigherPriorityTaskWoken);
    }
  }

  // TODO comment: what this does and why it's last [REF: freertos.org
  // portYIELD_FROM_ISR]
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument) {
  /* USER CODE BEGIN 5 */

  // TODO comment: UART task: runs every 5 ticks, start of the period [REF:
  // CMSIS-RTOS2 osKernelGetTickCount]
  uint32_t nextWakeTick = osKernelGetTickCount();

  /* Infinite loop */
  for (;;) {
    nextWakeTick += 5;
    uart_msg_t msg;

    char buf[32];

    // TODO comment: drain everything waiting; why timeout 0 [REF: CMSIS-RTOS2
    // osMessageQueueGet]
    while (osMessageQueueGet(UARTDataHandle, &msg, NULL, 0) == osOK) {
      int len = 0;
      // TODO comment: label chosen from msg.source
      switch (msg.source) {
      case SRC_SHARED:
        // TODO comment: format label + value into buf; why snprintf not sprintf
        len = snprintf(buf, sizeof(buf), "Shared: %u\r\n", msg.value);
        break;
      case SRC_COUNTER:
        len = snprintf(buf, sizeof(buf), "Counter: %u\r\n", msg.value);
        break;
      }
      // TODO comment: send the text [REF: UM1850 page number for
      // HAL_UART_Transmit]
      HAL_UART_Transmit(&huart1, (const uint8_t *)buf, len, HAL_MAX_DELAY);
    }

    // TODO comment: why osDelayUntil instead of osDelay [REF: CMSIS-RTOS2
    // osDelayUntil]
    osDelayUntil(nextWakeTick);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartIncreaseVar */
/**
 * @brief Function implementing the IncreaseVar thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartIncreaseVar */
void StartIncreaseVar(void *argument) {
  /* USER CODE BEGIN StartIncreaseVar */

  // TODO comment: IncreaseVar: every 10 ticks, next target built from the
  // previous one
  uint32_t timeToNextWakeup = osKernelGetTickCount();

  /* Infinite loop */
  for (;;) {
    timeToNextWakeup += 10;

    // TODO comment: why the mutex; why waiting forever is safe here [REF:
    // CMSIS-RTOS2 osMutexAcquire]
    osMutexAcquire(sharedMutexHandle, osWaitForever);

    shared++;

    // TODO comment: wrap rule from the spec
    if (shared > 200) {
      shared = 100;
    }

    uart_msg_t msg;
    msg.source = SRC_SHARED;
    msg.value = shared;

    // TODO comment: release BEFORE sending to the queue (why) [REF: CMSIS-RTOS2
    // osMutexRelease]
    osMutexRelease(sharedMutexHandle);

    // TODO comment: why timeout 5 and not forever [REF: CMSIS-RTOS2
    // osMessageQueuePut]
    osMessageQueuePut(UARTDataHandle, &msg, 0, 5);
    osDelayUntil(timeToNextWakeup);
  }
  /* USER CODE END StartIncreaseVar */
}

/* USER CODE BEGIN Header_StartDecreaseVar */
/**
 * @brief Function implementing the DecreaseVar thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDecreaseVar */
void StartDecreaseVar(void *argument) {
  /* USER CODE BEGIN StartDecreaseVar */

  // TODO comment: DecreaseVar: every 12 ticks
  uint32_t timeToNextWakeup = osKernelGetTickCount();

  /* Infinite loop */
  for (;;) {
    timeToNextWakeup += 12;

    // TODO comment: same mutex as IncreaseVar
    osMutexAcquire(sharedMutexHandle, osWaitForever);
    shared--;

    // TODO comment: wrap rule; why < and not <=
    if (shared < 100) {
      shared = 200;
    }

    uart_msg_t msg;
    msg.source = SRC_SHARED;
    msg.value = shared;

    // TODO comment: release before sending
    osMutexRelease(sharedMutexHandle);

    osMessageQueuePut(UARTDataHandle, &msg, 0, 5);
    osDelayUntil(timeToNextWakeup);
  }
  /* USER CODE END StartDecreaseVar */
}

/* USER CODE BEGIN Header_StartCalcSwitch1 */
/**
 * @brief Function implementing the CalcSwitch1 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartCalcSwitch1 */
void StartCalcSwitch1(void *argument) {
  /* USER CODE BEGIN StartCalcSwitch1 */
  /* Infinite loop */
  for (;;) {
    // TODO comment: wait up to 2 ticks for the button bit; your choices for
    // each argument [REF: freertos.org xEventGroupWaitBits]
    EventBits_t bits = xEventGroupWaitBits(buttonEvents, BUTTON_PRESSED_BIT,
                                           pdTRUE, pdFALSE, 2);
    if ((bits & BUTTON_PRESSED_BIT) == BUTTON_PRESSED_BIT) {
      // TODO comment: why read counter once into a copy
      uint8_t counterCopy = counter;

      counterCopy *= 10;

      uart_msg_t msg;
      msg.source = SRC_COUNTER;
      msg.value = counterCopy;

      osMessageQueuePut(UARTDataHandle, &msg, 0, 5);
    }
  }
  /* USER CODE END StartCalcSwitch1 */
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM4 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
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
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
