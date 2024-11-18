#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include "pgs_kbd_params.h"
#include "cJSON.h"

static uint8_t * alloc_file(const char * filename, uint32_t * size)
{
    uint8_t * data = NULL;
    lv_fs_file_t f;
    uint32_t data_size;
    uint32_t rn;
    lv_fs_res_t res;

    *size = 0;

    res = lv_fs_open(&f, filename, LV_FS_MODE_RD);
    if(res != LV_FS_RES_OK) {
        LV_LOG_WARN("can't open %s", filename);
        return NULL;
    }

    res = lv_fs_seek(&f, 0, LV_FS_SEEK_END);
    if(res != LV_FS_RES_OK) {
        goto failed;
    }

    res = lv_fs_tell(&f, &data_size);
    if(res != LV_FS_RES_OK) {
        goto failed;
    }

    res = lv_fs_seek(&f, 0, LV_FS_SEEK_SET);
    if(res != LV_FS_RES_OK) {
        goto failed;
    }

    /*Read file to buffer*/
    data = lv_malloc(data_size);
    if(data == NULL) {
        LV_LOG_WARN("malloc failed for data");
        goto failed;
    }

    res = lv_fs_read(&f, data, data_size, &rn);

    if(res == LV_FS_RES_OK && rn == data_size) {
        *size = rn;
    } else {
        LV_LOG_WARN("read file failed");
        lv_free(data);
        data = NULL;
    }

failed:
    lv_fs_close(&f);

    return data;
}

#define STRNCMP_UTIL(str1, str2) strncmp((str1), (str2), sizeof(str2))

static uint8_t pgs_get_state_type(const char * name)
{
    if(!STRNCMP_UTIL(name, "macro")) {
        return PGS_KBD_STATE_TYPE_MACRO;
    } else if(!STRNCMP_UTIL(name, "macro1")) {
        return PGS_KBD_STATE_TYPE_MACRO1;
    } else if(!STRNCMP_UTIL(name, "macro2")) {
        return PGS_KBD_STATE_TYPE_MACRO2;
    } else if(!STRNCMP_UTIL(name, "layer")) {
        return PGS_KBD_STATE_TYPE_LAYER;
    } else if(!STRNCMP_UTIL(name, "capslock")) {
        return PGS_KBD_STATE_TYPE_CAPSLOCK;
    } else if(!STRNCMP_UTIL(name, "numlock")) {
        return PGS_KBD_STATE_TYPE_NUMLOCK;
    } else if(!STRNCMP_UTIL(name, "ble1")) {
        return PGS_KBD_STATE_TYPE_BLE1;
    } else if(!STRNCMP_UTIL(name, "ble2")) {
        return PGS_KBD_STATE_TYPE_BLE2;
    } else if(!STRNCMP_UTIL(name, "ble3")) {
        return PGS_KBD_STATE_TYPE_BLE3;
    } else if(!STRNCMP_UTIL(name, "2g4")) {
        return PGS_KBD_STATE_TYPE_2G4;
    } else if(!STRNCMP_UTIL(name, "usb")) {
        return PGS_KBD_STATE_TYPE_USB;
    } else if(!STRNCMP_UTIL(name, "scr")) {
        return PGS_KBD_STATE_TYPE_SCR;
    } else if(!STRNCMP_UTIL(name, "bat")) {
        return PGS_KBD_STATE_TYPE_BAT;
    } else {
        return PGS_KBD_STATE_TYPE_UNKNOWN;
    }
}

static uint8_t pgs_get_state_anim(const char * anim)
{
    if(!STRNCMP_UTIL(anim, "erase")) {
        return PGS_KBD_STATE_ANIM_ERASE;
    } else if(!STRNCMP_UTIL(anim, "left_right")) {
        return PGS_KBD_STATE_ANIM_LEFT_RIGHT;
    } else if(!STRNCMP_UTIL(anim, "right_left")) {
        return PGS_KBD_STATE_ANIM_RIGHT_LEFT;
    } else if(!STRNCMP_UTIL(anim, "up_down")) {
        return PGS_KBD_STATE_ANIM_UP_DOWN;
    } else if(!STRNCMP_UTIL(anim, "down_up")) {
        return PGS_KBD_STATE_ANIM_DOWN_UP;
    } else {
        return PGS_KBD_STATE_ANIM_NONE;
    }
}

struct pgs_kbd_params * pgs_kbd_params_parse(const char * json_path)
{
    uint32_t size;

    if(!json_path) {
        return NULL;
    }

    const char * message = (void *)alloc_file(json_path, &size);
    if(message == NULL) {
        return NULL;
    }

    cJSON * cjson_root = NULL;

    cjson_root = cJSON_Parse(message);
    if(!cjson_root) {
        LV_LOG_WARN("parse failed\n");
        goto err1;
    }

    struct pgs_kbd_params * params = lv_malloc(sizeof(struct pgs_kbd_params));
    if(!cjson_root) {
        LV_LOG_WARN("malloc failed\n");
        goto err2;
    }
    lv_memzero(params, sizeof(struct pgs_kbd_params));
    params->cjson = cjson_root;

    cJSON * cjson_base   = NULL;
    cJSON * cjson_states = NULL;

    cjson_base = cJSON_GetObjectItem(cjson_root, "base");
    if(cjson_base && cJSON_IsString(cjson_base)) {
        params->base = cjson_base->valuestring;
    } else {
        params->base = "/usr/share/pgs/themes/default";
    }

    cjson_states = cJSON_GetObjectItem(cjson_root, "states");
    if(cjson_states && cJSON_IsArray(cjson_states)) {
        int count            = cJSON_GetArraySize(cjson_states);
        params->states       = lv_malloc(sizeof(struct pgs_kbd_state) * PGS_KBD_STATE_TYPE_MAX);
        params->states_count = 0;

        for(uint32_t i = 0; i < count; i++) {

            cJSON * cjson_state = cJSON_GetArrayItem(cjson_states, i);
            if(cjson_state) {
                cJSON * cjson_name = cJSON_GetObjectItem(cjson_state, "name");
                if(!(cjson_name && cJSON_IsString(cjson_name) && pgs_get_state_type(cjson_name->valuestring))) {
                    continue;
                }
                cJSON * cjson_enable = cJSON_GetObjectItem(cjson_state, "enable");
                if(!(cjson_enable && cJSON_IsBool(cjson_enable))) {
                    continue;
                }
                cJSON * cjson_prop = cJSON_GetObjectItem(cjson_state, "prop");
                if(!(cjson_prop && cJSON_IsObject(cjson_prop))) {
                    continue;
                }
                cJSON * cjson_x = cJSON_GetObjectItem(cjson_prop, "x");
                if(!(cjson_x && cJSON_IsNumber(cjson_x))) {
                    continue;
                }
                cJSON * cjson_y = cJSON_GetObjectItem(cjson_prop, "y");
                if(!(cjson_y && cJSON_IsNumber(cjson_y))) {
                    continue;
                }
                cJSON * cjson_anim = cJSON_GetObjectItem(cjson_prop, "anim");
                if(!(cjson_anim && cJSON_IsString(cjson_anim))) {
                    continue;
                }

                params->states[params->states_count].type   = pgs_get_state_type(cjson_name->valuestring);
                params->states[params->states_count].enable = cjson_enable->valueint;
                params->states[params->states_count].x      = cjson_x->valueint;
                params->states[params->states_count].y      = cjson_y->valueint;
                params->states[params->states_count].anim   = pgs_get_state_anim(cjson_anim->valuestring);
                params->states_count++;
                if(params->states_count >= PGS_KBD_STATE_TYPE_MAX) {
                    break;
                }
            }
        }
    } else {
        params->states_count = 0;
        params->states       = NULL;
    }

    cJSON * cjson_keyroll  = NULL;
    cJSON * cjson_apmchart = NULL;
    cJSON * cjson_apmlabel = NULL;
    cJSON * cjson_labels   = NULL;
    cJSON * cjson_images   = NULL;

    return params;

err3:
    lv_free(params);

err2:
    cJSON_Delete(cjson_root);

err1:
    lv_free(message);

    return NULL;
}

void pgs_kbd_params_delete(struct pgs_kbd_params * params)
{
    if(!params) {
        return;
    }
    if(params->states) lv_free(params->states);
    if(params->keyroll) lv_free(params->keyroll);
    if(params->apmchart) lv_free(params->apmchart);
    if(params->apmlabel) lv_free(params->apmlabel);
    if(params->labels) lv_free(params->labels);
    if(params->images) lv_free(params->images);
    if(params->cjson) cJSON_Delete(params->cjson);
    lv_free(params);
}
