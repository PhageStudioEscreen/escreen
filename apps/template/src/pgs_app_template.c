#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <stdlib.h>
#include "pgs_modules.h"
#include "pgs_utils.h"
#include "pgs_app_template.h"

static lv_obj_t * ui_container;
static lv_group_t * ui_group;
static void (*ui_key_cb)(uint32_t keycode);
char keyboard_path_buffer[PATH_MAX];

static void key_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if(code == LV_EVENT_KEY) {
        if(ui_key_cb) {
            ui_key_cb(lv_indev_get_key(lv_indev_active()));
        }
    } else if(code == LV_EVENT_CLICKED) {
    }
}

lv_obj_t * pgs_app_template_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode))
{
    ui_group  = group;
    ui_key_cb = key_cb;

    ui_container = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(ui_container);
    lv_obj_set_width(ui_container, lv_pct(100));
    lv_obj_set_height(ui_container, lv_pct(100));
    lv_obj_set_align(ui_container, LV_ALIGN_TOP_LEFT);
    lv_group_add_obj(ui_group, ui_container);
    lv_obj_add_event_cb(ui_container, key_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_container, key_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_clear_flag(ui_container, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    return ui_container;
}

void pgs_app_template_update(void)
{}
