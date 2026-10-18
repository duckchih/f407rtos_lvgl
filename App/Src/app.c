#include <stdio.h>

#include "app.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "main.h"
#include "lcd.h" 
#include "ui.h"
#include "lvgl.h"
#include "stdio.h"

/* 定义队列句柄 */
static QueueHandle_t led_count_queue = NULL; 

/* 内部声明任务函数 */
static void gui_task(void *pvParameters);
static void led_task(void *pvParameters);

  /* 创建 LED 任务 */
  // xTaskCreate(led_task,             /* 任务函数 */
  //             "LED",                /* 任务名（调试器里能看到） */
  //             128,                  /* 栈大小：128 words = 512 bytes */
  //             NULL,                 /* 传给任务的参数 */
  //             2,                    /* 优先级：2（数值越大优先级越高） */
  //             &led_task_handle);    /* 保存句柄 */


void App_Init(void)
{
    /* 创建一个队列，长度为 5，每个元素的大小是 uint32_t（4字节） */
    led_count_queue = xQueueCreate(5, sizeof(uint32_t));

    /* 创建 LED 任务 */
    xTaskCreate(led_task, "LED", 128, NULL, 2, NULL);
    
    /* 创建 GUI 任务 */
    xTaskCreate(gui_task, "GUI", 1024, NULL, 3, NULL);
}

static void led_task(void *pvParameters)
{
    /* 
     * FreeRTOS任务函数原型固定包含 void *pvParameters。
     * 由于当前任务不需要接收外部参数，因此使用 (void) 强制转换，
     * 告诉编译器忽略“未使用变量”的警告。
     * 
     * (void *)系统发的“万能快递盒”，但这里不拆包，纯占位防止编译器报警
     */
    (void)pvParameters;
    uint32_t blink_count = 0; // 记录闪烁次数

    for (;;) {
        /* 翻转 LED */
        HAL_GPIO_TogglePin(d2_red_GPIO_Port, d2_red_Pin);
        
        blink_count++;

        /* 将计数发送到队列。超时时间为 0：如果队列满了也不等，直接丢弃，绝不能阻塞 LED 闪烁 */
        xQueueSend(led_count_queue, &blink_count, 0);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void gui_task(void *pvParameters)
{
    (void)pvParameters;

    LCD_Init(); 
    UI_Init(); 

    uint32_t received_count = 0;
    char display_str[32];

    /* 初始化一下屏幕上的文字 */
    UI_UpdateText("LED Blink Count:\n0");

    for (;;) {
        /* 尝试从队列中读取数据，超时时间为 0（非阻塞，读不到就去处理 LVGL） */
        if (xQueueReceive(led_count_queue, &received_count, 0) == pdPASS) 
        {
            /* 如果读到了新的计数值，把它拼接成字符串 */
            snprintf(display_str, sizeof(display_str), "LED Blink Count:\n%lu", received_count);
            
            /* 更新到屏幕上 */
            UI_UpdateText(display_str);
        }

        /* LVGL 不断处理自身的渲染任务 */
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
