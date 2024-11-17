#ifndef __PGS_BACKLIST_H__
#define __PGS_BACKLIST_H__

#include <stdbool.h>
#include "pgs_modules.h"
#include "lvgl/lvgl.h"
#include "pgs_dlist.h"

struct pgs_backlist_item
{
    struct _pgs_list list;
    const char * text_src;
    const void * icon_src;
    lv_event_cb_t event_cb;
    lv_obj_t * container;
    lv_obj_t * text;
    lv_obj_t * icon;
};

lv_obj_t * pgs_backlist_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode));
lv_obj_t * pgs_backlist_init_nomenu(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode));
void pgs_backlist_add_item(const char * text, void * icon, lv_event_cb_t event_cb);
void pgs_backlist_hidden(bool hidden, bool noanim);

#endif
