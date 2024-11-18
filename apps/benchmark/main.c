#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "pgs_modules.h"
#include "pgs_utils.h"

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
    pgs_lvgl_init("benchmark");

    gmain = lv_group_create();
    gback = lv_group_create();
    pgs_benchmark(gmain, main_key_cb);
    pgs_backlist_init(lv_layer_top(), gback, back_key_cb);

    lv_indev_set_group(pgs_get_keyboard(), gmain);
    pgs_backlist_hidden(true, true);

    while(1) {
        usleep(1000 * lv_timer_handler());
    }

    return 0;
}
