#include "pgs_widgets.h"

struct pgs_widgets_gifs * pgs_widgets_gifs_create(lv_obj_t * obj, const char * base,
                                                  struct pgs_widgets_params_gif * gifs, uint32_t count)
{
    if(!base) {
        return NULL;
    }
    if(!gifs) {
        return NULL;
    }

    struct pgs_widgets_gifs * target = lv_malloc(sizeof(struct pgs_widgets_gifs));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct pgs_widgets_gifs));

    target->_gifs = gifs;
    target->count = count;

    target->gifs = lv_malloc(sizeof(lv_obj_t *) * count);
    if(!target->gifs) {
        lv_free(target);
        return NULL;
    }

    for(uint32_t i = 0; i < count; i++) {
        if(!gifs[i].enable) {
            target->gifs[i] = NULL;
            continue;
        }

        if(gifs[i].path[0] == '/'){
            lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "S:%s", base, gifs[i].path);
        }else{
            lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "S:%s/%s", base, gifs[i].path);
        }
        
        target->gifs[i] = lv_gif_create(obj);
        lv_gif_set_src(target->gifs[i], keyboard_path_buffer);
        lv_obj_set_x(target->gifs[i], gifs[i].x);
        lv_obj_set_y(target->gifs[i], gifs[i].y);
        lv_obj_set_align(target->gifs[i], gifs[i].align);
        lv_obj_set_style_opa(target->gifs[i], gifs[i].opa, 0);
        lv_obj_set_style_radius(target->gifs[i], gifs[i].radius, 0);
    }

    return target;
}
