#include "FreeRTOS.h"
#include "bsp_uart.h"
#include "usart.h" // CubeMX 生成的
#include <string.h>
#include "queue.h"


// 声明外部队列
extern QueueHandle_t uart_event_queue;
extern void UART_IRQ_Notify(uint16_t size); 

// DMA 接收缓冲
static uint8_t rx_buffer[UART_RX_BUF_SIZE];
static uint8_t tx_buffer[UART_TX_BUF_SIZE]; // 发送专用缓冲
static uint16_t last_rx_size = 0;

static SemaphoreHandle_t tx_sem = NULL; // 发送信号量
void BSP_UART_Init(void)
{
    // 1. 创建二值信号量（初始状态为给出，允许第一次发送）
    tx_sem = xSemaphoreCreateBinary();
    if (tx_sem != NULL) {
        xSemaphoreGive(tx_sem); 
    }
    // 清除空闲标志
    __HAL_UART_CLEAR_IDLEFLAG(&huart1);
    // 启动 DMA 接收，并开启 IDLE 中断，HAL 库专门处理 IDLE 中断的
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buffer, UART_RX_BUF_SIZE);
}

// 串口发送函数（带阻塞等待，防止覆盖）
void BSP_UART_SendString(const char *str)
{
    if (str == NULL || tx_sem == NULL) return;

    uint16_t len = strlen(str);
    if (len > UART_TX_BUF_SIZE) len = UART_TX_BUF_SIZE;

    // 1. 获取信号量（如果上一次还没发完，任务会在这里阻塞死等，让出 CPU）
    if (xSemaphoreTake(tx_sem, portMAX_DELAY) == pdTRUE) 
    {
        // 2. 必须深拷贝到全局 tx_buffer，防止 DMA 发送时源地址失效
        memcpy(tx_buffer, str, len);

        // 3. 启动 DMA 发送
        HAL_UART_Transmit_DMA(&huart1, tx_buffer, len);
    }
}

// 重写 HAL 库的回调函数（在中断上下文中被调用）
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1) 
    {
        if (Size > 0 && Size < UART_RX_BUF_SIZE) {
            rx_buffer[Size] = '\0'; // 极其关键：手动截断字符串
        }

        last_rx_size = Size; // 记录收到了多少个字节
        
        // 这里绝对不能做耗时操作！不能 printf，不能刷屏！
        // 必须唤醒任务！
        // 唤醒 App , 由 App 层决定怎么处理 , 这部分在 App 层用 RTOS 信号量或队列实现
        UART_IRQ_Notify(Size);
        // (一开始)重新启动接收（保险起见）
        /*
            Normal ：DMA 搬完设定长度就停。一次启动只收一次，收完/空闲后通常要软件重新启动。
            Circular ：DMA 搬完 128 字节自动回数组开头继续搬，硬件自己循环。软件只启动一次，之后只负责"消费数据"，不要每次事件都重新启动。
            Circular 模式下，DMA 自己回绕，HAL_UARTEx_RxEventCallback 只是“告诉你收到了一段数据/空闲了/半满/全满”，不是“接收结束了必须重启”。
            每次事件都重新调用 HAL_UARTEx_ReceiveToIdle_DMA 不是保险，而是重复启动一个仍在 BUSY 的接收流程，容易返回 HAL_BUSY 或引发状态机竞争。
            只有 Normal 模式、错误恢复、主动停止后重新开始 才需要重新开启接收。
        */
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buffer, UART_RX_BUF_SIZE);
    }
}

// 提供接口给 App 层读取数据
uint16_t BSP_UART_GetReceivedData(uint8_t **data_ptr)
{
    *data_ptr = rx_buffer;
    return last_rx_size;
}

// DMA接收+串口空闲中断代码完成后，新增：DMA 发送完成回调（由硬件中断自动调用）
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) 
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        
        // 发送完成，释放信号量，唤醒等待发送的任务
        xSemaphoreGiveFromISR(tx_sem, &xHigherPriorityTaskWoken);
        
        // 如果有任务被唤醒且优先级更高，触发任务切换
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
