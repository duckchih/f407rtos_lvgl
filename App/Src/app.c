#include <stdio.h>
#include "FreeRTOS.h"
#include "app.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "main.h"
#include "lcd.h" 
#include "ui.h"
#include "lvgl.h"
#include "bsp_uart.h"
#include "iwdg.h"
#include "bsp_flash.h"

/* 定义队列句柄 */
static QueueHandle_t led_count_queue = NULL; 

// 定义串口事件队列（传递数据长度）
static QueueHandle_t uart_event_queue = NULL;
// 定义保存队列
static QueueHandle_t flash_save_queue = NULL;


// 做优先级反转
// 定义互斥量和任务句柄
static SemaphoreHandle_t shared_mutex = NULL;
static TaskHandle_t task_H = NULL, task_M = NULL, task_L = NULL;
// 这个宏必须暴露给 bsp_uart.c 去用
void UART_IRQ_Notify(uint16_t size); 

// 定义定时器高精度统计变量（必须加 volatile，因为它会在中断中被修改）
volatile uint32_t ulHighFrequencyTimerTicks = 0;

static uint32_t system_health_flags = 0;
/* 内部声明任务函数 */
static void gui_task(void *pvParameters);
static void led_task(void *pvParameters);
static void monitor_task(void *pvParameters);
static void wdg_task(void *pvParameters);
static void task_H_func(void *pvParameters);
static void task_M_func(void *pvParameters);
static void task_L_func(void *pvParameters);
static void flash_task(void *pvParameters);
  /* 创建 LED 任务 */
  // xTaskCreate(led_task,             /* 任务函数 */
  //             "LED",                /* 任务名（调试器里能看到） */
  //             128,                  /* 栈大小：128 words = 512 bytes */
  //             NULL,                 /* 传给任务的参数 */
  //             2,                    /* 优先级：2（数值越大优先级越高） */
  //             &led_task_handle);    /* 保存句柄 */

uint32_t saved_count;
void App_Init(void)
{
    saved_count = BSP_Flash_ReadWord(FLASH_SAVE_ADDR);
    // 如果是 0xFFFFFFFF（全 1），说明是新芯片没写过
    if (saved_count == 0xFFFFFFFF) {
        saved_count = 0;
    }
    // 把读出来的初始值发给 LED 任务（或者存个全局变量）
    // 创建队列（长度 2 即可，不需要太长）
    flash_save_queue = xQueueCreate(2, sizeof(uint32_t));

    /* 创建一个队列，长度为 5，每个元素的大小是 uint32_t（4字节） */
    led_count_queue = xQueueCreate(5, sizeof(uint32_t));
    // 创建长度为 10 的队列，用于传递串口收到的字节数
    uart_event_queue = xQueueCreate(10, sizeof(uint16_t));
    // 创建互斥量（注意，Mutax 和 Binary Semaphore 创建 API 不同！）
    shared_mutex = xSemaphoreCreateMutex();

    BSP_UART_Init();


    /* 创建 LED 任务 */
    xTaskCreate(led_task, "LED", 128, NULL, 2, NULL);
    // xTaskCreate(flash_task, "Flash", 256, NULL, 1, NULL);// 创建任务（优先级必须比 LED 低，设为 1）
    /* 创建 GUI 任务 */
    xTaskCreate(gui_task, "GUI", 2048, NULL, 3, NULL);
    xTaskCreate(monitor_task, "MON", 1024, NULL, 1, NULL); // 优先级给 1 就行了
    xTaskCreate(wdg_task, "WDG", 256, NULL, 4, NULL);

    // 三个优先级反转测试任务，优先级严格区分
    // xTaskCreate(task_L_func, "Task_L", 256, NULL, 2, &task_L);
    // xTaskCreate(task_M_func, "Task_M", 256, NULL, 3, &task_M);
    // xTaskCreate(task_H_func, "Task_H", 256, NULL, 4, &task_H);
}

static void led_task(void *pvParameters)
{
    /* 
     * FreeRTOS任务函数原型固定包含 void *pvParameters。
     * 由于当前任务不需要接收外部参数，因此使用 (void) 强制转换，
     * 告诉编译器忽略“未使用变量”的警告。
     * (void *)系统发的“万能快递盒”，但这里不拆包，纯占位防止编译器报警
     */
    (void)pvParameters;
    // uint32_t blink_count = 0; // 记录闪烁次数
    uint32_t blink_count = saved_count; // 从 Flash 恢复的值开始数
    uint32_t last_saved_count = blink_count; // 记录上次存入的值
    for (;;) {
        /* 翻转 LED */
        HAL_GPIO_TogglePin(d2_red_GPIO_Port, d2_red_Pin);
        blink_count++;
        /* 将计数发送到队列。超时时间为 0：如果队列满了也不等，直接丢弃，绝不能阻塞 LED 闪烁 */
        xQueueSend(led_count_queue, &blink_count, 0);
        system_health_flags |= (1 << 0); // 打卡,防止喂狗

        // 🚀 优化：只有当数字是 10 的倍数（比如 10, 20, 30），且和上次存的数值不同时才写！
        if ((blink_count % 10 == 0) && (blink_count != last_saved_count)) {
            xQueueSend(flash_save_queue, &blink_count, 0); 
            last_saved_count = blink_count;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
        // 测试独立看门狗代码。在 led_task 的 for 循环里加上：
        // if (blink_count == 200) {
        //     while(1); // 故意死机
        // }
    }
}

// 看门狗监控任务
static void wdg_task(void *pvParameters)
{
    (void)pvParameters;
    const TickType_t xFrequency = pdMS_TO_TICKS(1000);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        if ((system_health_flags & (1 << 0)) != 0) {
            system_health_flags &= ~(1 << 0);
            HAL_IWDG_Refresh(&hiwdg); // 喂狗
        } else {
            BSP_UART_SendString("WDG: Task failed! System will reboot soon...\r\n");
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}


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

static void monitor_task(void *pvParameters)
{
    (void)pvParameters;
    char stats_buffer[256]; // 缓冲区要稍微大一点
    char stack_buffer[128];
    
    // 先将 RTOS 统计功能启动（初始化底座）
    vTaskDelay(pdMS_TO_TICKS(1000)); 

    for (;;) {
        // 1. 获取 CPU 占用率（底软神级 API）
        // 它会把所有任务和它们的 CPU 时间格式化到 stats_buffer 里
        vTaskGetRunTimeStats(stats_buffer);
        
        // 2. 获取 GUI 任务的栈剩余空间（单位为 Word，1 Word = 4 Bytes）
        // uxTaskGetStackHighWaterMark 返回的是历史最少剩余量
        UBaseType_t stack_left = uxTaskGetStackHighWaterMark(NULL); // NULL 代表当前任务
        
        snprintf(stack_buffer, sizeof(stack_buffer), "Stack Left: %lu B", stack_left * 4);
        
        // 3. 打印一下方便在串口看（可选）
        // printf("%s\n", stats_buffer);
        
        // 4. 发送给 GUI 更新 (注意屏幕很小，可能只显示前几行)
        UI_UpdateMonitor(stats_buffer, stack_buffer);
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // 每 2 秒更新一次
    }
}

static void flash_task(void *pvParameters)
{
    (void)pvParameters;
    uint32_t count_to_save = 0;

    for (;;) {
        // 一直阻塞等待，直到有人发数据过来
        if (xQueueReceive(flash_save_queue, &count_to_save, portMAX_DELAY) == pdPASS) 
        {
            // 此时会卡顿 1 秒，但只会卡住自己这个低优先级任务
            BSP_Flash_WriteWord(FLASH_SAVE_ADDR, count_to_save);
        }
    }
}


// 任务 H（高优先级）：尝试获取锁
static void task_H_func(void *pvParameters) {
    (void)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(100)); // 故意晚点启动，让 L 先拿到锁
    
    BSP_UART_SendString("H: Try Lock...\r\n");  
    xSemaphoreTake(shared_mutex, portMAX_DELAY); // 拿锁，如果 L 没释放，H 会阻塞 
    BSP_UART_SendString("H: Got Lock!\r\n");
    xSemaphoreGive(shared_mutex);
    
    for(;;) vTaskDelay(portMAX_DELAY); // 执行完就挂起
}

// 任务 M（中优先级）：纯粹来捣乱的
static void task_M_func(void *pvParameters) {
    (void)pvParameters;
    vTaskDelay(pdMS_TO_TICKS(200)); // 等 H 被阻塞了，再出来
    for (;;) {
        BSP_UART_SendString("M: Running wild (hogging CPU)!\r\n");
        vTaskDelay(pdMS_TO_TICKS(200)); // 故意一直占着 CPU 跑
    }
}

// 任务 L（低优先级）：拿到锁后慢慢干活
static void task_L_func(void *pvParameters) {
    (void)pvParameters;
    
    xSemaphoreTake(shared_mutex, portMAX_DELAY); // 先拿锁
    BSP_UART_SendString("L: Got Lock, doing heavy work...\r\n");
    // 加入打印优先级
    UBaseType_t current_priority = uxTaskPriorityGet(NULL);
    char str[32];
    snprintf(str, sizeof(str), "L current priority: %lu\n", current_priority);
    BSP_UART_SendString(str);

    // 模拟耗时操作（比如读大文件、刷长字符串）
    // for(uint32_t i = 0; i < 0xFFFFFFF; i++) { __NOP(); }

    // 模拟运算 300ms（这里不用死循环，用几个循环块）
    for(int i=0; i<3; i++) {
        // 约 100ms 的运算量
        for(uint32_t j = 0; j < 0x0FFFFFF; j++) { __NOP(); }
        
        // 每算完一段打印一次优先级
        snprintf(str, sizeof(str), "L: Working... prio = %lu\r\n", uxTaskPriorityGet(NULL));
        BSP_UART_SendString(str); // 如果 H 醒了，这里就会打印：4
    }

    BSP_UART_SendString("L: Release Lock.\r\n");
    xSemaphoreGive(shared_mutex);
    
    for(;;) vTaskDelay(portMAX_DELAY);
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
