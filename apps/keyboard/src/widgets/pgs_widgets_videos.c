#include "pgs_widgets.h"

// static void playdone_event_cb(lv_obj_t * obj, void * user_data)
// {
//     struct pgs_widgets_videos * target = lv_obj_get_user_data(obj);
//     uint32_t player_index              = (uint32_t)user_data;

//     target->index[player_index]++;
//     if(target->index[player_index] >= target->path_count[player_index]) {
//         target->index[player_index] = 0;
//     }

//     lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/%s", target->base,
//                 target->_videos[player_index].paths[target->index[player_index]]);

//     lv_ffmpeg_player_set_src(target->videos[player_index], keyboard_path_buffer);
// }

struct pgs_widgets_videos * pgs_widgets_videos_create(lv_obj_t * obj, const char * base,
                                                      struct pgs_widgets_params_video * videos, uint32_t count)
{
    if(!base) {
        return NULL;
    }
    if(!videos) {
        return NULL;
    }

    struct pgs_widgets_videos * target = lv_malloc(sizeof(struct pgs_widgets_videos));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct pgs_widgets_videos));

    target->_videos = videos;
    target->count   = count;
    target->base    = base;

    target->videos = lv_malloc(sizeof(lv_obj_t *) * count);
    if(!target->videos) {
        lv_free(target);
        return NULL;
    }

    target->index = lv_malloc(sizeof(uint32_t) * count);
    if(!target->index) {
        lv_free(target->videos);
        lv_free(target);
        return NULL;
    }

    target->path_count = lv_malloc(sizeof(uint32_t) * count);
    if(!target->path_count) {
        lv_free(target->index);
        lv_free(target->videos);
        lv_free(target);
        return NULL;
    }

    for(uint32_t i = 0; i < count; i++) {
        if(!videos[i].enable) {
            target->videos[i] = NULL;
            continue;
        }

        target->index[i]      = 0;
        target->path_count[i] = videos[i].count;

        if(videos[i].paths[0][0] == '/') {
            lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s", videos[i].paths[0]);
        } else {
            lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/%s", base, videos[i].paths[0]);
        }

        target->videos[i] = lv_ffmpeg_player_create(obj);
        lv_ffmpeg_player_set_src(target->videos[i], keyboard_path_buffer);
        lv_ffmpeg_player_set_auto_restart(target->videos[i], true);
        lv_ffmpeg_player_set_cmd(target->videos[i], LV_FFMPEG_PLAYER_CMD_START);
        // lv_ffmpeg_player_set_playdone_event(target->videos[i], playdone_event_cb, (void *)i);

        lv_obj_set_x(target->videos[i], videos[i].x);
        lv_obj_set_y(target->videos[i], videos[i].y);
        lv_obj_set_align(target->videos[i], videos[i].align);
        lv_obj_set_style_opa(target->videos[i], videos[i].opa, 0);
        lv_obj_set_style_radius(target->videos[i], videos[i].radius, 0);
        lv_obj_set_user_data(target->videos[i], target);
    }

    return target;
}
