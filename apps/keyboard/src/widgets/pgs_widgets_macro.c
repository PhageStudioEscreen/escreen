#include "pgs_widgets.h"

struct pgs_widgets_macro * pgs_widgets_macro_create(lv_obj_t * obj, const char * base,
                                                    struct pgs_widgets_params_state * macro,
                                                    struct pgs_widgets_params_state * macro1,
                                                    struct pgs_widgets_params_state * macro2)
{
    if(!base) {
        return NULL;
    }
    if(!macro || (macro->type != PGS_WIDGETS_TYPE_MACRO) || !macro->enable) {
        return NULL;
    }
    if(!macro1 || (macro1->type != PGS_WIDGETS_TYPE_MACRO1) || !macro1->enable) {
        return NULL;
    }
    if(!macro2 || (macro2->type != PGS_WIDGETS_TYPE_MACRO2) || !macro2->enable) {
        return NULL;
    }

    struct pgs_widgets_macro * target = lv_malloc(sizeof(struct pgs_widgets_macro));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct pgs_widgets_macro));

    target->_macro  = macro;
    target->_macro1 = macro1;
    target->_macro2 = macro2;

    /* macro pause */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/macro_pause.png", base);
    target->macro_pause                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro_pause = pgs_libpng_decode(keyboard_path_buffer);
    if(png_macro_pause) {
        lv_image_set_src(target->macro_pause, png_macro_pause);
    }
    lv_obj_align(target->macro_pause, macro->align, 0, 0);
    lv_obj_set_pos(target->macro_pause, macro->x, macro->y);
    lv_obj_set_style_opa(target->macro_pause, LV_OPA_0, 0);

    /* macro play */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/macro_play.png", base);
    target->macro_play                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro_play = pgs_libpng_decode(keyboard_path_buffer);
    if(png_macro_play) {
        lv_image_set_src(target->macro_play, png_macro_play);
    }
    lv_obj_align(target->macro_play, macro->align, 0, 0);
    lv_obj_set_pos(target->macro_play, macro->x, macro->y);
    lv_obj_set_style_opa(target->macro_play, LV_OPA_0, 0);

    /* macro record */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/macro_record.png", base);
    target->macro_record                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro_record = pgs_libpng_decode(keyboard_path_buffer);
    if(png_macro_record) {
        lv_image_set_src(target->macro_record, png_macro_record);
    }
    lv_obj_align(target->macro_record, macro->align, 0, 0);
    lv_obj_set_pos(target->macro_record, macro->x, macro->y);
    lv_obj_set_style_opa(target->macro_record, LV_OPA_0, 0);

    /* macro 1 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/macro1.png", base);
    target->macro1                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro1 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_macro1) {
        lv_image_set_src(target->macro1, png_macro1);
    }
    lv_obj_align(target->macro1, macro1->align, 0, 0);
    lv_obj_set_pos(target->macro1, macro1->x, macro1->y);
    lv_obj_set_style_opa(target->macro1, LV_OPA_0, 0);

    /* macro 2 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/macro2.png", base);
    target->macro2                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro2 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_macro2) {
        lv_image_set_src(target->macro2, png_macro2);
    }
    lv_obj_align(target->macro2, macro2->align, 0, 0);
    lv_obj_set_pos(target->macro2, macro2->x, macro2->y);
    lv_obj_set_style_opa(target->macro2, LV_OPA_0, 0);

    return target;
}

void pgs_widgets_macro_set_state(struct pgs_widgets_macro * macro, uint8_t state, uint8_t macro1, uint8_t macro2)
{
    if(!macro || !macro->_macro->enable || !macro->_macro1->enable || !macro->_macro2->enable) {
        return;
    }

    pgs_animation_stop(macro->macro1);
    pgs_animation_stop(macro->macro2);
    pgs_animation_stop(macro->macro_pause);
    pgs_animation_stop(macro->macro_play);
    pgs_animation_stop(macro->macro_record);

    if(macro1) {
        lv_obj_set_style_opa(macro->macro1, macro->_macro1->opa, 0);
    }

    if(macro2) {
        lv_obj_set_style_opa(macro->macro2, macro->_macro2->opa, 0);
    }

    switch(state) {
        case PGS_WIDGETS_MACRO_STATE_PAUSE:
            lv_obj_set_style_opa(macro->macro_pause, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro_play, macro->_macro->opa, 0);
            lv_obj_set_style_opa(macro->macro_record, LV_OPA_0, 0);
            break;
        case PGS_WIDGETS_MACRO_STATE_PLAY1:
            if(macro1) {
                lv_obj_set_style_opa(macro->macro_pause, macro->_macro->opa, 0);
                lv_obj_set_style_opa(macro->macro_play, LV_OPA_0, 0);
                lv_obj_set_style_opa(macro->macro_record, LV_OPA_0, 0);
                lv_obj_set_style_opa(macro->macro1, LV_OPA_0, 0);
                pgs_animation_blink_fade(macro->macro1, 250, 250, LV_ANIM_REPEAT_INFINITE, macro->_macro1->opa);
            }
            break;
        case PGS_WIDGETS_MACRO_STATE_PLAY2:
            if(macro2) {
                lv_obj_set_style_opa(macro->macro_pause, macro->_macro->opa, 0);
                lv_obj_set_style_opa(macro->macro_play, LV_OPA_0, 0);
                lv_obj_set_style_opa(macro->macro_record, LV_OPA_0, 0);
                lv_obj_set_style_opa(macro->macro2, LV_OPA_0, 0);
                pgs_animation_blink_fade(macro->macro2, 250, 250, LV_ANIM_REPEAT_INFINITE, macro->_macro2->opa);
            }
            break;
        case PGS_WIDGETS_MACRO_STATE_RECORD1:
            lv_obj_set_style_opa(macro->macro_pause, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro_play, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro_record, macro->_macro->opa, 0);
            lv_obj_set_style_opa(macro->macro1, LV_OPA_0, 0);
            pgs_animation_blink_fade(macro->macro1, 250, 250, LV_ANIM_REPEAT_INFINITE, macro->_macro1->opa);
            break;
        case PGS_WIDGETS_MACRO_STATE_RECORD2:
            lv_obj_set_style_opa(macro->macro_pause, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro_play, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro_record, macro->_macro->opa, 0);
            lv_obj_set_style_opa(macro->macro2, LV_OPA_0, 0);
            pgs_animation_blink_fade(macro->macro2, 250, 250, LV_ANIM_REPEAT_INFINITE, macro->_macro2->opa);
            break;
        default:
            lv_obj_set_style_opa(macro->macro_pause, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro_play, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro_record, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro1, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro2, LV_OPA_0, 0);
            pgs_animation_blink_fade(macro->macro_pause, 100, 100, LV_ANIM_REPEAT_INFINITE, macro->_macro->opa);
            pgs_animation_blink_fade(macro->macro_play, 100, 100, LV_ANIM_REPEAT_INFINITE, macro->_macro->opa);
            pgs_animation_blink_fade(macro->macro_record, 100, 100, LV_ANIM_REPEAT_INFINITE, macro->_macro->opa);
            pgs_animation_blink_fade(macro->macro1, 100, 100, LV_ANIM_REPEAT_INFINITE, macro->_macro1->opa);
            pgs_animation_blink_fade(macro->macro2, 100, 100, LV_ANIM_REPEAT_INFINITE, macro->_macro2->opa);
            break;
    }
}