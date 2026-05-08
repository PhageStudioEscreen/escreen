#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "pgs_modules.h"
#include "pgs_utils.h"
#include "src/flappybird_app.h"

static lv_group_t * gmain;
static lv_group_t * gback;

static void main_key_cb(uint32_t keycode)
{
    switch (keycode) {
        case LV_KEY_ESC:
            lv_indev_set_group(pgs_get_keyboard(), gback);
            pgs_backlist_hidden(false, false);
            break;
        default:
            flappybird_app_handle_key(keycode);
            break;
    }
}

static void back_key_cb(uint32_t keycode)
{
    if (keycode == LV_KEY_ESC) {
        lv_indev_set_group(pgs_get_keyboard(), gmain);
        pgs_backlist_hidden(true, false);
    }
}

static void sleep_ms(uint32_t ms)
{
    usleep(ms * 1000U);
}

int main(void)
{
    pgs_lvgl_init("flappybird");
    srand((unsigned int)time(NULL));

    gmain = lv_group_create();
    gback = lv_group_create();

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), LV_PART_MAIN);
    pgs_backlist_init(lv_layer_top(), gback, back_key_cb);

    flappybird_app_init(lv_screen_active(), gmain, main_key_cb);

    lv_indev_set_group(pgs_get_keyboard(), gmain);
    pgs_backlist_hidden(true, true);

    while (1) {
        uint32_t wait_ms;

        flappybird_app_update();
        wait_ms = lv_timer_handler();
        if (wait_ms < 1U) {
            wait_ms = 1U;
        }
        if (wait_ms > 5U) {
            wait_ms = 5U;
        }
        sleep_ms(wait_ms);
    }

    return 0;
}
