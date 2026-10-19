#include "ui.h"
#include "lv_port_disp.h"
#include "stdio.h"
// static lv_obj_t * status_label = NULL;
static lv_obj_t * led_label = NULL;
static lv_obj_t * uart_label = NULL;
static lv_obj_t * monitor_label = NULL;

void UI_Init(void)
{
    lv_init();
    lv_port_disp_init();

    // 黑底
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);

    // 1. 创建 LED 计数标签（放上面）
    led_label = lv_label_create(lv_scr_act());
    lv_label_set_text(led_label, "LED Count:\n0");
    lv_obj_set_style_text_color(led_label, lv_color_hex(0x00FF00), 0); // 绿色
    lv_obj_set_style_text_font(led_label, &lv_font_montserrat_18, 0);
    lv_obj_align(led_label, LV_ALIGN_TOP_MID, 0, 20);

    // 2. 创建 UART 接收标签（放下面）
    uart_label = lv_label_create(lv_scr_act());
    lv_label_set_text(uart_label, "UART Wait...");
    lv_obj_set_style_text_color(uart_label, lv_color_hex(0xFFFFFF), 0); // 白色
    lv_obj_set_style_text_font(uart_label, &lv_font_montserrat_18, 0);
    lv_obj_align(uart_label, LV_ALIGN_TOP_MID, 0, 60);

    monitor_label = lv_label_create(lv_scr_act());
    lv_label_set_text(monitor_label, "Stats...");
    lv_obj_set_style_text_color(monitor_label, lv_color_hex(0xFFFF00), 0); // 黄色
    lv_obj_set_style_text_font(monitor_label, &lv_font_montserrat_14, 0);
    lv_obj_align(monitor_label, LV_ALIGN_BOTTOM_LEFT, 1, -60);
}

// 只更新 LED 标签
void UI_UpdateLedCount(uint32_t count)
{
    if (led_label != NULL) {
        char str[32];
        snprintf(str, sizeof(str), "LED Count:\n%lu", count);
        lv_label_set_text(led_label, str);
    }
}
// 只更新串口标签
void UI_UpdateUartText(const char *text)
{
    if (uart_label != NULL) {
        lv_label_set_text(uart_label, text);
    }
}
// Monitor标签更新
void UI_UpdateMonitor(const char *cpu_stats, const char *stack_info)
{
    if (monitor_label != NULL) {
        char final_buf[384];
        // 把栈剩余和 CPU 统计拼在一起
        snprintf(final_buf, sizeof(final_buf), "%s\n%s", stack_info, cpu_stats);
        lv_label_set_text(monitor_label, final_buf);
    }
}


