#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

#include "pgs_modules.h"
#include "pgs_utils.h"
#include "pgs_app_keyboard.h"

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

static uint32_t randomseed(void)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if(fd < 0) {
        perror("Failed to open /dev/urandom");
        return 0;
    }

    uint32_t random_number;
    ssize_t result = read(fd, &random_number, sizeof(random_number));
    if(result != sizeof(random_number)) {
        perror("Failed to read from /dev/urandom");
        close(fd);
        return 0;
    }

    close(fd);

    return random_number;
}

int main(void)
{
    pgs_lvgl_init("keyboard");

    srand(randomseed());

    gmain = lv_group_create();
    gback = lv_group_create();

    keyboard_hidraw_init();

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), LV_PART_MAIN);

    pgs_backlist_init(lv_layer_top(), gback, back_key_cb);
    pgs_app_keyboard_init(lv_screen_active(), gmain, main_key_cb);

    lv_indev_set_group(pgs_get_keyboard(), gmain);
    pgs_backlist_hidden(true, true);

    while(1) {
        pgs_app_keyboard_update();
        usleep(1000 * lv_timer_handler());
    }

    return 0;
}
