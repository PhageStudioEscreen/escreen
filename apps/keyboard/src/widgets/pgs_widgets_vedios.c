#include "pgs_widgets.h"

// static void playdone_event_cb(lv_obj_t * obj, void * user_data)
// {
//     struct pgs_widgets_vedios * target = lv_obj_get_user_data(obj);
//     uint32_t player_index              = (uint32_t)user_data;

//     target->index[player_index]++;
//     if(target->index[player_index] >= target->path_count[player_index]) {
//         target->index[player_index] = 0;
//     }

//     lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/%s", target->base,
//                 target->_vedios[player_index].paths[target->index[player_index]]);

//     lv_ffmpeg_player_set_src(target->vedios[player_index], keyboard_path_buffer);
// }

struct pgs_widgets_vedios * pgs_widgets_vedios_create(lv_obj_t * obj, const char * base,
                                                      struct pgs_widgets_params_vedio * vedios, uint32_t count)
{
    if(!base) {
        return NULL;
    }
    if(!vedios) {
        return NULL;
    }

    struct pgs_widgets_vedios * target = lv_malloc(sizeof(struct pgs_widgets_vedios));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct pgs_widgets_vedios));

    target->_vedios = vedios;
    target->count   = count;
    target->base    = base;

    target->vedios = lv_malloc(sizeof(lv_obj_t *) * count);
    if(!target->vedios) {
        lv_free(target);
        return NULL;
    }

    target->index = lv_malloc(sizeof(uint32_t) * count);
    if(!target->index) {
        lv_free(target->vedios);
        lv_free(target);
        return NULL;
    }

    target->path_count = lv_malloc(sizeof(uint32_t) * count);
    if(!target->path_count) {
        lv_free(target->index);
        lv_free(target->vedios);
        lv_free(target);
        return NULL;
    }

    for(uint32_t i = 0; i < count; i++) {
        if(!vedios[i].enable) {
            target->vedios[i] = NULL;
            continue;
        }

        target->index[i]      = 0;
        target->path_count[i] = vedios[i].count;

        if(vedios[i].paths[0] == '/') {
            lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s", base, vedios[i].paths[0]);
        } else {
            lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/%s", base, vedios[i].paths[0]);
        }

        target->vedios[i] = lv_ffmpeg_player_create(obj);
        lv_ffmpeg_player_set_src(target->vedios[i], keyboard_path_buffer);
        lv_ffmpeg_player_set_auto_restart(target->vedios[i], true);
        lv_ffmpeg_player_set_cmd(target->vedios[i], LV_FFMPEG_PLAYER_CMD_START);
        // lv_ffmpeg_player_set_playdone_event(target->vedios[i], playdone_event_cb, (void *)i);

        lv_obj_set_x(target->vedios[i], vedios[i].x);
        lv_obj_set_y(target->vedios[i], vedios[i].y);
        lv_obj_set_align(target->vedios[i], vedios[i].align);
        lv_obj_set_style_opa(target->vedios[i], vedios[i].opa, 0);
        lv_obj_set_style_radius(target->vedios[i], vedios[i].radius, 0);
        lv_obj_set_user_data(target->vedios[i], target);
    }

    return target;
}
