#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "pgs_apps_menu.h"
#include "pgs_modules.h"
#include "pgs_utils.h"

int main(void)
{
    pgs_lvgl_init(PGS_DBUS_MENU);

    lv_indev_t * keyboard = pgs_get_keyboard();
    if(keyboard) {
        lv_indev_set_group(keyboard, lv_group_get_default());
    }

    pgs_apps_menu_init(lv_screen_active());

    while(1) {
        if(1 == pgs_get_lvgl_foreground()) {
            usleep(1000 * lv_timer_handler());
        } else {
            pgs_wait_become_foreground();
            pgs_lvgl_resume();
        }
    }

    return 0;
}
