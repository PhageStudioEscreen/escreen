#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
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

struct pgs_app_keyboard keyboard_inst;

static void key_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if(code == LV_EVENT_KEY) {
        if(ui_key_cb) {
            ui_key_cb(lv_indev_get_key(lv_indev_active()));
        }
    } else if(code == LV_EVENT_CLICKED) {
        pgs_widgets_keyroll_push(
            keyboard_inst.keyroll, rand() % KC_MAX_COUNT,
            keycap_color_random_with_coloration(keyboard_inst.keyroll->_keyroll->coloration)->index);
    }
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

    const char * config_json = "/usr/share/pgs/apps/keyboard/themes/default/config.json";

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

    keyboard_inst.keyroll = pgs_widgets_keyroll_create(ui_container, params->base, params->keyroll);

    keyboard_inst.wpmlabel = pgs_widgets_wpmlabel_create(ui_container, params->base, params->wpmlabel);

    keyboard_inst.labels = pgs_widgets_labels_create(ui_container, params->base, params->labels, params->labels_count);

    keyboard_inst.images = pgs_widgets_images_create(ui_container, params->base, params->images, params->images_count);

    keyboard_inst.gifs = pgs_widgets_gifs_create(ui_container, params->base, params->gifs, params->gifs_count);

    keyboard_inst.vedios = pgs_widgets_vedios_create(ui_container, params->base, params->vedios, params->vedios_count);

    pgs_widgets_macro_set_state(keyboard_inst.macro, PGS_WIDGETS_MACRO_STATE_PAUSE, true, true);
    pgs_widgets_layer_set_current(keyboard_inst.layer, 0);
    pgs_widgets_capslock_set_state(keyboard_inst.capslock, true);
    pgs_widgets_numlock_set_state(keyboard_inst.numlock, true);
    pgs_widgets_output_set_state(keyboard_inst.ble1, PGS_WIDGETS_OUTPUT_STATE_SEARCHING, false);
    pgs_widgets_output_set_state(keyboard_inst.ble2, PGS_WIDGETS_OUTPUT_STATE_DISCONNECT, false);
    pgs_widgets_output_set_state(keyboard_inst.ble3, PGS_WIDGETS_OUTPUT_STATE_DISCONNECT, false);
    pgs_widgets_output_set_state(keyboard_inst.g24, PGS_WIDGETS_OUTPUT_STATE_DISCONNECT, false);
    pgs_widgets_output_set_state(keyboard_inst.usb, PGS_WIDGETS_OUTPUT_STATE_CONNECT, true);
    pgs_widgets_output_set_state(keyboard_inst.scr, PGS_WIDGETS_OUTPUT_STATE_CONNECT, false);
    pgs_widgets_bat_set_state(keyboard_inst.bat, PGS_WIDGETS_BAT_STATE_CHARGING, 90);
    pgs_widgets_wpmlabel_set_wpm(keyboard_inst.wpmlabel, 110);

    return ui_container;
}
