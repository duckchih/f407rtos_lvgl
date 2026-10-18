#include <stdio.h>

#include "FreeRTOS.h"
#include "app.h"
#include "task.h"
#include "queue.h"
#include "main.h"
#include "lcd.h" 
#include "ui.h"
#include "lvgl.h"
#include "bsp_uart.h"

/* 定义队列句柄 */
static QueueHandle_t led_count_queue = NULL; 

// 定义串口事件队列（传递数据长度）
static QueueHandle_t uart_event_queue = NULL; 
// 这个宏必须暴露给 bsp_uart.c 去用
void UART_IRQ_Notify(uint16_t size); 

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
    // 创建长度为 10 的队列，用于传递串口收到的字节数
    uart_event_queue = xQueueCreate(10, sizeof(uint16_t));

    BSP_UART_Init();
    /* 创建 LED 任务 */
    xTaskCreate(led_task, "LED", 128, NULL, 2, NULL);
    
    /* 创建 GUI 任务 */
    xTaskCreate(gui_task, "GUI", 2048, NULL, 3, NULL);
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

// static void gui_task(void *pvParameters)
// {
//     (void)pvParameters;

//     LCD_Init(); 
//     UI_Init(); 

//     uint32_t received_count = 0;
//     char display_str[32];

//     uint8_t *rx_data;
//     uint16_t rx_len;
//     char uart_display_str[64];
//     /* 初始化一下屏幕上的文字 */
//     UI_UpdateText("LED Blink Count:\n0");

//     for (;;) {
//         /* 尝试从队列中读取数据，超时时间为 0（非阻塞，读不到就去处理 LVGL） */
//         if (xQueueReceive(led_count_queue, &received_count, 0) == pdPASS) 
//         {
//             /* 如果读到了新的计数值，把它拼接成字符串 */
//             snprintf(display_str, sizeof(display_str), "LED Blink Count:\n%lu", received_count);
            
//             /* 更新到屏幕上 */
//             UI_UpdateText(display_str);
//         }
//         // 2. 处理串口收到的数据（非阻塞）
//         if (xQueueReceive(uart_event_queue, &rx_len, 0) == pdPASS) 
//         {
//             // 读取 BSP 层收到的数据
//             BSP_UART_GetReceivedData(&rx_data);
            
//             // 将收到的数据转成字符串（假设发的是普通字符串）
//             //snprintf(uart_display_str, sizeof(uart_display_str), 
//             //         "UART RX (%d bytes):\n%.20s", rx_len, rx_data);

//             //UI_UpdateText(uart_display_str);
//             snprintf(display_str, sizeof(display_str), "Got Bytes: %d", rx_len);
//             UI_UpdateText(display_str);
//         }
//         /* LVGL 不断处理自身的渲染任务 */
//         lv_timer_handler();
//         vTaskDelay(pdMS_TO_TICKS(5));
//     }
// }

static void gui_task(void *pvParameters)
{
    (void)pvParameters;

    LCD_Init(); 
    UI_Init(); 

    uint32_t received_count = 0;
    uint16_t rx_len = 0;
    uint8_t *rx_data = NULL;
    char uart_str[64];

    for (;;) {
        // 1. 处理 LED 队列
        if (xQueueReceive(led_count_queue, &received_count, 0) == pdPASS) {
            UI_UpdateLedCount(received_count);
        }

        // 2. 处理 UART 队列
        if (xQueueReceive(uart_event_queue, &rx_len, 0) == pdPASS) 
        {
            BSP_UART_GetReceivedData(&rx_data);
            // 限制显示长度，防止把屏幕撑爆
            if(rx_len > 20) rx_len = 20; 
            snprintf(uart_str, sizeof(uart_str), "RX: %.*s", rx_len, rx_data);
            UI_UpdateUartText(uart_str);

            // 新加 DMA 回显：通过 DMA 发送回去（Echo）
            char echo_str[64];
            snprintf(echo_str, sizeof(echo_str), "Echo: %.*s\r\n", rx_len, rx_data);
            BSP_UART_SendString(echo_str);
        }
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

void UART_IRQ_Notify(uint16_t size)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // 必须使用带 FromISR 的 API
    xQueueSendFromISR(uart_event_queue, &size, &xHigherPriorityTaskWoken);
    // 如果发信后唤醒了更高优先级的任务，强制触发一次上下文切换
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}