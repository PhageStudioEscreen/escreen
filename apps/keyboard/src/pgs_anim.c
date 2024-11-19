#include "lvgl/lvgl.h"
#include "pgs_anim.h"

void pgs_anim_callback_free_user_data(lv_anim_t * a)
{
    lv_free(a->user_data);
    a->user_data = NULL;
}

void pgs_anim_callback_set_x(lv_anim_t * a, int32_t v)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    lv_obj_set_x(usr->target, v);
}

void pgs_anim_callback_set_y(lv_anim_t * a, int32_t v)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    lv_obj_set_y(usr->target, v);
}

void pgs_anim_callback_set_width(lv_anim_t * a, int32_t v)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    lv_obj_set_width(usr->target, v);
}

void pgs_anim_callback_set_height(lv_anim_t * a, int32_t v)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    lv_obj_set_height(usr->target, v);
}

void pgs_anim_callback_set_opacity(lv_anim_t * a, int32_t v)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    lv_obj_set_style_opa(usr->target, v, 0);
}

void pgs_anim_callback_set_image_zoom(lv_anim_t * a, int32_t v)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    lv_img_set_zoom(usr->target, v);
}

void pgs_anim_callback_set_image_angle(lv_anim_t * a, int32_t v)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    lv_img_set_angle(usr->target, v);
}

void pgs_anim_callback_set_image_frame(lv_anim_t * a, int32_t v)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    usr->val                   = v;

    if(v < 0) v = 0;
    if(v >= usr->imgset_size) v = usr->imgset_size - 1;
    lv_img_set_src(usr->target, usr->imgset[v]);
}

int32_t pgs_anim_callback_get_x(lv_anim_t * a)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    return lv_obj_get_x_aligned(usr->target);
}

int32_t pgs_anim_callback_get_y(lv_anim_t * a)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    return lv_obj_get_y_aligned(usr->target);
}

int32_t pgs_anim_callback_get_width(lv_anim_t * a)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    return lv_obj_get_width(usr->target);
}

int32_t pgs_anim_callback_get_height(lv_anim_t * a)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    return lv_obj_get_height(usr->target);
}

int32_t pgs_anim_callback_get_opacity(lv_anim_t * a)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    return lv_obj_get_style_opa(usr->target, 0);
}

int32_t pgs_anim_callback_get_image_zoom(lv_anim_t * a)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    return lv_img_get_zoom(usr->target);
}

int32_t pgs_anim_callback_get_image_angle(lv_anim_t * a)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    return lv_img_get_angle(usr->target);
}

int32_t pgs_anim_callback_get_image_frame(lv_anim_t * a)
{
    pgs_anim_user_data_t * usr = (pgs_anim_user_data_t *)a->user_data;
    return usr->val;
}

void pgs_animation_blink_fade(lv_obj_t * obj, uint32_t on, uint32_t off, uint32_t count)
{
    pgs_anim_user_data_t * usr = lv_malloc(sizeof(pgs_anim_user_data_t));
    usr->target                = obj;
    usr->val                   = -1;

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_duration(&anim, on);
    lv_anim_set_user_data(&anim, usr);
    lv_anim_set_custom_exec_cb(&anim, pgs_anim_callback_set_opacity);
    lv_anim_set_values(&anim, LV_OPA_0, LV_OPA_100);
    lv_anim_set_path_cb(&anim, lv_anim_path_linear);
    lv_anim_set_delay(&anim, 0);
    lv_anim_set_deleted_cb(&anim, pgs_anim_callback_free_user_data);
    lv_anim_set_playback_duration(&anim, off);
    lv_anim_set_playback_delay(&anim, 0);
    lv_anim_set_repeat_count(&anim, count);
    lv_anim_set_repeat_delay(&anim, 0);
    lv_anim_set_early_apply(&anim, false);
    lv_anim_set_get_value_cb(&anim, pgs_anim_callback_get_opacity);
    lv_anim_start(&anim);
}

void pgs_animation_stop(lv_obj_t * obj)
{
    lv_anim_delete(obj, NULL);
}
