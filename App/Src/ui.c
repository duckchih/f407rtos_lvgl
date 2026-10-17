#include "ui.h"
#include "lv_port_disp.h"

static lv_obj_t * status_label = NULL;

void UI_Init(void)
{
    lv_init();
    lv_port_disp_init();

    // 黑底
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);

    // 创建标签
    status_label = lv_label_create(lv_scr_act());
    lv_label_set_text(status_label, "System Boot...");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_18, 0); // 假设你用的是18号
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 0);
}

/* 留给 App 层调用的接口：修改屏幕上的字 */
void UI_UpdateText(const char *text)
{
    if (status_label != NULL) {
        lv_label_set_text(status_label, text);
    }
}
