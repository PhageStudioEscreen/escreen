#include "pgs_widgets.h"

struct pgs_widgets_labels * pgs_widgets_labels_create(lv_obj_t * obj, const char * base,
                                                      struct pgs_widgets_params_label * labels, uint32_t count)
{
    if(!base) {
        return NULL;
    }
    if(!labels) {
        return NULL;
    }

    struct pgs_widgets_labels * target = lv_malloc(sizeof(struct pgs_widgets_labels));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct pgs_widgets_labels));

    target->_labels = labels;
    target->count   = count;

    target->labels = lv_malloc(sizeof(lv_obj_t *) * count);
    if(!target->labels) {
        lv_free(target);
        return NULL;
    }

    for(uint32_t i = 0; i < count; i++) {
        if(!labels[i].enable) {
            target->labels[i] = NULL;
            continue;
        }

        target->labels[i] = lv_label_create(obj);
        lv_obj_set_width(target->labels[i], labels[i].w);
        lv_obj_set_height(target->labels[i], labels[i].h);
        lv_obj_set_x(target->labels[i], labels[i].x);
        lv_obj_set_y(target->labels[i], labels[i].y);
        lv_obj_set_align(target->labels[i], labels[i].align);
        lv_obj_set_style_opa(target->labels[i], labels[i].opa, 0);
        lv_label_set_text_fmt(target->labels[i], "%s", labels[i].text);
        lv_obj_set_style_text_color(target->labels[i], lv_color_hex(labels[i].color), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(target->labels[i], labels[i].text_align, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(target->labels[i], labels[i].font, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    return target;
}
