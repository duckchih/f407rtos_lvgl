#include "lv_port_disp.h"
#include "lcd.h"
#include "FreeRTOS.h"
#include "task.h"

/* 局部缓冲区：因为 STM32 RAM 有限，不能把整个屏幕都放进 RAM。
 * 这里给 172*40 个像素，大约是 13KB RAM */
#define MY_DISP_HOR_RES 172
#define MY_DISP_VER_RES 320
#define DISP_BUF_SIZE   (MY_DISP_HOR_RES * 40)

static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;

/* 这个是 LVGL 的核心回调函数：
 * 当 LVGL 画好一块区域后，会调用这个函数，让你发送给屏幕 */
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    // 1. 设置屏幕的显示窗口（之前写的函数）
    LCD_SetWindow(area->x1, area->y1, area->x2, area->y2);

    // 2. 直接拉低片选，进入写显存模式
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);

    // 3. 计算总字节数并发送
    uint32_t size = lv_area_get_size(area);
    // color_p 是 lv_color_t 数组，每个占 2 bytes，所以要 * 2
    HAL_SPI_Transmit(&hspi1, (uint8_t *)color_p, size * 2, HAL_MAX_DELAY);
    
    // 4. 恢复片选
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);

    // 5. 告诉 LVGL 这一块刷完了（非常重要！）
    lv_disp_flush_ready(disp_drv);
}

void lv_port_disp_init(void)
{
    /* 1. 初始化画布缓冲区 */
    static lv_color_t buf_1[DISP_BUF_SIZE];
    lv_disp_draw_buf_init(&draw_buf, buf_1, NULL, DISP_BUF_SIZE);

    /* 2. 注册显示驱动 */
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;

    /* 3. 绑定驱动到 LVGL */
    lv_disp_drv_register(&disp_drv);
}