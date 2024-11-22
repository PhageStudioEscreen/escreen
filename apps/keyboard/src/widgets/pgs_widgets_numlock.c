#include "pgs_widgets.h"

struct pgs_widgets_numlock * pgs_widgets_numlock_create(lv_obj_t * obj, const char * base,
                                                        struct pgs_widgets_params_state * numlock)
{
    if(!base) {
        return NULL;
    }
    if(!numlock || (numlock->type != PGS_WIDGETS_TYPE_NUMLOCK) || !numlock->enable) {
        return NULL;
    }

    struct pgs_widgets_numlock * target = lv_malloc(sizeof(struct pgs_widgets_numlock));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct pgs_widgets_numlock));

    target->_numlock = numlock;

    /* on */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/numlock_on.png", base);
    target->on                    = lv_image_create(obj);
    const lv_image_dsc_t * png_on = pgs_libpng_decode(keyboard_path_buffer);
    if(png_on) {
        lv_image_set_src(target->on, png_on);
    }
    lv_obj_align(target->on, numlock->align, 0, 0);
    lv_obj_set_pos(target->on, numlock->x, numlock->y);
    lv_obj_set_style_opa(target->on, LV_OPA_0, 0);

    /* off */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/numlock_off.png", base);
    target->off                    = lv_image_create(obj);
    const lv_image_dsc_t * png_off = pgs_libpng_decode(keyboard_path_buffer);
    if(png_off) {
        lv_image_set_src(target->off, png_off);
    }
    lv_obj_align(target->off, numlock->align, 0, 0);
    lv_obj_set_pos(target->off, numlock->x, numlock->y);
    lv_obj_set_style_opa(target->off, LV_OPA_0, 0);

    return target;
}

void pgs_widgets_numlock_set_state(struct pgs_widgets_numlock * numlock, bool on)
{
    if(!numlock || !numlock->_numlock->enable) {
        return;
    }

    if(on) {
        lv_obj_set_style_opa(numlock->off, LV_OPA_0, 0);
        lv_obj_set_style_opa(numlock->on, numlock->_numlock->opa, 0);
    } else {
        lv_obj_set_style_opa(numlock->on, LV_OPA_0, 0);
        lv_obj_set_style_opa(numlock->off, numlock->_numlock->opa, 0);
    }
}
