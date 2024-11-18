#ifndef __PGS_KBD_PARAMS_H__
#define __PGS_KBD_PARAMS_H__

#include <stdint.h>
#include <stdbool.h>
#include "lvgl/lvgl.h"

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

enum pgs_kbd_state_anim {
    PGS_KBD_STATE_ANIM_NONE = 0,
    PGS_KBD_STATE_ANIM_ERASE,
    PGS_KBD_STATE_ANIM_LEFT_RIGHT,
    PGS_KBD_STATE_ANIM_RIGHT_LEFT,
    PGS_KBD_STATE_ANIM_UP_DOWN,
    PGS_KBD_STATE_ANIM_DOWN_UP,
};

struct pgs_kbd_state
{
    uint8_t type;
    uint8_t enable;
    uint8_t anim;
    int32_t x;
    int32_t y;
};

struct pgs_kbd_keyroll
{
    uint8_t enable;
    uint8_t n;
    int32_t x;
    int32_t y;
};

struct pgs_kbd_apmchart
{
    uint8_t enable;
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
    int32_t x;
    int32_t y;
    uint16_t w;
    uint16_t h;
    uint32_t color;
};

struct pgs_kbd_label
{
    uint8_t enable;
    uint8_t size;
    uint8_t align;
    const char * text;
    int32_t x;
    int32_t y;
    uint16_t w;
    uint16_t h;
    uint32_t color;
};

struct pgs_kbd_image
{
    uint8_t enable;
    const char * path;
    int32_t x;
    int32_t y;
};

struct pgs_kbd_params
{
    uint8_t states_count;
    uint8_t labels_count;
    uint8_t images_count;

    const char * base;

    struct pgs_kbd_state * states;
    struct pgs_kbd_keyroll * keyroll;
    struct pgs_kbd_apmchart * apmchart;
    struct pgs_kbd_apmlabel * apmlabel;
    struct pgs_kbd_label * labels;
    struct pgs_kbd_image * images;

    void * cjson;
};

struct pgs_kbd_params * pgs_kbd_params_parse(const char * json_path);
void pgs_kbd_params_delete(struct pgs_kbd_params * params);

#endif
