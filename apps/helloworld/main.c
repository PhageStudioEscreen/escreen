#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "pgs_modules.h"
#include "pgs_utils.h"
#include "audio_player.h"

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

static uint32_t generate_random_color(void)
{
    uint8_t r = rand() % 256;
    uint8_t g = rand() % 256;
    uint8_t b = rand() % 256;

    return (r << 16) | (g << 8) | b;
}

static uint32_t generate_contrast_color(uint32_t color)
{
    uint8_t r = (color >> 16) & 0xff;
    uint8_t g = (color >> 8) & 0xff;
    uint8_t b = (color >> 0) & 0xff;

    return ((255 - r) << 16) | ((255 - g) << 8) | (255 - b);
}

static void apps_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if(code == LV_EVENT_CLICKED) {
        uint32_t fcolor = generate_random_color();
        uint32_t bcolor = generate_contrast_color(fcolor);
        lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(fcolor), LV_PART_MAIN);
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(bcolor), LV_PART_MAIN);
    } else if(code == LV_EVENT_KEY) {
        main_key_cb(lv_indev_get_key(lv_indev_active()));
    }
}

int main(void)
{
    pgs_lvgl_init("helloworld");

    gmain = lv_group_create();
    gback = lv_group_create();

    pgs_backlist_init(lv_layer_top(), gback, back_key_cb);

    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t * ui_container = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(ui_container);
    lv_obj_set_width(ui_container, lv_pct(100));
    lv_obj_set_height(ui_container, lv_pct(100));
    lv_obj_set_align(ui_container, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_container, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * label = lv_label_create(ui_container);
    lv_label_set_text(label, "HELLO WORLD");
    lv_obj_set_style_text_font(label, &lv_font_helveticarounded_28, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(label, apps_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(label, apps_event_cb, LV_EVENT_KEY, NULL);

    lv_group_add_obj(gmain, label);

    lv_indev_set_group(pgs_get_keyboard(), gmain);
    pgs_backlist_hidden(true, true);

    const char *mp3_list = getenv("PGS_MP3_LIST");
    if (mp3_list && *mp3_list) {
        audio_player_start_list(mp3_list);
    } else {
        const char *mp3_path = getenv("PGS_MP3_PATH");
        if (!mp3_path || !*mp3_path)
            mp3_path = "/usr/share/pgs/apps/helloworld/test.mp3";
        audio_player_start(mp3_path);
    }

    while(1) {
        usleep(1000 * lv_timer_handler());
    }

    return 0;
}
