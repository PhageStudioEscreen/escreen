#include "pgs_widgets.h"

struct pgs_widgets_capslock * pgs_widgets_capslock_create(lv_obj_t * obj, const char * base,
                                                          struct pgs_widgets_params_state * capslock)
{
    if(!base) {
        return NULL;
    }
    if(!capslock || (capslock->type != PGS_WIDGETS_TYPE_CAPSLOCK) || !capslock->enable) {
        return NULL;
    }

    struct pgs_widgets_capslock * target = lv_malloc(sizeof(struct pgs_widgets_capslock));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct pgs_widgets_capslock));

    target->_capslock = capslock;

    /* on */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/capslock_on.png", base);
    target->on                    = lv_image_create(obj);
    const lv_image_dsc_t * png_on = pgs_libpng_decode(keyboard_path_buffer);
    if(png_on) {
        lv_image_set_src(target->on, png_on);
    }
    lv_obj_align(target->on, capslock->align, 0, 0);
    lv_obj_set_pos(target->on, capslock->x, capslock->y);
    lv_obj_set_style_opa(target->on, LV_OPA_0, 0);

    /* off */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/capslock_off.png", base);
    target->off                    = lv_image_create(obj);
    const lv_image_dsc_t * png_off = pgs_libpng_decode(keyboard_path_buffer);
    if(png_off) {
        lv_image_set_src(target->off, png_off);
    }
    lv_obj_align(target->off, capslock->align, 0, 0);
    lv_obj_set_pos(target->off, capslock->x, capslock->y);
    lv_obj_set_style_opa(target->off, LV_OPA_0, 0);

    return target;
}

void pgs_widgets_capslock_set_state(struct pgs_widgets_capslock * capslock, bool on)
{
    if(!capslock || !capslock->_capslock->enable) {
        return;
    }

    if(on) {
        lv_obj_set_style_opa(capslock->off, LV_OPA_0, 0);
        lv_obj_set_style_opa(capslock->on, capslock->_capslock->opa, 0);
    } else {
        lv_obj_set_style_opa(capslock->on, LV_OPA_0, 0);
        lv_obj_set_style_opa(capslock->off, capslock->_capslock->opa, 0);
    }
}
