#include "pgs_widgets.h"

struct pgs_widgets_images * pgs_widgets_images_create(lv_obj_t * obj, const char * base,
                                                      struct pgs_widgets_params_image * images, uint32_t count)
{
    if(!base) {
        return NULL;
    }
    if(!images || !images->enable) {
        return NULL;
    }

    struct pgs_widgets_images * target = lv_malloc(sizeof(struct pgs_widgets_images));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct pgs_widgets_images));

    target->_images = images;
    target->count   = count;

    target->images = lv_malloc(sizeof(lv_obj_t *) * count);
    if(!target->images) {
        lv_free(target);
        return NULL;
    }

    for(uint32_t i = 0; i < count; i++) {
        if(!images[i].enable) {
            target->images[i] = NULL;
            continue;
        }

        if(images[i].path == '/'){
            lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s", base, images[i].path);
        }else{
            lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/%s", base, images[i].path);
        }

        lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/%s", base, images[i].path);
        target->images[i]          = lv_image_create(obj);
        const lv_image_dsc_t * png = pgs_libpng_decode(keyboard_path_buffer);
        if(png) {
            lv_image_set_src(target->images[i], png);
        }

        lv_obj_set_x(target->images[i], images[i].x);
        lv_obj_set_y(target->images[i], images[i].y);
        lv_obj_set_align(target->images[i], images[i].align);
        lv_obj_set_style_opa(target->images[i], images[i].opa, 0);
        lv_obj_set_style_radius(target->images[i], images[i].radius, 0);
    }

    return target;
}
