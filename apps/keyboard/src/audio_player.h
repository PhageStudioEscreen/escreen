#pragma once

struct audio_player_instance;

typedef struct audio_player_instance * audio_player_handle;

audio_player_handle audio_player_create(void);
int audio_player_start_ex(audio_player_handle handle, const char *path);
int audio_player_stop(audio_player_handle handle);
int audio_player_is_running(audio_player_handle handle);
void audio_player_destroy(audio_player_handle handle);
int audio_player_start_list_ex(audio_player_handle handle, const char *list_paths);

int audio_player_start(const char *path);
int audio_player_start_list(const char *list_paths);

int audio_player_keybeep(uint8_t mode, const char *path);

/**
 * Get current audio playback clock in milliseconds
 * @param handle audio player handle
 * @return current position in milliseconds, -1 if not running
 */
int64_t audio_player_get_clock(audio_player_handle handle);
int64_t audio_player_get_global_clock(void);
int audio_player_restart(audio_player_handle handle);
