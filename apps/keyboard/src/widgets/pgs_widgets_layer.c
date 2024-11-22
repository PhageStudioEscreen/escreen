#include "pgs_widgets.h"

struct pgs_widgets_layer * pgs_widgets_layer_create(lv_obj_t * obj, const char * base,
                                                    struct pgs_widgets_params_state * layer)
{
    if(!base) {
        return NULL;
    }
    if(!layer || (layer->type != PGS_WIDGETS_TYPE_LAYER) || !layer->enable) {
        return NULL;
    }

    struct pgs_widgets_layer * target = lv_malloc(sizeof(struct pgs_widgets_layer));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct pgs_widgets_layer));

    target->_layer = layer;

    for(uint8_t i = 0; i < 8; i++) {
        lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/layer%d.png", base, i);
        target->layers[i]                = lv_image_create(obj);
        const lv_image_dsc_t * png_layer = pgs_libpng_decode(keyboard_path_buffer);
        if(png_layer) {
            lv_image_set_src(target->layers[i], png_layer);
        }
        lv_obj_align(target->layers[i], layer->align, 0, 0);
        lv_obj_set_pos(target->layers[i], layer->x, layer->y);
        lv_obj_set_style_opa(target->layers[i], LV_OPA_0, 0);
    }

    return target;
}

void pgs_widgets_layer_set_current(struct pgs_widgets_layer * layer, uint8_t clayer)
{
    if(!layer || !layer->_layer->enable) {
        return;
    }

    for(uint8_t i = 0; i < 8; i++) {
        if(i == clayer) {
            lv_obj_set_style_opa(layer->layers[i], layer->_layer->opa, 0);
        } else {
            lv_obj_set_style_opa(layer->layers[i], LV_OPA_0, 0);
        }
    }
}
