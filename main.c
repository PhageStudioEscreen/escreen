#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

static const char * getenv_default(const char * name, const char * dflt)
{
    return getenv(name) ?: dflt;
}

#if LV_USE_LINUX_FBDEV
static void lv_linux_disp_init(void)
{
    const char * device = getenv_default("PGS_ESCREEN_FBDEV_DEVICE", "/dev/fb0");
    lv_display_t * disp = lv_linux_fbdev_create();
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);

    lv_linux_fbdev_set_file(disp, device);
}
#elif LV_USE_LINUX_DRM
static void lv_linux_disp_init(void)
{
    const char * device = getenv_default("PGS_ESCREEN_DRM_CARD", "/dev/dri/card0");
    lv_display_t * disp = lv_linux_drm_create();
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);

    lv_linux_drm_set_file(disp, device, -1);
}
#elif LV_USE_SDL
static void lv_linux_disp_init(void)
{
    const int width  = atoi(getenv("PGS_ESCREEN_SDL_VIDEO_WIDTH") ?: "800");
    const int height = atoi(getenv("PGS_ESCREEN_SDL_VIDEO_HEIGHT") ?: "480");

    lv_sdl_window_create(width, height);
}
#else
#error Unsupported configuration
#endif

static lv_indev_t * indev_keyboard;
static lv_indev_t * indev_mouse;

static void lv_linux_input_init(void)
{
    const char * indev_keyboard_path = getenv_default("PGS_ESCREEN_KEYBOARD", "/dev/input/event1");
    const char * indev_mouse_path    = getenv_default("PGS_ESCREEN_MOUSE", "/dev/input/event2");

    indev_keyboard = lv_libinput_create(LV_INDEV_TYPE_KEYPAD, indev_keyboard_path);
    if(!indev_keyboard) {
        printf("Unable to open device for keyboard on %s\n", indev_keyboard_path);
    } else {
        printf("Open device %s\n", indev_keyboard_path);
    }

    indev_mouse = lv_libinput_create(LV_INDEV_TYPE_POINTER, indev_mouse_path);
    if(!indev_mouse) {
        printf("Unable to open device for mouse on %s\n", indev_mouse_path);
    } else {
        printf("Open device %s\n", indev_mouse_path);
        LV_IMG_DECLARE(mouse_cursor_icon);
        lv_obj_t * cursor_obj = lv_image_create(lv_screen_active());
        lv_image_set_src(cursor_obj, &mouse_cursor_icon);
        lv_indev_set_cursor(indev_mouse, cursor_obj);
    }
}

int main(void)
{
    lv_init();

    /*Linux display device init*/
    lv_linux_disp_init();
    lv_linux_input_init();
    /*Create a Demo*/
    // lv_demo_widgets();
    // lv_demo_widgets_start_slideshow();
    // lv_example_ffmpeg_1();
    // lv_example_ffmpeg_2();

    lv_demo_music();

    /*Handle LVGL tasks*/
    while(1) {
        usleep(1000 * lv_timer_handler());
    }

    return 0;
}
