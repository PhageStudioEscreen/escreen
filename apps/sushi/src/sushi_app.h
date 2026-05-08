#ifndef __SUSHI_APP_H__
#define __SUSHI_APP_H__

#include <stdint.h>

typedef struct lv_obj_t lv_obj_t;
typedef struct lv_group_t lv_group_t;

lv_obj_t * sushi_app_init(lv_obj_t * parent, lv_group_t * group, void (*key_cb)(uint32_t keycode));
void sushi_app_update(void);
void sushi_app_handle_key(uint32_t keycode);
void sushi_app_request_pause(void);
void sushi_app_request_resume(void);
void sushi_app_request_stop(void);

#endif
