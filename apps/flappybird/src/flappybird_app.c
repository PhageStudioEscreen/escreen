#include "flappybird_app.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pgs_modules.h"
#include "pgs_utils.h"

#define FLAPPY_SCREEN_WIDTH 536
#define FLAPPY_SCREEN_HEIGHT 240
#define FLAPPY_FLOOR_Y 201
#define FLAPPY_FLOOR_HEIGHT (FLAPPY_SCREEN_HEIGHT - FLAPPY_FLOOR_Y)
#define FLAPPY_PLAY_HEIGHT FLAPPY_FLOOR_Y

#define FLAPPY_BIRD_X 110.0f
#define FLAPPY_BIRD_DRAW_H 28
#define FLAPPY_PIPE_WIDTH 52
#define FLAPPY_PIPE_HEIGHT 320
#define FLAPPY_PIPE_GAP 92.0f
#define FLAPPY_PIPE_GAP_MIN 28.0f
#define FLAPPY_PIPE_GAP_MAX 82.0f
#define FLAPPY_PIPE_SPACING 210.0f
#define FLAPPY_PIPE_SPEED 150.0f

#define FLAPPY_BIRD_GRAVITY 820.0f
#define FLAPPY_BIRD_FLAP_VELOCITY (-255.0f)
#define FLAPPY_BIRD_MAX_FALL_SPEED 280.0f

#define FLAPPY_BASE_SPEED 150.0f
#define FLAPPY_SPLASH_BOB_SPEED 3.6f
#define FLAPPY_SPLASH_BOB_AMPLITUDE 8.0f

#define FLAPPY_SCORE_Y 12
#define FLAPPY_MESSAGE_Y 12
#define FLAPPY_GAME_OVER_Y 28

#define FLAPPY_GAME_OVER_X 172
#define FLAPPY_GAME_OVER_Y_FIXED 99
#define FLAPPY_SCORE_PANEL_X 195
#define FLAPPY_SCORE_PANEL_Y 36
#define FLAPPY_SCORE_VALUE_X 318
#define FLAPPY_SCORE_VALUE_Y 50

#define FLAPPY_HUD_SCORE_X 10
#define FLAPPY_HUD_SCORE_Y 10
#define FLAPPY_HUD_SPLASH_X 36
#define FLAPPY_HUD_SPLASH_Y 10
#define FLAPPY_HUD_BEST_X 64
#define FLAPPY_HUD_BEST_Y 10

#define FLAPPY_MESSAGE_HEIGHT 96
#define FLAPPY_GAME_OVER_HEIGHT 42

#define FLAPPY_MAX_PIPES 4

typedef enum {
    FLAPPY_STATE_SPLASH = 0,
    FLAPPY_STATE_PLAYING,
    FLAPPY_STATE_GAME_OVER,
} flappy_state_t;

typedef struct {
    float x;
    float gap_y;
    bool scored;
} flappy_pipe_t;

typedef struct {
    const lv_image_dsc_t * background;
    const lv_image_dsc_t * background_day;
    const lv_image_dsc_t * background_night;
    const lv_image_dsc_t * base;
    const lv_image_dsc_t * pipe;
    const lv_image_dsc_t * bird[3];
    const lv_image_dsc_t * digits[10];
    const lv_image_dsc_t * message;
    const lv_image_dsc_t * game_over;
    const lv_image_dsc_t * score_panel;
    const lv_image_dsc_t * splash;
} flappy_assets_t;

typedef struct {
    lv_obj_t * root;
    lv_obj_t * image;
    lv_group_t * group;
    void (*key_cb)(uint32_t keycode);

    lv_image_dsc_t screen_dsc;
    uint8_t * framebuffer;

    flappy_assets_t assets;
    char asset_base[256];
    bool assets_loaded;

    flappy_state_t state;
    uint32_t last_tick_ms;
    float splash_time;
    float bird_y;
    float bird_velocity;
    float bird_base_y;
    float bird_rotation;
    float base_offset;
    float game_over_delay;
    uint32_t score;
    uint32_t best_score;
    uint32_t frame_counter;

    flappy_pipe_t pipes[FLAPPY_MAX_PIPES];
    uint32_t pipe_count;
} flappy_app_t;

static flappy_app_t g_app;

static void flappy_log(const char * fmt, ...);

static const char * flappy_audio_root(void)
{
    return "/usr/share/pgs/apps/flappybird/resources/audio";
}

static void play_sound(const char * name)
{
    char path[320];
    char cmd[384];
    int rc;

    lv_snprintf(path, sizeof(path), "%s/%s", flappy_audio_root(), name);
    if (access(path, R_OK) != 0) {
        flappy_log("audio missing: %s", path);
        return;
    }

    lv_snprintf(cmd, sizeof(cmd), "aplay -q %s >/dev/null 2>&1 &", path);
    rc = system(cmd);
    flappy_log("audio play: %s rc=%d", path, rc);
}

static const char * flappy_best_path(void)
{
    return "/usr/share/pgs/apps/flappybird/best";
}

static void load_best_score(void)
{
    FILE * f = fopen(flappy_best_path(), "r");
    if (!f) {
        g_app.best_score = 0;
        return;
    }

    if (fscanf(f, "%u", &g_app.best_score) != 1) {
        g_app.best_score = 0;
    }

    fclose(f);
}

static void save_best_score(uint32_t value)
{
    FILE * f = fopen(flappy_best_path(), "w");
    if (!f) {
        return;
    }
    fprintf(f, "%u", value);
    fclose(f);
}

static void flappy_log(const char * fmt, ...)
{
    FILE * f = fopen("/root/flappybird.log", "a");
    if (!f) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fputc('\n', f);
    fclose(f);
}

static void key_event_cb(lv_event_t * event)
{
    if (lv_event_get_code(event) == LV_EVENT_KEY && g_app.key_cb) {
        g_app.key_cb(lv_indev_get_key(lv_indev_active()));
    }
}

static const char * get_asset_root(void)
{
    lv_snprintf(g_app.asset_base, sizeof(g_app.asset_base), "/usr/share/pgs/apps/flappybird/resources/assets");
    flappy_log("assets base fixed: %s", g_app.asset_base);
    return g_app.asset_base;
}

static const lv_image_dsc_t * load_png(const char * name)
{
    char path[320];
    const char * root = get_asset_root();

    lv_snprintf(path, sizeof(path), "%s/%s", root, name);
    flappy_log("load png: %s", path);
    return pgs_libpng_decode(path);
}

static bool load_assets(void)
{
    static const char * digit_names[10] = {
        "0.png", "1.png", "2.png", "3.png", "4.png",
        "5.png", "6.png", "7.png", "8.png", "9.png",
    };

    memset(&g_app.assets, 0, sizeof(g_app.assets));

    g_app.assets.background_day = load_png("background-day.png");
    g_app.assets.background_night = load_png("background-night.png");
    g_app.assets.background = g_app.assets.background_day;
    g_app.assets.base = load_png("base.png");
    g_app.assets.pipe = load_png("pipe-green.png");
    g_app.assets.bird[0] = load_png("yellowbird-upflap.png");
    g_app.assets.bird[1] = load_png("yellowbird-midflap.png");
    g_app.assets.bird[2] = load_png("yellowbird-downflap.png");
    g_app.assets.message = load_png("message.png");
    g_app.assets.game_over = load_png("gameover.png");
    g_app.assets.score_panel = load_png("score.png");
    g_app.assets.splash = load_png("splash.png");

    for (uint32_t i = 0; i < 10; i++) {
        g_app.assets.digits[i] = load_png(digit_names[i]);
    }

    if (!g_app.assets.background_day || !g_app.assets.background_night ||
        !g_app.assets.base || !g_app.assets.pipe ||
        !g_app.assets.bird[0] || !g_app.assets.bird[1] || !g_app.assets.bird[2] ||
        !g_app.assets.message || !g_app.assets.game_over ||
        !g_app.assets.score_panel || !g_app.assets.splash) {
        flappy_log("asset load failed: day=%p night=%p base=%p pipe=%p bird0=%p bird1=%p bird2=%p message=%p gameover=%p",
                   g_app.assets.background_day, g_app.assets.background_night, g_app.assets.base, g_app.assets.pipe,
                   g_app.assets.bird[0], g_app.assets.bird[1], g_app.assets.bird[2],
                   g_app.assets.message, g_app.assets.game_over);
        flappy_log("asset load failed: score_panel=%p splash=%p",
                   g_app.assets.score_panel, g_app.assets.splash);
        return false;
    }

    for (uint32_t i = 0; i < 10; i++) {
        if (!g_app.assets.digits[i]) {
            flappy_log("asset load failed: digit %u missing", i);
            return false;
        }
    }

    flappy_log("assets loaded ok");
    g_app.assets_loaded = true;
    return true;
}

static uint32_t image_width(const lv_image_dsc_t * img)
{
    return img ? img->header.w : 0;
}

static uint32_t image_height(const lv_image_dsc_t * img)
{
    return img ? img->header.h : 0;
}

static void fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t b, uint8_t g, uint8_t r, uint8_t a)
{
    if (!g_app.framebuffer || w <= 0 || h <= 0) {
        return;
    }

    int32_t x0 = x < 0 ? 0 : x;
    int32_t y0 = y < 0 ? 0 : y;
    int32_t x1 = x + w > FLAPPY_SCREEN_WIDTH ? FLAPPY_SCREEN_WIDTH : x + w;
    int32_t y1 = y + h > FLAPPY_SCREEN_HEIGHT ? FLAPPY_SCREEN_HEIGHT : y + h;

    for (int32_t py = y0; py < y1; py++) {
        uint8_t * row = g_app.framebuffer + (size_t)py * FLAPPY_SCREEN_WIDTH * 4U;
        for (int32_t px = x0; px < x1; px++) {
            uint8_t * dst = row + (size_t)px * 4U;
            dst[0] = b;
            dst[1] = g;
            dst[2] = r;
            dst[3] = a;
        }
    }
}

static void clear_frame(void)
{
    fill_rect(0, 0, FLAPPY_SCREEN_WIDTH, FLAPPY_SCREEN_HEIGHT, 0xE9, 0xBE, 0x70, 0xFF);
}

static void blend_bgra_pixel(uint8_t * dst, const uint8_t * src)
{
    uint32_t alpha = src[3];

    if (alpha == 0) {
        return;
    }

    if (alpha == 255) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = 0xFF;
        return;
    }

    uint32_t inv = 255U - alpha;
    dst[0] = (uint8_t)((src[0] * alpha + dst[0] * inv) / 255U);
    dst[1] = (uint8_t)((src[1] * alpha + dst[1] * inv) / 255U);
    dst[2] = (uint8_t)((src[2] * alpha + dst[2] * inv) / 255U);
    dst[3] = 0xFF;
}

static void draw_image_scaled(const lv_image_dsc_t * img, int32_t dx, int32_t dy, int32_t dw, int32_t dh, bool flip_y)
{
    if (!img || !img->data || dw <= 0 || dh <= 0) {
        return;
    }

    int32_t clip_x0 = dx < 0 ? 0 : dx;
    int32_t clip_y0 = dy < 0 ? 0 : dy;
    int32_t clip_x1 = dx + dw > FLAPPY_SCREEN_WIDTH ? FLAPPY_SCREEN_WIDTH : dx + dw;
    int32_t clip_y1 = dy + dh > FLAPPY_SCREEN_HEIGHT ? FLAPPY_SCREEN_HEIGHT : dy + dh;
    int32_t src_w = (int32_t)img->header.w;
    int32_t src_h = (int32_t)img->header.h;
    const uint8_t * src_data = img->data;

    for (int32_t py = clip_y0; py < clip_y1; py++) {
        int32_t local_y = py - dy;
        int32_t sy = (local_y * src_h) / dh;
        if (flip_y) {
            sy = src_h - 1 - sy;
        }

        for (int32_t px = clip_x0; px < clip_x1; px++) {
            int32_t local_x = px - dx;
            int32_t sx = (local_x * src_w) / dw;
            const uint8_t * src = src_data + ((size_t)sy * src_w + (size_t)sx) * 4U;
            uint8_t * dst = g_app.framebuffer + ((size_t)py * FLAPPY_SCREEN_WIDTH + (size_t)px) * 4U;
            blend_bgra_pixel(dst, src);
        }
    }
}

static int32_t scaled_width_for_height(const lv_image_dsc_t * img, int32_t target_h)
{
    uint32_t src_w = image_width(img);
    uint32_t src_h = image_height(img);

    if (!src_w || !src_h || target_h <= 0) {
        return 0;
    }

    return (int32_t)((int64_t)src_w * target_h / (int32_t)src_h);
}

static void spawn_pipe(float x)
{
    if (g_app.pipe_count >= FLAPPY_MAX_PIPES) {
        return;
    }

    float gap_range = FLAPPY_PIPE_GAP_MAX - FLAPPY_PIPE_GAP_MIN;
    float gap_y = FLAPPY_PIPE_GAP_MIN;

    if (gap_range > 0.0f) {
        gap_y += ((float)rand() / (float)RAND_MAX) * gap_range;
    }

    g_app.pipes[g_app.pipe_count].x = x;
    g_app.pipes[g_app.pipe_count].gap_y = gap_y;
    g_app.pipes[g_app.pipe_count].scored = false;
    g_app.pipe_count++;
}

static void reset_round(void)
{
    if (rand() % 2 == 0) {
        g_app.assets.background = g_app.assets.background_day;
    } else {
        g_app.assets.background = g_app.assets.background_night;
    }
    g_app.state = FLAPPY_STATE_SPLASH;
    g_app.splash_time = 0.0f;
    g_app.bird_base_y = 72.0f;
    g_app.bird_y = g_app.bird_base_y;
    g_app.bird_velocity = 0.0f;
    g_app.bird_rotation = 0.0f;
    g_app.base_offset = 0.0f;
    g_app.game_over_delay = 0.0f;
    g_app.score = 0;
    g_app.frame_counter = 0;
    g_app.pipe_count = 0;

    spawn_pipe(FLAPPY_SCREEN_WIDTH + 80.0f);
    spawn_pipe(FLAPPY_SCREEN_WIDTH + 80.0f + FLAPPY_PIPE_SPACING);
    spawn_pipe(FLAPPY_SCREEN_WIDTH + 80.0f + FLAPPY_PIPE_SPACING * 2.0f);
}

static void start_playing(void)
{
    g_app.state = FLAPPY_STATE_PLAYING;
    g_app.bird_velocity = FLAPPY_BIRD_FLAP_VELOCITY;
    g_app.bird_rotation = -18.0f;
}

static void trigger_game_over(void)
{
    play_sound("hit.wav");
    play_sound("die.wav");
    if (g_app.score > g_app.best_score) {
        g_app.best_score = g_app.score;
        save_best_score(g_app.best_score);
    }
    g_app.state = FLAPPY_STATE_GAME_OVER;
    g_app.game_over_delay = 0.0f;
}

static void compact_pipes(void)
{
    uint32_t write_idx = 0;

    for (uint32_t i = 0; i < g_app.pipe_count; i++) {
        if (g_app.pipes[i].x + FLAPPY_PIPE_WIDTH > -8.0f) {
            if (write_idx != i) {
                g_app.pipes[write_idx] = g_app.pipes[i];
            }
            write_idx++;
        }
    }

    g_app.pipe_count = write_idx;
}

static float rightmost_pipe_x(void)
{
    float x = 0.0f;

    for (uint32_t i = 0; i < g_app.pipe_count; i++) {
        if (g_app.pipes[i].x > x) {
            x = g_app.pipes[i].x;
        }
    }

    return x;
}

static void update_pipes(float dt)
{
    for (uint32_t i = 0; i < g_app.pipe_count; i++) {
        flappy_pipe_t * pipe = &g_app.pipes[i];
        pipe->x -= FLAPPY_PIPE_SPEED * dt;

        if (!pipe->scored && pipe->x + FLAPPY_PIPE_WIDTH < FLAPPY_BIRD_X) {
            pipe->scored = true;
            g_app.score++;
            play_sound("point.wav");
        }
    }

    compact_pipes();

    while (g_app.pipe_count < 3) {
        float x = rightmost_pipe_x();
        if (x < FLAPPY_SCREEN_WIDTH * 0.5f) {
            x = FLAPPY_SCREEN_WIDTH + 80.0f;
        }
        spawn_pipe(x + FLAPPY_PIPE_SPACING);
    }
}

static bool bird_hits_pipe(const flappy_pipe_t * pipe)
{
    int32_t bird_w = scaled_width_for_height(g_app.assets.bird[1], FLAPPY_BIRD_DRAW_H);
    float bird_left = FLAPPY_BIRD_X + 4.0f;
    float bird_right = FLAPPY_BIRD_X + (float)bird_w - 4.0f;
    float bird_top = g_app.bird_y + 4.0f;
    float bird_bottom = g_app.bird_y + FLAPPY_BIRD_DRAW_H - 4.0f;
    float pipe_left = pipe->x;
    float pipe_right = pipe->x + FLAPPY_PIPE_WIDTH;
    float gap_top = pipe->gap_y;
    float gap_bottom = pipe->gap_y + FLAPPY_PIPE_GAP;

    if (bird_right <= pipe_left || bird_left >= pipe_right) {
        return false;
    }

    return bird_top < gap_top || bird_bottom > gap_bottom;
}

static void update_playing(float dt)
{
    g_app.base_offset += FLAPPY_BASE_SPEED * dt;
    g_app.bird_velocity += FLAPPY_BIRD_GRAVITY * dt;
    if (g_app.bird_velocity > FLAPPY_BIRD_MAX_FALL_SPEED) {
        g_app.bird_velocity = FLAPPY_BIRD_MAX_FALL_SPEED;
    }

    g_app.bird_y += g_app.bird_velocity * dt;
    g_app.bird_rotation += 160.0f * dt;
    if (g_app.bird_rotation > 75.0f) {
        g_app.bird_rotation = 75.0f;
    }

    update_pipes(dt);

    if (g_app.bird_y < 0.0f) {
        g_app.bird_y = 0.0f;
    }

    if (g_app.bird_y + FLAPPY_BIRD_DRAW_H >= FLAPPY_PLAY_HEIGHT) {
        g_app.bird_y = FLAPPY_PLAY_HEIGHT - FLAPPY_BIRD_DRAW_H;
        trigger_game_over();
        return;
    }

    for (uint32_t i = 0; i < g_app.pipe_count; i++) {
        if (bird_hits_pipe(&g_app.pipes[i])) {
            trigger_game_over();
            return;
        }
    }
}

static void update_game_over(float dt)
{
    g_app.game_over_delay += dt;
    if (g_app.bird_y + FLAPPY_BIRD_DRAW_H < FLAPPY_PLAY_HEIGHT) {
        g_app.bird_velocity += FLAPPY_BIRD_GRAVITY * dt;
        if (g_app.bird_velocity > FLAPPY_BIRD_MAX_FALL_SPEED) {
            g_app.bird_velocity = FLAPPY_BIRD_MAX_FALL_SPEED;
        }
        g_app.bird_y += g_app.bird_velocity * dt;
        if (g_app.bird_y + FLAPPY_BIRD_DRAW_H > FLAPPY_PLAY_HEIGHT) {
            g_app.bird_y = FLAPPY_PLAY_HEIGHT - FLAPPY_BIRD_DRAW_H;
        }
    }
    g_app.bird_rotation += 220.0f * dt;
    if (g_app.bird_rotation > 90.0f) {
        g_app.bird_rotation = 90.0f;
    }
}

static void update_splash(float dt)
{
    g_app.base_offset += FLAPPY_BASE_SPEED * dt;
    g_app.splash_time += dt;
    g_app.bird_y = g_app.bird_base_y + sinf(g_app.splash_time * FLAPPY_SPLASH_BOB_SPEED) * FLAPPY_SPLASH_BOB_AMPLITUDE;
}

static void draw_background(void)
{
    draw_image_scaled(g_app.assets.background, 0, 0, FLAPPY_SCREEN_WIDTH, FLAPPY_SCREEN_HEIGHT, false);
}

static void draw_pipes(void)
{
    for (uint32_t i = 0; i < g_app.pipe_count; i++) {
        int32_t x = (int32_t)lroundf(g_app.pipes[i].x);
        int32_t gap_y = (int32_t)lroundf(g_app.pipes[i].gap_y);
        draw_image_scaled(g_app.assets.pipe, x, gap_y - FLAPPY_PIPE_HEIGHT, FLAPPY_PIPE_WIDTH, FLAPPY_PIPE_HEIGHT, true);
        draw_image_scaled(g_app.assets.pipe, x, gap_y + (int32_t)FLAPPY_PIPE_GAP, FLAPPY_PIPE_WIDTH, FLAPPY_PIPE_HEIGHT, false);
    }
}

static void draw_base(void)
{
    int32_t base_w = scaled_width_for_height(g_app.assets.base, FLAPPY_FLOOR_HEIGHT);
    int32_t x = -(int32_t)fmodf(g_app.base_offset, (float)base_w);

    fill_rect(0, FLAPPY_FLOOR_Y, FLAPPY_SCREEN_WIDTH, FLAPPY_FLOOR_HEIGHT, 0x7E, 0xF0, 0xDE, 0xFF);

    while (x < FLAPPY_SCREEN_WIDTH) {
        draw_image_scaled(g_app.assets.base, x, FLAPPY_FLOOR_Y, base_w, FLAPPY_FLOOR_HEIGHT, false);
        x += base_w;
    }
}

static void draw_bird(void)
{
    uint32_t frame_idx = (g_app.frame_counter / 8U) % 3U;
    int32_t bird_w = scaled_width_for_height(g_app.assets.bird[frame_idx], FLAPPY_BIRD_DRAW_H);
    int32_t bird_x = (int32_t)lroundf(FLAPPY_BIRD_X);
    int32_t bird_y = (int32_t)lroundf(g_app.bird_y);

    (void)g_app.bird_rotation;
    draw_image_scaled(g_app.assets.bird[frame_idx], bird_x, bird_y, bird_w, FLAPPY_BIRD_DRAW_H, false);
}

static void draw_number(uint32_t value, int32_t center_x, int32_t y)
{
    char text[16];
    int32_t widths[16] = {0};
    int32_t total_w = 0;
    int count = lv_snprintf(text, sizeof(text), "%u", value);

    for (int i = 0; i < count; i++) {
        uint32_t digit = (uint32_t)(text[i] - '0');
        widths[i] = (int32_t)image_width(g_app.assets.digits[digit]);
        total_w += widths[i];
    }

    int32_t x = center_x - total_w / 2;
    for (int i = 0; i < count; i++) {
        uint32_t digit = (uint32_t)(text[i] - '0');
        draw_image_scaled(g_app.assets.digits[digit], x, y, widths[i], (int32_t)image_height(g_app.assets.digits[digit]), false);
        x += widths[i];
    }
}

static void draw_number_left(uint32_t value, int32_t x, int32_t y)
{
    char text[16];
    int32_t widths[16] = {0};
    int count = lv_snprintf(text, sizeof(text), "%u", value);

    for (int i = 0; i < count; i++) {
        uint32_t digit = (uint32_t)(text[i] - '0');
        widths[i] = (int32_t)image_width(g_app.assets.digits[digit]);
    }

    for (int i = 0; i < count; i++) {
        uint32_t digit = (uint32_t)(text[i] - '0');
        draw_image_scaled(g_app.assets.digits[digit], x, y, widths[i], (int32_t)image_height(g_app.assets.digits[digit]), false);
        x += widths[i];
    }
}

static void draw_overlay(void)
{
    if (g_app.state == FLAPPY_STATE_SPLASH) {
        int32_t w = (int32_t)image_width(g_app.assets.message);
        int32_t h = (int32_t)image_height(g_app.assets.message);
        int32_t x = (FLAPPY_SCREEN_WIDTH - w) / 2;
        draw_image_scaled(g_app.assets.message, x, FLAPPY_MESSAGE_Y, w, h, false);
    }

    if (g_app.state == FLAPPY_STATE_PLAYING) {
        draw_number_left(g_app.score, FLAPPY_HUD_SCORE_X, FLAPPY_HUD_SCORE_Y);
        draw_image_scaled(
            g_app.assets.splash,
            FLAPPY_HUD_SPLASH_X,
            FLAPPY_HUD_SPLASH_Y,
            (int32_t)image_width(g_app.assets.splash),
            (int32_t)image_height(g_app.assets.splash),
            false);
        draw_number_left(g_app.best_score, FLAPPY_HUD_BEST_X, FLAPPY_HUD_BEST_Y);
    }

    if (g_app.state == FLAPPY_STATE_GAME_OVER) {
        draw_image_scaled(
            g_app.assets.game_over,
            FLAPPY_GAME_OVER_X,
            FLAPPY_GAME_OVER_Y_FIXED,
            (int32_t)image_width(g_app.assets.game_over),
            (int32_t)image_height(g_app.assets.game_over),
            false);
        draw_image_scaled(
            g_app.assets.score_panel,
            FLAPPY_SCORE_PANEL_X,
            FLAPPY_SCORE_PANEL_Y,
            (int32_t)image_width(g_app.assets.score_panel),
            (int32_t)image_height(g_app.assets.score_panel),
            false);
        draw_number_left(g_app.score, FLAPPY_SCORE_VALUE_X, FLAPPY_SCORE_VALUE_Y);
    }
}

static void render_scene(void)
{
    clear_frame();
    draw_background();
    draw_pipes();
    draw_base();
    draw_bird();
    draw_overlay();
    lv_obj_invalidate(g_app.image);
}

lv_obj_t * flappybird_app_init(lv_obj_t * parent, lv_group_t * group, void (*key_cb)(uint32_t keycode))
{
    memset(&g_app, 0, sizeof(g_app));
    g_app.group = group;
    g_app.key_cb = key_cb;

    g_app.framebuffer = lv_malloc(FLAPPY_SCREEN_WIDTH * FLAPPY_SCREEN_HEIGHT * 4U);
    if (!g_app.framebuffer) {
        return NULL;
    }
    memset(g_app.framebuffer, 0, FLAPPY_SCREEN_WIDTH * FLAPPY_SCREEN_HEIGHT * 4U);

    g_app.screen_dsc.header.cf = LV_COLOR_FORMAT_ARGB8888;
    g_app.screen_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    g_app.screen_dsc.header.w = FLAPPY_SCREEN_WIDTH;
    g_app.screen_dsc.header.h = FLAPPY_SCREEN_HEIGHT;
    g_app.screen_dsc.header.stride = FLAPPY_SCREEN_WIDTH * 4U;
    g_app.screen_dsc.data_size = FLAPPY_SCREEN_WIDTH * FLAPPY_SCREEN_HEIGHT * 4U;
    g_app.screen_dsc.data = g_app.framebuffer;

    g_app.root = lv_obj_create(parent);
    lv_obj_remove_style_all(g_app.root);
    lv_obj_set_width(g_app.root, lv_pct(100));
    lv_obj_set_height(g_app.root, lv_pct(100));
    lv_obj_set_align(g_app.root, LV_ALIGN_TOP_LEFT);
    lv_obj_clear_flag(g_app.root, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(g_app.root, key_event_cb, LV_EVENT_KEY, NULL);

    g_app.image = lv_image_create(g_app.root);
    lv_image_set_src(g_app.image, &g_app.screen_dsc);
    lv_obj_center(g_app.image);

    if (group) {
        lv_group_add_obj(group, g_app.root);
        lv_group_focus_obj(g_app.root);
    }

    if (!load_assets()) {
        flappy_log("app init failed: assets not loaded");
        clear_frame();
        fill_rect(0, 0, FLAPPY_SCREEN_WIDTH, FLAPPY_SCREEN_HEIGHT, 0x20, 0x20, 0x20, 0xFF);
        lv_obj_invalidate(g_app.image);
        return g_app.root;
    }

    load_best_score();
    g_app.last_tick_ms = lv_tick_get();
    reset_round();
    render_scene();
    return g_app.root;
}

void flappybird_app_handle_key(uint32_t keycode)
{
    bool is_flap = false;

    switch (keycode) {
        case LV_KEY_UP:
        case LV_KEY_RIGHT:
        case ' ':
        case '\n':
        case '\r':
            is_flap = true;
            break;
        default:
            break;
    }

    if (!is_flap || !g_app.assets_loaded) {
        return;
    }

    if (g_app.state == FLAPPY_STATE_SPLASH) {
        start_playing();
        play_sound("wing.wav");
        return;
    }

    if (g_app.state == FLAPPY_STATE_PLAYING) {
        g_app.bird_velocity = FLAPPY_BIRD_FLAP_VELOCITY;
        g_app.bird_rotation = -18.0f;
        play_sound("wing.wav");
        return;
    }

    if (g_app.state == FLAPPY_STATE_GAME_OVER && g_app.game_over_delay >= 0.35f) {
        reset_round();
    }
}

void flappybird_app_update(void)
{
    if (!g_app.assets_loaded) {
        return;
    }

    uint32_t now_ms = lv_tick_get();
    uint32_t delta_ms = now_ms - g_app.last_tick_ms;
    float dt = (float)delta_ms / 1000.0f;

    if (dt <= 0.0f) {
        dt = 1.0f / 60.0f;
    } else if (dt > 0.05f) {
        dt = 0.05f;
    }

    g_app.last_tick_ms = now_ms;
    g_app.frame_counter++;

    if (g_app.state == FLAPPY_STATE_SPLASH) {
        update_splash(dt);
    } else if (g_app.state == FLAPPY_STATE_PLAYING) {
        update_playing(dt);
    } else {
        update_game_over(dt);
    }

    render_scene();
}
