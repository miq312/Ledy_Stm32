/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stdbool.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
	GPIO_TypeDef* port;
	uint16_t pin;
}	pin_port_t;

typedef void (*button_event_t)();

typedef struct
{
	GPIO_PinState lastState;
	pin_port_t buttonPin;
	button_event_t onRelease;
	button_event_t onLongPress;
	uint32_t bounceStartTimeMillis;
	uint32_t longPressTimeMillis;
} button_t;

typedef enum
{
	MODE_MANUAL,
	MODE_ANIMATED
} mode_t;

typedef enum {
	BACKWARD = -1,
	FORWARD = 1
} direction_t;

typedef void (*transition_handler_t)();

typedef enum {
	TRANSITION_SIMPLE,
	TRANSITION_BOUNCE,
	TRANSITION_TOGGLE_SIMPLE,
	TRANSITION_TOGGLE_BOUNCE,
	TRANSITION_TWOCOLORS,
	TRANSITION_CROSSROADS,
	TRANSITION_NOISE,
	TRANSITION_RAINDROPS,
	TRANSITIONS_COUNT
} transition_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define BOUNCE_TIME_MILLIS 50
#define LONGPRESS_TIME_MILLIS 500

#define TRANSITION_INITIAL_LEVEL 8
#define TRANSITION_INTERVAL_BASE 2

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#define ARRLEN(x) (sizeof(x)/sizeof(x)[0])

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
RNG_HandleTypeDef hrng;

/* USER CODE BEGIN PV */
static int led = 0;

static const pin_port_t ld[] = {
		{ ld1_GPIO_Port, ld1_Pin },
		{ ld2_GPIO_Port, ld2_Pin },
		{ ld3_GPIO_Port, ld3_Pin },
		{ ld4_GPIO_Port, ld4_Pin },
		{ ld5_GPIO_Port, ld5_Pin },
		{ ld6_GPIO_Port, ld6_Pin },
		{ ld7_GPIO_Port, ld7_Pin },
		{ ld8_GPIO_Port, ld8_Pin },
		{ ld9_GPIO_Port, ld9_Pin },
		{ ld10_GPIO_Port, ld10_Pin }
};
static const int LEDS_COUNT = ARRLEN(ld);
static uint32_t interval = 128;
static uint32_t transitionTimer = 0;

static direction_t direction = FORWARD;
static int intervalLevel = TRANSITION_INITIAL_LEVEL;

static mode_t mode = MODE_MANUAL;

static transition_t transition = TRANSITION_SIMPLE;

static uint32_t randResult = 0;
static int rainAnimationStage;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RNG_Init(void);
/* USER CODE BEGIN PFP */
void ld_set(int led, bool turn_on);
void ld_toogle();
int randLED();
void checkButtonsEvents();
void animate(bool force);
void longPressBackward();
void longPressForward();
void changeMode();
void pressBackward();
void pressForward();
void changeTransition();
void simpleTransitionAnimation();
void bounceAnimation();
void toggleSimpleAnimation();
void bounceToggleAnimation();
void twocolorsAnimation();
void crossroadsAnimation();
void noiseAnimation();
void raindropsAnimation();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//Dioda
void ld_set(int led, bool turn_on)
{
	GPIO_PinState state = (turn_on) ? GPIO_PIN_SET : GPIO_PIN_RESET;

	if (led >= 0 && led < 10)
		HAL_GPIO_WritePin(ld[led].port, ld[led].pin, state);
}

void leds_clear()
{
	for(int i = 0; i < LEDS_COUNT; ++i)
		ld_set(i, false);
}

//Przycisk
static button_t buttons[]= {
		{GPIO_PIN_RESET, {button1_GPIO_Port, button1_Pin}, pressBackward, longPressBackward, 0, 0},
		{GPIO_PIN_RESET, {button2_GPIO_Port, button2_Pin}, pressForward, longPressForward, 0, 0},
		{GPIO_PIN_RESET, {button3_GPIO_Port, button3_Pin}, changeTransition, changeMode, 0, 0}
};

static const int buttonsCount = ARRLEN(buttons);

void checkButtonsEvents()
{
	uint32_t millis = HAL_GetTick();

	for (int i = 0; i < buttonsCount; ++i)
	{
		button_t *button = &buttons[i];

		if (millis - button->bounceStartTimeMillis > BOUNCE_TIME_MILLIS)
		{
			GPIO_TypeDef *port = button->buttonPin.port;
			int pin = button->buttonPin.pin;
			GPIO_PinState currentState = HAL_GPIO_ReadPin(port, pin);

			if (button->lastState == GPIO_PIN_RESET	&& currentState == GPIO_PIN_SET)
			{
				//on release event
				if (button->longPressTimeMillis > 0)
				{
					button->bounceStartTimeMillis = millis;
					button->onRelease();
				}
			}

			else if (button->lastState == GPIO_PIN_SET && currentState == GPIO_PIN_RESET)
			{
				//on press event -> on longpress event
				button->longPressTimeMillis = millis;
			}

			else if (currentState == GPIO_PIN_RESET)
			{
				if (button->longPressTimeMillis > 0)
				{
					if(millis - button->longPressTimeMillis > LONGPRESS_TIME_MILLIS){
						button->longPressTimeMillis = 0;
						button->onLongPress();
					}
				}
			}

			button->lastState = currentState;
		}
	}
}

//Tryby pracy
static const transition_handler_t animations[TRANSITIONS_COUNT] = {simpleTransitionAnimation, bounceAnimation, toggleSimpleAnimation, bounceToggleAnimation,
		                                                           twocolorsAnimation, crossroadsAnimation, noiseAnimation, raindropsAnimation};

void animate(bool force)
{
	if(force || ((mode == MODE_ANIMATED) && (HAL_GetTick() - transitionTimer > interval)))
	{
		animations[transition]();

		if(!force)
			transitionTimer += interval;
	}
}

uint32_t powint(uint32_t base, int power){
	uint32_t result = 1;
	for (int i = 0; i < power; ++i)
		result *= base;
	return result;
}

void pressForward() {
	if(mode == MODE_MANUAL){
		direction = FORWARD;
		animate(true);
	}
	else {
		if(--intervalLevel < 1)
			intervalLevel = 1;
		interval = powint(TRANSITION_INTERVAL_BASE, intervalLevel);
	}
}

void pressBackward() {
	if(mode == MODE_MANUAL){
		direction = BACKWARD;
		animate(true);
	}
	else{
		++intervalLevel;
		interval = powint(TRANSITION_INTERVAL_BASE, intervalLevel);
	}
}

void longPressBackward()
{
	direction = FORWARD;
}
void longPressForward()
{
	direction = BACKWARD;
}
void changeMode()
{
	if(mode == MODE_MANUAL)
	{
		transitionTimer = HAL_GetTick();
		mode = MODE_ANIMATED;
	}
	else
		mode = MODE_MANUAL;
	return;
}

void changeTransition()
{
	if(mode != MODE_MANUAL){
		transitionTimer = HAL_GetTick();
		leds_clear();
		led = 0;
		direction = FORWARD;
	}

	if(++transition >= TRANSITIONS_COUNT)
		transition = TRANSITION_SIMPLE;
}

//Animacje
int randLED()
{
	HAL_RNG_GenerateRandomNumber(&hrng, &randResult);
	return randResult % LEDS_COUNT;
}

void simpleTransitionAnimation()
{

	ld_set(led, false);

	led += direction;
	if(direction == BACKWARD && led < 0)
		led += LEDS_COUNT;
	else if(direction == FORWARD && led >= LEDS_COUNT)
		led -= LEDS_COUNT;

	ld_set(led, true);
}

void bounceAnimation()
{
	ld_set(led, false);

	led += direction;
	if(direction == BACKWARD && led < 0)
	{
		led = -led;
		direction = FORWARD;
	}
	else if(direction == FORWARD && led >= LEDS_COUNT)
	{
		led = led - LEDS_COUNT + 1;
		led = LEDS_COUNT - led - 1;
		direction = BACKWARD;
	}

	ld_set(led, true);
}

void led_toggle(int led)
{
	if (led >= 0 && led < LEDS_COUNT)
		HAL_GPIO_TogglePin(ld[led].port, ld[led].pin);
}

void toggleSimpleAnimation()
{
	led_toggle(led);

	led += direction;
	if(direction == BACKWARD && led < 0)
		led += LEDS_COUNT;
	else if(direction == FORWARD && led >= LEDS_COUNT)
		led -= LEDS_COUNT;
}

void bounceToggleAnimation()
{
	led_toggle(led);

	led += direction;
	if(direction == BACKWARD && led < 0){
		led = -led-1;
		direction = FORWARD;
	}
	else if(direction == FORWARD && led >= LEDS_COUNT){
		led = led - LEDS_COUNT + 1;
		led = LEDS_COUNT - led ;
		direction = BACKWARD;
	}
}

void twocolorsAnimation()
{
	for (int i = 0; i < LEDS_COUNT/2; ++i)
		ld_set(i*2+led, false);

	led ^= 1;

	for (int i = 0; i < LEDS_COUNT/2; ++i)
		ld_set(i*2+led, true);
}

void crossroadsAnimation()
{
	ld_set(led, false);
	ld_set(LEDS_COUNT - led - 1, false);

	led += direction;
	if(direction == BACKWARD && led < 0)
		led += LEDS_COUNT;
	else if(direction == FORWARD && led >= LEDS_COUNT)
		led -= LEDS_COUNT;

	ld_set(led, true);
	ld_set(LEDS_COUNT - led - 1, true);
}

void noiseAnimation(){

	ld_set(led, false);

	led = randLED();

	ld_set(led, true);
}

void raindropsAnimation() {
	if(rainAnimationStage == 0) {
		led = randLED();
		ld_set(led, true);
	} else if(rainAnimationStage == 1) {
		ld_set(led - 1, true);
		ld_set(led + 1, true);
	} else if(rainAnimationStage == 2) {
		ld_set(led, false);
		ld_set(led - 2, true);
		ld_set(led + 2, true);
	} else if(rainAnimationStage == 3) {
		ld_set(led - 1, false);
		ld_set(led + 1, false);
	} else if(rainAnimationStage == 4) {
		ld_set(led - 2, false);
		ld_set(led + 2, false);
	}

	if(++rainAnimationStage > 4)
		rainAnimationStage = 0;
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
  MX_RNG_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
	 checkButtonsEvents();
	 animate(false);
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

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

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
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, ld6_Pin|ld7_Pin|ld8_Pin|ld9_Pin
                          |ld10_Pin|ld1_Pin|ld2_Pin|ld3_Pin
                          |ld4_Pin|ld5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : button1_Pin button2_Pin button3_Pin */
  GPIO_InitStruct.Pin = button1_Pin|button2_Pin|button3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : ld6_Pin ld7_Pin ld8_Pin ld9_Pin
                           ld10_Pin ld1_Pin ld2_Pin ld3_Pin
                           ld4_Pin ld5_Pin */
  GPIO_InitStruct.Pin = ld6_Pin|ld7_Pin|ld8_Pin|ld9_Pin
                          |ld10_Pin|ld1_Pin|ld2_Pin|ld3_Pin
                          |ld4_Pin|ld5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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
