#include "lvgl/lvgl.h"
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include "pgs_modules.h"
#include "pgs_utils.h"

static const char * getenv_default(const char * name, const char * dflt)
{
    return getenv(name) ?: dflt;
}

static lv_display_t * display;
#if LV_USE_LINUX_FBDEV
static void lv_linux_disp_init(void)
{
    const char * device = getenv_default("PGS_FBDEV_DEVICE", "/dev/fb0");
    display             = lv_linux_fbdev_create();
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);
    lv_linux_fbdev_set_file(display, device);
    lv_display_set_default(display);
}
#elif LV_USE_LINUX_DRM

static void lv_linux_disp_init(void)
{
    const char * device = getenv_default("PGS_DRM_CARD", "/dev/dri/card0");
    display             = lv_linux_drm_create();
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_270);
    lv_linux_drm_set_file(display, device, -1);
    lv_display_set_default(display);
}
#else
#error Unsupported configuration
#endif

static lv_indev_t * indev_keyboard;
static lv_indev_t * indev_mouse;

static void lv_linux_input_init(void)
{
    const char * indev_keyboard_path = getenv_default("PGS_KEYBOARD", "/dev/input/event2");
    const char * indev_mouse_path    = getenv_default("PGS_MOUSE", "/dev/input/event3");
    lv_group_t * g                   = lv_group_create();

    lv_group_set_default(g);

    indev_keyboard = lv_libinput_create(LV_INDEV_TYPE_KEYPAD, indev_keyboard_path);
    if(!indev_keyboard) {
        printf("Unable to open device for keyboard on %s\n", indev_keyboard_path);
    }

    indev_mouse = lv_libinput_create(LV_INDEV_TYPE_POINTER, indev_mouse_path);
    if(!indev_mouse) {
        printf("Unable to open device for mouse on %s\n", indev_mouse_path);
    } else {
        LV_IMG_DECLARE(mouse_cursor_icon);
        lv_obj_t * cursor_obj = lv_image_create(lv_screen_active());
        lv_image_set_src(cursor_obj, &mouse_cursor_icon);
        lv_indev_set_cursor(indev_mouse, cursor_obj);
    }
}

static char pgs_name[128];
static char pgs_path[128];

static void redirection_ttyFIQ0(void)
{
    const char * tty_device = "/dev/ttyFIQ0";
    int fd                  = open(tty_device, O_WRONLY);
    if(fd < 0) {
        perror("Failed to open TTY");
        return;
    }
    if(dup2(fd, STDOUT_FILENO) < 0) {
        perror("Failed to redirect stdout");
        close(fd);
        return;
    }
    if(dup2(fd, STDERR_FILENO) < 0) {
        perror("Failed to redirect stderr");
        close(fd);
        return;
    }
    close(fd);

    setvbuf(stdout, NULL, _IOLBF, 0);
}

void pgs_cleanup(void)
{
    pgs_lvgl_suspend();

    /* wake up menu, then kill */
    pgs_dbus_method_call("com.pgsapp.menu", "/com/pgsapp/menu", 1, getpid());

    while(1) {
        /* wait for kill */
        sleep(1);
    }
}

static void signal_handler(int signum)
{
    pgs_cleanup();
}

static void redirection_signal(void)
{
    if(atexit(pgs_cleanup) != 0) {
        pgs_cleanup();
    }
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

void pgs_lvgl_init(const char * name)
{
    redirection_ttyFIQ0();

    /* if not menu, register exit callback */
    if(strncmp(name, PGS_DBUS_MENU, sizeof(PGS_DBUS_MENU - 1))) {
        printf("Initialization Application %s\n", name);
        redirection_signal();
    } else {
        printf("Initialization Desktop\n");
    }

    lv_init();

    lv_memzero(pgs_name, sizeof(pgs_name));
    lv_memzero(pgs_path, sizeof(pgs_path));

    lv_snprintf(pgs_name, sizeof(pgs_name), PGS_DBUS_NAME_PREFIX "%s", name);
    lv_snprintf(pgs_path, sizeof(pgs_path), PGS_DBUS_PATH_PREFIX "%s", name);

    pgs_dbus_handler_init(pgs_name, pgs_path);

    /* if not menu, make menu suspend */
    if(strncmp(name, PGS_DBUS_MENU, sizeof(PGS_DBUS_MENU - 1))) {
        pgs_dbus_method_call(PGS_DBUS_MENU_NAME, PGS_DBUS_MENU_PATH, 0, 0);
        pgs_wait_become_foreground();
    }

    lv_linux_disp_init();
    lv_linux_input_init();
}

void pgs_lvgl_resume(void)
{
    lv_linux_disp_init();
}

void pgs_lvgl_suspend(void)
{
    /* release display */
    lv_linux_drm_destory(pgs_get_display());
}

const char * pgs_get_name(void)
{
    return pgs_name;
}

const char * pgs_get_path(void)
{
    return pgs_path;
}

lv_display_t * pgs_get_display(void)
{
    return display;
}

lv_indev_t * pgs_get_keyboard(void)
{
    return indev_keyboard;
}

lv_indev_t * pgs_get_mouse(void)
{
    return indev_mouse;
}
