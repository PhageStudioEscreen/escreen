#ifndef __PGS_ANIM_H__
#define __PGS_ANIM_H__

#include "lvgl/lvgl.h"

typedef struct
{
    lv_obj_t * target;
    lv_img_dsc_t ** imgset;
    int32_t imgset_size;
    int32_t val;
} pgs_anim_user_data_t;

void pgs_anim_callback_free_user_data(lv_anim_t * a);

void pgs_anim_callback_set_x(lv_anim_t * a, int32_t v);
void pgs_anim_callback_set_y(lv_anim_t * a, int32_t v);
void pgs_anim_callback_set_width(lv_anim_t * a, int32_t v);
void pgs_anim_callback_set_height(lv_anim_t * a, int32_t v);
void pgs_anim_callback_set_opacity(lv_anim_t * a, int32_t v);
void pgs_anim_callback_set_image_zoom(lv_anim_t * a, int32_t v);
void pgs_anim_callback_set_image_angle(lv_anim_t * a, int32_t v);
void pgs_anim_callback_set_image_frame(lv_anim_t * a, int32_t v);

int32_t pgs_anim_callback_get_x(lv_anim_t * a);
int32_t pgs_anim_callback_get_y(lv_anim_t * a);
int32_t pgs_anim_callback_get_width(lv_anim_t * a);
int32_t pgs_anim_callback_get_height(lv_anim_t * a);
int32_t pgs_anim_callback_get_opacity(lv_anim_t * a);
int32_t pgs_anim_callback_get_image_zoom(lv_anim_t * a);
int32_t pgs_anim_callback_get_image_angle(lv_anim_t * a);
int32_t pgs_anim_callback_get_image_frame(lv_anim_t * a);

void pgs_animation_stop(lv_obj_t * obj);
void pgs_animation_blink_none(lv_obj_t * obj, uint32_t on, uint32_t off, uint32_t count);
void pgs_animation_blink_fade(lv_obj_t * obj, uint32_t on, uint32_t off, uint32_t count);

#endif
