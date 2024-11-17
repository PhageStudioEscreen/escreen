#ifndef __PGS_APPS_MENU_H__
#define __PGS_APPS_MENU_H__

#include "lvgl/lvgl.h"
#include "lvgl/src/draw/lv_image_decoder_private.h"
#include "pgs_dlist.h"

struct pgs_application
{
    struct _pgs_list list;
    const char * name;
    const char * path;
    lv_obj_t * button;
    lv_obj_t * icon;
};

lv_obj_t * pgs_apps_menu_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode));

#endif
