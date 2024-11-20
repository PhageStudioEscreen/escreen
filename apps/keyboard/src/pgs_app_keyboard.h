#ifndef __PGS_APP_KEYBOARD_H__
#define __PGS_APP_KEYBOARD_H__

#include "lvgl/lvgl.h"
#include "keyboard_params.h"
#include "pgs_widgets.h"

struct pgs_app_keyboard
{
    struct pgs_widgets_macro * macro;
    struct pgs_widgets_layer * layer;
    struct pgs_widgets_capslock * capslock;
    struct pgs_widgets_numlock * numlock;
    struct pgs_widgets_output * ble1;
    struct pgs_widgets_output * ble2;
    struct pgs_widgets_output * ble3;
    struct pgs_widgets_output * g24;
    struct pgs_widgets_output * usb;
    struct pgs_widgets_output * scr;
    struct pgs_widgets_bat * bat;
    struct pgs_widgets_keyroll * keyroll;
    struct pgs_widgets_apmchart * apmchart;
};

extern struct pgs_app_keyboard keyboard_inst;

lv_obj_t * pgs_app_keyboard_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode));

#endif
