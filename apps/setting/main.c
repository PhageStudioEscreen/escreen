#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "pgs_modules.h"
#include "pgs_utils.h"
#include "pgs_app_setting.h"

static lv_group_t * gmain;
static lv_group_t * gback;

static void main_key_cb(uint32_t keycode)
{

    switch(keycode) {
        case LV_KEY_ESC:
            lv_indev_set_group(pgs_get_keyboard(), gback);
            pgs_backlist_hidden(false, false);
            break;
        default: break;
    }
}

static void back_key_cb(uint32_t keycode)
{
    switch(keycode) {
        case LV_KEY_ESC: lv_indev_set_group(pgs_get_keyboard(), gmain); break;
        default: break;
    }
}

int main(void)
{
    pgs_lvgl_init("setting");

    gmain = lv_group_create();
    gback = lv_group_create();

    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), LV_PART_MAIN);

    pgs_backlist_init(lv_layer_top(), gback, back_key_cb);
    pgs_app_setting_init(lv_screen_active(), gmain, main_key_cb);

    lv_indev_set_group(pgs_get_keyboard(), gmain);
    pgs_backlist_hidden(true, true);

    /* 设备 */
    /* WIFI */
    /* 蓝牙 */
    /* 锁屏 */
    /* 显示 */
    /* 声音 */
    /* 应用 */
    /* 更多 */

    while(1) {
        pgs_app_setting_update();
        usleep(1000 * lv_timer_handler());
    }

    return 0;
}
