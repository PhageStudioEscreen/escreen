#include "pgs_widgets.h"

struct pgs_widgets_bat * pgs_widgets_bat_create(lv_obj_t * obj, const char * base,
                                                struct pgs_widgets_params_state * bat)
{
    if(!base) {
        return NULL;
    }
    if(!bat || (bat->type != PGS_WIDGETS_TYPE_BAT) || !bat->enable) {
        return NULL;
    }

    struct pgs_widgets_bat * target = lv_malloc(sizeof(struct pgs_widgets_bat));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct pgs_widgets_bat));

    target->_bat = bat;

    /* alert */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat_alert.png", base);
    target->alert                    = lv_image_create(obj);
    const lv_image_dsc_t * png_alert = pgs_libpng_decode(keyboard_path_buffer);
    if(png_alert) {
        lv_image_set_src(target->alert, png_alert);
    }
    lv_obj_align(target->alert, bat->align, 0, 0);
    lv_obj_set_pos(target->alert, bat->x, bat->y);
    lv_obj_set_style_opa(target->alert, LV_OPA_0, 0);

    /* unknown */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat_unknown.png", base);
    target->unknown                    = lv_image_create(obj);
    const lv_image_dsc_t * png_unknown = pgs_libpng_decode(keyboard_path_buffer);
    if(png_unknown) {
        lv_image_set_src(target->unknown, png_unknown);
    }
    lv_obj_align(target->unknown, bat->align, 0, 0);
    lv_obj_set_pos(target->unknown, bat->x, bat->y);
    lv_obj_set_style_opa(target->unknown, LV_OPA_0, 0);

    /* charging 20 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat20_charging.png", base);
    target->charging_20                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_20 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_charging_20) {
        lv_image_set_src(target->charging_20, png_charging_20);
    }
    lv_obj_align(target->charging_20, bat->align, 0, 0);
    lv_obj_set_pos(target->charging_20, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_20, LV_OPA_0, 0);

    /* charging 30 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat30_charging.png", base);
    target->charging_30                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_30 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_charging_30) {
        lv_image_set_src(target->charging_30, png_charging_30);
    }
    lv_obj_align(target->charging_30, bat->align, 0, 0);
    lv_obj_set_pos(target->charging_30, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_30, LV_OPA_0, 0);

    /* charging 50 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat50_charging.png", base);
    target->charging_50                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_50 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_charging_50) {
        lv_image_set_src(target->charging_50, png_charging_50);
    }
    lv_obj_align(target->charging_50, bat->align, 0, 0);
    lv_obj_set_pos(target->charging_50, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_50, LV_OPA_0, 0);

    /* charging 60 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat60_charging.png", base);
    target->charging_60                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_60 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_charging_60) {
        lv_image_set_src(target->charging_60, png_charging_60);
    }
    lv_obj_align(target->charging_60, bat->align, 0, 0);
    lv_obj_set_pos(target->charging_60, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_60, LV_OPA_0, 0);

    /* charging 80 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat80_charging.png", base);
    target->charging_80                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_80 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_charging_80) {
        lv_image_set_src(target->charging_80, png_charging_80);
    }
    lv_obj_align(target->charging_80, bat->align, 0, 0);
    lv_obj_set_pos(target->charging_80, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_80, LV_OPA_0, 0);

    /* charging 90 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat90_charging.png", base);
    target->charging_90                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_90 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_charging_90) {
        lv_image_set_src(target->charging_90, png_charging_90);
    }
    lv_obj_align(target->charging_90, bat->align, 0, 0);
    lv_obj_set_pos(target->charging_90, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_90, LV_OPA_0, 0);

    /* charging 100 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat100_charging.png", base);
    target->charging_100                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_100 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_charging_100) {
        lv_image_set_src(target->charging_100, png_charging_100);
    }
    lv_obj_align(target->charging_100, bat->align, 0, 0);
    lv_obj_set_pos(target->charging_100, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_100, LV_OPA_0, 0);

    /* bat 20 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat20.png", base);
    target->bat_20                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_20 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_bat_20) {
        lv_image_set_src(target->bat_20, png_bat_20);
    }
    lv_obj_align(target->bat_20, bat->align, 0, 0);
    lv_obj_set_pos(target->bat_20, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_20, LV_OPA_0, 0);

    /* bat 30 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat30.png", base);
    target->bat_30                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_30 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_bat_30) {
        lv_image_set_src(target->bat_30, png_bat_30);
    }
    lv_obj_align(target->bat_30, bat->align, 0, 0);
    lv_obj_set_pos(target->bat_30, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_30, LV_OPA_0, 0);

    /* bat 50 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat50.png", base);
    target->bat_50                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_50 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_bat_50) {
        lv_image_set_src(target->bat_50, png_bat_50);
    }
    lv_obj_align(target->bat_50, bat->align, 0, 0);
    lv_obj_set_pos(target->bat_50, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_50, LV_OPA_0, 0);

    /* bat 60 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat60.png", base);
    target->bat_60                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_60 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_bat_60) {
        lv_image_set_src(target->bat_60, png_bat_60);
    }
    lv_obj_align(target->bat_60, bat->align, 0, 0);
    lv_obj_set_pos(target->bat_60, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_60, LV_OPA_0, 0);

    /* bat 80 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat80.png", base);
    target->bat_80                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_80 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_bat_80) {
        lv_image_set_src(target->bat_80, png_bat_80);
    }
    lv_obj_align(target->bat_80, bat->align, 0, 0);
    lv_obj_set_pos(target->bat_80, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_80, LV_OPA_0, 0);

    /* bat 90 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat90.png", base);
    target->bat_90                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_90 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_bat_90) {
        lv_image_set_src(target->bat_90, png_bat_90);
    }
    lv_obj_align(target->bat_90, bat->align, 0, 0);
    lv_obj_set_pos(target->bat_90, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_90, LV_OPA_0, 0);

    /* bat 100 */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/bat100.png", base);
    target->bat_100                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_100 = pgs_libpng_decode(keyboard_path_buffer);
    if(png_bat_100) {
        lv_image_set_src(target->bat_100, png_bat_100);
    }
    lv_obj_align(target->bat_100, bat->align, 0, 0);
    lv_obj_set_pos(target->bat_100, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_100, LV_OPA_0, 0);

    return target;
}

void pgs_widgets_bat_set_state(struct pgs_widgets_bat * bat, uint8_t state, uint8_t level)
{
    if(!bat || !bat->_bat->enable) {
        return;
    }

    lv_obj_set_style_opa(bat->unknown, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->alert, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->bat_20, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->bat_30, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->bat_50, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->bat_60, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->bat_80, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->bat_90, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->bat_100, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->charging_20, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->charging_30, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->charging_50, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->charging_60, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->charging_80, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->charging_90, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->charging_100, LV_OPA_0, 0);
    pgs_animation_stop(bat->unknown);

    if(state == PGS_WIDGETS_BAT_STATE_IDLE) {
        if(level > 95) {
            lv_obj_set_style_opa(bat->bat_100, bat->_bat->opa, 0);
        } else if(level > 85) {
            lv_obj_set_style_opa(bat->bat_90, bat->_bat->opa, 0);
        } else if(level > 70) {
            lv_obj_set_style_opa(bat->bat_80, bat->_bat->opa, 0);
        } else if(level > 55) {
            lv_obj_set_style_opa(bat->bat_60, bat->_bat->opa, 0);
        } else if(level > 40) {
            lv_obj_set_style_opa(bat->bat_50, bat->_bat->opa, 0);
        } else if(level > 25) {
            lv_obj_set_style_opa(bat->bat_30, bat->_bat->opa, 0);
        } else if(level > 15) {
            lv_obj_set_style_opa(bat->bat_20, bat->_bat->opa, 0);
        } else {
            lv_obj_set_style_opa(bat->alert, bat->_bat->opa, 0);
        }
    } else if(state == PGS_WIDGETS_BAT_STATE_CHARGING) {
        if(level < 15) {
            lv_obj_set_style_opa(bat->charging_20, bat->_bat->opa, 0);
        } else if(level < 25) {
            lv_obj_set_style_opa(bat->charging_30, bat->_bat->opa, 0);
        } else if(level < 45) {
            lv_obj_set_style_opa(bat->charging_50, bat->_bat->opa, 0);
        } else if(level < 65) {
            lv_obj_set_style_opa(bat->charging_60, bat->_bat->opa, 0);
        } else if(level < 75) {
            lv_obj_set_style_opa(bat->charging_80, bat->_bat->opa, 0);
        } else if(level < 95) {
            lv_obj_set_style_opa(bat->charging_90, bat->_bat->opa, 0);
        } else {
            lv_obj_set_style_opa(bat->charging_100, bat->_bat->opa, 0);
        }
    } else {
        pgs_animation_blink_fade(bat->unknown, 250, 250, LV_ANIM_REPEAT_INFINITE, bat->_bat->opa);
    }
}