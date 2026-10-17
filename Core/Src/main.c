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
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"
#include "lvgl.h"
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

/* USER CODE BEGIN PV */
// static TaskHandle_t led_task_handle = NULL;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void led_red_task(void *pvParameters)
{
    (void)pvParameters;   /* 避免 unused parameter 警告 */

    for (;;) {
        HAL_GPIO_TogglePin(d2_red_GPIO_Port, d2_red_Pin);   /* 换成你的引脚 */
        vTaskDelay(pdMS_TO_TICKS(500));          /* 阻塞 500ms，让出 CPU */
    }
}

static void led_green_task(void *pvParameters)
{
    (void)pvParameters;   /* 避免 unused parameter 警告 */

    for (;;) {
        HAL_GPIO_TogglePin(d2_green_GPIO_Port, d2_green_Pin);   /* 换成你的引脚 */
        vTaskDelay(pdMS_TO_TICKS(200));          /* 阻塞 200ms，让出 CPU */
    }
}


#include "lv_port_disp.h"
static void gui_task(void *pvParameters)
{
    (void)pvParameters;

    /* 1. 初始化屏幕（你的 ST7789 驱动） */
    LCD_Init(); 
    LCD_Fill(50, 50, 120, 100, GREEN); 
    /* 2. 初始化 LVGL 核心库 */
    lv_init();

    /* 3. 初始化 LVGL 显示驱动（刚才写的移植层） */
    lv_port_disp_init();

    /* 4. 把默认屏幕背景改成黑色 */
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);

    /* 5. 创建一个标签（Label） */
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello LVGL!\nF407 + FreeRTOS\nST7789 SPI");
    
    /* 6. 设置文字颜色为绿色，字体稍大一点 */
    lv_obj_set_style_text_color(label, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0); // 文字居中对齐
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0); // 整个标签在屏幕居中

    for ( ; ; ) {
        /* 7. 喂狗：LVGL 需要不断调用这个来处理任务、画图 */
        lv_timer_handler();
        /* 给 5ms 延时让出 CPU，不要死等 */
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/* 这个函数由 FreeRTOS 自动调用，每 1ms 触发一次 */
void vApplicationTickHook(void)
{
    lv_tick_inc(1); 
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
  /* USER CODE BEGIN 2 */
  // LCD_Init();
  // LCD_Fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, RED); // 刷成红色
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  /* 创建 LED 任务 */
  // xTaskCreate(led_task,             /* 任务函数 */
  //             "LED",                /* 任务名（调试器里能看到） */
  //             128,                  /* 栈大小：128 words = 512 bytes */
  //             NULL,                 /* 传给任务的参数 */
  //             2,                    /* 优先级：2（数值越大优先级越高） */
  //             &led_task_handle);    /* 保存句柄 */

  xTaskCreate(led_red_task,   "LED_R",  32, NULL, 2, NULL);
  xTaskCreate(led_green_task, "LED_G",  32, NULL, 2, NULL);
  xTaskCreate(gui_task,       "GUI",    2048, NULL, 3, NULL); // 给 GUI 2KB 栈
  /* 启动调度器。这一句之后，CPU 交给 FreeRTOS 控制 */
  vTaskStartScheduler();

  /* 只有调度器启动失败（堆空间不够），才会执行到这里 */
  for (;;);

  while (1)
  {
    HAL_GPIO_TogglePin(d2_red_GPIO_Port, d2_red_Pin);
    HAL_Delay(500);

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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
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
