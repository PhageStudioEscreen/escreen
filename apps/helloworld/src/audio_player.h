#pragma once

int audio_player_start(const char *path);
int audio_player_stop(void);
int audio_player_is_running(void);
int audio_player_start_list(const char *list_paths);
