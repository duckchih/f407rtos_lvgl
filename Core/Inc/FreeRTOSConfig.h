#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f4xx_hal.h"   /* 为了拿到 configPRIO_BITS */

/* ============ 调度器配置 ============ */
#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configCPU_CLOCK_HZ                      168000000UL   /* 168MHz */
#define configTICK_RATE_HZ                      1000          /* 1ms 一个 tick */
#define configUSE_16_BIT_TICKS                  0
#define configMAX_PRIORITIES                    7
#define configMINIMAL_STACK_SIZE                128           /* 单位：word，128*4=512B */
#define configTOTAL_HEAP_SIZE                   (32 * 1024)   /* 32KB */
#define configMAX_TASK_NAME_LEN                 16

/* ============ 中断优先级（关键） ============ */
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* ============ 可选功能开关 ============ */
#define configUSE_TICKLESS_IDLE                 0
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0    /* 现在不用，LVGL 阶段再打开 */
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         0
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_TIMERS                        0    /* 先不开软件定时器 */

/* ============ INCLUDE_xxx 开关 ============ */
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xTaskGetCurrentTaskHandle       1

/* ============ 断言：出错就停 ============ */
#define configASSERT(x)  if ((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;); }


/* ============ 中断向量映射 ============ */
#define vPortSVCHandler         SVC_Handler
#define xPortPendSVHandler      PendSV_Handler
/* xPortSysTickHandler 不在这里定义，因为我们手动在 stm32f4xx_it.c 里调用 */

#endif /* FREERTOS_CONFIG_H */