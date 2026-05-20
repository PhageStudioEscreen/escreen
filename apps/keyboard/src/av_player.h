#pragma once

#include <stdbool.h>

struct lv_obj_t;

struct av_player_instance;
typedef struct av_player_instance * av_player_handle;

av_player_handle av_player_create(struct lv_obj_t * parent, const char * path, bool loop);
struct lv_obj_t * av_player_get_obj(av_player_handle handle);
int av_player_start(av_player_handle handle);
int av_player_stop(av_player_handle handle);
void av_player_destroy(av_player_handle handle);
