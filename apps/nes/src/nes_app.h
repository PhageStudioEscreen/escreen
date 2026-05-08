#ifndef __NES_APP_H__
#define __NES_APP_H__

#include "lvgl/lvgl.h"

lv_obj_t * nes_app_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode));
void nes_app_update(void);
void nes_app_handle_key(uint32_t keycode);
void nes_app_request_pause(void);
void nes_app_request_resume(void);
void nes_app_request_stop(void);
void nes_app_set_rom(const char * rom_path);

#endif
