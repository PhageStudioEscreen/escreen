#ifndef __PGS_KBD_PARAMS_H__
#define __PGS_KBD_PARAMS_H__

#include <stdint.h>
#include <stdbool.h>
#include "lvgl/lvgl.h"

enum pgs_widgets_macro_state {
    PGS_WIDGETS_MACRO_STATE_UNKNOWN = 0,

    PGS_WIDGETS_MACRO_STATE_PAUSE,
    PGS_WIDGETS_MACRO_STATE_PLAY1,
    PGS_WIDGETS_MACRO_STATE_PLAY2,
    PGS_WIDGETS_MACRO_STATE_RECORD1,
    PGS_WIDGETS_MACRO_STATE_RECORD2,

    PGS_WIDGETS_MACRO_STATE_MAX,
};

enum pgs_widgets_output_state {
    PGS_WIDGETS_OUTPUT_STATE_UNKNOWN = 0,

    PGS_WIDGETS_OUTPUT_STATE_SUSPEND,
    PGS_WIDGETS_OUTPUT_STATE_DISCONNECT,
    PGS_WIDGETS_OUTPUT_STATE_CONNECT,
    PGS_WIDGETS_OUTPUT_STATE_SEARCHING,
    PGS_WIDGETS_OUTPUT_STATE_CONFIRM_SELECT,
    PGS_WIDGETS_OUTPUT_STATE_CONFIRM_ERASE,
    PGS_WIDGETS_OUTPUT_STATE_ERASE_ADV,

    PGS_WIDGETS_OUTPUT_STATE_MAX,
};

enum pgs_widgets_output_mode {
    PGS_WIDGETS_OUTPUT_MODE_UNKNOWN = 0,

    PGS_WIDGETS_OUTPUT_MODE_AUTO,
    PGS_WIDGETS_OUTPUT_MODE_USB,
    PGS_WIDGETS_OUTPUT_MODE_WLS,
    PGS_WIDGETS_OUTPUT_MODE_SCR,

    PGS_WIDGETS_OUTPUT_MODE_MAX,
};

enum pgs_widgets_output_target {
    PGS_WIDGETS_OUTPUT_TARGET_UNKNOWN = 0,

    PGS_WIDGETS_OUTPUT_TARGET_USB,
    PGS_WIDGETS_OUTPUT_TARGET_SCR,
    PGS_WIDGETS_OUTPUT_TARGET_BLE1,
    PGS_WIDGETS_OUTPUT_TARGET_BLE2,
    PGS_WIDGETS_OUTPUT_TARGET_BLE3,
    PGS_WIDGETS_OUTPUT_TARGET_2G4,

    PGS_WIDGETS_OUTPUT_TARGET_MAX,
};

enum pgs_widgets_bat_state {
    PGS_WIDGETS_BAT_STATE_UNKNOWN = 0,

    PGS_WIDGETS_BAT_STATE_IDLE,
    PGS_WIDGETS_BAT_STATE_CHARGING,

    PGS_WIDGETS_BAT_STATE_MAX,
};

enum pgs_widgets_type {
    PGS_WIDGETS_TYPE_UNKNOWN = 0,

    PGS_WIDGETS_TYPE_MACRO,
    PGS_WIDGETS_TYPE_MACRO1,
    PGS_WIDGETS_TYPE_MACRO2,
    PGS_WIDGETS_TYPE_LAYER,
    PGS_WIDGETS_TYPE_CAPSLOCK,
    PGS_WIDGETS_TYPE_NUMLOCK,
    PGS_WIDGETS_TYPE_BLE1,
    PGS_WIDGETS_TYPE_BLE2,
    PGS_WIDGETS_TYPE_BLE3,
    PGS_WIDGETS_TYPE_2G4,
    PGS_WIDGETS_TYPE_USB,
    PGS_WIDGETS_TYPE_SCR,
    PGS_WIDGETS_TYPE_BAT,

    PGS_WIDGETS_TYPE_MAX,
};

enum pgs_widgets_anim {
    PGS_WIDGETS_ANIM_NONE = 0,

    PGS_WIDGETS_ANIM_FADE,
    PGS_WIDGETS_ANIM_LEFT_RIGHT,
    PGS_WIDGETS_ANIM_RIGHT_LEFT,
    PGS_WIDGETS_ANIM_UP_DOWN,
    PGS_WIDGETS_ANIM_DOWN_UP,

    PGS_WIDGETS_ANIM_MAX,
};

enum pgs_widgets_zindex {
    PGS_WIDGETS_ZINDEX_DEFAULT = 0xffff,
};

struct pgs_widgets_params_state
{
    uint8_t enable;
    uint8_t align;
    uint8_t anim;
    uint8_t opa;
    uint16_t zindex;

    int32_t x;
    int32_t y;

    uint8_t type;
};

struct pgs_widgets_params_keyroll
{
    uint8_t enable;
    uint8_t align;
    uint8_t anim;
    uint8_t opa;
    uint16_t zindex;

    int32_t x;
    int32_t y;

    uint8_t n;

    const char * coloration;
};

struct pgs_widgets_params_wpmchart
{
    uint8_t enable;
    uint8_t align;
    uint8_t anim;
    uint8_t opa;
    uint16_t zindex;

    int32_t x;
    int32_t y;
    uint16_t w;
    uint16_t h;
    uint32_t color;
};

struct pgs_widgets_params_wpmlabel
{
    uint8_t enable;
    uint8_t align;
    uint8_t anim;
    uint8_t opa;
    uint16_t zindex;
    uint8_t text_align;

    int32_t x;
    int32_t y;
    uint16_t w;
    uint16_t h;
    uint32_t color;

    const lv_font_t * font;
};

struct pgs_widgets_params_label
{
    uint8_t enable;
    uint8_t align;
    uint8_t anim;
    uint8_t opa;
    uint16_t zindex;
    uint8_t text_align;

    int32_t x;
    int32_t y;
    uint16_t w;
    uint16_t h;
    uint32_t color;

    const lv_font_t * font;

    const char * text;
};

struct pgs_widgets_params_image
{
    uint8_t enable;
    uint8_t align;
    uint8_t anim;
    uint8_t opa;
    uint16_t zindex;

    int32_t radius;
    int32_t x;
    int32_t y;

    const char * path;
};

struct pgs_widgets_params_gif
{
    uint8_t enable;
    uint8_t align;
    uint8_t anim;
    uint8_t opa;
    uint16_t zindex;

    int32_t radius;
    int32_t x;
    int32_t y;

    const char * path;
};

struct pgs_widgets_params_video
{
    uint8_t enable;
    uint8_t align;
    uint8_t anim;
    uint8_t opa;
    uint16_t zindex;

    int32_t radius;
    int32_t x;
    int32_t y;

    uint32_t count;
    const char ** paths;
};

struct keyboard_params
{
    uint32_t states_count;
    uint32_t labels_count;
    uint32_t images_count;
    uint32_t gifs_count;
    uint32_t videos_count;

    const char * base;

    struct pgs_widgets_params_state * states;
    struct pgs_widgets_params_keyroll * keyroll;
    struct pgs_widgets_params_wpmchart * wpmchart;
    struct pgs_widgets_params_wpmlabel * wpmlabel;
    struct pgs_widgets_params_label * labels;
    struct pgs_widgets_params_image * images;
    struct pgs_widgets_params_gif * gifs;
    struct pgs_widgets_params_video * videos;

    void * cjson;
};

void keyboard_params_delete(struct keyboard_params * params);
struct keyboard_params * keyboard_params_parse(const char * json_path);
struct pgs_widgets_params_state * keyboard_params_state_get(struct keyboard_params * params, uint8_t type);

#endif
