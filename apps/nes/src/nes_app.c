#include "nes_app.h"
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <linux/input.h>

#include "InfoNES.h"
#include "InfoNES_System.h"
#include "pgs_modules.h"

#define EVDEV_PATH "/dev/input/event3"

extern DWORD dwPad1;
extern DWORD dwPad2;
extern DWORD dwSystem;
extern WORD WorkFrame[];
extern WORD FrameSkip;
extern int start_application(char *filename);
extern void InfoNES_Main(void);
extern int SaveSRAM(void);

#define NES_SCREEN_WIDTH  256
#define NES_SCREEN_HEIGHT 240

#if LV_COLOR_DEPTH == 16
static uint16_t nes_framebuffer[2][NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT];
#else
static uint32_t nes_framebuffer[2][NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT];
#endif

typedef struct {
    lv_obj_t * img;
    lv_img_dsc_t dsc;
    char * rom_path;
    pthread_t emu_thread;
    int running;
    int paused;
    int key_states[14];
    int key_pressed;
    int key_release_counter;
} nes_app_data_t;

static nes_app_data_t nes_app_inst;
static lv_timer_t * flush_timer;
static void (*ui_key_cb)(uint32_t keycode);
static lv_obj_t * ui_container;
static int evdev_fd = -1;
static pthread_t evdev_thread;
static int nes_stop_requested = 0;
static int nes_core_stopped = 0;
static int nes_rom_pending = 0;
static int nes_evdev_thread_started = 0;
static int nes_emu_thread_started = 0;
static int nes_evdev_thread_alive = 0;
static int nes_emu_thread_alive = 0;
static int nes_paused = 0;
static const lv_image_dsc_t * nes_mask_dsc = NULL;
static int nes_mask_applied = 0;

static int key_map[256] = {0};

static void * evdev_read_thread(void * arg)
{
    (void)arg;
    struct input_event ev;

    nes_evdev_thread_alive = 1;
    
    evdev_fd = open(EVDEV_PATH, O_RDONLY | O_NONBLOCK);
    if (evdev_fd < 0) {
        printf("Failed to open %s\n", EVDEV_PATH);
        nes_evdev_thread_alive = 0;
        return NULL;
    }
    
    while (nes_app_inst.running) {
        ssize_t n = read(evdev_fd, &ev, sizeof(ev));
        if (n == sizeof(ev)) {
            if (ev.type == EV_KEY) {
                key_map[ev.code] = (ev.value == 1 || ev.value == 2) ? 1 : 0;
                nes_app_inst.key_release_counter = 0;
            }
        }
        usleep(1000);
    }
    
    close(evdev_fd);
    nes_evdev_thread_alive = 0;
    return NULL;
}

static void key_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);
    
    if (code == LV_EVENT_KEY) {
        nes_app_inst.key_pressed = 1;
        nes_app_inst.key_release_counter = 0;
        if (ui_key_cb) {
            ui_key_cb(lv_indev_get_key(lv_indev_active()));
        }
    }
}

static void update_flush_timer(lv_timer_t * timer)
{
    (void)timer;
    nes_app_data_t * app = &nes_app_inst;
    if (!app->running) return;

    uint32_t x, y, index = 0;

#if LV_COLOR_DEPTH == 16
    for (y = 0; y < NES_SCREEN_HEIGHT; y++) {
        for (x = 0; x < NES_SCREEN_WIDTH; x++) {
            nes_framebuffer[0][index] = (WorkFrame[index] >> 3) << 4 | (WorkFrame[index] & 0x001f);
            index++;
        }
    }
#else
    uint8_t red, green, blue;
    uint32_t buf_index = 0;

    for (y = 0; y < NES_SCREEN_HEIGHT; y++) {
        for (x = 0; x < NES_SCREEN_WIDTH; x++) {
            red   = (WorkFrame[index] & 0xff0000) >> 16;
            green = (WorkFrame[index] & 0xff00) >> 8;
            blue  = WorkFrame[index] & 0xff;
            nes_framebuffer[0][buf_index++] = (0xff << 24) | (red << 16) | (green << 8) | blue;
            index++;
        }
    }
#endif

    if (evdev_fd >= 0) {
        app->key_states[0] = key_map[103];
        app->key_states[1] = key_map[108];
        app->key_states[2] = key_map[105];
        app->key_states[3] = key_map[106];
        app->key_states[4] = key_map[54];
        app->key_states[5] = key_map[97];
        app->key_states[6] = key_map[28];
        app->key_states[13] = key_map[43];  // \ = Select
        // 2P
        app->key_states[7] = key_map[17];   // W = 上
        app->key_states[8] = key_map[31];   // S = 下
        app->key_states[9] = key_map[30];   // A = 左
        app->key_states[10] = key_map[32];  // D = 右
        app->key_states[11] = key_map[42];  // LEFTSHIFT = A
        app->key_states[12] = key_map[29];  // LEFTCTRL = B
        app->key_release_counter = 0;
    }

    app->dsc.data = (const uint8_t *)nes_framebuffer[0];
    lv_img_set_src(app->img, &app->dsc);

    dwPad1 = 0;
    if (app->key_states[0]) {
        dwPad1 |= (1 << 4);
    }
    if (app->key_states[1]) {
        dwPad1 |= (1 << 5);
    }
    if (app->key_states[2]) {
        dwPad1 |= (1 << 6);
    }
    if (app->key_states[3]) {
        dwPad1 |= (1 << 7);
    }
    if (app->key_states[4]) {
        dwPad1 |= (1 << 1);
    }
    if (app->key_states[5]) {
        dwPad1 |= (1 << 0);
    }
    if (app->key_states[6]) {
        dwPad1 |= (1 << 3);
    }
    if (app->key_states[13]) {
        dwPad1 |= (1 << 2);  // Select
    }

    // 2P
    dwPad2 = 0;
    if (app->key_states[7]) {
        dwPad2 |= (1 << 4);
    }
    if (app->key_states[8]) {
        dwPad2 |= (1 << 5);
    }
    if (app->key_states[9]) {
        dwPad2 |= (1 << 6);
    }
    if (app->key_states[10]) {
        dwPad2 |= (1 << 7);
    }
    if (app->key_states[11]) {
        dwPad2 |= (1 << 1);
    }
    if (app->key_states[12]) {
        dwPad2 |= (1 << 0);
    }
    
    if (!app->key_pressed) {
        app->key_release_counter++;
        if (app->key_release_counter > 10) {
            memset(app->key_states, 0, sizeof(app->key_states));
        }
    } else {
        app->key_pressed = 0;
    }
}

static void * nes_emulator_thread(void * arg)
{
    nes_app_data_t * app = (nes_app_data_t *)arg;

    nes_emu_thread_alive = 1;

    if (app->rom_path == NULL) {
        printf("No ROM file specified!\n");
        nes_emu_thread_alive = 0;
        return NULL;
    }

    if (start_application(app->rom_path) == 0) {
        printf("Failed to load ROM: %s\n", app->rom_path);
        nes_emu_thread_alive = 0;
        return NULL;
    }

    SaveSRAM();

    InfoNES_Main();

    nes_emu_thread_alive = 0;
    return NULL;
}

static void nes_start_emulator(void)
{
    if (!nes_app_inst.rom_path || !*nes_app_inst.rom_path) {
        printf("No ROM file specified!\n");
        return;
    }

    nes_paused = 0;
    InfoNES_SetPause(0);
    InfoNES_RequestQuit(0);

    memset(nes_app_inst.key_states, 0, sizeof(nes_app_inst.key_states));
    nes_app_inst.key_pressed = 0;
    nes_app_inst.key_release_counter = 0;

    dwPad1 = 0;
    dwPad2 = 0;
    dwSystem = 0;
    FrameSkip = 0;

    nes_app_inst.running = 1;
    nes_evdev_thread_started = (pthread_create(&evdev_thread, NULL, evdev_read_thread, NULL) == 0);
    nes_emu_thread_started = (pthread_create(&nes_app_inst.emu_thread, NULL, nes_emulator_thread, &nes_app_inst) == 0);

    if (flush_timer) {
        lv_timer_resume(flush_timer);
    }

    if (!nes_mask_applied) {
        if (!nes_mask_dsc) {
            nes_mask_dsc = pgs_libpng_decode("/usr/share/pgs/apps/nes/resources/mask.png");
        }
        if (nes_mask_dsc) {
            lv_obj_set_style_bg_img_src(lv_screen_active(), nes_mask_dsc, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, LV_PART_MAIN);
            nes_mask_applied = 1;
        }
    }
}

lv_obj_t * nes_app_init(lv_obj_t * parent, lv_group_t * group, void (*key_cb)(uint32_t keycode))
{
    ui_key_cb = key_cb;
    
    memset(&nes_app_inst, 0, sizeof(nes_app_inst));

    ui_container = lv_obj_create(parent);
    lv_obj_remove_style_all(ui_container);
    lv_obj_set_width(ui_container, lv_pct(100));
    lv_obj_set_height(ui_container, lv_pct(100));
    lv_obj_set_align(ui_container, LV_ALIGN_TOP_LEFT);
    lv_group_add_obj(group, ui_container);
    lv_obj_add_event_cb(ui_container, key_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_clear_flag(ui_container, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    const char * rom_path = getenv("PGS_NES_ROM");
    if (rom_path == NULL || *rom_path == '\0') {
        rom_path = "/usr/share/pgs/apps/nes/resources/roms/Mario.nes";
    }
    nes_app_inst.rom_path = strdup(rom_path);

    nes_app_inst.img = lv_image_create(parent);
    lv_obj_center(nes_app_inst.img);
    lv_img_set_zoom(nes_app_inst.img, 300);


    memset(&nes_app_inst.dsc, 0, sizeof(nes_app_inst.dsc));
#if LV_COLOR_DEPTH == 16
    nes_app_inst.dsc.data_size = 2 * NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT;
#else
    nes_app_inst.dsc.data_size = 4 * NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT;
#endif
    nes_app_inst.dsc.header.w = NES_SCREEN_WIDTH;
    nes_app_inst.dsc.header.h = NES_SCREEN_HEIGHT;
    nes_app_inst.dsc.header.cf = LV_COLOR_FORMAT_ARGB8888;

    dwPad1 = 0;
    dwPad2 = 0;
    dwSystem = 0;
    FrameSkip = 0;

    flush_timer = lv_timer_create(update_flush_timer, 16, &nes_app_inst);
    lv_timer_pause(flush_timer);

    nes_start_emulator();

    return parent;
}

void nes_app_update(void)
{
    if (nes_stop_requested) {
        nes_app_inst.running = 0;

        if (!nes_evdev_thread_alive && !nes_emu_thread_alive) {
            if (nes_evdev_thread_started) {
                pthread_join(evdev_thread, NULL);
                nes_evdev_thread_started = 0;
            }
            if (nes_emu_thread_started) {
                pthread_join(nes_app_inst.emu_thread, NULL);
                nes_emu_thread_started = 0;
            }

            if (flush_timer) {
                lv_timer_pause(flush_timer);
            }

            if (nes_mask_applied) {
                lv_obj_set_style_bg_img_src(lv_screen_active(), NULL, LV_PART_MAIN);
                lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_TRANSP, LV_PART_MAIN);
                nes_mask_applied = 0;
            }
            if (nes_mask_dsc) {
                pgs_libpng_free(nes_mask_dsc);
                nes_mask_dsc = NULL;
            }

            nes_stop_requested = 0;
            nes_core_stopped = 1;
        }
    }

    if (nes_core_stopped && nes_rom_pending) {
        nes_rom_pending = 0;
        nes_core_stopped = 0;
        nes_start_emulator();
    }
}

void nes_app_handle_key(uint32_t keycode)
{
    switch(keycode) {
        case LV_KEY_UP:
            nes_app_inst.key_states[0] = 1;
            break;
        case LV_KEY_DOWN:
            nes_app_inst.key_states[1] = 1;
            break;
        case LV_KEY_LEFT:
            nes_app_inst.key_states[2] = 1;
            break;
        case LV_KEY_RIGHT:
            nes_app_inst.key_states[3] = 1;
            break;
        case 'z':
        case 'Z':
            nes_app_inst.key_states[4] = 1;
            break;
        case 'x':
        case 'X':
            nes_app_inst.key_states[5] = 1;
            break;
        case LV_KEY_ENTER:
        case ' ':
            nes_app_inst.key_states[6] = 1;
            break;
        default:
            break;
    }
}

void nes_app_request_stop(void)
{
    if (!nes_app_inst.running) return;
    if (nes_paused) {
        nes_paused = 0;
        InfoNES_SetPause(0);
        if (flush_timer) {
            lv_timer_resume(flush_timer);
        }
    }
    InfoNES_RequestQuit(1);
    nes_stop_requested = 1;
}

void nes_app_request_pause(void)
{
    if (!nes_app_inst.running || nes_paused) return;
    nes_paused = 1;
    InfoNES_SetPause(1);
    if (flush_timer) {
        lv_timer_pause(flush_timer);
    }
}

void nes_app_request_resume(void)
{
    if (!nes_app_inst.running || !nes_paused) return;
    nes_paused = 0;
    InfoNES_SetPause(0);
    if (flush_timer) {
        lv_timer_resume(flush_timer);
    }
}

void nes_app_set_rom(const char * rom_path)
{
    if (!rom_path || !*rom_path) return;

    char * next = strdup(rom_path);
    if (!next) return;

    if (nes_app_inst.rom_path) {
        free(nes_app_inst.rom_path);
    }
    nes_app_inst.rom_path = next;
    nes_rom_pending = 1;
}
