#ifndef __UI_H
#define __UI_H

#include "lvgl.h"

void UI_Init(void);
//由于UI 抢占问题，串口数据“有时很短就跳回led计数”，舍弃，增加分屏控制
//void UI_UpdateUI(const char *text);
// 分开lvgl接口，避免互相覆盖
void UI_UpdateLedCount(uint32_t count);
void UI_UpdateUartText(const char *text);
void UI_UpdateMonitor(const char *cpu_stats, const char *stack_info);

#endif