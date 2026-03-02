#include "pgs_widgets.h"

static void playdone_event_cb(lv_obj_t * obj, void * user_data, uint32_t index)
{
    struct pgs_widgets_keyanim * keyanim = lv_obj_get_user_data(obj);

    keyanim->playflag &= ~(1 << index);

    if(!keyanim->playflag) {
        if(keyanim->waitanims) {
            for(uint32_t i = 0; i < keyanim->waitcount; i++) {
                if(!keyanim->_keyanim->waitanims[i].enable) {
                    continue;
                }
                if(keyanim->waitanims[i]) {
                    lv_obj_remove_flag(keyanim->waitanims[i], LV_OBJ_FLAG_HIDDEN);
                }
            }
        }
    }

    lv_obj_add_flag(keyanim->anim[index], LV_OBJ_FLAG_HIDDEN);
}

static void playdone_event_cb0(lv_obj_t * obj, void * user_data)
{
    playdone_event_cb(obj, user_data, 0);
}

static void playdone_event_cb1(lv_obj_t * obj, void * user_data)
{
    playdone_event_cb(obj, user_data, 1);
}

static void ext_draw_size_event_cb(lv_event_t * e)
{
    lv_event_set_ext_draw_size(e, LV_MAX(LV_HOR_RES, LV_VER_RES));
}

static void delay_show_timer_cb(lv_timer_t * timer, uint32_t index)
{
    struct pgs_widgets_keyanim * keyanim = lv_timer_get_user_data(timer);

    lv_timer_pause(timer);

    keyanim->playflag &= ~(1 << index);

    if(!keyanim->playflag) {
        if(keyanim->waitanims) {
            for(uint32_t i = 0; i < keyanim->waitcount; i++) {
                if(!keyanim->_keyanim->waitanims[i].enable) {
                    continue;
                }
                if(keyanim->waitanims[i]) {
                    lv_obj_remove_flag(keyanim->waitanims[i], LV_OBJ_FLAG_HIDDEN);
                }
            }
        }
    }

    lv_obj_add_flag(keyanim->anim[index], LV_OBJ_FLAG_HIDDEN);
}

static void delay_show_timer_cb0(lv_timer_t * timer)
{
    delay_show_timer_cb(timer, 0);
}

static void delay_show_timer_cb1(lv_timer_t * timer)
{
    delay_show_timer_cb(timer, 1);
}

struct pgs_widgets_keyanim * pgs_widgets_keyanim_create(lv_obj_t * obj, const char * base,
                                                        struct pgs_widgets_params_keyanim * keyanim)
{
    if(!base) {
        return NULL;
    }
    if(!keyanim || !keyanim->enable) {
        return NULL;
    }

    struct pgs_widgets_keyanim * target = lv_malloc(sizeof(struct pgs_widgets_keyanim));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct pgs_widgets_keyanim));

    target->_keyanim  = keyanim;
    target->base      = base;
    target->waitcount = keyanim->waitcount;

    target->container = lv_obj_create(obj);
    lv_obj_remove_style_all(target->container);
    lv_obj_set_size(target->container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_flag(target->container, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_add_event_cb(target->container, ext_draw_size_event_cb, LV_EVENT_REFR_EXT_DRAW_SIZE, NULL);
    lv_obj_set_x(target->container, keyanim->x);
    lv_obj_set_y(target->container, keyanim->y);
    lv_obj_set_align(target->container, keyanim->align);
    lv_obj_set_style_opa(target->container, keyanim->opa, 0);
    lv_obj_set_style_radius(target->container, keyanim->radius, 0);
    if(keyanim->zindex != PGS_WIDGETS_ZINDEX_DEFAULT) {
        lv_obj_move_to_index(target->container, keyanim->zindex);
    }

    if(target->waitcount && keyanim->waitanims) {
        target->waitanims = lv_malloc(sizeof(lv_obj_t *) * target->waitcount);
        if(target->waitanims) {
            lv_memzero(target->waitanims, sizeof(lv_obj_t *) * target->waitcount);
            for(uint32_t i = 0; i < target->waitcount; i++) {
                if(!keyanim->waitanims[i].enable) {
                    continue;
                }

                if(keyanim->waitanims[i].path[0] == '/') {
                    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s", keyanim->waitanims[i].path);
                } else {
                    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/%s", base,
                                keyanim->waitanims[i].path);
                }

                target->waitanims[i] = lv_ffmpeg_player_create(target->container);
                lv_ffmpeg_player_set_auto_restart(target->waitanims[i], true);
                lv_obj_set_x(target->waitanims[i], keyanim->waitanims[i].x);
                lv_obj_set_y(target->waitanims[i], keyanim->waitanims[i].y);
                lv_obj_set_style_radius(target->waitanims[i], keyanim->waitanims[i].radius, 0);
                lv_obj_remove_flag(target->waitanims[i], LV_OBJ_FLAG_HIDDEN);
                lv_ffmpeg_player_set_src(target->waitanims[i], keyboard_path_buffer);
                lv_ffmpeg_player_set_cmd(target->waitanims[i], LV_FFMPEG_PLAYER_CMD_START);
            }
        }
    }

    target->anim[0] = lv_ffmpeg_player_create(target->container);
    lv_ffmpeg_player_set_auto_restart(target->anim[0], false);
    lv_obj_set_user_data(target->anim[0], target);
    lv_obj_add_flag(target->anim[0], LV_OBJ_FLAG_HIDDEN);

    target->anim[1] = lv_ffmpeg_player_create(target->container);
    lv_ffmpeg_player_set_auto_restart(target->anim[1], false);
    lv_obj_set_user_data(target->anim[1], target);
    lv_obj_add_flag(target->anim[1], LV_OBJ_FLAG_HIDDEN);

    if(keyanim->duration == 0) {
        lv_ffmpeg_player_set_playdone_event(target->anim[0], playdone_event_cb0, (void *)target);
        lv_ffmpeg_player_set_playdone_event(target->anim[1], playdone_event_cb1, (void *)target);
    }

    target->playflag = 0;
    target->index    = 0;

    if(keyanim->duration) {
        target->delay_timer[0] = lv_timer_create(delay_show_timer_cb0, keyanim->duration, target);
        lv_timer_pause(target->delay_timer[0]);
        target->delay_timer[1] = lv_timer_create(delay_show_timer_cb1, keyanim->duration, target);
        lv_timer_pause(target->delay_timer[1]);
    }

    return target;
}

void pgs_widgets_keyanim_push(struct pgs_widgets_keyanim * keyanim, uint32_t keycode)
{
    if(!keyanim || !keyanim->_keyanim->enable) {
        return;
    }

    if(keyanim->_keyanim->path[0] == '/') {
        lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/0x%04X%s", keyanim->_keyanim->path, keycode,
                    keyanim->_keyanim->suffix);
    } else {
        lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/%s/0x%04X%s", keyanim->base,
                    keyanim->_keyanim->path, keycode, keyanim->_keyanim->suffix);
    }

    struct stat statbuf;
    if(stat(keyboard_path_buffer, &statbuf) != 0) {
        return;
    }

    if(keyanim->keycode != keycode) {
        keyanim->keycode = keycode;
        keyanim->index++;
        lv_ffmpeg_player_set_src(keyanim->anim[keyanim->index & 1], keyboard_path_buffer);
    }

    lv_ffmpeg_player_set_cmd(keyanim->anim[keyanim->index & 1], LV_FFMPEG_PLAYER_CMD_START);

    lv_ffmpeg_player_set_cmd(keyanim->anim[keyanim->index & 1], LV_FFMPEG_PLAYER_CMD_START);
    lv_obj_remove_flag(keyanim->anim[keyanim->index & 1], LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(keyanim->anim[1 - (keyanim->index & 1)], LV_OBJ_FLAG_HIDDEN);

    if(keyanim->waitanims) {
        for(uint32_t i = 0; i < keyanim->waitcount; i++) {
            if(!keyanim->_keyanim->waitanims[i].enable) {
                continue;
            }
            if(keyanim->waitanims[i]) {
                lv_obj_add_flag(keyanim->waitanims[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    keyanim->playflag |= 1 << (keyanim->index & 1);

    if(keyanim->_keyanim->duration) {
        lv_timer_reset(keyanim->delay_timer[keyanim->index & 1]);
        lv_timer_resume(keyanim->delay_timer[keyanim->index & 1]);
    }
}
