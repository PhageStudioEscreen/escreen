#ifndef __PGS_APP_SETTING_H__
#define __PGS_APP_SETTING_H__

#include "lvgl/lvgl.h"

lv_obj_t * pgs_app_setting_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode));
void pgs_app_setting_update(void);

#endif
