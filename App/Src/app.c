#include "app.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "lcd.h" 
#include "ui.h"
#include "lvgl.h"

/* 内部声明任务函数 */
static void gui_task(void *pvParameters);
static void led_task(void *pvParameters);

void App_Init(void)
{
    /* 创建 LED 任务 */
    xTaskCreate(led_task, "LED", 128, NULL, 2, NULL);
    
    /* 创建 GUI 任务 */
    xTaskCreate(gui_task, "GUI", 1024, NULL, 3, NULL);
}

static void led_task(void *pvParameters)
{
    (void)pvParameters;
    for (;;) {
        HAL_GPIO_TogglePin(d2_red_GPIO_Port, d2_red_Pin);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void gui_task(void *pvParameters)
{
    (void)pvParameters;

    /* 1. 初始化硬件屏幕和 LVGL */
    LCD_Init(); 
    UI_Init();  // 调用 UI 层初始化

    /* 2. 测试：更新文字 */
    UI_UpdateText("App Decoupled!\nRunning Fine.");

    /* 3. LVGL 主循环 */
    for (;;) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/* 放在 app.c 里面或者单独一个文件里都可以
   这个函数由 FreeRTOS 自动调用，每 1ms 触发一次 */
void vApplicationTickHook(void)
{
    lv_tick_inc(1);
}
