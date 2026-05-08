#ifndef FLAPPYBIRD_APP_H
#define FLAPPYBIRD_APP_H

#include <stdint.h>

#include "lvgl/lvgl.h"

lv_obj_t * flappybird_app_init(lv_obj_t * parent, lv_group_t * group, void (*key_cb)(uint32_t keycode));
void flappybird_app_update(void);
void flappybird_app_handle_key(uint32_t keycode);

#endif
