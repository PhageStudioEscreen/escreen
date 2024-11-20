#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include "menu.h"
#include "pgs_modules.h"
#include "pgs_utils.h"
#include "pgs_kbd_params.h"
#include "pgs_anim.h"
#include "keyboard_state.h"
#include "keycode_params.h"
#include "keycap_colors.h"

static lv_obj_t * ui_container;
static lv_group_t * ui_group;
static void (*ui_key_cb)(uint32_t keycode);
static char path_buffer[PATH_MAX];

static void key_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if(code == LV_EVENT_KEY) {
        if(ui_key_cb) {
            ui_key_cb(lv_indev_get_key(lv_indev_active()));
        }
    } else if(code == LV_EVENT_CLICKED) {
        keyboard_state_set_keyroll(kbd_state.keyroll, rand() % KC_MAX_COUNT,
                                   keycap_color_random_with_coloration(kbd_state.keyroll->_keyroll->coloration)->index);
    }
}

static void key_focused_event_cb(lv_event_t * event)
{
    lv_obj_t * cur_key   = lv_event_get_current_target(event);
    lv_obj_t * container = lv_obj_get_parent(cur_key);
    uint32_t keycount    = lv_obj_get_child_cnt(container);
    uint32_t mid_key_idx = (keycount - 1) / 2;
    uint32_t cur_key_idx = lv_obj_get_index(cur_key);

    if(cur_key_idx > mid_key_idx) {
        lv_obj_scroll_to_view(lv_obj_get_child(container, mid_key_idx), LV_ANIM_OFF);
        lv_obj_scroll_to_view(lv_obj_get_child(container, mid_key_idx + 1), LV_ANIM_ON);
        lv_obj_move_to_index(lv_obj_get_child(container, 0), -1);
    }
}

static struct lv_kbd_macro * create_macro(lv_obj_t * obj, const char * base, struct pgs_kbd_state * macro,
                                          struct pgs_kbd_state * macro1, struct pgs_kbd_state * macro2)
{
    if(!base) {
        return NULL;
    }
    if(!macro || (macro->type != PGS_KBD_STATE_TYPE_MACRO) || !macro->enable) {
        return NULL;
    }
    if(!macro1 || (macro1->type != PGS_KBD_STATE_TYPE_MACRO1) || !macro1->enable) {
        return NULL;
    }
    if(!macro2 || (macro2->type != PGS_KBD_STATE_TYPE_MACRO2) || !macro2->enable) {
        return NULL;
    }

    struct lv_kbd_macro * target = lv_malloc(sizeof(struct lv_kbd_macro));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct lv_kbd_macro));

    target->_macro  = macro;
    target->_macro1 = macro1;
    target->_macro2 = macro2;

    /* macro pause */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/macro_pause.png", base);
    target->macro_pause                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro_pause = pgs_libpng_decode(path_buffer);
    if(png_macro_pause) {
        lv_image_set_src(target->macro_pause, png_macro_pause);
    }
    lv_obj_align(target->macro_pause, macro->align, 0, 0);
    lv_obj_set_pos(target->macro_pause, macro->x, macro->y);
    lv_obj_set_style_opa(target->macro_pause, LV_OPA_0, 0);

    /* macro play */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/macro_play.png", base);
    target->macro_play                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro_play = pgs_libpng_decode(path_buffer);
    if(png_macro_play) {
        lv_image_set_src(target->macro_play, png_macro_play);
    }
    lv_obj_align(target->macro_play, macro->align, 0, 0);
    lv_obj_set_pos(target->macro_play, macro->x, macro->y);
    lv_obj_set_style_opa(target->macro_play, LV_OPA_0, 0);

    /* macro record */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/macro_record.png", base);
    target->macro_record                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro_record = pgs_libpng_decode(path_buffer);
    if(png_macro_record) {
        lv_image_set_src(target->macro_record, png_macro_record);
    }
    lv_obj_align(target->macro_record, macro->align, 0, 0);
    lv_obj_set_pos(target->macro_record, macro->x, macro->y);
    lv_obj_set_style_opa(target->macro_record, LV_OPA_0, 0);

    /* macro 1 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/macro1.png", base);
    target->macro1                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro1 = pgs_libpng_decode(path_buffer);
    if(png_macro1) {
        lv_image_set_src(target->macro1, png_macro1);
    }
    lv_obj_align(target->macro1, macro1->align, 0, 0);
    lv_obj_set_pos(target->macro1, macro1->x, macro1->y);
    lv_obj_set_style_opa(target->macro1, LV_OPA_0, 0);

    /* macro 2 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/macro2.png", base);
    target->macro2                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro2 = pgs_libpng_decode(path_buffer);
    if(png_macro2) {
        lv_image_set_src(target->macro2, png_macro2);
    }
    lv_obj_align(target->macro2, macro2->align, 0, 0);
    lv_obj_set_pos(target->macro2, macro2->x, macro2->y);
    lv_obj_set_style_opa(target->macro2, LV_OPA_0, 0);

    return target;
}

static struct lv_kbd_layer * create_layer(lv_obj_t * obj, const char * base, struct pgs_kbd_state * layer)
{
    if(!base) {
        return NULL;
    }
    if(!layer || (layer->type != PGS_KBD_STATE_TYPE_LAYER) || !layer->enable) {
        return NULL;
    }

    struct lv_kbd_layer * target = lv_malloc(sizeof(struct lv_kbd_layer));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct lv_kbd_layer));

    target->_layer = layer;

    for(uint8_t i = 0; i < 8; i++) {
        lv_snprintf(path_buffer, sizeof(path_buffer), "%s/layer%d.png", base, i);
        target->layers[i]                = lv_image_create(obj);
        const lv_image_dsc_t * png_layer = pgs_libpng_decode(path_buffer);
        if(png_layer) {
            lv_image_set_src(target->layers[i], png_layer);
        }
        lv_obj_align(target->layers[i], layer->align, 0, 0);
        lv_obj_set_pos(target->layers[i], layer->x, layer->y);
        lv_obj_set_style_opa(target->layers[i], LV_OPA_0, 0);
    }

    return target;
}

static struct lv_kbd_capslock * create_capslock(lv_obj_t * obj, const char * base, struct pgs_kbd_state * capslock)
{
    if(!base) {
        return NULL;
    }
    if(!capslock || (capslock->type != PGS_KBD_STATE_TYPE_CAPSLOCK) || !capslock->enable) {
        return NULL;
    }

    struct lv_kbd_capslock * target = lv_malloc(sizeof(struct lv_kbd_capslock));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct lv_kbd_capslock));

    target->_capslock = capslock;

    /* on */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/capslock_on.png", base);
    target->on                    = lv_image_create(obj);
    const lv_image_dsc_t * png_on = pgs_libpng_decode(path_buffer);
    if(png_on) {
        lv_image_set_src(target->on, png_on);
    }
    lv_obj_align(target->on, capslock->align, 0, 0);
    lv_obj_set_pos(target->on, capslock->x, capslock->y);
    lv_obj_set_style_opa(target->on, LV_OPA_0, 0);

    /* off */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/capslock_off.png", base);
    target->off                    = lv_image_create(obj);
    const lv_image_dsc_t * png_off = pgs_libpng_decode(path_buffer);
    if(png_off) {
        lv_image_set_src(target->off, png_off);
    }
    lv_obj_align(target->off, capslock->align, 0, 0);
    lv_obj_set_pos(target->off, capslock->x, capslock->y);
    lv_obj_set_style_opa(target->off, LV_OPA_0, 0);

    return target;
}

static struct lv_kbd_numlock * create_numlock(lv_obj_t * obj, const char * base, struct pgs_kbd_state * numlock)
{
    if(!base) {
        return NULL;
    }
    if(!numlock || (numlock->type != PGS_KBD_STATE_TYPE_NUMLOCK) || !numlock->enable) {
        return NULL;
    }

    struct lv_kbd_numlock * target = lv_malloc(sizeof(struct lv_kbd_numlock));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct lv_kbd_numlock));

    target->_numlock = numlock;

    /* on */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/numlock_on.png", base);
    target->on                    = lv_image_create(obj);
    const lv_image_dsc_t * png_on = pgs_libpng_decode(path_buffer);
    if(png_on) {
        lv_image_set_src(target->on, png_on);
    }
    lv_obj_align(target->on, numlock->align, 0, 0);
    lv_obj_set_pos(target->on, numlock->x, numlock->y);
    lv_obj_set_style_opa(target->on, LV_OPA_0, 0);

    /* off */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/numlock_off.png", base);
    target->off                    = lv_image_create(obj);
    const lv_image_dsc_t * png_off = pgs_libpng_decode(path_buffer);
    if(png_off) {
        lv_image_set_src(target->off, png_off);
    }
    lv_obj_align(target->off, numlock->align, 0, 0);
    lv_obj_set_pos(target->off, numlock->x, numlock->y);
    lv_obj_set_style_opa(target->off, LV_OPA_0, 0);

    return target;
}

static struct lv_kbd_output * create_output(lv_obj_t * obj, const char * base, struct pgs_kbd_state * output)
{
    if(!base) {
        return NULL;
    }
    if(!output || !output->enable) {
        return NULL;
    }
    const char * disconnected_path;
    const char * connected_path;
    const char * searching_path;
    switch(output->type) {
        case PGS_KBD_STATE_TYPE_BLE1:
            disconnected_path = "ble1";
            connected_path    = "ble1_connected";
            searching_path    = "ble1_searching";
            break;
        case PGS_KBD_STATE_TYPE_BLE2:
            disconnected_path = "ble2";
            connected_path    = "ble2_connected";
            searching_path    = "ble2_searching";
            break;
        case PGS_KBD_STATE_TYPE_BLE3:
            disconnected_path = "ble3";
            connected_path    = "ble3_connected";
            searching_path    = "ble3_searching";
            break;
        case PGS_KBD_STATE_TYPE_2G4:
            disconnected_path = "2g4";
            connected_path    = "2g4_connected";
            searching_path    = "2g4_searching";
            break;
        case PGS_KBD_STATE_TYPE_USB:
            disconnected_path = "usb";
            connected_path    = "usb_connected";
            searching_path    = NULL;
            break;
        case PGS_KBD_STATE_TYPE_SCR:
            disconnected_path = "scr";
            connected_path    = "scr_connected";
            searching_path    = NULL;
            break;
        default: return NULL;
    }

    struct lv_kbd_output * target = lv_malloc(sizeof(struct lv_kbd_output));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct lv_kbd_output));

    target->_output = output;

    /* disconnected */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/%s.png", base, disconnected_path);
    target->disconnected                    = lv_image_create(obj);
    const lv_image_dsc_t * png_disconnected = pgs_libpng_decode(path_buffer);
    if(png_disconnected) {
        lv_image_set_src(target->disconnected, png_disconnected);
    }
    lv_obj_align(target->disconnected, output->align, 0, 0);
    lv_obj_set_pos(target->disconnected, output->x, output->y);
    lv_obj_set_style_opa(target->disconnected, LV_OPA_0, 0);

    /* connected */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/%s.png", base, connected_path);
    target->connected                    = lv_image_create(obj);
    const lv_image_dsc_t * png_connected = pgs_libpng_decode(path_buffer);
    if(png_connected) {
        lv_image_set_src(target->connected, png_connected);
    }
    lv_obj_align(target->connected, output->align, 0, 0);
    lv_obj_set_pos(target->connected, output->x, output->y);
    lv_obj_set_style_opa(target->connected, LV_OPA_0, 0);

    if(!searching_path) {
        return target;
    }

    /* searching */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/%s.png", base, searching_path);
    target->searching                    = lv_image_create(obj);
    const lv_image_dsc_t * png_searching = pgs_libpng_decode(path_buffer);
    if(png_searching) {
        lv_image_set_src(target->searching, png_searching);
    }
    lv_obj_align(target->searching, output->align, 0, 0);
    lv_obj_set_pos(target->searching, output->x, output->y);
    lv_obj_set_style_opa(target->searching, LV_OPA_0, 0);

    return target;
}

static struct lv_kbd_bat * create_bat(lv_obj_t * obj, const char * base, struct pgs_kbd_state * bat)
{
    if(!base) {
        return NULL;
    }
    if(!bat || (bat->type != PGS_KBD_STATE_TYPE_BAT) || !bat->enable) {
        return NULL;
    }

    struct lv_kbd_bat * target = lv_malloc(sizeof(struct lv_kbd_bat));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct lv_kbd_bat));

    target->_bat = bat;

    /* alert */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat_alert.png", base);
    target->alert                    = lv_image_create(obj);
    const lv_image_dsc_t * png_alert = pgs_libpng_decode(path_buffer);
    if(png_alert) {
        lv_image_set_src(target->alert, png_alert);
    }
    lv_obj_align(target->alert, bat->align, 0, 0);
    lv_obj_set_pos(target->alert, bat->x, bat->y);
    lv_obj_set_style_opa(target->alert, LV_OPA_0, 0);

    /* unknown */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat_unknown.png", base);
    target->unknown                    = lv_image_create(obj);
    const lv_image_dsc_t * png_unknown = pgs_libpng_decode(path_buffer);
    if(png_unknown) {
        lv_image_set_src(target->unknown, png_unknown);
    }
    lv_obj_align(target->unknown, bat->align, 0, 0);
    lv_obj_set_pos(target->unknown, bat->x, bat->y);
    lv_obj_set_style_opa(target->unknown, LV_OPA_0, 0);

    /* charging 20 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat20_charging.png", base);
    target->charging_20                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_20 = pgs_libpng_decode(path_buffer);
    if(png_charging_20) {
        lv_image_set_src(target->charging_20, png_charging_20);
    }
    lv_obj_align(target->charging_20, bat->align, 0, 0);
    lv_obj_set_pos(target->charging_20, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_20, LV_OPA_0, 0);

    /* charging 30 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat30_charging.png", base);
    target->charging_30                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_30 = pgs_libpng_decode(path_buffer);
    if(png_charging_30) {
        lv_image_set_src(target->charging_30, png_charging_30);
    }
    lv_obj_align(target->charging_30, bat->align, 0, 0);
    lv_obj_set_pos(target->charging_30, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_30, LV_OPA_0, 0);

    /* charging 50 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat50_charging.png", base);
    target->charging_50                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_50 = pgs_libpng_decode(path_buffer);
    if(png_charging_50) {
        lv_image_set_src(target->charging_50, png_charging_50);
    }
    lv_obj_align(target->charging_50, bat->align, 0, 0);
    lv_obj_set_pos(target->charging_50, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_50, LV_OPA_0, 0);

    /* charging 60 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat60_charging.png", base);
    target->charging_60                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_60 = pgs_libpng_decode(path_buffer);
    if(png_charging_60) {
        lv_image_set_src(target->charging_60, png_charging_60);
    }
    lv_obj_align(target->charging_60, bat->align, 0, 0);
    lv_obj_set_pos(target->charging_60, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_60, LV_OPA_0, 0);

    /* charging 80 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat80_charging.png", base);
    target->charging_80                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_80 = pgs_libpng_decode(path_buffer);
    if(png_charging_80) {
        lv_image_set_src(target->charging_80, png_charging_80);
    }
    lv_obj_align(target->charging_80, bat->align, 0, 0);
    lv_obj_set_pos(target->charging_80, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_80, LV_OPA_0, 0);

    /* charging 90 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat90_charging.png", base);
    target->charging_90                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_90 = pgs_libpng_decode(path_buffer);
    if(png_charging_90) {
        lv_image_set_src(target->charging_90, png_charging_90);
    }
    lv_obj_align(target->charging_90, bat->align, 0, 0);
    lv_obj_set_pos(target->charging_90, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_90, LV_OPA_0, 0);

    /* charging 100 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat100_charging.png", base);
    target->charging_100                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_100 = pgs_libpng_decode(path_buffer);
    if(png_charging_100) {
        lv_image_set_src(target->charging_100, png_charging_100);
    }
    lv_obj_align(target->charging_100, bat->align, 0, 0);
    lv_obj_set_pos(target->charging_100, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_100, LV_OPA_0, 0);

    /* bat 20 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat20.png", base);
    target->bat_20                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_20 = pgs_libpng_decode(path_buffer);
    if(png_bat_20) {
        lv_image_set_src(target->bat_20, png_bat_20);
    }
    lv_obj_align(target->bat_20, bat->align, 0, 0);
    lv_obj_set_pos(target->bat_20, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_20, LV_OPA_0, 0);

    /* bat 30 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat30.png", base);
    target->bat_30                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_30 = pgs_libpng_decode(path_buffer);
    if(png_bat_30) {
        lv_image_set_src(target->bat_30, png_bat_30);
    }
    lv_obj_align(target->bat_30, bat->align, 0, 0);
    lv_obj_set_pos(target->bat_30, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_30, LV_OPA_0, 0);

    /* bat 50 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat50.png", base);
    target->bat_50                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_50 = pgs_libpng_decode(path_buffer);
    if(png_bat_50) {
        lv_image_set_src(target->bat_50, png_bat_50);
    }
    lv_obj_align(target->bat_50, bat->align, 0, 0);
    lv_obj_set_pos(target->bat_50, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_50, LV_OPA_0, 0);

    /* bat 60 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat60.png", base);
    target->bat_60                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_60 = pgs_libpng_decode(path_buffer);
    if(png_bat_60) {
        lv_image_set_src(target->bat_60, png_bat_60);
    }
    lv_obj_align(target->bat_60, bat->align, 0, 0);
    lv_obj_set_pos(target->bat_60, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_60, LV_OPA_0, 0);

    /* bat 80 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat80.png", base);
    target->bat_80                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_80 = pgs_libpng_decode(path_buffer);
    if(png_bat_80) {
        lv_image_set_src(target->bat_80, png_bat_80);
    }
    lv_obj_align(target->bat_80, bat->align, 0, 0);
    lv_obj_set_pos(target->bat_80, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_80, LV_OPA_0, 0);

    /* bat 90 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat90.png", base);
    target->bat_90                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_90 = pgs_libpng_decode(path_buffer);
    if(png_bat_90) {
        lv_image_set_src(target->bat_90, png_bat_90);
    }
    lv_obj_align(target->bat_90, bat->align, 0, 0);
    lv_obj_set_pos(target->bat_90, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_90, LV_OPA_0, 0);

    /* bat 100 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat100.png", base);
    target->bat_100                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_100 = pgs_libpng_decode(path_buffer);
    if(png_bat_100) {
        lv_image_set_src(target->bat_100, png_bat_100);
    }
    lv_obj_align(target->bat_100, bat->align, 0, 0);
    lv_obj_set_pos(target->bat_100, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_100, LV_OPA_0, 0);

    return target;
}

static struct lv_kbd_keyroll * create_keyroll(lv_obj_t * obj, const char * base, struct pgs_kbd_keyroll * keyroll)
{
    if(!base) {
        return NULL;
    }
    if(!keyroll || !keyroll->enable) {
        return NULL;
    }

    struct lv_kbd_keyroll * target = lv_malloc(sizeof(struct lv_kbd_keyroll));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct lv_kbd_keyroll));

    target->_keyroll = keyroll;

    /* keycap */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/keycap.png", base);
    const lv_image_dsc_t * png_keycap = pgs_libpng_decode(path_buffer);

    target->group = lv_group_create();

    target->container = lv_obj_create(obj);
    lv_obj_remove_style_all(target->container);
    lv_obj_set_width(target->container, 197);
    lv_obj_set_height(target->container, 96);
    lv_obj_set_align(target->container, keyroll->align);
    lv_obj_set_style_opa(target->container, keyroll->opa, 0);
    lv_obj_set_scrollbar_mode(target->container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_x(target->container, LV_SCROLL_SNAP_START);
    lv_obj_set_style_bg_opa(target->container, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(target->container, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(target->container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(target->container, 5, LV_PART_MAIN);
    lv_obj_set_x(target->container, keyroll->x);
    lv_obj_set_y(target->container, keyroll->y);
    lv_obj_set_flex_flow(target->container, LV_FLEX_FLOW_ROW_REVERSE);
    lv_obj_set_flex_align(target->container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END);

    for(uint8_t i = 0; i < (sizeof(target->keys) / sizeof(lv_obj_t *)); i++) {
        target->keys[i] = lv_button_create(target->container);
        lv_obj_remove_style_all(target->keys[i]);
        lv_obj_set_width(target->keys[i], 96);
        lv_obj_set_height(target->keys[i], 96);
        lv_obj_set_style_opa(target->keys[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(target->keys[i], lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(target->keys[i], 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(target->keys[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(target->keys[i], 8, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t * label_keycap_top = lv_label_create(target->keys[i]);
        lv_obj_set_width(label_keycap_top, 24);
        lv_obj_set_height(label_keycap_top, 24);
        lv_obj_set_x(label_keycap_top, 23);
        lv_obj_set_y(label_keycap_top, 15);
        lv_label_set_text_fmt(label_keycap_top, " ");
        lv_obj_set_style_text_color(label_keycap_top, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_keycap_top, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(label_keycap_top, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_keycap_top, &lv_font_helveticarounded_24, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t * label_keycap_mid = lv_label_create(target->keys[i]);
        lv_obj_set_width(label_keycap_mid, 58);
        lv_obj_set_height(label_keycap_mid, 20);
        lv_obj_set_x(label_keycap_mid, 20);
        lv_obj_set_y(label_keycap_mid, 34);
        lv_label_set_long_mode(label_keycap_mid, LV_LABEL_LONG_CLIP);
        lv_label_set_text(label_keycap_mid, " ");
        lv_obj_set_style_text_color(label_keycap_mid, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_keycap_mid, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(label_keycap_mid, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_keycap_mid, &lv_font_helveticarounded_24, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t * label_keycap_btm = lv_label_create(target->keys[i]);
        lv_obj_set_width(label_keycap_btm, 24);
        lv_obj_set_height(label_keycap_btm, 24);
        lv_obj_set_x(label_keycap_btm, 23);
        lv_obj_set_y(label_keycap_btm, 50);
        lv_label_set_text_fmt(label_keycap_btm, " ");
        lv_obj_set_style_text_color(label_keycap_btm, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_keycap_btm, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(label_keycap_btm, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_keycap_btm, &lv_font_helveticarounded_24, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t * label_keycap_sgl = lv_label_create(target->keys[i]);
        lv_obj_set_width(label_keycap_sgl, 24);
        lv_obj_set_height(label_keycap_sgl, 24);
        lv_obj_set_x(label_keycap_sgl, 23);
        lv_obj_set_y(label_keycap_sgl, 15);
        lv_label_set_text_fmt(label_keycap_sgl, " ");
        lv_obj_set_style_text_color(label_keycap_sgl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_keycap_sgl, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(label_keycap_sgl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_keycap_sgl, &lv_font_helveticarounded_28, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t * image_keycap = lv_image_create(target->keys[i]);
        if(png_keycap) {
            lv_image_set_src(image_keycap, png_keycap);
        }
        lv_obj_set_width(image_keycap, LV_SIZE_CONTENT);
        lv_obj_set_height(image_keycap, LV_SIZE_CONTENT);
        lv_obj_set_align(image_keycap, LV_ALIGN_CENTER);
        lv_obj_remove_flag(image_keycap, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }

    for(uint8_t i = 0; i < (sizeof(target->keys) / sizeof(lv_obj_t *)); i++) {
        lv_obj_clear_state(target->keys[i], LV_STATE_FOCUS_KEY);
        lv_group_add_obj(target->group, target->keys[i]);
    }

    lv_group_focus_obj(target->keys[2]);
    lv_obj_add_state(target->keys[2], LV_STATE_FOCUS_KEY);

    for(uint8_t i = 0; i < (sizeof(target->keys) / sizeof(lv_obj_t *)); i++) {
        lv_obj_add_event_cb(target->keys[i], key_focused_event_cb, LV_EVENT_FOCUSED, NULL);
    }

    for(uint8_t i = 0; i < (sizeof(target->keys) / sizeof(lv_obj_t *)) + 1; i++) {
        lv_group_set_editing(target->group, false);
        lv_group_focus_next(target->group);
    }

    return target;
}

void keyboard_state_set_macro_state(struct lv_kbd_macro * macro, uint8_t state, uint8_t macro1, uint8_t macro2)
{
    if(!macro) {
        return;
    }

    pgs_animation_stop(macro->macro1);
    pgs_animation_stop(macro->macro2);

    if(macro1) {
        lv_obj_set_style_opa(macro->macro1, macro->_macro1->opa, 0);
    }

    if(macro2) {
        lv_obj_set_style_opa(macro->macro2, macro->_macro2->opa, 0);
    }

    switch(state) {
        case PGS_KBD_STATE_MACRO_STATE_PAUSE:
            lv_obj_set_style_opa(macro->macro_pause, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro_play, macro->_macro->opa, 0);
            lv_obj_set_style_opa(macro->macro_record, LV_OPA_0, 0);
            break;
        case PGS_KBD_STATE_MACRO_STATE_PLAY1:
            if(macro1) {
                lv_obj_set_style_opa(macro->macro_pause, macro->_macro->opa, 0);
                lv_obj_set_style_opa(macro->macro_play, LV_OPA_0, 0);
                lv_obj_set_style_opa(macro->macro_record, LV_OPA_0, 0);
                lv_obj_set_style_opa(macro->macro1, LV_OPA_0, 0);
                pgs_animation_blink_fade(macro->macro1, 250, 250, LV_ANIM_REPEAT_INFINITE, macro->_macro1->opa);
            }
            break;
        case PGS_KBD_STATE_MACRO_STATE_PLAY2:
            if(macro2) {
                lv_obj_set_style_opa(macro->macro_pause, macro->_macro->opa, 0);
                lv_obj_set_style_opa(macro->macro_play, LV_OPA_0, 0);
                lv_obj_set_style_opa(macro->macro_record, LV_OPA_0, 0);
                lv_obj_set_style_opa(macro->macro2, LV_OPA_0, 0);
                pgs_animation_blink_fade(macro->macro2, 250, 250, LV_ANIM_REPEAT_INFINITE, macro->_macro2->opa);
            }
            break;
        case PGS_KBD_STATE_MACRO_STATE_RECORD1:
            lv_obj_set_style_opa(macro->macro_pause, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro_play, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro_record, macro->_macro->opa, 0);
            lv_obj_set_style_opa(macro->macro1, LV_OPA_0, 0);
            pgs_animation_blink_fade(macro->macro1, 250, 250, LV_ANIM_REPEAT_INFINITE, macro->_macro1->opa);
            break;
        case PGS_KBD_STATE_MACRO_STATE_RECORD2:
            lv_obj_set_style_opa(macro->macro_pause, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro_play, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro_record, macro->_macro->opa, 0);
            lv_obj_set_style_opa(macro->macro2, LV_OPA_0, 0);
            pgs_animation_blink_fade(macro->macro2, 250, 250, LV_ANIM_REPEAT_INFINITE, macro->_macro2->opa);
            break;
        default:
            lv_obj_set_style_opa(macro->macro_pause, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro_play, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro_record, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro1, LV_OPA_0, 0);
            lv_obj_set_style_opa(macro->macro2, LV_OPA_0, 0);
            pgs_animation_blink_fade(macro->macro_pause, 100, 100, LV_ANIM_REPEAT_INFINITE, macro->_macro->opa);
            pgs_animation_blink_fade(macro->macro_play, 100, 100, LV_ANIM_REPEAT_INFINITE, macro->_macro->opa);
            pgs_animation_blink_fade(macro->macro_record, 100, 100, LV_ANIM_REPEAT_INFINITE, macro->_macro->opa);
            pgs_animation_blink_fade(macro->macro1, 100, 100, LV_ANIM_REPEAT_INFINITE, macro->_macro1->opa);
            pgs_animation_blink_fade(macro->macro2, 100, 100, LV_ANIM_REPEAT_INFINITE, macro->_macro2->opa);
            break;
    }
}

void keyboard_state_set_layer_state(struct lv_kbd_layer * layer, uint8_t clayer)
{
    if(!layer) {
        return;
    }

    for(uint8_t i = 0; i < 8; i++) {
        if(i == clayer) {
            lv_obj_set_style_opa(layer->layers[i], layer->_layer->opa, 0);
        } else {
            lv_obj_set_style_opa(layer->layers[i], LV_OPA_0, 0);
        }
    }
}

void keyboard_state_set_capslock_state(struct lv_kbd_capslock * capslock, bool on)
{
    if(!capslock) {
        return;
    }

    if(on) {
        lv_obj_set_style_opa(capslock->off, LV_OPA_0, 0);
        lv_obj_set_style_opa(capslock->on, capslock->_capslock->opa, 0);
    } else {
        lv_obj_set_style_opa(capslock->on, LV_OPA_0, 0);
        lv_obj_set_style_opa(capslock->off, capslock->_capslock->opa, 0);
    }
}

void keyboard_state_set_numlock_state(struct lv_kbd_numlock * numlock, bool on)
{
    if(!numlock) {
        return;
    }

    if(on) {
        lv_obj_set_style_opa(numlock->off, LV_OPA_0, 0);
        lv_obj_set_style_opa(numlock->on, numlock->_numlock->opa, 0);
    } else {
        lv_obj_set_style_opa(numlock->on, LV_OPA_0, 0);
        lv_obj_set_style_opa(numlock->off, numlock->_numlock->opa, 0);
    }
}

void keyboard_state_set_output_state(struct lv_kbd_output * output, uint8_t state, bool select)
{
    if(!output) {
        return;
    }

    if(select) {
        lv_obj_set_style_opa(output->disconnected, LV_OPA_0, 0);
        lv_obj_set_style_opa(output->connected, LV_OPA_0, 0);
        if(output->searching) {
            lv_obj_set_style_opa(output->searching, LV_OPA_0, 0);
        }

        switch(state) {
            case PGS_KBD_STATE_OUTPUT_STATE_DISCONNECT:
                pgs_animation_blink_fade(output->disconnected, 250, 250, LV_ANIM_REPEAT_INFINITE, output->_output->opa);
                break;
            case PGS_KBD_STATE_OUTPUT_STATE_CONNECT:
                pgs_animation_blink_fade(output->connected, 250, 250, LV_ANIM_REPEAT_INFINITE, output->_output->opa);
                break;
            case PGS_KBD_STATE_OUTPUT_STATE_SEARCHING:
                if(output->searching) {
                    pgs_animation_blink_fade(output->searching, 250, 250, LV_ANIM_REPEAT_INFINITE,
                                             output->_output->opa);
                }
                break;
            default:
                pgs_animation_blink_fade(output->disconnected, 100, 100, LV_ANIM_REPEAT_INFINITE, output->_output->opa);
                pgs_animation_blink_fade(output->connected, 100, 100, LV_ANIM_REPEAT_INFINITE, output->_output->opa);
                if(output->searching) {
                    pgs_animation_blink_fade(output->searching, 100, 100, LV_ANIM_REPEAT_INFINITE,
                                             output->_output->opa);
                }
                break;
        }
    } else {
        switch(state) {
            case PGS_KBD_STATE_OUTPUT_STATE_DISCONNECT:
                lv_obj_set_style_opa(output->disconnected, output->_output->opa, 0);
                lv_obj_set_style_opa(output->connected, LV_OPA_0, 0);
                if(output->searching) {
                    lv_obj_set_style_opa(output->searching, LV_OPA_0, 0);
                }
                break;
            case PGS_KBD_STATE_OUTPUT_STATE_CONNECT:
                lv_obj_set_style_opa(output->disconnected, LV_OPA_0, 0);
                lv_obj_set_style_opa(output->connected, output->_output->opa, 0);
                if(output->searching) {
                    lv_obj_set_style_opa(output->searching, LV_OPA_0, 0);
                }
                break;
            case PGS_KBD_STATE_OUTPUT_STATE_SEARCHING:
                lv_obj_set_style_opa(output->disconnected, LV_OPA_0, 0);
                lv_obj_set_style_opa(output->connected, LV_OPA_0, 0);
                if(output->searching) {
                    lv_obj_set_style_opa(output->searching, output->_output->opa, 0);
                }
                break;
            default:
                lv_obj_set_style_opa(output->disconnected, LV_OPA_0, 0);
                lv_obj_set_style_opa(output->connected, LV_OPA_0, 0);
                if(output->searching) {
                    lv_obj_set_style_opa(output->searching, LV_OPA_0, 0);
                }
                pgs_animation_blink_fade(output->disconnected, 100, 100, LV_ANIM_REPEAT_INFINITE, output->_output->opa);
                pgs_animation_blink_fade(output->connected, 100, 100, LV_ANIM_REPEAT_INFINITE, output->_output->opa);
                if(output->searching) {
                    pgs_animation_blink_fade(output->searching, 100, 100, LV_ANIM_REPEAT_INFINITE,
                                             output->_output->opa);
                }
                break;
        }
    }
}

void keyboard_state_set_bat_state(struct lv_kbd_bat * bat, uint8_t state, uint8_t level)
{
    if(!bat) {
        return;
    }

    lv_obj_set_style_opa(bat->unknown, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->alert, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->bat_20, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->bat_30, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->bat_50, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->bat_60, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->bat_80, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->bat_90, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->bat_100, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->charging_20, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->charging_30, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->charging_50, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->charging_60, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->charging_80, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->charging_90, LV_OPA_0, 0);
    lv_obj_set_style_opa(bat->charging_100, LV_OPA_0, 0);

    if(state == PGS_KBD_STATE_BAT_STATE_IDLE) {
        if(level > 95) {
            lv_obj_set_style_opa(bat->bat_100, bat->_bat->opa, 0);
        } else if(level > 85) {
            lv_obj_set_style_opa(bat->bat_90, bat->_bat->opa, 0);
        } else if(level > 70) {
            lv_obj_set_style_opa(bat->bat_80, bat->_bat->opa, 0);
        } else if(level > 55) {
            lv_obj_set_style_opa(bat->bat_60, bat->_bat->opa, 0);
        } else if(level > 40) {
            lv_obj_set_style_opa(bat->bat_50, bat->_bat->opa, 0);
        } else if(level > 25) {
            lv_obj_set_style_opa(bat->bat_30, bat->_bat->opa, 0);
        } else if(level > 15) {
            lv_obj_set_style_opa(bat->bat_20, bat->_bat->opa, 0);
        } else {
            lv_obj_set_style_opa(bat->alert, bat->_bat->opa, 0);
        }
    } else if(state == PGS_KBD_STATE_BAT_STATE_CHARGING) {
        if(level < 15) {
            lv_obj_set_style_opa(bat->charging_20, bat->_bat->opa, 0);
        } else if(level < 25) {
            lv_obj_set_style_opa(bat->charging_30, bat->_bat->opa, 0);
        } else if(level < 45) {
            lv_obj_set_style_opa(bat->charging_50, bat->_bat->opa, 0);
        } else if(level < 65) {
            lv_obj_set_style_opa(bat->charging_60, bat->_bat->opa, 0);
        } else if(level < 75) {
            lv_obj_set_style_opa(bat->charging_80, bat->_bat->opa, 0);
        } else if(level < 95) {
            lv_obj_set_style_opa(bat->charging_90, bat->_bat->opa, 0);
        } else {
            lv_obj_set_style_opa(bat->charging_100, bat->_bat->opa, 0);
        }
    } else {
        pgs_animation_blink_fade(bat->unknown, 250, 250, LV_ANIM_REPEAT_INFINITE, bat->_bat->opa);
    }
}

void keyboard_state_set_keyroll(struct lv_kbd_keyroll * keyroll, uint32_t keycode, uint32_t keycolor)
{
    if(!keyroll) {
        return;
    }

    struct keycode_param * _keycode = keycode_param_get(keycode);
    struct keycap_color * _keycolor = keycap_color_get_by_index(keycolor);

    uint32_t keycount     = lv_obj_get_child_cnt(keyroll->container);
    lv_obj_t * key        = lv_obj_get_child(keyroll->container, 0);
    lv_obj_t * keycap_top = lv_obj_get_child(key, 0);
    lv_obj_t * keycap_mid = lv_obj_get_child(key, 1);
    lv_obj_t * keycap_btm = lv_obj_get_child(key, 2);
    lv_obj_t * keycap_sgl = lv_obj_get_child(key, 3);

    lv_label_set_text_fmt(keycap_top, "%s", _keycode->top);
    lv_label_set_text_fmt(keycap_mid, "%s", _keycode->mid);
    lv_label_set_text_fmt(keycap_btm, "%s", _keycode->btm);
    lv_label_set_text_fmt(keycap_sgl, "%s", _keycode->sgl);

    lv_obj_set_style_bg_color(key, lv_color_hex(_keycolor->color), LV_PART_MAIN | LV_STATE_DEFAULT);

    for(uint8_t i = 0; i < keycount; i++) {
        lv_obj_t * ckey = lv_obj_get_child(keyroll->container, i);
        if((255 != lv_obj_get_style_opa(ckey, LV_PART_MAIN | LV_STATE_DEFAULT)) &&
           !lv_color_eq(lv_obj_get_style_bg_color(ckey, LV_PART_MAIN | LV_STATE_DEFAULT), lv_color_hex(0x000000))) {
            lv_obj_set_style_opa(ckey, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    lv_group_set_editing(keyroll->group, false);
    lv_group_focus_next(keyroll->group);
}

struct lv_kbd_state kbd_state;

lv_obj_t * keyboard_state_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode))
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

    struct pgs_kbd_params * params = pgs_kbd_params_parse(config_json);
    if(!params) {
        lv_obj_t * label = lv_label_create(ui_container);
        lv_label_set_text_fmt(label, "Unable to parse config file\n%s", config_json);
        lv_obj_set_style_text_font(label, &lv_font_helveticarounded_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_align(ui_container, LV_ALIGN_CENTER);
        lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), LV_PART_MAIN);
        return ui_container;
    }

    kbd_state.macro =
        create_macro(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_MACRO),
                     pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_MACRO1),
                     pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_MACRO2));

    kbd_state.layer =
        create_layer(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_LAYER));

    kbd_state.capslock =
        create_capslock(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_CAPSLOCK));

    kbd_state.numlock =
        create_numlock(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_NUMLOCK));

    kbd_state.ble1 =
        create_output(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_BLE1));

    kbd_state.ble2 =
        create_output(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_BLE2));

    kbd_state.ble3 =
        create_output(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_BLE3));

    kbd_state.g24 = create_output(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_2G4));

    kbd_state.usb = create_output(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_USB));

    kbd_state.scr = create_output(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_SCR));

    kbd_state.bat = create_bat(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_BAT));

    kbd_state.keyroll = create_keyroll(ui_container, params->base, params->keyroll);

    keyboard_state_set_macro_state(kbd_state.macro, PGS_KBD_STATE_MACRO_STATE_PAUSE, true, true);
    keyboard_state_set_layer_state(kbd_state.layer, 0);
    keyboard_state_set_capslock_state(kbd_state.capslock, true);
    keyboard_state_set_numlock_state(kbd_state.numlock, true);
    keyboard_state_set_output_state(kbd_state.ble1, PGS_KBD_STATE_OUTPUT_STATE_SEARCHING, false);
    keyboard_state_set_output_state(kbd_state.ble2, PGS_KBD_STATE_OUTPUT_STATE_DISCONNECT, false);
    keyboard_state_set_output_state(kbd_state.ble3, PGS_KBD_STATE_OUTPUT_STATE_DISCONNECT, false);
    keyboard_state_set_output_state(kbd_state.g24, PGS_KBD_STATE_OUTPUT_STATE_DISCONNECT, false);
    keyboard_state_set_output_state(kbd_state.usb, PGS_KBD_STATE_OUTPUT_STATE_CONNECT, true);
    keyboard_state_set_output_state(kbd_state.scr, PGS_KBD_STATE_OUTPUT_STATE_CONNECT, false);
    keyboard_state_set_bat_state(kbd_state.bat, PGS_KBD_STATE_BAT_STATE_CHARGING, 90);

    return ui_container;
}
