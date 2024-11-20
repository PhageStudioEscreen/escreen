#ifndef __PGS_KBD_PARAMS_H__
#define __PGS_KBD_PARAMS_H__

#include <stdint.h>
#include <stdbool.h>
#include "lvgl/lvgl.h"

enum pgs_kbd_state_macro_state {
    PGS_KBD_STATE_MACRO_STATE_UNKNOWN = 0,

    PGS_KBD_STATE_MACRO_STATE_PAUSE,
    PGS_KBD_STATE_MACRO_STATE_PLAY1,
    PGS_KBD_STATE_MACRO_STATE_PLAY2,
    PGS_KBD_STATE_MACRO_STATE_RECORD1,
    PGS_KBD_STATE_MACRO_STATE_RECORD2,

    PGS_KBD_STATE_MACRO_STATE_MAX
};

enum pgs_kbd_state_output_state {
    PGS_KBD_STATE_OUTPUT_STATE_UNKNOWN = 0,

    PGS_KBD_STATE_OUTPUT_STATE_DISCONNECT,
    PGS_KBD_STATE_OUTPUT_STATE_CONNECT,
    PGS_KBD_STATE_OUTPUT_STATE_SEARCHING,

    PGS_KBD_STATE_OUTPUT_STATE_MAX
};

enum pgs_kbd_state_bat_state {
    PGS_KBD_STATE_BAT_STATE_UNKNOWN = 0,

    PGS_KBD_STATE_BAT_STATE_IDLE,
    PGS_KBD_STATE_BAT_STATE_CHARGING,

    PGS_KBD_STATE_BAT_STATE_MAX
};

enum pgs_kbd_state_type {
    PGS_KBD_STATE_TYPE_UNKNOWN = 0,

    PGS_KBD_STATE_TYPE_MACRO,
    PGS_KBD_STATE_TYPE_MACRO1,
    PGS_KBD_STATE_TYPE_MACRO2,
    PGS_KBD_STATE_TYPE_LAYER,
    PGS_KBD_STATE_TYPE_CAPSLOCK,
    PGS_KBD_STATE_TYPE_NUMLOCK,
    PGS_KBD_STATE_TYPE_BLE1,
    PGS_KBD_STATE_TYPE_BLE2,
    PGS_KBD_STATE_TYPE_BLE3,
    PGS_KBD_STATE_TYPE_2G4,
    PGS_KBD_STATE_TYPE_USB,
    PGS_KBD_STATE_TYPE_SCR,
    PGS_KBD_STATE_TYPE_BAT,

    PGS_KBD_STATE_TYPE_MAX,
};

enum pgs_kbd_anim {
    PGS_KBD_ANIM_NONE = 0,
    PGS_KBD_ANIM_FADE,
    PGS_KBD_ANIM_LEFT_RIGHT,
    PGS_KBD_ANIM_RIGHT_LEFT,
    PGS_KBD_ANIM_UP_DOWN,
    PGS_KBD_ANIM_DOWN_UP,
};

struct pgs_kbd_state
{
    uint8_t enable;
    uint8_t align;
    uint8_t anim;
    uint8_t opa;

    int32_t x;
    int32_t y;

    uint8_t type;
};

struct pgs_kbd_keyroll
{
    uint8_t enable;
    uint8_t align;
    uint8_t anim;
    uint8_t opa;

    int32_t x;
    int32_t y;

    uint8_t n;

    const char *coloration;
};

struct pgs_kbd_apmchart
{
    uint8_t enable;
    uint8_t align;
    uint8_t anim;
    uint8_t opa;

    int32_t x;
    int32_t y;
    uint16_t w;
    uint16_t h;
    uint32_t color;
};

struct pgs_kbd_apmlabel
{
    uint8_t enable;
    uint8_t align;
    uint8_t anim;
    uint8_t opa;
    uint8_t text_align;

    int32_t x;
    int32_t y;
    uint16_t w;
    uint16_t h;
    uint32_t color;

    const lv_font_t * font;
};

struct pgs_kbd_label
{
    uint8_t enable;
    uint8_t align;
    uint8_t anim;
    uint8_t opa;
    uint8_t text_align;

    int32_t x;
    int32_t y;
    uint16_t w;
    uint16_t h;
    uint32_t color;

    const lv_font_t * font;

    const char * text;
};

struct pgs_kbd_image
{
    uint8_t enable;
    uint8_t align;
    uint8_t anim;
    uint8_t opa;

    int32_t x;
    int32_t y;

    const char * path;
};

struct pgs_kbd_params
{
    uint32_t states_count;
    uint32_t labels_count;
    uint32_t images_count;

    const char * base;

    struct pgs_kbd_state * states;
    struct pgs_kbd_keyroll * keyroll;
    struct pgs_kbd_apmchart * apmchart;
    struct pgs_kbd_apmlabel * apmlabel;
    struct pgs_kbd_label * labels;
    struct pgs_kbd_image * images;

    void * cjson;
};

void pgs_kbd_params_delete(struct pgs_kbd_params * params);
struct pgs_kbd_params * pgs_kbd_params_parse(const char * json_path);
struct pgs_kbd_state * pgs_kbd_params_state_get(struct pgs_kbd_params * params, uint8_t type);

#endif
