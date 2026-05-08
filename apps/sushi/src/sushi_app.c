#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#include <unistd.h>
#define sushi_access access

#include <stdbool.h>

#include "pgs_modules.h"
#include "pgs_utils.h"

#include "sushi_app.h"
#include "Magic-Sushi/Magic-Sushi-Wrapper.h"

#define SUSHI_CLIP_STACK_MAX 8
#define SUSHI_GAMEOVER_SCORE_X 287
#define SUSHI_GAMEOVER_SCORE_Y 84
#define SUSHI_GAMEOVER_SCORE_LABEL_X 170
#define SUSHI_GAMEOVER_SCORE_LABEL_Y 57
#define SUSHI_GAMEOVER_TITLE_X 194
#define SUSHI_GAMEOVER_TITLE_Y 25
#define SUSHI_GAMEOVER_SCORE_BG_X 170
#define SUSHI_GAMEOVER_SCORE_BG_Y 57
#define SUSHI_GAMEOVER_FOOD_X 86
#define SUSHI_GAMEOVER_FOOD_Y 131

#ifndef LV_KEY_ENTER
#define LV_KEY_ENTER 0x0D
#endif

typedef struct {
    TEXTURE id;
    const char * file;
} sushi_asset_t;

static const sushi_asset_t sushi_assets[] = {
    { IMG_ID_GX_MAGICSUSHI_NUMBER_0, "gx_magicsushi_num_0.png" },
    { IMG_ID_GX_MAGICSUSHI_NUMBER_1, "gx_magicsushi_num_1.png" },
    { IMG_ID_GX_MAGICSUSHI_NUMBER_2, "gx_magicsushi_num_2.png" },
    { IMG_ID_GX_MAGICSUSHI_NUMBER_3, "gx_magicsushi_num_3.png" },
    { IMG_ID_GX_MAGICSUSHI_NUMBER_4, "gx_magicsushi_num_4.png" },
    { IMG_ID_GX_MAGICSUSHI_NUMBER_5, "gx_magicsushi_num_5.png" },
    { IMG_ID_GX_MAGICSUSHI_NUMBER_6, "gx_magicsushi_num_6.png" },
    { IMG_ID_GX_MAGICSUSHI_NUMBER_7, "gx_magicsushi_num_7.png" },
    { IMG_ID_GX_MAGICSUSHI_NUMBER_8, "gx_magicsushi_num_8.png" },
    { IMG_ID_GX_MAGICSUSHI_NUMBER_9, "gx_magicsushi_num_9.png" },
    { IMG_ID_GX_MAGICSUSHI_NUMBER_SLASH, "gx_magicsushi_num_splash.png" },
    { IMG_ID_GX_MAGICSUSHI_TYPE_NULL, "gx_magicsushi_item_null.png" },
    { IMG_ID_GX_MAGICSUSHI_SELECTED, "gx_magicsushi_select.png" },
    { IMG_ID_GX_MAGICSUSHI_TYPE_0, "gx_magicsushi_item_1.png" },
    { IMG_ID_GX_MAGICSUSHI_TYPE_1, "gx_magicsushi_item_2.png" },
    { IMG_ID_GX_MAGICSUSHI_TYPE_2, "gx_magicsushi_item_3.png" },
    { IMG_ID_GX_MAGICSUSHI_TYPE_3, "gx_magicsushi_item_4.png" },
    { IMG_ID_GX_MAGICSUSHI_TYPE_4, "gx_magicsushi_item_5.png" },
    { IMG_ID_GX_MAGICSUSHI_TYPE_5, "gx_magicsushi_item_6.png" },
    { IMG_ID_GX_MAGICSUSHI_TYPE_6, "gx_magicsushi_item_7.png" },
    { IMG_ID_GX_MAGICSUSHI_TYPE_7, "gx_magicsushi_item_8.png" },
    { IMG_ID_GX_MAGICSUSHI_TYPE_MAGIC1, "gx_magicsushi_magic_1.png" },
    { IMG_ID_GX_MAGICSUSHI_TYPE_MAGIC2, "gx_magicsushi_magic_2.png" },
    { IMG_ID_GX_MAGICSUSHI_TYPE_MAGIC3, "gx_magicsushi_magic_3.png" },
    { IMG_ID_GX_MAGICSUSHI_GAME_BACKGROUND, "gx_magicsushi_background.png" },
    { IMG_ID_GX_MAGICSUSHI_GAMEOVER, "gx_magicsushi_gameover_TimeOut_E.png" },
    { IMG_ID_GX_MAGICSUSHI_UPLEVEL, "gx_magicsushi_uplevel.png" },
    { IMG_ID_GX_MAGICSUSHI_NOMOREMOVE, "gx_magicsushi_nomoremove.png" },
    { IMG_ID_GX_MAGICSUSHI_CURSOR, "gx_magicsushi_select1.png" },
    { IMG_ID_GX_MAGICSUSHI_DOWN, "gx_magicsushi_down.png" },
    { IMG_ID_GX_MAGICSUSHI_UP, "gx_magicsushi_up.png" },
    { IMG_ID_GX_MAGICSUSHI_GRADEMAP, "gx_magicsushi_ScoreBG.png" },
    { IMG_ID_GX_MAGICSUSHI_GOPIC, "gx_magicsushi_Food.png" }
};

static const lv_image_dsc_t * sushi_textures[TEXTURE_MAX];
static lv_image_dsc_t sushi_screen_dsc;
static lv_obj_t * sushi_img;
static uint8_t * sushi_fb;
static lv_area_t sushi_clip_stack[SUSHI_CLIP_STACK_MAX];
static int sushi_clip_sp = -1;
static lv_area_t sushi_clip_current;
static lv_timer_t * sushi_timer;
static void (*sushi_timer_callback)(void);
static void (*sushi_key_callback)(uint32_t keycode);
static bool sushi_paused;
static uint32_t sushi_timer_period;
static uint32_t sushi_timer_last_tick;
static uint32_t sushi_timer_log_tick;
static FILE * sushi_log_file;
static int sushi_log_fd = -1;
static int sushi_log_draw_count;
static char sushi_sound_move[256];
static char sushi_sound_select[256];
static char sushi_bgm[256];
static char sushi_bgm_gameover[256];
static pid_t sushi_bgm_pid = -1;
static pid_t sushi_sfx_pid = -1;
static pid_t sushi_bgm_pgid = -1;

static void sushi_log(const char *fmt, ...)
{
    if (sushi_log_file == NULL) {
        sushi_log_file = fopen("/root/sushi.log", "a");
        if (sushi_log_file == NULL) {
            return;
        }
        setvbuf(sushi_log_file, NULL, _IOLBF, 0);
        sushi_log_fd = fileno(sushi_log_file);
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(sushi_log_file, fmt, args);
    va_end(args);
    fputc('\n', sushi_log_file);
    fflush(sushi_log_file);
}

static void sushi_audio_init(void)
{
    const char * base = pgs_get_path();
    if (base == NULL || base[0] == '\0') {
        base = "/usr/share/pgs/apps/sushi";
    }
    snprintf(sushi_sound_move, sizeof(sushi_sound_move), "%s/resources/gx_magicsushi_move.wav", base);
    snprintf(sushi_sound_select, sizeof(sushi_sound_select), "%s/resources/gx_magicsushi_select.wav", base);
    snprintf(sushi_bgm, sizeof(sushi_bgm), "%s/resources/gx_magicsushi_bgm.wav", base);
    snprintf(sushi_bgm_gameover, sizeof(sushi_bgm_gameover), "%s/resources/gx_magicsushi_timeout.wav", base);

    if (sushi_access(sushi_sound_move, R_OK) != 0) {
        snprintf(sushi_sound_move, sizeof(sushi_sound_move), "/usr/share/pgs/apps/sushi/resources/gx_magicsushi_move.wav");
    }
    if (sushi_access(sushi_sound_select, R_OK) != 0) {
        snprintf(sushi_sound_select, sizeof(sushi_sound_select), "/usr/share/pgs/apps/sushi/resources/gx_magicsushi_select.wav");
    }
    if (sushi_access(sushi_bgm, R_OK) != 0) {
        snprintf(sushi_bgm, sizeof(sushi_bgm), "/usr/share/pgs/apps/sushi/resources/gx_magicsushi_bgm.wav");
    }
    if (sushi_access(sushi_bgm_gameover, R_OK) != 0) {
        snprintf(sushi_bgm_gameover, sizeof(sushi_bgm_gameover), "/usr/share/pgs/apps/sushi/resources/gx_magicsushi_timeout.wav");
    }

    sushi_log("audio paths: move=%s", sushi_sound_move);
    sushi_log("audio paths: select=%s", sushi_sound_select);
    sushi_log("audio paths: bgm=%s", sushi_bgm);
    sushi_log("audio paths: gameover=%s", sushi_bgm_gameover);

    {
        int rc_master = system("amixer -q sset Master 70%" );
        int rc_pcm = system("amixer -q sset PCM 70%" );
        sushi_log("audio volume set rc master=%d pcm=%d", rc_master, rc_pcm);
    }
}

static void sushi_play_once(const char *path)
{
    if (path == NULL || path[0] == '\0') {
        return;
    }
    if (sushi_access(path, R_OK) != 0) {
        sushi_log("audio missing %s", path);
        return;
    }
    sushi_log("audio once %s", path);
    if (sushi_sfx_pid > 0) {
        kill(sushi_sfx_pid, SIGTERM);
        waitpid(sushi_sfx_pid, NULL, WNOHANG);
        sushi_sfx_pid = -1;
    }
    sushi_sfx_pid = fork();
    if (sushi_sfx_pid == 0) {
        char *argv[] = {"aplay", "-q", (char *)path, NULL};
        execvp("aplay", argv);
        sushi_log("audio once exec failed: %s", strerror(errno));
        _exit(0);
    }
}

static void sushi_play_loop(const char *path)
{
    if (path == NULL || path[0] == '\0') {
        return;
    }
    if (sushi_access(path, R_OK) != 0) {
        sushi_log("audio missing %s", path);
        return;
    }
    sushi_log("audio loop %s", path);
    if (sushi_bgm_pid > 0) {
        kill(sushi_bgm_pid, SIGTERM);
        if (sushi_bgm_pgid > 0) {
            kill(-sushi_bgm_pgid, SIGTERM);
        }
        waitpid(sushi_bgm_pid, NULL, WNOHANG);
        sushi_bgm_pid = -1;
        sushi_bgm_pgid = -1;
    }
    sushi_bgm_pid = fork();
    if (sushi_bgm_pid == 0) {
        setpgid(0, 0);
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "while true; do aplay -q '%s'; done", path);
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        sushi_log("audio loop exec failed: %s", strerror(errno));
        _exit(0);
    }
    sushi_bgm_pgid = sushi_bgm_pid;
}

static void sushi_signal_handler(int sig)
{
    if (sushi_log_fd < 0) {
        sushi_log_file = fopen("/root/sushi.log", "a");
        if (sushi_log_file) {
            sushi_log_fd = fileno(sushi_log_file);
        }
    }
    if (sushi_log_fd >= 0) {
        char msg[64];
        int len = snprintf(msg, sizeof(msg), "signal %d\n", sig);
        if (len > 0) {
            write(sushi_log_fd, msg, (size_t)len);
        }
    }
    signal(sig, SIG_DFL);
    raise(sig);
}

static void sushi_set_clip_full(void)
{
    sushi_clip_current.x1 = 0;
    sushi_clip_current.y1 = 0;
    sushi_clip_current.x2 = WINDOW_WIDTH - 1;
    sushi_clip_current.y2 = WINDOW_HEIGHT - 1;
}

static void sushi_clip_set(int x1, int y1, int x2, int y2)
{
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= WINDOW_WIDTH) x2 = WINDOW_WIDTH - 1;
    if (y2 >= WINDOW_HEIGHT) y2 = WINDOW_HEIGHT - 1;
    if (x2 < x1 || y2 < y1) {
        sushi_clip_current.x1 = 0;
        sushi_clip_current.y1 = 0;
        sushi_clip_current.x2 = -1;
        sushi_clip_current.y2 = -1;
        return;
    }
    sushi_clip_current.x1 = x1;
    sushi_clip_current.y1 = y1;
    sushi_clip_current.x2 = x2;
    sushi_clip_current.y2 = y2;
}

static void sushi_blend_pixel(uint32_t *dst, uint32_t src)
{
    uint8_t sa = (uint8_t)(src >> 24);
    if (sa == 255) {
        *dst = src;
        return;
    }
    if (sa == 0) {
        return;
    }
    uint32_t d = *dst;
    uint8_t dr = (uint8_t)(d >> 16);
    uint8_t dg = (uint8_t)(d >> 8);
    uint8_t db = (uint8_t)d;
    uint8_t sr = (uint8_t)(src >> 16);
    uint8_t sg = (uint8_t)(src >> 8);
    uint8_t sb = (uint8_t)src;
    uint8_t inv = (uint8_t)(255 - sa);
    uint8_t r = (uint8_t)((sa * sr + inv * dr) / 255);
    uint8_t g = (uint8_t)((sa * sg + inv * dg) / 255);
    uint8_t b = (uint8_t)((sa * sb + inv * db) / 255);
    *dst = (0xFFu << 24) | (r << 16) | (g << 8) | b;
}

static void sushi_log_draw(const char *fmt, ...)
{
    if (sushi_log_draw_count > 200) {
        return;
    }
    sushi_log_draw_count++;

    if (sushi_log_file == NULL) {
        sushi_log_file = fopen("/root/sushi.log", "a");
        if (sushi_log_file == NULL) {
            return;
        }
        setvbuf(sushi_log_file, NULL, _IOLBF, 0);
        sushi_log_fd = fileno(sushi_log_file);
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(sushi_log_file, fmt, args);
    va_end(args);
    fputc('\n', sushi_log_file);
    fflush(sushi_log_file);
}

static void sushi_blit_image(int dst_x, int dst_y, const lv_image_dsc_t *img)
{
    if (sushi_fb == NULL || img == NULL || img->data == NULL) {
        sushi_log_draw("blit skip fb=%p img=%p data=%p", sushi_fb, img, img ? img->data : NULL);
        return;
    }
    if (sushi_clip_current.x2 < sushi_clip_current.x1 || sushi_clip_current.y2 < sushi_clip_current.y1) {
        sushi_log_draw("blit skip clip %d,%d %d,%d", sushi_clip_current.x1, sushi_clip_current.y1,
                       sushi_clip_current.x2, sushi_clip_current.y2);
        return;
    }

    int img_w = (int)img->header.w;
    int img_h = (int)img->header.h;
    int x1 = dst_x;
    int y1 = dst_y;
    int x2 = dst_x + img_w - 1;
    int y2 = dst_y + img_h - 1;

    if (x2 < sushi_clip_current.x1 || x1 > sushi_clip_current.x2 ||
        y2 < sushi_clip_current.y1 || y1 > sushi_clip_current.y2) {
        sushi_log_draw("blit clip out %d,%d %d,%d", x1, y1, x2, y2);
        return;
    }

    int start_x = x1 < sushi_clip_current.x1 ? sushi_clip_current.x1 : x1;
    int start_y = y1 < sushi_clip_current.y1 ? sushi_clip_current.y1 : y1;
    int end_x = x2 > sushi_clip_current.x2 ? sushi_clip_current.x2 : x2;
    int end_y = y2 > sushi_clip_current.y2 ? sushi_clip_current.y2 : y2;

    const uint32_t *src = (const uint32_t *)img->data;
    uint32_t *dst = (uint32_t *)sushi_fb;
    int dst_stride = WINDOW_WIDTH;

    for (int y = start_y; y <= end_y; y++) {
        int src_y = y - dst_y;
        int src_row = src_y * img_w;
        int dst_row = y * dst_stride;
        for (int x = start_x; x <= end_x; x++) {
            int src_x = x - dst_x;
            uint32_t s = src[src_row + src_x];
            sushi_blend_pixel(&dst[dst_row + x], s);
        }
    }
}

static void sushi_blit_background_rect(int x1, int y1, int x2, int y2)
{
    if (sushi_fb == NULL) {
        sushi_log_draw("bg skip fb null");
        return;
    }
    if (sushi_clip_current.x2 < sushi_clip_current.x1 || sushi_clip_current.y2 < sushi_clip_current.y1) {
        sushi_log_draw("bg skip clip %d,%d %d,%d", sushi_clip_current.x1, sushi_clip_current.y1,
                       sushi_clip_current.x2, sushi_clip_current.y2);
        return;
    }
    const lv_image_dsc_t *bg = sushi_textures[IMG_ID_GX_MAGICSUSHI_GAME_BACKGROUND];
    if (bg == NULL || bg->data == NULL) {
        sushi_log_draw("bg missing");
        uint32_t *dst = (uint32_t *)sushi_fb;
        for (int y = y1; y <= y2; y++) {
            int row = y * WINDOW_WIDTH;
            for (int x = x1; x <= x2; x++) {
                dst[row + x] = 0x00000000u;
            }
        }
        return;
    }

    int bg_w = (int)bg->header.w;
    int bg_h = (int)bg->header.h;
    const uint32_t *src = (const uint32_t *)bg->data;
    uint32_t *dst = (uint32_t *)sushi_fb;

    for (int y = y1; y <= y2; y++) {
        int src_y = y < bg_h ? y : (bg_h - 1);
        int src_row = src_y * bg_w;
        int dst_row = y * WINDOW_WIDTH;
        for (int x = x1; x <= x2; x++) {
            int src_x = x < bg_w ? x : (bg_w - 1);
            dst[dst_row + x] = src[src_row + src_x] | 0xFF000000u;
        }
    }
}

static uint32_t sushi_color_from_gdi(U32 c)
{
    switch (c) {
        case GDI_COLOR_GREEN: return 0xFF00FF00u;
        case GDI_COLOR_RED: return 0xFFFF0000u;
        case GDI_COLOR_BLUE: return 0xFF0000FFu;
        case fg_color: return 0xFFFFFFFFu;
        case bg_color: return 0xFF000000u;
        default: return c;
    }
}

static void sushi_fill_rect(int x1, int y1, int x2, int y2, uint32_t color)
{
    if (sushi_fb == NULL) {
        sushi_log_draw("fill skip fb null");
        return;
    }
    if (sushi_clip_current.x2 < sushi_clip_current.x1 || sushi_clip_current.y2 < sushi_clip_current.y1) {
        sushi_log_draw("fill skip clip %d,%d %d,%d", sushi_clip_current.x1, sushi_clip_current.y1,
                       sushi_clip_current.x2, sushi_clip_current.y2);
        return;
    }
    if (x2 < sushi_clip_current.x1 || x1 > sushi_clip_current.x2 ||
        y2 < sushi_clip_current.y1 || y1 > sushi_clip_current.y2) {
        return;
    }
    int start_x = x1 < sushi_clip_current.x1 ? sushi_clip_current.x1 : x1;
    int start_y = y1 < sushi_clip_current.y1 ? sushi_clip_current.y1 : y1;
    int end_x = x2 > sushi_clip_current.x2 ? sushi_clip_current.x2 : x2;
    int end_y = y2 > sushi_clip_current.y2 ? sushi_clip_current.y2 : y2;

    uint32_t *dst = (uint32_t *)sushi_fb;
    for (int y = start_y; y <= end_y; y++) {
        int row = y * WINDOW_WIDTH;
        for (int x = start_x; x <= end_x; x++) {
            dst[row + x] = color;
        }
    }
}

static void sushi_timer_dispatch(lv_timer_t *timer)
{
    (void)timer;
    if (sushi_timer_callback && !sushi_paused) {
        sushi_timer_callback();
    }
}

static void sushi_assets_load(void)
{
    const char * base = pgs_get_path();
    char full[256];
    size_t count = sizeof(sushi_assets) / sizeof(sushi_assets[0]);

    if (base == NULL || base[0] == '\0') {
        base = "/usr/share/pgs/apps/sushi";
    }

    snprintf(full, sizeof(full), "%s/resources", base);
    if (sushi_access(full, R_OK) != 0) {
        base = "/usr/share/pgs/apps/sushi";
    }

    sushi_log("assets base=%s", base);

    for (size_t i = 0; i < count; i++) {
        snprintf(full, sizeof(full), "%s/resources/%s", base, sushi_assets[i].file);
        sushi_textures[sushi_assets[i].id] = pgs_libpng_decode(full);
        if (sushi_textures[sushi_assets[i].id] == NULL) {
            sushi_log("missing %s", full);
            snprintf(full, sizeof(full), "%s/resources/%s", "/root/pgs/apps/sushi", sushi_assets[i].file);
            sushi_textures[sushi_assets[i].id] = pgs_libpng_decode(full);
        }
        if (sushi_textures[sushi_assets[i].id] == NULL) {
            sushi_log("missing %s", full);
            snprintf(full, sizeof(full), "%s/resources/%s", "/usr/share/pgs/apps/sushi", sushi_assets[i].file);
            sushi_textures[sushi_assets[i].id] = pgs_libpng_decode(full);
        }
        if (sushi_textures[sushi_assets[i].id] == NULL) {
            sushi_log("failed %s", sushi_assets[i].file);
        } else {
            sushi_log("loaded %s", sushi_assets[i].file);
        }
    }
}

static void sushi_event_cb(lv_event_t *event)
{
    int32_t code = lv_event_get_code(event);
    if (code == LV_EVENT_KEY && sushi_key_callback) {
        sushi_key_callback(lv_indev_get_key(lv_indev_active()));
    }
}

lv_obj_t * sushi_app_init(lv_obj_t * parent, lv_group_t * group, void (*key_cb)(uint32_t keycode))
{
    size_t buffer_size = WINDOW_WIDTH * WINDOW_HEIGHT * 4;

    sushi_key_callback = key_cb;
    sushi_paused = false;

    sushi_log("init %dx%d", WINDOW_WIDTH, WINDOW_HEIGHT);
    sushi_log("before assets");

    sushi_fb = malloc(buffer_size);
    if (sushi_fb) {
        memset(sushi_fb, 0, buffer_size);
    }
    else {
        sushi_log("framebuffer alloc failed: %u", (unsigned)buffer_size);
        return NULL;
    }

    signal(SIGSEGV, sushi_signal_handler);
    signal(SIGABRT, sushi_signal_handler);
#ifdef SIGBUS
    signal(SIGBUS, sushi_signal_handler);
#endif

    sushi_screen_dsc.header.cf = LV_COLOR_FORMAT_ARGB8888;
    sushi_screen_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    sushi_screen_dsc.header.w = WINDOW_WIDTH;
    sushi_screen_dsc.header.h = WINDOW_HEIGHT;
    sushi_screen_dsc.header.stride = WINDOW_WIDTH * 4;
    sushi_screen_dsc.data_size = buffer_size;
    sushi_screen_dsc.data = sushi_fb;

    sushi_img = lv_image_create(parent);
    lv_image_set_src(sushi_img, &sushi_screen_dsc);
    lv_obj_center(sushi_img);
    lv_obj_add_event_cb(sushi_img, sushi_event_cb, LV_EVENT_KEY, NULL);
    if (group) {
        lv_group_add_obj(group, sushi_img);
        lv_group_focus_obj(sushi_img);
    }

    sushi_set_clip_full();
    sushi_assets_load();
    sushi_log("after assets");

    sushi_audio_init();

    mmi_gx_magicsushi_enter_game();
    sushi_log("after enter game");
    mmi_gx_magicsushi_render();
    sushi_log("after render");
    if (sushi_img) {
        lv_obj_invalidate(sushi_img);
    }

    return sushi_img;
}

void sushi_app_update(void)
{
    if (lv_tick_elaps(sushi_timer_log_tick) >= 1000) {
        sushi_timer_log_tick = lv_tick_get();
        sushi_log("tick %u", (unsigned)sushi_timer_log_tick);
    }
    if (sushi_timer_callback && !sushi_paused && sushi_timer_period > 0) {
        if (lv_tick_elaps(sushi_timer_last_tick) >= sushi_timer_period) {
            sushi_timer_last_tick = lv_tick_get();
            sushi_timer_callback();
        }
    }
    if (sushi_img) {
        lv_obj_invalidate(sushi_img);
    }
}

void sushi_app_handle_key(uint32_t keycode)
{
    sushi_log_draw("key %u", (unsigned)keycode);
    switch (keycode) {
        case LV_KEY_UP:
            mmi_gx_magicsushi_key_2_release();
            break;
        case LV_KEY_DOWN:
            mmi_gx_magicsushi_key_8_release();
            break;
        case LV_KEY_LEFT:
            mmi_gx_magicsushi_key_4_release();
            break;
        case LV_KEY_RIGHT:
            mmi_gx_magicsushi_key_6_down();
            mmi_gx_magicsushi_key_6_release();
            break;
        case LV_KEY_ENTER:
        case 28:
        case 96:
            mmi_gx_magicsushi_key_5_release();
            break;
        default:
            break;
    }
}

void sushi_app_request_pause(void)
{
    sushi_paused = true;
}

void sushi_app_request_resume(void)
{
    sushi_paused = false;
}

void sushi_app_request_stop(void)
{
    sushi_paused = true;
    GFX_STOP_BACKGROUND_SOUND(0);
}

void SetKeyHandler(void (*handler)(void), KEY key, EVENT event)
{
    (void)handler;
    (void)key;
    (void)event;
}

void gui_cancel_timer(void (*callback)(void))
{
    (void)callback;
    sushi_timer_callback = NULL;
    sushi_timer_period = 0;
}

void gui_start_timer(U32 milliseconds, void (*callback)(void))
{
    sushi_timer_callback = callback;
    sushi_timer_period = milliseconds;
    sushi_timer_last_tick = lv_tick_get();
}

void gdi_layer_free(gdi_handle handle)
{
    (void)handle;
}

void gdi_layer_set_active(gdi_handle handle)
{
    (void)handle;
}

void gdi_layer_multi_layer_disable(void)
{
}

void gdi_layer_multi_layer_enable(void)
{
}

void gdi_layer_get_base_handle(gdi_handle *handle)
{
    if (handle) {
        *handle = (gdi_handle)1;
    }
}

void gdi_layer_create(S32 x, S32 y, S32 w, S32 h, gdi_handle *handle)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    if (handle) {
        *handle = (gdi_handle)2;
    }
}

void gdi_layer_set_source_key(BOOL enable, U32 color)
{
    (void)enable;
    (void)color;
}

void gdi_layer_flatten_to_base(gdi_handle base, gdi_handle layer, S32 x, S32 y)
{
    (void)base;
    (void)layer;
    (void)x;
    (void)y;
}

void gdi_layer_blt(gdi_handle layer0, gdi_handle layer1, gdi_handle layer2, gdi_handle layer3,
                   S32 x, S32 y, S32 w, S32 h)
{
    (void)layer0;
    (void)layer1;
    (void)layer2;
    (void)layer3;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    if (sushi_img) {
        lv_obj_invalidate(sushi_img);
    }
}

void wgui_register_pen_down_handler(void (*handler)(mmi_pen_point_struct pos))
{
    (void)handler;
}

void wgui_register_pen_up_handler(void (*handler)(mmi_pen_point_struct pos))
{
    (void)handler;
}

void wgui_register_pen_move_handler(void (*handler)(mmi_pen_point_struct pos))
{
    (void)handler;
}

void gdi_layer_push_clip(void)
{
    if (sushi_clip_sp + 1 < SUSHI_CLIP_STACK_MAX) {
        sushi_clip_stack[++sushi_clip_sp] = sushi_clip_current;
    }
}

void gdi_layer_pop_clip(void)
{
    if (sushi_clip_sp >= 0) {
        sushi_clip_current = sushi_clip_stack[sushi_clip_sp--];
    } else {
        sushi_set_clip_full();
    }
}

void gdi_layer_set_clip(S32 x, S32 y, S32 w, S32 h)
{
    sushi_clip_set(x, y, w, h);
}

void gdi_layer_clear_background(U32 c)
{
    sushi_log_draw("clear bg c=%u fb=%p", c, sushi_fb);
    if (c == GDI_COLOR_TRANSPARENT || c == GDI_COLOR_BLUE) {
        const lv_image_dsc_t *bg = sushi_textures[IMG_ID_GX_MAGICSUSHI_GAME_BACKGROUND];
        sushi_log_draw("clear bg use background %p", bg);
        if (bg) {
            sushi_log_draw("bg size %u %u", (unsigned)bg->header.w, (unsigned)bg->header.h);
        }
        sushi_blit_background_rect(0, 0, WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1);
        return;
    }
    sushi_fill_rect(0, 0, WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1, sushi_color_from_gdi(c));
}

void gdi_image_draw_id(S32 x, S32 y, TEXTURE texture_id)
{
    if (texture_id >= TEXTURE_MAX) {
        return;
    }
    sushi_blit_image(x, y, sushi_textures[texture_id]);
}

void gdi_draw_solid_rect(S32 x, S32 y, S32 w, S32 h, U32 c)
{
    if (c == GDI_COLOR_TRANSPARENT) {
        int x1 = x < sushi_clip_current.x1 ? sushi_clip_current.x1 : x;
        int y1 = y < sushi_clip_current.y1 ? sushi_clip_current.y1 : y;
        int x2 = w > sushi_clip_current.x2 ? sushi_clip_current.x2 : w;
        int y2 = h > sushi_clip_current.y2 ? sushi_clip_current.y2 : h;
        if (x1 <= x2 && y1 <= y2) {
            sushi_blit_background_rect(x1, y1, x2, y2);
        }
        return;
    }
    sushi_fill_rect(x, y, w, h, sushi_color_from_gdi(c));
}

void mmi_gfx_draw_gameover_screen(S32 gameover_id, S32 field_id, S32 pic_id, U16 grade)
{
    gdi_draw_solid_rect(0, 0, WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1, 0xFFFCDF82u);
    gdi_image_draw_id(SUSHI_GAMEOVER_TITLE_X, SUSHI_GAMEOVER_TITLE_Y, (TEXTURE)gameover_id);
    gdi_image_draw_id(SUSHI_GAMEOVER_SCORE_BG_X, SUSHI_GAMEOVER_SCORE_BG_Y, (TEXTURE)field_id);
    gdi_image_draw_id(SUSHI_GAMEOVER_FOOD_X, SUSHI_GAMEOVER_FOOD_Y, (TEXTURE)pic_id);
    gdi_image_draw_id(SUSHI_GAMEOVER_SCORE_LABEL_X, SUSHI_GAMEOVER_SCORE_LABEL_Y, (TEXTURE)IMG_ID_GX_MAGICSUSHI_GRADEMAP);
    mmi_gx_magicsushi_draw_digit(
        SUSHI_GAMEOVER_SCORE_X,
        SUSHI_GAMEOVER_SCORE_Y,
        grade);
    if (sushi_img) {
        lv_obj_invalidate(sushi_img);
    }
}

void GFX_PLAY_SOUND_EFFECTS_MIDI(S32 music_id)
{
    switch (music_id) {
        case SOUND_MOVE:
            sushi_play_once(sushi_sound_move);
            break;
        case SOUND_SELECT:
            sushi_play_once(sushi_sound_select);
            break;
        default:
            break;
    }
}

void GFX_PLAY_AUDIO_MIDI(const U8 *audio, S32 len, S32 device)
{
    (void)audio;
    (void)len;
    (void)device;
}

void GFX_STOP_SOUND_EFFECTS_MIDI(S32 music_id)
{
    (void)music_id;
}

void GFX_PLAY_BACKGROUND_SOUND(S32 music_id)
{
    switch (music_id) {
        case MUSIC_BACKGROUND:
            if (sushi_bgm_pid > 0) {
                break;
            }
            sushi_play_loop(sushi_bgm);
            break;
        case MUSIC_GAMEOVER:
            GFX_STOP_BACKGROUND_SOUND(0);
            sushi_play_once(sushi_bgm_gameover);
            break;
        default:
            break;
    }
}

void GFX_STOP_BACKGROUND_SOUND(S32 music_id)
{
    (void)music_id;
    if (sushi_bgm_pid > 0) {
        kill(sushi_bgm_pid, SIGTERM);
        if (sushi_bgm_pgid > 0) {
            kill(-sushi_bgm_pgid, SIGTERM);
        }
        waitpid(sushi_bgm_pid, NULL, WNOHANG);
        if (sushi_bgm_pgid > 0) {
            kill(-sushi_bgm_pgid, SIGKILL);
        }
        kill(sushi_bgm_pid, SIGKILL);
        waitpid(sushi_bgm_pid, NULL, WNOHANG);
        sushi_bgm_pid = -1;
        sushi_bgm_pgid = -1;
    }
    if (sushi_sfx_pid > 0) {
        kill(sushi_sfx_pid, SIGTERM);
        waitpid(sushi_sfx_pid, NULL, WNOHANG);
        sushi_sfx_pid = -1;
    }
}

void GoBackHistory(void)
{
    if (sushi_key_callback) {
        sushi_key_callback(LV_KEY_ESC);
    }
}

void sushi_debug_mark(int step)
{
    sushi_log("mark %d", step);
}

void sushi_debug_time(U16 tick, S16 remain_time, S16 remainder)
{
    sushi_log("time tick=%u remain=%d remainder=%d", (unsigned)tick, remain_time, remainder);
}
