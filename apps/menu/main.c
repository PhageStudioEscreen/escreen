#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <sys/syscall.h>

#include "pgs_modules.h"
#include "pgs_utils.h"
#include "menu.h"

static lv_group_t * gmain;
static lv_group_t * gback;

static void reboot_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if(code == LV_EVENT_CLICKED) {
        syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART, NULL);
    }
}

static void fastboot_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if(code == LV_EVENT_CLICKED) {
        syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART2, "loader");
    }
}

static void menu_key_cb(uint32_t keycode)
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
    pgs_lvgl_init(PGS_DBUS_MENU);

    gmain = lv_group_create();
    gback = lv_group_create();
    pgs_apps_menu_init(lv_screen_active(), gmain, menu_key_cb);
    pgs_backlist_init_nomenu(lv_layer_top(), gback, back_key_cb);

    LV_IMG_DECLARE(icont_reboot);
    pgs_backlist_add_item("REBOOT", &icont_reboot, reboot_event_cb, NULL);

    LV_IMG_DECLARE(icont_fastboot);
    pgs_backlist_add_item("LOADER", &icont_fastboot, fastboot_event_cb, NULL);

    lv_indev_set_group(pgs_get_keyboard(), gmain);
    pgs_backlist_hidden(true, true);

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
