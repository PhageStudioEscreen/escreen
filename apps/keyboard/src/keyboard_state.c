#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include "menu.h"
#include "pgs_modules.h"
#include "pgs_utils.h"

static lv_obj_t * ui_container;
static lv_group_t * ui_group;
static void (*ui_key_cb)(uint32_t keycode);

lv_obj_t * keyboard_state_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode))
{
    ui_group  = group;
    ui_key_cb = key_cb;

    ui_container = lv_obj_create(obj);
    lv_obj_remove_style_all(ui_container);
    lv_obj_set_width(ui_container, 320);
    lv_obj_set_height(ui_container, 172);
    lv_obj_set_align(ui_container, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_container, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

}
