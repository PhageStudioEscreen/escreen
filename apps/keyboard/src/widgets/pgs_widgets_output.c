#include "pgs_widgets.h"

struct pgs_widgets_output * pgs_widgets_output_create(lv_obj_t * obj, const char * base,
                                                      struct pgs_widgets_params_state * output)
{
    if(!base) {
        return NULL;
    }
    if(!output || !output->enable) {
        return NULL;
    }
    const char * disconnected_path;
    const char * connected_path;
    const char * searching_path;
    switch(output->type) {
        case PGS_WIDGETS_TYPE_BLE1:
            disconnected_path = "ble1";
            connected_path    = "ble1_connected";
            searching_path    = "ble1_searching";
            break;
        case PGS_WIDGETS_TYPE_BLE2:
            disconnected_path = "ble2";
            connected_path    = "ble2_connected";
            searching_path    = "ble2_searching";
            break;
        case PGS_WIDGETS_TYPE_BLE3:
            disconnected_path = "ble3";
            connected_path    = "ble3_connected";
            searching_path    = "ble3_searching";
            break;
        case PGS_WIDGETS_TYPE_2G4:
            disconnected_path = "2g4";
            connected_path    = "2g4_connected";
            searching_path    = "2g4_searching";
            break;
        case PGS_WIDGETS_TYPE_USB:
            disconnected_path = "usb";
            connected_path    = "usb_connected";
            searching_path    = NULL;
            break;
        case PGS_WIDGETS_TYPE_SCR:
            disconnected_path = "scr";
            connected_path    = "scr_connected";
            searching_path    = NULL;
            break;
        default: return NULL;
    }

    struct pgs_widgets_output * target = lv_malloc(sizeof(struct pgs_widgets_output));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct pgs_widgets_output));

    target->_output = output;

    /* disconnected */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/%s.png", base, disconnected_path);
    target->disconnected                    = lv_image_create(obj);
    const lv_image_dsc_t * png_disconnected = pgs_libpng_decode(keyboard_path_buffer);
    if(png_disconnected) {
        lv_image_set_src(target->disconnected, png_disconnected);
    }
    lv_obj_align(target->disconnected, output->align, 0, 0);
    lv_obj_set_pos(target->disconnected, output->x, output->y);
    lv_obj_set_style_opa(target->disconnected, LV_OPA_0, 0);

    /* connected */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/%s.png", base, connected_path);
    target->connected                    = lv_image_create(obj);
    const lv_image_dsc_t * png_connected = pgs_libpng_decode(keyboard_path_buffer);
    if(png_connected) {
        lv_image_set_src(target->connected, png_connected);
    }
    lv_obj_align(target->connected, output->align, 0, 0);
    lv_obj_set_pos(target->connected, output->x, output->y);
    lv_obj_set_style_opa(target->connected, LV_OPA_0, 0);

    if(!searching_path) {
        return target;
    }

    /* searching */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/%s.png", base, searching_path);
    target->searching                    = lv_image_create(obj);
    const lv_image_dsc_t * png_searching = pgs_libpng_decode(keyboard_path_buffer);
    if(png_searching) {
        lv_image_set_src(target->searching, png_searching);
    }
    lv_obj_align(target->searching, output->align, 0, 0);
    lv_obj_set_pos(target->searching, output->x, output->y);
    lv_obj_set_style_opa(target->searching, LV_OPA_0, 0);

    return target;
}

void pgs_widgets_output_set_state(struct pgs_widgets_output * output, uint8_t state, bool select)
{
    if(!output || !output->_output->enable) {
        return;
    }

    pgs_animation_stop(output->disconnected);
    pgs_animation_stop(output->connected);
    pgs_animation_stop(output->searching);

    if(select) {
        lv_obj_set_style_opa(output->disconnected, LV_OPA_0, 0);
        lv_obj_set_style_opa(output->connected, LV_OPA_0, 0);
        if(output->searching) {
            lv_obj_set_style_opa(output->searching, LV_OPA_0, 0);
        }

        switch(state) {
            case PGS_WIDGETS_OUTPUT_STATE_SUSPEND:
                pgs_animation_blink_fade(output->connected, 1000, 1000, LV_ANIM_REPEAT_INFINITE,
                                         output->_output->opa / 2);
                break;
            case PGS_WIDGETS_OUTPUT_STATE_DISCONNECT:
                pgs_animation_blink_fade(output->disconnected, 1000, 1000, LV_ANIM_REPEAT_INFINITE,
                                         output->_output->opa);
                break;
            case PGS_WIDGETS_OUTPUT_STATE_CONNECT:
                pgs_animation_blink_fade(output->connected, 1000, 1000, LV_ANIM_REPEAT_INFINITE, output->_output->opa);
                break;
            case PGS_WIDGETS_OUTPUT_STATE_SEARCHING:
                if(output->searching) {
                    pgs_animation_blink_fade(output->searching, 500, 500, LV_ANIM_REPEAT_INFINITE,
                                             output->_output->opa);
                }
                break;
            case PGS_WIDGETS_OUTPUT_STATE_CONFIRM_SELECT:
            case PGS_WIDGETS_OUTPUT_STATE_CONFIRM_ERASE:
                pgs_animation_blink_fade(output->connected, 250, 250, LV_ANIM_REPEAT_INFINITE,
                                         output->_output->opa / 2);
                break;
            case PGS_WIDGETS_OUTPUT_STATE_ERASE_ADV:
                if(output->searching) {
                    pgs_animation_blink_fade(output->searching, 250, 250, LV_ANIM_REPEAT_INFINITE,
                                             output->_output->opa);
                }
                break;
            default:
                pgs_animation_blink_fade(output->disconnected, 100, 100, LV_ANIM_REPEAT_INFINITE, output->_output->opa);
                pgs_animation_blink_fade(output->connected, 100, 100, LV_ANIM_REPEAT_INFINITE, output->_output->opa);
                if(output->searching) {
                    pgs_animation_blink_fade(output->searching, 100, 100, LV_ANIM_REPEAT_INFINITE,
                                             output->_output->opa);
                }
                break;
        }
    } else {
        lv_obj_set_style_opa(output->disconnected, LV_OPA_0, 0);
        lv_obj_set_style_opa(output->connected, LV_OPA_0, 0);
        if(output->searching) {
            lv_obj_set_style_opa(output->searching, LV_OPA_0, 0);
        }

        switch(state) {
            case PGS_WIDGETS_OUTPUT_STATE_SUSPEND:
                lv_obj_set_style_opa(output->connected, output->_output->opa / 2, 0);
                break;
            case PGS_WIDGETS_OUTPUT_STATE_DISCONNECT:
                lv_obj_set_style_opa(output->disconnected, output->_output->opa, 0);
                break;
            case PGS_WIDGETS_OUTPUT_STATE_CONNECT:
                lv_obj_set_style_opa(output->connected, output->_output->opa, 0);
                break;
            case PGS_WIDGETS_OUTPUT_STATE_SEARCHING:
                if(output->searching) {
                    lv_obj_set_style_opa(output->searching, output->_output->opa, 0);
                }
                break;
            case PGS_WIDGETS_OUTPUT_STATE_CONFIRM_SELECT:
            case PGS_WIDGETS_OUTPUT_STATE_CONFIRM_ERASE:
                pgs_animation_blink_fade(output->connected, 250, 250, LV_ANIM_REPEAT_INFINITE,
                                         output->_output->opa / 2);
                break;
            case PGS_WIDGETS_OUTPUT_STATE_ERASE_ADV:
                if(output->searching) {
                    pgs_animation_blink_fade(output->searching, 250, 250, LV_ANIM_REPEAT_INFINITE,
                                             output->_output->opa);
                }
                break;

            default:
                pgs_animation_blink_fade(output->disconnected, 100, 100, LV_ANIM_REPEAT_INFINITE, output->_output->opa);
                pgs_animation_blink_fade(output->connected, 100, 100, LV_ANIM_REPEAT_INFINITE, output->_output->opa);
                if(output->searching) {
                    pgs_animation_blink_fade(output->searching, 100, 100, LV_ANIM_REPEAT_INFINITE,
                                             output->_output->opa);
                }
                break;
        }
    }
}
