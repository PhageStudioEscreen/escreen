#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <stdlib.h>
#include "pgs_modules.h"
#include "pgs_utils.h"
#include "keyboard_params.h"
#include "pgs_app_keyboard.h"
#include "keycode_params.h"
#include "keycap_colors.h"

static lv_obj_t * ui_container;
static lv_group_t * ui_group;
static void (*ui_key_cb)(uint32_t keycode);
char keyboard_path_buffer[PATH_MAX];
static const char * input_file = "/usr/share/pgs/apps/keyboard/themes/current";

struct pgs_app_keyboard keyboard_inst;

static void key_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if(code == LV_EVENT_KEY) {
        if(ui_key_cb) {
            ui_key_cb(lv_indev_get_key(lv_indev_active()));
        }
    } else if(code == LV_EVENT_CLICKED) {
    }
}

static void theme_event_cb(lv_event_t * event)
{
    lv_obj_t * target    = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);

    if(code == LV_EVENT_CLICKED && target && lv_obj_get_user_data(target)) {
        FILE * file = fopen(input_file, "w");
        if(file == NULL) {
            return;
        }
        if(fprintf(file, "%s\n", (const char *)lv_obj_get_user_data(target)) < 0) {
            fclose(file);
            return;
        }

        fclose(file);

        lv_obj_t * button = lv_button_create(lv_screen_active());
        lv_obj_remove_style_all(button);
        lv_obj_set_size(button, 240, 60);
        lv_obj_set_style_bg_opa(button, LV_OPA_0, LV_PART_MAIN);
        lv_obj_align(button, LV_ALIGN_CENTER, 0, 0);

        lv_obj_set_style_radius(button, 6, LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);
        lv_obj_set_style_bg_color(button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(button, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);
        lv_obj_set_style_outline_color(button, lv_color_hex(0x505050),
                                       LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);
        lv_obj_set_style_outline_opa(button, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(button, 1, LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);
        lv_obj_set_style_outline_pad(button, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);

        lv_obj_t * text = lv_label_create(button);
        lv_obj_set_width(text, LV_SIZE_CONTENT);
        lv_obj_set_height(text, LV_SIZE_CONTENT);
        lv_obj_set_x(text, 20);
        lv_obj_set_y(text, 5);
        lv_obj_align(text, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(text, "Exit to apply new theme");
        lv_obj_set_style_text_color(text, lv_color_hex(0xF0F0F0), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(text, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(text, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_long_mode(text, LV_LABEL_LONG_SCROLL_CIRCULAR);
        pgs_cleanup_reboot();
        return;
    }
}

static char full_path[PATH_MAX];

static void theme_find(const char * dir_path)
{
    DIR * dir;
    struct dirent * entry;
    struct stat statbuf;

    LV_IMG_DECLARE(icon_theme);

    dir = opendir(dir_path);
    if(dir == NULL) {
        return;
    }

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        lv_snprintf(full_path, sizeof(full_path), "%s/%s/config.json", dir_path, entry->d_name);

        if(stat(full_path, &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
            const char * name = lv_malloc(32);
            if(name == NULL) {
                continue;
            }
            lv_snprintf(name, 32, "THEME %s", entry->d_name);
            pgs_backlist_add_item(name, &icon_theme, theme_event_cb, strdup(full_path));
        }
    }

    closedir(dir);
}

lv_obj_t * pgs_app_keyboard_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode))
{
    ui_group  = group;
    ui_key_cb = key_cb;

    ui_container = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(ui_container);
    lv_obj_set_width(ui_container, lv_pct(100));
    lv_obj_set_height(ui_container, lv_pct(100));
    lv_obj_set_align(ui_container, LV_ALIGN_TOP_LEFT);
    lv_group_add_obj(ui_group, ui_container);
    lv_obj_add_event_cb(ui_container, key_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_container, key_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_clear_flag(ui_container, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    theme_find("/usr/share/pgs/apps/keyboard/themes");
    theme_find("/root/themes");

    const char * config_json = NULL;
    do {
        FILE * file = fopen(input_file, "r");
        if(file == NULL) {
            break;
        }

        if(fgets(full_path, sizeof(full_path), file) != NULL) {
            full_path[strcspn(full_path, "\n")] = '\0';

            struct stat statbuf;
            if(stat(full_path, &statbuf) == 0) {
                config_json = full_path;
            }
        }

        fclose(file);
    } while(0);

    if(!config_json) {
        config_json = "/usr/share/pgs/apps/keyboard/themes/default/config.json";
    }

    struct keyboard_params * params = keyboard_params_parse(config_json);
    if(!params) {
        lv_obj_t * label = lv_label_create(ui_container);
        lv_label_set_text_fmt(label, "Unable to parse config file\n%s", config_json);
        lv_obj_set_style_text_font(label, &lv_font_helveticarounded_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_align(ui_container, LV_ALIGN_CENTER);
        lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), LV_PART_MAIN);
        return ui_container;
    }

    keyboard_inst.videos = pgs_widgets_videos_create(ui_container, params->base, params->videos, params->videos_count);

    keyboard_inst.gifs = pgs_widgets_gifs_create(ui_container, params->base, params->gifs, params->gifs_count);

    keyboard_inst.images = pgs_widgets_images_create(ui_container, params->base, params->images, params->images_count);

    keyboard_inst.labels = pgs_widgets_labels_create(ui_container, params->base, params->labels, params->labels_count);

    keyboard_inst.wpmlabel = pgs_widgets_wpmlabel_create(ui_container, params->base, params->wpmlabel);

    keyboard_inst.keyroll = pgs_widgets_keyroll_create(ui_container, params->base, params->keyroll);

    keyboard_inst.macro =
        pgs_widgets_macro_create(ui_container, params->base, keyboard_params_state_get(params, PGS_WIDGETS_TYPE_MACRO),
                                 keyboard_params_state_get(params, PGS_WIDGETS_TYPE_MACRO1),
                                 keyboard_params_state_get(params, PGS_WIDGETS_TYPE_MACRO2));

    keyboard_inst.layer =
        pgs_widgets_layer_create(ui_container, params->base, keyboard_params_state_get(params, PGS_WIDGETS_TYPE_LAYER));

    keyboard_inst.capslock = pgs_widgets_capslock_create(ui_container, params->base,
                                                         keyboard_params_state_get(params, PGS_WIDGETS_TYPE_CAPSLOCK));

    keyboard_inst.numlock = pgs_widgets_numlock_create(ui_container, params->base,
                                                       keyboard_params_state_get(params, PGS_WIDGETS_TYPE_NUMLOCK));

    keyboard_inst.ble1 =
        pgs_widgets_output_create(ui_container, params->base, keyboard_params_state_get(params, PGS_WIDGETS_TYPE_BLE1));

    keyboard_inst.ble2 =
        pgs_widgets_output_create(ui_container, params->base, keyboard_params_state_get(params, PGS_WIDGETS_TYPE_BLE2));

    keyboard_inst.ble3 =
        pgs_widgets_output_create(ui_container, params->base, keyboard_params_state_get(params, PGS_WIDGETS_TYPE_BLE3));

    keyboard_inst.g24 =
        pgs_widgets_output_create(ui_container, params->base, keyboard_params_state_get(params, PGS_WIDGETS_TYPE_2G4));

    keyboard_inst.usb =
        pgs_widgets_output_create(ui_container, params->base, keyboard_params_state_get(params, PGS_WIDGETS_TYPE_USB));

    keyboard_inst.scr =
        pgs_widgets_output_create(ui_container, params->base, keyboard_params_state_get(params, PGS_WIDGETS_TYPE_SCR));

    keyboard_inst.bat =
        pgs_widgets_bat_create(ui_container, params->base, keyboard_params_state_get(params, PGS_WIDGETS_TYPE_BAT));

    pgs_widgets_macro_set_state(keyboard_inst.macro, PGS_WIDGETS_MACRO_STATE_PAUSE, false, false);
    pgs_widgets_layer_set_current(keyboard_inst.layer, 0);
    pgs_widgets_capslock_set_state(keyboard_inst.capslock, false);
    pgs_widgets_numlock_set_state(keyboard_inst.numlock, false);
    pgs_widgets_output_set_state(keyboard_inst.ble1, PGS_WIDGETS_OUTPUT_STATE_DISCONNECT, false);
    pgs_widgets_output_set_state(keyboard_inst.ble2, PGS_WIDGETS_OUTPUT_STATE_DISCONNECT, false);
    pgs_widgets_output_set_state(keyboard_inst.ble3, PGS_WIDGETS_OUTPUT_STATE_DISCONNECT, false);
    pgs_widgets_output_set_state(keyboard_inst.g24, PGS_WIDGETS_OUTPUT_STATE_DISCONNECT, false);
    pgs_widgets_output_set_state(keyboard_inst.usb, PGS_WIDGETS_OUTPUT_STATE_DISCONNECT, false);
    pgs_widgets_output_set_state(keyboard_inst.scr, PGS_WIDGETS_OUTPUT_STATE_DISCONNECT, false);
    pgs_widgets_bat_set_state(keyboard_inst.bat, PGS_WIDGETS_BAT_STATE_IDLE, 100);
    pgs_widgets_wpmlabel_set_wpm(keyboard_inst.wpmlabel, 0);

    return ui_container;
}

static void pgs_app_keyboard_hidraw_via_recv(void)
{}

static void pgs_app_keyboard_hidraw_escreen_recv(void)
{
    uint8_t buffer[21];
    uint8_t * command = &buffer[0];
    uint8_t * data    = &buffer[1];

    if(sizeof(buffer) == chry_ringbuffer_read(&hidraw_rxrb, buffer, sizeof(buffer))) {
        switch(*command) {
            case id_scr_get_protocol_version: {
                printf("escreen hidraw protocol version 0x%04x\n", ((uint16_t)data[0] | ((uint16_t)data[1] << 8)));
                break;
            }

            case id_scr_get_macro: {
                pgs_widgets_macro_set_state(keyboard_inst.macro, data[0], data[1], data[2]);
                break;
            }

            case id_scr_get_layer: {
                pgs_widgets_layer_set_current(keyboard_inst.layer, data[0]);
                break;
            }

            case id_scr_get_led: {
                keyboard_led_t led;
                led.raw = data[0];
                pgs_widgets_capslock_set_state(keyboard_inst.capslock, led.caps_lock);
                pgs_widgets_numlock_set_state(keyboard_inst.numlock, led.num_lock);
                break;
            }

            case id_scr_get_output: {
                bool select_usb  = data[1] == PGS_WIDGETS_OUTPUT_TARGET_USB;
                bool select_scr  = data[1] == PGS_WIDGETS_OUTPUT_TARGET_SCR;
                bool select_ble1 = data[1] == PGS_WIDGETS_OUTPUT_TARGET_BLE1;
                bool select_ble2 = data[1] == PGS_WIDGETS_OUTPUT_TARGET_BLE2;
                bool select_ble3 = data[1] == PGS_WIDGETS_OUTPUT_TARGET_BLE3;
                bool select_g24  = data[1] == PGS_WIDGETS_OUTPUT_TARGET_2G4;
                pgs_widgets_output_set_state(keyboard_inst.usb, data[2], select_usb);
                pgs_widgets_output_set_state(keyboard_inst.scr, data[3], select_scr);
                pgs_widgets_output_set_state(keyboard_inst.ble1, data[4], select_ble1);
                pgs_widgets_output_set_state(keyboard_inst.ble2, data[5], select_ble2);
                pgs_widgets_output_set_state(keyboard_inst.ble3, data[6], select_ble3);
                pgs_widgets_output_set_state(keyboard_inst.g24, data[7], select_g24);
                break;
            }

            case id_scr_get_bat: {
                pgs_widgets_bat_set_state(keyboard_inst.bat, data[0], data[1]);
                break;
            }

            case id_scr_get_wpm: {
                pgs_widgets_wpmlabel_set_wpm(keyboard_inst.wpmlabel, data[0]);
                break;
            }

            case id_scr_get_sleep: {
                if(data[0] || data[1]) {
                    system("echo 0 > /sys/class/backlight/panel-backlight/brightness");
                } else {
                    system("echo 1 > /sys/class/backlight/panel-backlight/brightness");
                }
                break;
            }
        }
    } else {
        printf("pgs_app_keyboard_hidraw_escreen_recv error, reset read\n");
        chry_ringbuffer_reset_read(&hidraw_rxrb);
    }
}

static void pgs_app_keyboard_hidraw_monitor_recv(void)
{
    struct __attribute__((__packed__))
    {
        uint8_t keycode;
        uint8_t col;
        uint8_t row;
        uint8_t timel;
        uint8_t timeh;
        uint8_t type;
        uint8_t pressed;
    } key;

    if(sizeof(key) == chry_ringbuffer_read(&hidraw_rxrb, &key, sizeof(key))) {
        if(!keyboard_inst.keyroll || !keyboard_inst.keyroll->_keyroll->enable) {
            return;
        }

        if(key.pressed) {
            pgs_widgets_keyroll_push(
                keyboard_inst.keyroll, key.keycode,
                keycap_color_random_with_coloration(keyboard_inst.keyroll->_keyroll->coloration)->index);
        }
    } else {
        printf("chry_ringbuffer_reset_read error, reset read\n");
        chry_ringbuffer_reset_read(&hidraw_rxrb);
    }
}

void pgs_app_keyboard_update(void)
{
    pthread_mutex_lock(&hidraw_mutex);
next:
    if(!chry_ringbuffer_check_empty(&hidraw_rxrb)) {
        uint8_t report_id;
        bool success = chry_ringbuffer_read_byte(&hidraw_rxrb, &report_id);
        if(success) {
            switch(report_id) {
                case ESCREEN_REPORT_ID_KEYBOARD: chry_ringbuffer_drop(&hidraw_rxrb, 8); break;
                case ESCREEN_REPORT_ID_MOUSE: chry_ringbuffer_drop(&hidraw_rxrb, 5); break;
                case ESCREEN_REPORT_ID_SYSTEM_CTRL: chry_ringbuffer_drop(&hidraw_rxrb, 2); break;
                case ESCREEN_REPORT_ID_CONSUMER_CTRL: chry_ringbuffer_drop(&hidraw_rxrb, 2); break;
                case ESCREEN_REPORT_ID_PROGRAMMABLE_BUTTON: chry_ringbuffer_reset_read(&hidraw_rxrb); break;
                case ESCREEN_REPORT_ID_NKRO: chry_ringbuffer_drop(&hidraw_rxrb, 20); break;
                case ESCREEN_REPORT_ID_JOYSTICK: chry_ringbuffer_reset_read(&hidraw_rxrb); break;
                case ESCREEN_REPORT_ID_DIGITIZER: chry_ringbuffer_reset_read(&hidraw_rxrb); break;
                case ESCREEN_REPORT_ID_DIAL: chry_ringbuffer_reset_read(&hidraw_rxrb); break;
                case ESCREEN_REPORT_ID_VIA_INPUT: pgs_app_keyboard_hidraw_via_recv(); break;
                case ESCREEN_REPORT_ID_ESCREEN_INPUT: pgs_app_keyboard_hidraw_escreen_recv(); break;
                case ESCREEN_REPORT_ID_MONITOR_INPUT: pgs_app_keyboard_hidraw_monitor_recv(); break;
                default: chry_ringbuffer_reset_read(&hidraw_rxrb); break;
            }
        } else {
            printf("pgs_app_keyboard_update error, reset read\n");
            chry_ringbuffer_reset_read(&hidraw_rxrb);
        }
        goto next;
    }
    pthread_mutex_unlock(&hidraw_mutex);
}
