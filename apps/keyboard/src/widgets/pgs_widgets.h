#ifndef __PGS_WIDGETS_H__
#define __PGS_WIDGETS_H__

#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include "lvgl/lvgl.h"
#include "pgs_modules.h"
#include "pgs_utils.h"
#include "pgs_anim.h"
#include "pgs_app_keyboard.h"
#include "keyboard_params.h"
#include "keycode_params.h"
#include "keycap_colors.h"

extern char keyboard_path_buffer[PATH_MAX];

struct pgs_widgets_macro
{
    struct pgs_widgets_params_state * _macro;
    struct pgs_widgets_params_state * _macro1;
    struct pgs_widgets_params_state * _macro2;
    lv_obj_t * macro_pause;
    lv_obj_t * macro_play;
    lv_obj_t * macro_record;
    lv_obj_t * macro1;
    lv_obj_t * macro2;
};

struct pgs_widgets_layer
{
    struct pgs_widgets_params_state * _layer;
    lv_obj_t * layers[8];
};

struct pgs_widgets_capslock
{
    struct pgs_widgets_params_state * _capslock;
    lv_obj_t * on;
    lv_obj_t * off;
};

struct pgs_widgets_numlock
{
    struct pgs_widgets_params_state * _numlock;
    lv_obj_t * on;
    lv_obj_t * off;
};

struct pgs_widgets_output
{
    struct pgs_widgets_params_state * _output;
    lv_obj_t * disconnected;
    lv_obj_t * connected;
    lv_obj_t * searching;
};

struct pgs_widgets_bat
{
    struct pgs_widgets_params_state * _bat;
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

struct pgs_widgets_keyroll
{
    struct pgs_widgets_params_keyroll * _keyroll;
    lv_group_t * group;
    lv_obj_t * container;
    lv_obj_t * keys[3];
};

struct pgs_widgets_keyanim
{
    const char * base;
    struct pgs_widgets_params_keyanim * _keyanim;
    uint32_t waitcount;
    uint32_t keycode;
    uint32_t index;
    uint32_t group;
    uint32_t group_max;
    uint8_t playflag;
    lv_obj_t * container;
    lv_obj_t * anim[2];
    lv_obj_t ** waitanims;
    lv_timer_t * delay_timer[2];
};

struct pgs_widgets_keysnd
{
    const char * base;
    struct pgs_widgets_params_keysnd * _keysnd;
    lv_obj_t ** snd;
};

struct pgs_widgets_wpmchart
{
    struct pgs_widgets_params_wpmchart * _wpmchart;
    lv_obj_t * contanier;
};

struct pgs_widgets_wpmlabel
{
    struct pgs_widgets_params_wpmlabel * _wpmlabel;
    lv_obj_t * text;
};

struct pgs_widgets_labels
{
    struct pgs_widgets_params_label * _labels;
    uint32_t count;
    lv_obj_t ** labels;
};

struct pgs_widgets_images
{
    struct pgs_widgets_params_image * _images;
    uint32_t count;
    lv_obj_t ** images;
};

struct pgs_widgets_gifs
{
    struct pgs_widgets_params_gif * _gifs;
    uint32_t count;
    lv_obj_t ** gifs;
};

struct pgs_widgets_videos
{
    const char * base;
    struct pgs_widgets_params_video * _videos;
    uint32_t count;
    uint32_t * index;
    uint32_t * path_count;
    lv_obj_t ** videos;
};

struct pgs_widgets_macro * pgs_widgets_macro_create(lv_obj_t * obj, const char * base,
                                                    struct pgs_widgets_params_state * macro,
                                                    struct pgs_widgets_params_state * macro1,
                                                    struct pgs_widgets_params_state * macro2);
void pgs_widgets_macro_set_state(struct pgs_widgets_macro * macro, uint8_t state, uint8_t macro1, uint8_t macro2);

struct pgs_widgets_layer * pgs_widgets_layer_create(lv_obj_t * obj, const char * base,
                                                    struct pgs_widgets_params_state * layer);
void pgs_widgets_layer_set_current(struct pgs_widgets_layer * layer, uint8_t clayer);

struct pgs_widgets_capslock * pgs_widgets_capslock_create(lv_obj_t * obj, const char * base,
                                                          struct pgs_widgets_params_state * capslock);
void pgs_widgets_capslock_set_state(struct pgs_widgets_capslock * capslock, bool on);

struct pgs_widgets_numlock * pgs_widgets_numlock_create(lv_obj_t * obj, const char * base,
                                                        struct pgs_widgets_params_state * numlock);
void pgs_widgets_numlock_set_state(struct pgs_widgets_numlock * numlock, bool on);

struct pgs_widgets_output * pgs_widgets_output_create(lv_obj_t * obj, const char * base,
                                                      struct pgs_widgets_params_state * output);
void pgs_widgets_output_set_state(struct pgs_widgets_output * output, uint8_t state, bool select);

struct pgs_widgets_bat * pgs_widgets_bat_create(lv_obj_t * obj, const char * base,
                                                struct pgs_widgets_params_state * bat);
void pgs_widgets_bat_set_state(struct pgs_widgets_bat * bat, uint8_t state, uint8_t level);

struct pgs_widgets_keyroll * pgs_widgets_keyroll_create(lv_obj_t * obj, const char * base,
                                                        struct pgs_widgets_params_keyroll * keyroll);
void pgs_widgets_keyroll_push(struct pgs_widgets_keyroll * keyroll, uint32_t keycode, uint32_t keycolor);

struct pgs_widgets_keyanim * pgs_widgets_keyanim_create(lv_obj_t * obj, const char * base,
                                                        struct pgs_widgets_params_keyanim * keyanim);

void pgs_widgets_keyanim_push(struct pgs_widgets_keyanim * keyanim, uint32_t keycode);

struct pgs_widgets_wpmlabel * pgs_widgets_wpmlabel_create(lv_obj_t * obj, const char * base,
                                                          struct pgs_widgets_params_wpmlabel * wpmlabel);
void pgs_widgets_wpmlabel_set_wpm(struct pgs_widgets_wpmlabel * wpmlabel, uint32_t wpm);

struct pgs_widgets_labels * pgs_widgets_labels_create(lv_obj_t * obj, const char * base,
                                                      struct pgs_widgets_params_label * labels, uint32_t count);

struct pgs_widgets_images * pgs_widgets_images_create(lv_obj_t * obj, const char * base,
                                                      struct pgs_widgets_params_image * images, uint32_t count);

struct pgs_widgets_gifs * pgs_widgets_gifs_create(lv_obj_t * obj, const char * base,
                                                  struct pgs_widgets_params_gif * gifs, uint32_t count);

struct pgs_widgets_videos * pgs_widgets_videos_create(lv_obj_t * obj, const char * base,
                                                      struct pgs_widgets_params_video * videos, uint32_t count);
#endif
