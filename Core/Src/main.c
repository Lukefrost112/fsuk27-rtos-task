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
#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

// Enum to store where the data came from (shared, counter)
typedef enum { SRC_SHARED, SRC_COUNTER } source_t;

// a struct holding one uart message (where it came from and the value of the
// message)
typedef struct {
  source_t source;
  uint8_t value;
} uart_msg_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// Button press event bit
#define BUTTON_PRESSED_BIT (1 << 0)

// Minimum ticks between button presses
// 50 because the avg time between bounces is ~10ms and between fast human
// presses is ~100ms So it's a good in-between
#define DEBOUNCE_TICKS 50

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

// Global shared variable: changed by IncreaseVar and DecreaseVar, kept in
// 100-200, protected by sharedMutex
uint8_t shared = 100;

// Event group the button ISR uses to wake CalcSwitch1 (NULL until created)
EventGroupHandle_t buttonEvents = NULL;

// UARTQueue to hold the data to be sent across uart
QueueHandle_t UARTDataQueue = NULL;

// Mutex for the global shared variable
SemaphoreHandle_t sharedMutex = NULL;

// volatile since the ISR can change it at any time, so it must always be
// read from RAM
volatile uint8_t counter = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

void UartTask(void *argument);
void IncreaseVar(void *argument);
void DecreaseVar(void *argument);
void CalcSwitch1(void *argument);

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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  UARTDataQueue = xQueueCreate(5, sizeof(uart_msg_t));
  sharedMutex = xSemaphoreCreateMutex();
  buttonEvents = xEventGroupCreate();

  if (UARTDataQueue == NULL)
    Error_Handler();
  if (sharedMutex == NULL)
    Error_Handler();
  if (buttonEvents == NULL)
    Error_Handler();

  if (xTaskCreate(UartTask, "UART", 256, NULL, tskIDLE_PRIORITY + 1, NULL) !=
      pdPASS)
    Error_Handler();
  if (xTaskCreate(IncreaseVar, "IncreaseVar", 128, NULL, tskIDLE_PRIORITY + 1,
                  NULL) != pdPASS)
    Error_Handler();
  if (xTaskCreate(DecreaseVar, "DecreaseVar", 128, NULL, tskIDLE_PRIORITY + 1,
                  NULL) != pdPASS)
    Error_Handler();
  if (xTaskCreate(CalcSwitch1, "CalcSwitch1", 128, NULL, tskIDLE_PRIORITY + 1,
                  NULL) != pdPASS)
    Error_Handler();

  vTaskStartScheduler();

  /* USER CODE END 2 */

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
void SystemClock_Config(void)
{
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

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
  if (HAL_UART_Init(&huart1) != HAL_OK)
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

// REF: UM1850 p.228 (HAL_GPIO_EXTI_Callback, weak hook we override)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE; // FreeRTOS fills this in
  // Static lastPress variable to make sure it persists between function calls
  static TickType_t lastPress = 0;
  // REF: FreeRTOS task.h:1360 /
  // https://www.freertos.org/Documentation/02-Kernel/04-API-references/03-Task-utilities/00-Task-utilities
  TickType_t now = xTaskGetTickCountFromISR();

  if (GPIO_Pin == Button_Pin && buttonEvents != NULL) {

    // The difference survives the wrap of uint32_t
    if (now - lastPress >= DEBOUNCE_TICKS) {
      counter++;

      // The sheet didnt specify directly if 9 is allowed so this code assumes
      // it isnt
      if (counter >= 9) {
        counter = 0;
      }

      lastPress = now;

      // REF:
      // https://freertos.org/Documentation/02-Kernel/04-API-references/12-Event-groups-or-flags/06-xEventGroupSetBitsFromISR
      // we use the from ISR version here because FreeRTOS doesnt allow non
      // deterministic operation in an interrupt so it defers setting the bit
      // to the timer daemon task
      xEventGroupSetBitsFromISR(buttonEvents, BUTTON_PRESSED_BIT,
                                &xHigherPriorityTaskWoken);
    }
  }

  // FreeRTOS book v1.1.0 §7.2.5 (p.160); Cortex-M3 port: portmacro.h:93
  // Requests if xHigherPriorityTaskWoken is true that a switch occurs if not
  // then just does nothing
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void IncreaseVar(void *argument) {
  // initializes timeToNextWakeUp to current ticks
  TickType_t lastWake = xTaskGetTickCount();

  /* Infinite loop */
  for (;;) {

    // mutex is used to make sure the shared variable is protected and
    // osWaitForever is safe since the other task only uses it a few
    // microseconds
    // REF: CMSIS-RTOS2 osMutexAcquire:
    xSemaphoreTake(sharedMutex, portMAX_DELAY);

    shared++;

    // Wrapping according to the spec sheet
    if (shared > 200) {
      shared = 100;
    }

    uart_msg_t msg;
    msg.source = SRC_SHARED;
    msg.value = shared;

    // we release before sending it to the uart queue because if the queue hangs
    // it doesnt stop runtime
    xSemaphoreGive(sharedMutex);

    // timeout is 5 since thats how long it takes for the queue to clear and if
    // the clear queue task is hung it should NOT hang this task too
    xQueueSend(UARTDataQueue, &msg, 5);

    vTaskDelayUntil(&lastWake, 10);
  }
}

void DecreaseVar(void *argument) {

  // DecreaseVar: every 12 ticks
  TickType_t lastWake = xTaskGetTickCount();
  /* Infinite loop */
  for (;;) {

    // same mutex as IncreaseVar
    xSemaphoreTake(sharedMutex, portMAX_DELAY);
    shared--;

    // here we use < and not <= since 100 is allowed as a value
    if (shared < 100) {
      shared = 200;
    }

    uart_msg_t msg;
    msg.source = SRC_SHARED;
    msg.value = shared;

    // release before sending
    xSemaphoreGive(sharedMutex);

    xQueueSend(UARTDataQueue, &msg, 5);
    vTaskDelayUntil(&lastWake, 12);
  }
}

void CalcSwitch1(void *argument) {
  /* Infinite loop */
  for (;;) {

    // waits for up to 2 ticks to read the button bit
    // REF:
    // https://freertos.org/Documentation/02-Kernel/04-API-references/12-Event-groups-or-flags/04-xEventGroupWaitBits
    EventBits_t bits = xEventGroupWaitBits(buttonEvents, BUTTON_PRESSED_BIT,
                                           pdTRUE, pdFALSE, 2);
    if ((bits & BUTTON_PRESSED_BIT) == BUTTON_PRESSED_BIT) {

      // we copy it since the value of the counter can change between the
      // multiply and the send
      uint8_t counterCopy = counter;

      counterCopy *= 10;

      uart_msg_t msg;
      msg.source = SRC_COUNTER;
      msg.value = counterCopy;

      xQueueSend(UARTDataQueue, &msg, 5);
    }
  }
}

void UartTask(void *argument) {
  // This initializes the nextWakeTick variable to be used in the infinite loop

  TickType_t lastWake = xTaskGetTickCount();
  /* Infinite loop */
  for (;;) {
    uart_msg_t msg;

    char buf[32];

    // Drains the whole queue: takes messages until it's empty, then stops
    // (timeout 0 = don't wait for more)
    while (xQueueReceive(UARTDataQueue, &msg, 0) == pdPASS) {
      int len = 0;

      // Switch case on where the msg came from (shared or counter)
      switch (msg.source) {
      case SRC_SHARED:

        // snprintf formats the label + value into buf; the n limits it to
        // sizeof(buf) so it can never write past the end of the buffer
        len = snprintf(buf, sizeof(buf), "Shared: %u\r\n", msg.value);
        break;
      case SRC_COUNTER:
        len = snprintf(buf, sizeof(buf), "Counter: %u\r\n", msg.value);
        break;
      }

      // send the text thru the UART peripheral
      // REF: UM1850 p.556 (HAL_UART_Transmit)
      HAL_UART_Transmit(&huart1, (const uint8_t *)buf, len, HAL_MAX_DELAY);
    }

    // osDelayUntil to ensure it runs every 5 ticks no matter how much time it
    // took to get the work done
    vTaskDelayUntil(&lastWake, 5);
  }
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
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
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
