#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#include "pgs_modules.h"
#include "pgs_utils.h"
#include "nes_app.h"

static lv_group_t * gmain;
static lv_group_t * gback;

static void rom_event_cb(lv_event_t * event)
{
    lv_obj_t * target = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_CLICKED && target && lv_obj_get_user_data(target)) {
        const char * rom_path = (const char *)lv_obj_get_user_data(target);
        
        FILE * f = fopen("/usr/share/pgs/apps/nes/current", "w");
        if (f) {
            fprintf(f, "%s", rom_path);
            fclose(f);
        }

        setenv("PGS_NES_ROM", rom_path, 1);
        nes_app_request_stop();
        nes_app_set_rom(rom_path);
        
        lv_indev_set_group(pgs_get_keyboard(), gmain);
        pgs_backlist_hidden(true, false);
    }
}

static void rom_find(const char * dir_path)
{
    DIR * dir;
    struct dirent * entry;
    struct stat statbuf;

    LV_IMG_DECLARE(icon_theme);

    dir = opendir(dir_path);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[512];
        lv_snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        if (stat(full_path, &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
            const char * ext = strrchr(entry->d_name, '.');
            if (ext && (strcmp(ext, ".nes") == 0 || strcmp(ext, ".NES") == 0)) {
                char * name = lv_malloc(64);
                if (name == NULL) {
                    continue;
                }
                lv_snprintf(name, 64, "%s", entry->d_name);
                pgs_backlist_add_item(name, &icon_theme, rom_event_cb, strdup(full_path));
            }
        }
    }

    closedir(dir);
}

static void main_key_cb(uint32_t keycode)
{
    switch(keycode) {
        case LV_KEY_ESC:
            nes_app_request_pause();
            lv_indev_set_group(pgs_get_keyboard(), gback);
            pgs_backlist_hidden(false, false);
            break;
        default:
            nes_app_handle_key(keycode);
            break;
    }
}

static void back_key_cb(uint32_t keycode)
{
    switch(keycode) {
        case LV_KEY_ESC:
            nes_app_request_resume();
            lv_indev_set_group(pgs_get_keyboard(), gmain);
            pgs_backlist_hidden(true, false);
            break;
        default: break;
    }
}

static uint32_t randomseed(void)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if(fd < 0) {
        perror("Failed to open /dev/urandom");
        return 0;
    }

    uint32_t random_number;
    ssize_t result = read(fd, &random_number, sizeof(random_number));
    if(result != sizeof(random_number)) {
        perror("Failed to read from /dev/urandom");
        close(fd);
        return 0;
    }

    close(fd);

    return random_number;
}

int main(void)
{
    pgs_lvgl_init("nes");

    srand(randomseed());

    gmain = lv_group_create();
    gback = lv_group_create();

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), LV_PART_MAIN);

    pgs_backlist_init(lv_layer_top(), gback, back_key_cb);
    
    rom_find("/userdata/nes");
    rom_find("/usr/share/pgs/apps/nes/resources/roms");
    
    const char * rom_path = getenv("PGS_NES_ROM");
    if (rom_path == NULL || *rom_path == '\0') {
        FILE * f = fopen("/usr/share/pgs/apps/nes/current", "r");
        if (f) {
            static char selected_rom[512];
            if (fgets(selected_rom, sizeof(selected_rom), f)) {
                char * p = strchr(selected_rom, '\n');
                if (p) *p = '\0';
                rom_path = selected_rom;
                setenv("PGS_NES_ROM", rom_path, 1);
            }
            fclose(f);
        }
    }
    if (rom_path == NULL || *rom_path == '\0') {
        rom_path = "/usr/share/pgs/apps/nes/resources/roms/Mario.nes";
    }
    
    nes_app_init(lv_screen_active(), gmain, main_key_cb);

    lv_indev_set_group(pgs_get_keyboard(), gmain);
    pgs_backlist_hidden(true, true);

    while(1) {
        nes_app_update();
        uint32_t wait_ms = lv_timer_handler();
        if (wait_ms < 1) wait_ms = 1;
        if (wait_ms > 5) wait_ms = 5;
        usleep(1000 * wait_ms);
    }
    return 0;
}
