#ifndef __KEYBOARD_STATE_H__
#define __KEYBOARD_STATE_H__

#include "lvgl/lvgl.h"
#include "pgs_kbd_params.h"

lv_obj_t * keyboard_state_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode));

#endif
