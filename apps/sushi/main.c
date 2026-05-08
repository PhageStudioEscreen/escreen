#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "pgs_modules.h"
#include "pgs_utils.h"
#include "../../modules/backlist/pgs_backlist.h"
#include "src/sushi_app.h"
extern void mmi_gx_magicsushi_key_2_release(void);
extern void mmi_gx_magicsushi_key_4_release(void);
extern void mmi_gx_magicsushi_key_6_down(void);
extern void mmi_gx_magicsushi_key_6_release(void);
extern void mmi_gx_magicsushi_key_8_release(void);
extern void mmi_gx_magicsushi_key_5_release(void);

static void sushi_main_log(const char *msg)
{
    FILE *f = fopen("/root/sushi.log", "a");
    if (!f) {
        return;
    }
    fprintf(f, "%s\n", msg);
    fflush(f);
    fclose(f);
}

static lv_group_t * gmain;
static lv_group_t * gback;
static bool backlist_active;

static void back_key_cb(uint32_t keycode);

static void main_key_cb(uint32_t keycode)
{
    FILE *f = fopen("/root/sushi.log", "a");
    if (f) {
        fprintf(f, "main key %u\n", (unsigned)keycode);
        fflush(f);
        fclose(f);
    }
    switch (keycode) {
        case LV_KEY_ESC:
            sushi_app_request_stop();
            GFX_STOP_BACKGROUND_SOUND(0);
            lv_indev_set_group(pgs_get_keyboard(), gback);
            pgs_backlist_hidden(false, false);
            backlist_active = true;
            break;
        default:
            sushi_app_handle_key(keycode);
            break;
    }
}

static int evdev_fd = -1;
static const char * evdev_path = "/dev/input/event3";

static void evdev_init(void)
{
    evdev_fd = open(evdev_path, O_RDONLY | O_NONBLOCK);
    if (evdev_fd < 0) {
        FILE *f = fopen("/root/sushi.log", "a");
        if (f) {
            fprintf(f, "evdev open failed %s errno=%d\n", evdev_path, errno);
            fflush(f);
            fclose(f);
        }
    }
}

static void evdev_handle_key(uint16_t code, int32_t value)
{
    FILE *f = fopen("/root/sushi.log", "a");
    if (f) {
        fprintf(f, "ev key code=%u\n", (unsigned)code);
        fflush(f);
        fclose(f);
    }
    if (backlist_active) {
        return;
    }

    if (value != 1) {
        return;
    }

    switch (code) {
        case KEY_UP:
            mmi_gx_magicsushi_key_2_release();
            break;
        case KEY_DOWN:
            mmi_gx_magicsushi_key_8_release();
            break;
        case KEY_LEFT:
            mmi_gx_magicsushi_key_4_release();
            break;
        case KEY_RIGHT:
            mmi_gx_magicsushi_key_6_down();
            mmi_gx_magicsushi_key_6_release();
            break;
        case KEY_ENTER:
        case KEY_KPENTER:
        case KEY_OK:
        case KEY_SELECT:
            mmi_gx_magicsushi_key_5_release();
            break;
        default:
            break;
    }
}

static void back_key_cb(uint32_t keycode)
{
    switch (keycode) {
        case LV_KEY_ESC:
            sushi_app_request_resume();
            lv_indev_set_group(pgs_get_keyboard(), gmain);
            pgs_backlist_hidden(true, false);
            backlist_active = false;
            break;
        default:
            break;
    }
}

int main(void)
{
    sushi_main_log("main start");
    pgs_lvgl_init("sushi");
    sushi_main_log("after pgs_lvgl_init");

    srand((unsigned)time(NULL));

    gmain = lv_group_create();
    gback = lv_group_create();

    pgs_backlist_init(lv_layer_top(), gback, back_key_cb);

    if (!sushi_app_init(lv_screen_active(), gmain, main_key_cb)) {
        sushi_main_log("sushi_app_init failed");
        return 1;
    }
    sushi_main_log("after sushi_app_init");

    backlist_active = false;

    evdev_init();

    lv_indev_set_group(pgs_get_keyboard(), gmain);
    pgs_backlist_hidden(true, true);

    while (1) {
        if (evdev_fd >= 0) {
            struct input_event ev;
            ssize_t n;
            while ((n = read(evdev_fd, &ev, sizeof(ev))) == sizeof(ev)) {
                if (ev.type == EV_KEY) {
                    evdev_handle_key(ev.code, ev.value);
                }
            }
        }
        sushi_app_update();
        uint32_t wait_ms = lv_timer_handler();
        if (wait_ms < 1) wait_ms = 1;
        if (wait_ms > 5) wait_ms = 5;
#ifdef _WIN32
        Sleep(wait_ms);
#else
        usleep(1000 * wait_ms);
#endif
    }

    return 0;
}
