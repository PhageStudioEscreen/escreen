#include "pgs_widgets.h"

struct pgs_widgets_wpmlabel * pgs_widgets_wpmlabel_create(lv_obj_t * obj, const char * base,
                                                          struct pgs_widgets_params_wpmlabel * wpmlabel)
{
    if(!base) {
        return NULL;
    }
    if(!wpmlabel || !wpmlabel->enable) {
        return NULL;
    }

    struct pgs_widgets_wpmlabel * target = lv_malloc(sizeof(struct pgs_widgets_wpmlabel));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct pgs_widgets_wpmlabel));

    target->_wpmlabel = wpmlabel;

    target->text = lv_label_create(obj);
    lv_obj_set_width(target->text, wpmlabel->w);
    lv_obj_set_height(target->text, wpmlabel->h);
    lv_obj_set_x(target->text, wpmlabel->x);
    lv_obj_set_y(target->text, wpmlabel->y);
    lv_obj_set_align(target->text, wpmlabel->align);
    lv_obj_set_style_opa(target->text, wpmlabel->opa, 0);
    lv_label_set_text_fmt(target->text, "N/A WPM");
    lv_obj_set_style_text_color(target->text, lv_color_hex(wpmlabel->color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(target->text, wpmlabel->text_align, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(target->text, wpmlabel->font, LV_PART_MAIN | LV_STATE_DEFAULT);

    return target;
}

void pgs_widgets_wpmlabel_set_wpm(struct pgs_widgets_wpmlabel * wpmlabel, uint32_t wpm)
{
    if(!wpmlabel || !wpmlabel->_wpmlabel->enable) {
        return;
    }

    lv_label_set_text_fmt(wpmlabel->text, "%d WPM", wpm);
}
