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

static lv_obj_t * ui_container;
static lv_group_t * ui_group;
static void (*ui_key_cb)(uint32_t keycode);

static char path_buffer[PATH_MAX];

struct lv_kbd_macro
{
    lv_obj_t * macro_pause;
    lv_obj_t * macro_play;
    lv_obj_t * macro_record;
    lv_obj_t * macro1;
    lv_obj_t * macro2;
};

struct lv_kbd_layer
{
    lv_obj_t * layer[8];
};

struct lv_kbd_capslock
{
    lv_obj_t * on;
    lv_obj_t * off;
};

struct lv_kbd_numlock
{
    lv_obj_t * on;
    lv_obj_t * off;
};

struct lv_kbd_output
{
    lv_obj_t * disconnected;
    lv_obj_t * connected;
    lv_obj_t * searching;
};

struct lv_kbd_bat
{
    lv_obj_t * unknown;
    lv_obj_t * alert;
    lv_obj_t * bat_20;
    lv_obj_t * bat_30;
    lv_obj_t * bat_50;
    lv_obj_t * bat_60;
    lv_obj_t * bat_80;
    lv_obj_t * bat_90;
    lv_obj_t * bat_100;
    lv_obj_t * charging_20;
    lv_obj_t * charging_30;
    lv_obj_t * charging_50;
    lv_obj_t * charging_60;
    lv_obj_t * charging_80;
    lv_obj_t * charging_90;
    lv_obj_t * charging_100;
};

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

    /* macro pause */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/macro_pause.png", base);
    target->macro_pause                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro_pause = pgs_libpng_decode(path_buffer);
    if(png_macro_pause) {
        lv_image_set_src(target->macro_pause, png_macro_pause);
    }
    lv_obj_align(target->macro_pause, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->macro_pause, macro->x, macro->y);
    lv_obj_set_style_opa(target->macro_pause, LV_OPA_0, 0);

    /* macro play */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/macro_play.png", base);
    target->macro_play                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro_play = pgs_libpng_decode(path_buffer);
    if(png_macro_play) {
        lv_image_set_src(target->macro_play, png_macro_play);
    }
    lv_obj_align(target->macro_play, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->macro_play, macro->x, macro->y);
    lv_obj_set_style_opa(target->macro_play, LV_OPA_0, 0);

    /* macro record */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/macro_record.png", base);
    target->macro_record                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro_record = pgs_libpng_decode(path_buffer);
    if(png_macro_record) {
        lv_image_set_src(target->macro_record, png_macro_record);
    }
    lv_obj_align(target->macro_record, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->macro_record, macro->x, macro->y);
    lv_obj_set_style_opa(target->macro_record, LV_OPA_0, 0);

    /* macro 1 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/macro1.png", base);
    target->macro1                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro1 = pgs_libpng_decode(path_buffer);
    if(png_macro1) {
        lv_image_set_src(target->macro1, png_macro1);
    }
    lv_obj_align(target->macro1, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->macro1, macro1->x, macro1->y);
    lv_obj_set_style_opa(target->macro1, LV_OPA_0, 0);

    /* macro 2 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/macro2.png", base);
    target->macro2                    = lv_image_create(obj);
    const lv_image_dsc_t * png_macro2 = pgs_libpng_decode(path_buffer);
    if(png_macro2) {
        lv_image_set_src(target->macro2, png_macro2);
    }
    lv_obj_align(target->macro2, LV_ALIGN_TOP_LEFT, 0, 0);
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

    for(uint8_t i = 0; i < 8; i++) {
        lv_snprintf(path_buffer, sizeof(path_buffer), "%s/layer%d.png", base, i);
        target->layer[i]                 = lv_image_create(obj);
        const lv_image_dsc_t * png_layer = pgs_libpng_decode(path_buffer);
        if(png_layer) {
            lv_image_set_src(target->layer[i], png_layer);
        }
        lv_obj_align(target->layer[i], LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_pos(target->layer[i], layer->x, layer->y);
        lv_obj_set_style_opa(target->layer[i], LV_OPA_0, 0);
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

    /* on */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/capslock_on.png", base);
    target->on                    = lv_image_create(obj);
    const lv_image_dsc_t * png_on = pgs_libpng_decode(path_buffer);
    if(png_on) {
        lv_image_set_src(target->on, png_on);
    }
    lv_obj_align(target->on, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->on, capslock->x, capslock->y);
    lv_obj_set_style_opa(target->on, LV_OPA_0, 0);

    /* off */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/capslock_off.png", base);
    target->off                    = lv_image_create(obj);
    const lv_image_dsc_t * png_off = pgs_libpng_decode(path_buffer);
    if(png_off) {
        lv_image_set_src(target->off, png_off);
    }
    lv_obj_align(target->off, LV_ALIGN_TOP_LEFT, 0, 0);
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

    /* on */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/numlock_on.png", base);
    target->on                    = lv_image_create(obj);
    const lv_image_dsc_t * png_on = pgs_libpng_decode(path_buffer);
    if(png_on) {
        lv_image_set_src(target->on, png_on);
    }
    lv_obj_align(target->on, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->on, numlock->x, numlock->y);
    lv_obj_set_style_opa(target->on, LV_OPA_0, 0);

    /* off */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/numlock_off.png", base);
    target->off                    = lv_image_create(obj);
    const lv_image_dsc_t * png_off = pgs_libpng_decode(path_buffer);
    if(png_off) {
        lv_image_set_src(target->off, png_off);
    }
    lv_obj_align(target->off, LV_ALIGN_TOP_LEFT, 0, 0);
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

    /* disconnected */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/%s.png", base, disconnected_path);
    target->disconnected                    = lv_image_create(obj);
    const lv_image_dsc_t * png_disconnected = pgs_libpng_decode(path_buffer);
    if(png_disconnected) {
        lv_image_set_src(target->disconnected, png_disconnected);
    }
    lv_obj_align(target->disconnected, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->disconnected, output->x, output->y);
    lv_obj_set_style_opa(target->disconnected, LV_OPA_0, 0);

    /* connected */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/%s.png", base, connected_path);
    target->connected                    = lv_image_create(obj);
    const lv_image_dsc_t * png_connected = pgs_libpng_decode(path_buffer);
    if(png_connected) {
        lv_image_set_src(target->connected, png_connected);
    }
    lv_obj_align(target->connected, LV_ALIGN_TOP_LEFT, 0, 0);
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
    lv_obj_align(target->searching, LV_ALIGN_TOP_LEFT, 0, 0);
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

    /* alert */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat_alert.png", base);
    target->alert                    = lv_image_create(obj);
    const lv_image_dsc_t * png_alert = pgs_libpng_decode(path_buffer);
    if(png_alert) {
        lv_image_set_src(target->alert, png_alert);
    }
    lv_obj_align(target->alert, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->alert, bat->x, bat->y);
    lv_obj_set_style_opa(target->alert, LV_OPA_0, 0);

    /* unknown */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat_unknown.png", base);
    target->unknown                    = lv_image_create(obj);
    const lv_image_dsc_t * png_unknown = pgs_libpng_decode(path_buffer);
    if(png_unknown) {
        lv_image_set_src(target->unknown, png_unknown);
    }
    lv_obj_align(target->unknown, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->unknown, bat->x, bat->y);
    lv_obj_set_style_opa(target->unknown, LV_OPA_0, 0);

    /* charging 20 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat20_charging.png", base);
    target->charging_20                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_20 = pgs_libpng_decode(path_buffer);
    if(png_charging_20) {
        lv_image_set_src(target->charging_20, png_charging_20);
    }
    lv_obj_align(target->charging_20, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->charging_20, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_20, LV_OPA_0, 0);

    /* charging 30 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat30_charging.png", base);
    target->charging_30                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_30 = pgs_libpng_decode(path_buffer);
    if(png_charging_30) {
        lv_image_set_src(target->charging_30, png_charging_30);
    }
    lv_obj_align(target->charging_30, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->charging_30, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_30, LV_OPA_0, 0);

    /* charging 50 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat50_charging.png", base);
    target->charging_50                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_50 = pgs_libpng_decode(path_buffer);
    if(png_charging_50) {
        lv_image_set_src(target->charging_50, png_charging_50);
    }
    lv_obj_align(target->charging_50, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->charging_50, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_50, LV_OPA_0, 0);

    /* charging 60 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat60_charging.png", base);
    target->charging_60                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_60 = pgs_libpng_decode(path_buffer);
    if(png_charging_60) {
        lv_image_set_src(target->charging_60, png_charging_60);
    }
    lv_obj_align(target->charging_60, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->charging_60, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_60, LV_OPA_0, 0);

    /* charging 80 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat80_charging.png", base);
    target->charging_80                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_80 = pgs_libpng_decode(path_buffer);
    if(png_charging_80) {
        lv_image_set_src(target->charging_80, png_charging_80);
    }
    lv_obj_align(target->charging_80, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->charging_80, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_80, LV_OPA_0, 0);

    /* charging 90 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat90_charging.png", base);
    target->charging_90                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_90 = pgs_libpng_decode(path_buffer);
    if(png_charging_90) {
        lv_image_set_src(target->charging_90, png_charging_90);
    }
    lv_obj_align(target->charging_90, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->charging_90, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_90, LV_OPA_0, 0);

    /* charging 100 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat100_charging.png", base);
    target->charging_100                    = lv_image_create(obj);
    const lv_image_dsc_t * png_charging_100 = pgs_libpng_decode(path_buffer);
    if(png_charging_100) {
        lv_image_set_src(target->charging_100, png_charging_100);
    }
    lv_obj_align(target->charging_100, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->charging_100, bat->x, bat->y);
    lv_obj_set_style_opa(target->charging_100, LV_OPA_0, 0);

    /* bat 20 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat20.png", base);
    target->bat_20                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_20 = pgs_libpng_decode(path_buffer);
    if(png_bat_20) {
        lv_image_set_src(target->bat_20, png_bat_20);
    }
    lv_obj_align(target->bat_20, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->bat_20, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_20, LV_OPA_0, 0);

    /* bat 30 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat30.png", base);
    target->bat_30                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_30 = pgs_libpng_decode(path_buffer);
    if(png_bat_30) {
        lv_image_set_src(target->bat_30, png_bat_30);
    }
    lv_obj_align(target->bat_30, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->bat_30, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_30, LV_OPA_0, 0);

    /* bat 50 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat50.png", base);
    target->bat_50                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_50 = pgs_libpng_decode(path_buffer);
    if(png_bat_50) {
        lv_image_set_src(target->bat_50, png_bat_50);
    }
    lv_obj_align(target->bat_50, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->bat_50, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_50, LV_OPA_0, 0);

    /* bat 60 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat60.png", base);
    target->bat_60                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_60 = pgs_libpng_decode(path_buffer);
    if(png_bat_60) {
        lv_image_set_src(target->bat_60, png_bat_60);
    }
    lv_obj_align(target->bat_60, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->bat_60, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_60, LV_OPA_0, 0);

    /* bat 80 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat80.png", base);
    target->bat_80                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_80 = pgs_libpng_decode(path_buffer);
    if(png_bat_80) {
        lv_image_set_src(target->bat_80, png_bat_80);
    }
    lv_obj_align(target->bat_80, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->bat_80, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_80, LV_OPA_0, 0);

    /* bat 90 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat90.png", base);
    target->bat_90                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_90 = pgs_libpng_decode(path_buffer);
    if(png_bat_90) {
        lv_image_set_src(target->bat_90, png_bat_90);
    }
    lv_obj_align(target->bat_90, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->bat_90, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_90, LV_OPA_0, 0);

    /* bat 100 */
    lv_snprintf(path_buffer, sizeof(path_buffer), "%s/bat100.png", base);
    target->bat_100                    = lv_image_create(obj);
    const lv_image_dsc_t * png_bat_100 = pgs_libpng_decode(path_buffer);
    if(png_bat_100) {
        lv_image_set_src(target->bat_100, png_bat_100);
    }
    lv_obj_align(target->bat_100, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(target->bat_100, bat->x, bat->y);
    lv_obj_set_style_opa(target->bat_100, LV_OPA_0, 0);

    return target;
}

static void keyboard_state_set_macro_state(struct lv_kbd_macro * kbd_macro, uint8_t state, uint8_t macro1,
                                           uint8_t macro2)
{
    if(!kbd_macro) {
        return;
    }

    pgs_animation_stop(kbd_macro->macro1);
    pgs_animation_stop(kbd_macro->macro2);

    if(macro1) {
        lv_obj_set_style_opa(kbd_macro->macro1, LV_OPA_100, 0);
    }

    if(macro2) {
        lv_obj_set_style_opa(kbd_macro->macro2, LV_OPA_100, 0);
    }

    switch(state) {
        case PGS_KBD_STATE_MACRO_STATE_PAUSE:
            lv_obj_set_style_opa(kbd_macro->macro_pause, LV_OPA_0, 0);
            lv_obj_set_style_opa(kbd_macro->macro_play, LV_OPA_100, 0);
            lv_obj_set_style_opa(kbd_macro->macro_record, LV_OPA_0, 0);
            break;
        case PGS_KBD_STATE_MACRO_STATE_PLAY1:
            if(macro1) {
                lv_obj_set_style_opa(kbd_macro->macro_pause, LV_OPA_100, 0);
                lv_obj_set_style_opa(kbd_macro->macro_play, LV_OPA_0, 0);
                lv_obj_set_style_opa(kbd_macro->macro_record, LV_OPA_0, 0);
                lv_obj_set_style_opa(kbd_macro->macro1, LV_OPA_0, 0);
                pgs_animation_blink_fade(kbd_macro->macro1, 250, 250, LV_ANIM_REPEAT_INFINITE);
            }
            break;
        case PGS_KBD_STATE_MACRO_STATE_PLAY2:
            if(macro2) {
                lv_obj_set_style_opa(kbd_macro->macro_pause, LV_OPA_100, 0);
                lv_obj_set_style_opa(kbd_macro->macro_play, LV_OPA_0, 0);
                lv_obj_set_style_opa(kbd_macro->macro_record, LV_OPA_0, 0);
                lv_obj_set_style_opa(kbd_macro->macro2, LV_OPA_0, 0);
                pgs_animation_blink_fade(kbd_macro->macro2, 250, 250, LV_ANIM_REPEAT_INFINITE);
            }
            break;
        case PGS_KBD_STATE_MACRO_STATE_RECORD1:
            lv_obj_set_style_opa(kbd_macro->macro_pause, LV_OPA_0, 0);
            lv_obj_set_style_opa(kbd_macro->macro_play, LV_OPA_0, 0);
            lv_obj_set_style_opa(kbd_macro->macro_record, LV_OPA_100, 0);
            lv_obj_set_style_opa(kbd_macro->macro1, LV_OPA_0, 0);
            pgs_animation_blink_fade(kbd_macro->macro1, 250, 250, LV_ANIM_REPEAT_INFINITE);
            break;
        case PGS_KBD_STATE_MACRO_STATE_RECORD2:
            lv_obj_set_style_opa(kbd_macro->macro_pause, LV_OPA_0, 0);
            lv_obj_set_style_opa(kbd_macro->macro_play, LV_OPA_0, 0);
            lv_obj_set_style_opa(kbd_macro->macro_record, LV_OPA_100, 0);
            lv_obj_set_style_opa(kbd_macro->macro2, LV_OPA_0, 0);
            pgs_animation_blink_fade(kbd_macro->macro2, 250, 250, LV_ANIM_REPEAT_INFINITE);
            break;
        default:
            lv_obj_set_style_opa(kbd_macro->macro_pause, LV_OPA_0, 0);
            lv_obj_set_style_opa(kbd_macro->macro_play, LV_OPA_0, 0);
            lv_obj_set_style_opa(kbd_macro->macro_record, LV_OPA_0, 0);
            lv_obj_set_style_opa(kbd_macro->macro1, LV_OPA_0, 0);
            lv_obj_set_style_opa(kbd_macro->macro2, LV_OPA_0, 0);
            pgs_animation_blink_fade(kbd_macro->macro_pause, 100, 100, LV_ANIM_REPEAT_INFINITE);
            pgs_animation_blink_fade(kbd_macro->macro_play, 100, 100, LV_ANIM_REPEAT_INFINITE);
            pgs_animation_blink_fade(kbd_macro->macro_record, 100, 100, LV_ANIM_REPEAT_INFINITE);
            pgs_animation_blink_fade(kbd_macro->macro1, 100, 100, LV_ANIM_REPEAT_INFINITE);
            pgs_animation_blink_fade(kbd_macro->macro2, 100, 100, LV_ANIM_REPEAT_INFINITE);
            break;
    }
}

static void keyboard_state_set_layer_state(struct lv_kbd_layer * kbd_layer, uint8_t layer)
{
    if(!kbd_layer) {
        return;
    }

    for(uint8_t i = 0; i < 8; i++) {
        if(i == layer) {
            lv_obj_set_style_opa(kbd_layer->layer[i], LV_OPA_100, 0);
        } else {
            lv_obj_set_style_opa(kbd_layer->layer[i], LV_OPA_0, 0);
        }
    }
}

static void keyboard_state_set_capslock_state(struct lv_kbd_capslock * capslock, bool on)
{
    if(!capslock) {
        return;
    }

    if(on) {
        lv_obj_set_style_opa(capslock->off, LV_OPA_0, 0);
        lv_obj_set_style_opa(capslock->on, LV_OPA_100, 0);
    } else {
        lv_obj_set_style_opa(capslock->on, LV_OPA_0, 0);
        lv_obj_set_style_opa(capslock->off, LV_OPA_100, 0);
    }
}

static void keyboard_state_set_numlock_state(struct lv_kbd_numlock * numlock, bool on)
{
    if(!numlock) {
        return;
    }

    if(on) {
        lv_obj_set_style_opa(numlock->off, LV_OPA_0, 0);
        lv_obj_set_style_opa(numlock->on, LV_OPA_100, 0);
    } else {
        lv_obj_set_style_opa(numlock->on, LV_OPA_0, 0);
        lv_obj_set_style_opa(numlock->off, LV_OPA_100, 0);
    }
}

static void keyboard_state_set_output_state(struct lv_kbd_output * output, uint8_t state, bool select)
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
                pgs_animation_blink_fade(output->disconnected, 250, 250, LV_ANIM_REPEAT_INFINITE);
                break;
            case PGS_KBD_STATE_OUTPUT_STATE_CONNECT:
                pgs_animation_blink_fade(output->connected, 250, 250, LV_ANIM_REPEAT_INFINITE);
                break;
            case PGS_KBD_STATE_OUTPUT_STATE_SEARCHING:
                if(output->searching) {
                    pgs_animation_blink_fade(output->searching, 250, 250, LV_ANIM_REPEAT_INFINITE);
                }
                break;
            default:
                pgs_animation_blink_fade(output->disconnected, 100, 100, LV_ANIM_REPEAT_INFINITE);
                pgs_animation_blink_fade(output->connected, 100, 100, LV_ANIM_REPEAT_INFINITE);
                if(output->searching) {
                    pgs_animation_blink_fade(output->searching, 100, 100, LV_ANIM_REPEAT_INFINITE);
                }
                break;
        }
    } else {
        switch(state) {
            case PGS_KBD_STATE_OUTPUT_STATE_DISCONNECT:
                lv_obj_set_style_opa(output->disconnected, LV_OPA_100, 0);
                lv_obj_set_style_opa(output->connected, LV_OPA_0, 0);
                if(output->searching) {
                    lv_obj_set_style_opa(output->searching, LV_OPA_0, 0);
                }
                break;
            case PGS_KBD_STATE_OUTPUT_STATE_CONNECT:
                lv_obj_set_style_opa(output->disconnected, LV_OPA_0, 0);
                lv_obj_set_style_opa(output->connected, LV_OPA_100, 0);
                if(output->searching) {
                    lv_obj_set_style_opa(output->searching, LV_OPA_0, 0);
                }
                break;
            case PGS_KBD_STATE_OUTPUT_STATE_SEARCHING:
                lv_obj_set_style_opa(output->disconnected, LV_OPA_0, 0);
                lv_obj_set_style_opa(output->connected, LV_OPA_0, 0);
                if(output->searching) {
                    lv_obj_set_style_opa(output->searching, LV_OPA_100, 0);
                }
                break;
            default:
                lv_obj_set_style_opa(output->disconnected, LV_OPA_0, 0);
                lv_obj_set_style_opa(output->connected, LV_OPA_0, 0);
                if(output->searching) {
                    lv_obj_set_style_opa(output->searching, LV_OPA_0, 0);
                }
                pgs_animation_blink_fade(output->disconnected, 100, 100, LV_ANIM_REPEAT_INFINITE);
                pgs_animation_blink_fade(output->connected, 100, 100, LV_ANIM_REPEAT_INFINITE);
                if(output->searching) {
                    pgs_animation_blink_fade(output->searching, 100, 100, LV_ANIM_REPEAT_INFINITE);
                }
                break;
        }
    }
}

static void keyboard_state_set_bat_state(struct lv_kbd_bat * bat, uint8_t state, uint8_t level)
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

    printf("%d %d %d %d\n", state, level);

    if(state == PGS_KBD_STATE_BAT_STATE_IDLE) {
        if(level > 95) {
            lv_obj_set_style_opa(bat->bat_100, LV_OPA_100, 0);
        } else if(level > 85) {
            lv_obj_set_style_opa(bat->bat_90, LV_OPA_100, 0);
        } else if(level > 70) {
            lv_obj_set_style_opa(bat->bat_80, LV_OPA_100, 0);
        } else if(level > 55) {
            lv_obj_set_style_opa(bat->bat_60, LV_OPA_100, 0);
        } else if(level > 40) {
            lv_obj_set_style_opa(bat->bat_50, LV_OPA_100, 0);
        } else if(level > 25) {
            lv_obj_set_style_opa(bat->bat_30, LV_OPA_100, 0);
        } else if(level > 15) {
            lv_obj_set_style_opa(bat->bat_20, LV_OPA_100, 0);
        } else {
            lv_obj_set_style_opa(bat->alert, LV_OPA_100, 0);
        }
    } else if(state == PGS_KBD_STATE_BAT_STATE_CHARGING) {
        if(level < 15) {
            lv_obj_set_style_opa(bat->charging_20, LV_OPA_100, 0);
        } else if(level < 25) {
            lv_obj_set_style_opa(bat->charging_30, LV_OPA_100, 0);
        } else if(level < 45) {
            lv_obj_set_style_opa(bat->charging_50, LV_OPA_100, 0);
        } else if(level < 65) {
            lv_obj_set_style_opa(bat->charging_60, LV_OPA_100, 0);
        } else if(level < 75) {
            lv_obj_set_style_opa(bat->charging_80, LV_OPA_100, 0);
        } else if(level < 95) {
            lv_obj_set_style_opa(bat->charging_90, LV_OPA_100, 0);
        } else {
            lv_obj_set_style_opa(bat->charging_100, LV_OPA_100, 0);
        }
    } else {
        pgs_animation_blink_fade(bat->unknown, 250, 250, LV_ANIM_REPEAT_INFINITE);
    }
}

static void key_event_cb(lv_event_t * event)
{
    if(ui_key_cb) {
        ui_key_cb(lv_indev_get_key(lv_indev_active()));
    }
}

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

    struct lv_kbd_macro * kbd_macro =
        create_macro(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_MACRO),
                     pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_MACRO1),
                     pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_MACRO2));

    struct lv_kbd_layer * kbd_layer =
        create_layer(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_LAYER));

    struct lv_kbd_capslock * kbd_capslock =
        create_capslock(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_CAPSLOCK));

    struct lv_kbd_numlock * kbd_numlock =
        create_numlock(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_NUMLOCK));

    struct lv_kbd_output * kbd_ble1 =
        create_output(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_BLE1));

    struct lv_kbd_output * kbd_ble2 =
        create_output(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_BLE2));

    struct lv_kbd_output * kbd_ble3 =
        create_output(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_BLE3));

    struct lv_kbd_output * kbd_2g4 =
        create_output(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_2G4));

    struct lv_kbd_output * kbd_usb =
        create_output(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_USB));

    struct lv_kbd_output * kbd_scr =
        create_output(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_SCR));

    struct lv_kbd_bat * kbd_bat =
        create_bat(ui_container, params->base, pgs_kbd_params_state_get(params, PGS_KBD_STATE_TYPE_BAT));

    keyboard_state_set_macro_state(kbd_macro, PGS_KBD_STATE_MACRO_STATE_PAUSE, true, true);
    keyboard_state_set_layer_state(kbd_layer, 0);
    keyboard_state_set_capslock_state(kbd_capslock, true);
    keyboard_state_set_numlock_state(kbd_numlock, true);
    keyboard_state_set_output_state(kbd_ble1, PGS_KBD_STATE_OUTPUT_STATE_SEARCHING, false);
    keyboard_state_set_output_state(kbd_ble2, PGS_KBD_STATE_OUTPUT_STATE_DISCONNECT, false);
    keyboard_state_set_output_state(kbd_ble3, PGS_KBD_STATE_OUTPUT_STATE_DISCONNECT, false);
    keyboard_state_set_output_state(kbd_2g4, PGS_KBD_STATE_OUTPUT_STATE_DISCONNECT, false);
    keyboard_state_set_output_state(kbd_usb, PGS_KBD_STATE_OUTPUT_STATE_CONNECT, true);
    keyboard_state_set_output_state(kbd_scr, PGS_KBD_STATE_OUTPUT_STATE_CONNECT, false);
    keyboard_state_set_bat_state(kbd_bat, PGS_KBD_STATE_BAT_STATE_CHARGING, 90);
}
