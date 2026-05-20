#include "pgs_widgets.h"
#include <stdio.h>
#include <time.h>

#define SYNC_THRESHOLD_MS       400
#define SYNC_RESUME_MS          120
#define SYNC_CHECK_PERIOD       20
#define SYNC_SEEK_BACK_MS      100
#define SYNC_SEEK_COOLDOWN_MS  300

static int64_t g_last_video_pts[4] = {0};
static int64_t g_last_sync_time[4] = {0};
static int64_t g_last_seek_time[4] = {0};
static int g_initial_sync_done[4] = {0};
static int g_video_paused[4] = {0};
static int64_t g_ignore_restart_until[4] = {0};

struct video_event_ctx {
    struct pgs_widgets_videos * parent;
    uint32_t index;
};

static void video_playdone_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    struct video_event_ctx * ctx = (struct video_event_ctx *)user_data;
    if(!ctx || !ctx->parent) return;
    uint32_t idx = ctx->index;
    if(!ctx->parent->audio_players || !ctx->parent->audio_players[idx]) return;

    audio_player_restart(ctx->parent->audio_players[idx]);
    g_last_video_pts[idx] = 0;
    g_last_sync_time[idx] = lv_tick_get();
    g_initial_sync_done[idx] = 0;
    g_video_paused[idx] = 0;
    g_ignore_restart_until[idx] = g_last_sync_time[idx] + 1000;
}

static void video_sync_timer_callback(lv_timer_t * timer)
{
    struct pgs_widgets_videos * target = (struct pgs_widgets_videos *)lv_timer_get_user_data(timer);
    if(!target) return;

    int64_t now = lv_tick_get();

    for(uint32_t i = 0; i < target->count; i++) {
        if(!target->videos[i] || !target->audio_players[i]) continue;

        int64_t audio_clock = audio_player_get_clock(target->audio_players[i]);
        if(audio_clock < 0) continue;

        int64_t video_pts_ms = lv_ffmpeg_player_get_pts(target->videos[i]);
        if(video_pts_ms < 0) video_pts_ms = 0;

        int64_t diff = audio_clock - video_pts_ms;

        if(g_video_paused[i]) {
            if(diff >= -SYNC_RESUME_MS) {
                lv_ffmpeg_player_set_cmd(target->videos[i], LV_FFMPEG_PLAYER_CMD_RESUME);
                g_video_paused[i] = 0;
            }
        }

        if(g_last_video_pts[i] > 1000 && video_pts_ms < 100) {
            if(g_ignore_restart_until[i] > 0 && now < g_ignore_restart_until[i]) {
                g_last_video_pts[i] = video_pts_ms;
                continue;
            }
            audio_player_restart(target->audio_players[i]);
            g_last_sync_time[i] = now;
            g_last_video_pts[i] = video_pts_ms;
            continue;
        }

        if(now - g_last_sync_time[i] > SYNC_CHECK_PERIOD) {
            if(!g_initial_sync_done[i]) {
                lv_ffmpeg_player_seek(target->videos[i], audio_clock - 100);
                g_initial_sync_done[i] = 1;
                g_last_seek_time[i] = now;
            } else if((diff > SYNC_THRESHOLD_MS || diff < -SYNC_THRESHOLD_MS)) {
                int64_t seek_cooldown = SYNC_SEEK_COOLDOWN_MS;
                if(diff > 2000) {
                    seek_cooldown = 1200;
                }
                if(now - g_last_seek_time[i] <= seek_cooldown) {
                    g_last_video_pts[i] = video_pts_ms;
                    continue;
                }
                int64_t duration = lv_ffmpeg_player_get_duration(target->videos[i]);
                int near_end = (duration > 0 && video_pts_ms > duration - 10000);
                int do_seek = 0;
                if(diff > 0 && !near_end) {
                    lv_ffmpeg_player_seek(target->videos[i], audio_clock - SYNC_SEEK_BACK_MS);
                    do_seek = 1;
                } else if(diff < 0) {
                    if(!g_video_paused[i]) {
                        lv_ffmpeg_player_set_cmd(target->videos[i], LV_FFMPEG_PLAYER_CMD_PAUSE);
                        g_video_paused[i] = 1;
                        g_last_seek_time[i] = now;
                    }
                }
                if(do_seek) {
                    g_last_seek_time[i] = now;
                }
            }
            g_last_sync_time[i] = now;
        }

        g_last_video_pts[i] = video_pts_ms;
    }
}

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

    target->audio_players = lv_malloc(sizeof(audio_player_handle) * count);
    if(!target->audio_players) {
        lv_free(target->path_count);
        lv_free(target->index);
        lv_free(target->videos);
        lv_free(target);
        return NULL;
    }
    lv_memzero(target->audio_players, sizeof(audio_player_handle) * count);

    target->event_ctx = lv_malloc(sizeof(struct video_event_ctx) * count);
    if(!target->event_ctx) {
        lv_free(target->audio_players);
        lv_free(target->path_count);
        lv_free(target->index);
        lv_free(target->videos);
        lv_free(target);
        return NULL;
    }
    lv_memzero(target->event_ctx, sizeof(struct video_event_ctx) * count);

    int has_audio = 0;

    for(uint32_t i = 0; i < count; i++) {
        if(!videos[i].enable) {
            target->videos[i] = NULL;
            target->audio_players[i] = NULL;
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

        struct video_event_ctx * ev = &((struct video_event_ctx *)target->event_ctx)[i];
        ev->parent = target;
        ev->index = i;
        lv_ffmpeg_player_set_playdone_event(target->videos[i], video_playdone_cb, ev);

        if(videos[i].audio && videos[i].count > 0) {
            target->audio_players[i] = audio_player_create();
            if(target->audio_players[i]) {
                if(videos[i].paths[0][0] == '/') {
                    audio_player_start_ex(target->audio_players[i], videos[i].paths[0]);
                } else {
                    char audio_path[PATH_MAX];
                    lv_snprintf(audio_path, sizeof(audio_path), "%s/%s", base, videos[i].paths[0]);
                    audio_player_start_ex(target->audio_players[i], audio_path);
                }
                has_audio = 1;
            }
        }

        lv_ffmpeg_player_set_cmd(target->videos[i], LV_FFMPEG_PLAYER_CMD_START);

        lv_obj_set_x(target->videos[i], videos[i].x);
        lv_obj_set_y(target->videos[i], videos[i].y);
        if(videos[i].zindex != PGS_WIDGETS_ZINDEX_DEFAULT) {
            lv_obj_move_to_index(target->videos[i], videos[i].zindex);
        }
        lv_obj_set_align(target->videos[i], videos[i].align);
        lv_obj_set_style_opa(target->videos[i], videos[i].opa, 0);
        lv_obj_set_style_radius(target->videos[i], videos[i].radius, 0);
        lv_obj_set_user_data(target->videos[i], target);
    }

    if(has_audio) {
        target->sync_timer = lv_timer_create(video_sync_timer_callback, SYNC_CHECK_PERIOD, target);
    }

    return target;
}
