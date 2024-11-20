#ifndef __KEYBOARD_STATE_H__
#define __KEYBOARD_STATE_H__

#include "lvgl/lvgl.h"
#include "pgs_kbd_params.h"

struct lv_kbd_macro
{
    struct pgs_kbd_state * _macro;
    struct pgs_kbd_state * _macro1;
    struct pgs_kbd_state * _macro2;
    lv_obj_t * macro_pause;
    lv_obj_t * macro_play;
    lv_obj_t * macro_record;
    lv_obj_t * macro1;
    lv_obj_t * macro2;
};

struct lv_kbd_layer
{
    struct pgs_kbd_state * _layer;
    lv_obj_t * layers[8];
};

struct lv_kbd_capslock
{
    struct pgs_kbd_state * _capslock;
    lv_obj_t * on;
    lv_obj_t * off;
};

struct lv_kbd_numlock
{
    struct pgs_kbd_state * _numlock;
    lv_obj_t * on;
    lv_obj_t * off;
};

struct lv_kbd_output
{
    struct pgs_kbd_state * _output;
    lv_obj_t * disconnected;
    lv_obj_t * connected;
    lv_obj_t * searching;
};

struct lv_kbd_bat
{
    struct pgs_kbd_state * _bat;
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

struct lv_kbd_keyroll
{
    struct pgs_kbd_keyroll * _keyroll;
    lv_group_t * group;
    lv_obj_t * container;
    lv_obj_t * keys[3];
};

struct lv_kbd_state
{
    struct lv_kbd_macro * macro;
    struct lv_kbd_layer * layer;
    struct lv_kbd_capslock * capslock;
    struct lv_kbd_numlock * numlock;
    struct lv_kbd_output * ble1;
    struct lv_kbd_output * ble2;
    struct lv_kbd_output * ble3;
    struct lv_kbd_output * g24;
    struct lv_kbd_output * usb;
    struct lv_kbd_output * scr;
    struct lv_kbd_bat * bat;
    struct lv_kbd_keyroll * keyroll;
};

extern struct lv_kbd_state kbd_state;

lv_obj_t * keyboard_state_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode));
void keyboard_state_set_macro_state(struct lv_kbd_macro * macro, uint8_t state, uint8_t macro1, uint8_t macro2);
void keyboard_state_set_layer_state(struct lv_kbd_layer * layer, uint8_t clayer);
void keyboard_state_set_capslock_state(struct lv_kbd_capslock * capslock, bool on);
void keyboard_state_set_numlock_state(struct lv_kbd_numlock * numlock, bool on);
void keyboard_state_set_output_state(struct lv_kbd_output * output, uint8_t state, bool select);
void keyboard_state_set_bat_state(struct lv_kbd_bat * bat, uint8_t state, uint8_t level);
void keyboard_state_set_keyroll(struct lv_kbd_keyroll * keyroll, uint32_t keycode, uint32_t keycolor);

#endif
