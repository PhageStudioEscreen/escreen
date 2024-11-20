#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include "pgs_utils.h"
#include "pgs_kbd_params.h"
#include "cJSON.h"

#define STRNCMP_UTIL(str1, str2) strncmp((str1), (str2), sizeof(str2))

#define CJSON_PARSE_BEG do
#define CSJON_PARSE_END while(0)

#define CJSON_PARSE_ARRAY_BEG(start, end) for(uint32_t i = (start); i < (end); i++)
#define CJSON_PARSE_ARRAY_END

#define CJSON_PARSE_NAME(cjson_obj)                                                                                    \
    cJSON * cjson_name = cJSON_GetObjectItem(cjson_obj, "name");                                                       \
    if(!(cjson_name && cJSON_IsString(cjson_name) && pgs_parse_state_type(cjson_name->valuestring))) {                 \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_ENABLE(cjson_obj)                                                                                  \
    cJSON * cjson_enable = cJSON_GetObjectItem(cjson_obj, "enable");                                                   \
    if(!(cjson_enable && cJSON_IsBool(cjson_enable))) {                                                                \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_ALIGN(cjson_obj)                                                                                   \
    cJSON * cjson_align = cJSON_GetObjectItem(cjson_obj, "align");                                                     \
    if(!(cjson_align && cJSON_IsString(cjson_align))) {                                                                \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_TEXT_ALIGN(cjson_obj)                                                                              \
    cJSON * cjson_text_align = cJSON_GetObjectItem(cjson_obj, "text-align");                                           \
    if(!(cjson_text_align && cJSON_IsString(cjson_text_align))) {                                                      \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_ANIM(cjson_obj)                                                                                    \
    cJSON * cjson_anim = cJSON_GetObjectItem(cjson_obj, "anim");                                                       \
    if(!(cjson_anim && cJSON_IsString(cjson_anim))) {                                                                  \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_COLOR(cjson_obj)                                                                                   \
    cJSON * cjson_color = cJSON_GetObjectItem(cjson_obj, "color");                                                     \
    if(!(cjson_color && cJSON_IsString(cjson_color))) {                                                                \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_COLORATION(cjson_obj)                                                                              \
    cJSON * cjson_coloration = cJSON_GetObjectItem(cjson_obj, "coloration");                                           \
    if(!(cjson_coloration && cJSON_IsString(cjson_coloration))) {                                                      \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_X(cjson_obj)                                                                                       \
    cJSON * cjson_x = cJSON_GetObjectItem(cjson_obj, "x");                                                             \
    if(!(cjson_x && cJSON_IsNumber(cjson_x))) {                                                                        \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_Y(cjson_obj)                                                                                       \
    cJSON * cjson_y = cJSON_GetObjectItem(cjson_obj, "y");                                                             \
    if(!(cjson_y && cJSON_IsNumber(cjson_y))) {                                                                        \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_W(cjson_obj)                                                                                       \
    cJSON * cjson_w = cJSON_GetObjectItem(cjson_obj, "w");                                                             \
    if(!(cjson_w && cJSON_IsNumber(cjson_w))) {                                                                        \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_H(cjson_obj)                                                                                       \
    cJSON * cjson_h = cJSON_GetObjectItem(cjson_obj, "h");                                                             \
    if(!(cjson_h && cJSON_IsNumber(cjson_h))) {                                                                        \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_N(cjson_obj)                                                                                       \
    cJSON * cjson_n = cJSON_GetObjectItem(cjson_obj, "n");                                                             \
    if(!(cjson_n && cJSON_IsNumber(cjson_n))) {                                                                        \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_FONT(cjson_obj)                                                                                    \
    cJSON * cjson_font = cJSON_GetObjectItem(cjson_obj, "font");                                                       \
    if(!(cjson_font && cJSON_IsString(cjson_font))) {                                                                  \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_TEXT(cjson_obj)                                                                                    \
    cJSON * cjson_text = cJSON_GetObjectItem(cjson_obj, "text");                                                       \
    if(!(cjson_text && cJSON_IsString(cjson_text))) {                                                                  \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_PATH(cjson_obj)                                                                                    \
    cJSON * cjson_path = cJSON_GetObjectItem(cjson_obj, "path");                                                       \
    if(!(cjson_path && cJSON_IsString(cjson_path))) {                                                                  \
        continue;                                                                                                      \
    }

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

static uint8_t pgs_parse_state_type(const char * name)
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

static uint8_t pgs_parse_anim(const char * anim, uint8_t default_anim)
{
    if(!STRNCMP_UTIL(anim, "fade")) {
        return PGS_KBD_ANIM_FADE;
    } else if(!STRNCMP_UTIL(anim, "left_right")) {
        return PGS_KBD_ANIM_LEFT_RIGHT;
    } else if(!STRNCMP_UTIL(anim, "right_left")) {
        return PGS_KBD_ANIM_RIGHT_LEFT;
    } else if(!STRNCMP_UTIL(anim, "up_down")) {
        return PGS_KBD_ANIM_UP_DOWN;
    } else if(!STRNCMP_UTIL(anim, "down_up")) {
        return PGS_KBD_ANIM_DOWN_UP;
    } else {
        return default_anim;
    }
}

static uint32_t pgs_parse_color(const char * hexcolor, uint32_t default_color)
{
    char * endptr;
    uint32_t color = strtoul(hexcolor, &endptr, 16);
    if(*endptr != '\0') {
        return default_color & 0xFFFFFF;
    }
    return color & 0xFFFFFF;
}

static uint8_t pgs_parse_opa(const char * hexcolor, uint8_t default_opa)
{
    char * endptr;
    uint32_t color = strtoul(hexcolor, &endptr, 16);
    if(*endptr != '\0') {
        return default_opa;
    }
    return (color >> 24) & 0xFF;
}

static int32_t pgs_parse_align(const char * align)
{
    if(!STRNCMP_UTIL(align, "default")) {
        return LV_ALIGN_DEFAULT;
    } else if(!STRNCMP_UTIL(align, "top-left")) {
        return LV_ALIGN_TOP_LEFT;
    } else if(!STRNCMP_UTIL(align, "top-mid")) {
        return LV_ALIGN_TOP_MID;
    } else if(!STRNCMP_UTIL(align, "top-right")) {
        return LV_ALIGN_TOP_RIGHT;
    } else if(!STRNCMP_UTIL(align, "bottom-left")) {
        return LV_ALIGN_BOTTOM_LEFT;
    } else if(!STRNCMP_UTIL(align, "bottom-mid")) {
        return LV_ALIGN_BOTTOM_MID;
    } else if(!STRNCMP_UTIL(align, "bottom-right")) {
        return LV_ALIGN_BOTTOM_RIGHT;
    } else if(!STRNCMP_UTIL(align, "left-mid")) {
        return LV_ALIGN_LEFT_MID;
    } else if(!STRNCMP_UTIL(align, "right-mid")) {
        return LV_ALIGN_RIGHT_MID;
    } else if(!STRNCMP_UTIL(align, "center")) {
        return LV_ALIGN_CENTER;
    } else if(!STRNCMP_UTIL(align, "out-top-left")) {
        return LV_ALIGN_OUT_TOP_LEFT;
    } else if(!STRNCMP_UTIL(align, "out-top-mid")) {
        return LV_ALIGN_OUT_TOP_MID;
    } else if(!STRNCMP_UTIL(align, "out-top-right")) {
        return LV_ALIGN_OUT_TOP_RIGHT;
    } else if(!STRNCMP_UTIL(align, "out-bottom-left")) {
        return LV_ALIGN_OUT_BOTTOM_LEFT;
    } else if(!STRNCMP_UTIL(align, "out-bottom-mid")) {
        return LV_ALIGN_OUT_BOTTOM_MID;
    } else if(!STRNCMP_UTIL(align, "out-bottom-right")) {
        return LV_ALIGN_OUT_BOTTOM_RIGHT;
    } else if(!STRNCMP_UTIL(align, "out-left-top")) {
        return LV_ALIGN_OUT_LEFT_TOP;
    } else if(!STRNCMP_UTIL(align, "out-left-mid")) {
        return LV_ALIGN_OUT_LEFT_MID;
    } else if(!STRNCMP_UTIL(align, "out-left-bottom")) {
        return LV_ALIGN_OUT_LEFT_BOTTOM;
    } else if(!STRNCMP_UTIL(align, "out-right-top")) {
        return LV_ALIGN_OUT_RIGHT_TOP;
    } else if(!STRNCMP_UTIL(align, "out-right-mid")) {
        return LV_ALIGN_OUT_RIGHT_MID;
    } else if(!STRNCMP_UTIL(align, "out-right-bottom")) {
        return LV_ALIGN_OUT_RIGHT_BOTTOM;
    } else {
        return LV_ALIGN_DEFAULT;
    }
}

static lv_font_t * pgs_parse_font(const char * font, lv_font_t * default_font)
{
#if LV_FONT_HELVETICAROUNDED_8
    if(!STRNCMP_UTIL(font, "helveticarounded_8")) {
        return &lv_font_helveticarounded_8;
    }
#endif
#if LV_FONT_HELVETICAROUNDED_10
    else if(!STRNCMP_UTIL(font, "helveticarounded_10")) {
        return &lv_font_helveticarounded_10;
    }
#endif
#if LV_FONT_HELVETICAROUNDED_12
    else if(!STRNCMP_UTIL(font, "helveticarounded_12")) {
        return &lv_font_helveticarounded_12;
    }
#endif
#if LV_FONT_HELVETICAROUNDED_14
    else if(!STRNCMP_UTIL(font, "helveticarounded_14")) {
        return &lv_font_helveticarounded_14;
    }
#endif
#if LV_FONT_HELVETICAROUNDED_16
    else if(!STRNCMP_UTIL(font, "helveticarounded_16")) {
        return &lv_font_helveticarounded_16;
    }
#endif
#if LV_FONT_HELVETICAROUNDED_18
    else if(!STRNCMP_UTIL(font, "helveticarounded_18")) {
        return &lv_font_helveticarounded_18;
    }
#endif
#if LV_FONT_HELVETICAROUNDED_20
    else if(!STRNCMP_UTIL(font, "helveticarounded_20")) {
        return &lv_font_helveticarounded_20;
    }
#endif
#if LV_FONT_HELVETICAROUNDED_22
    else if(!STRNCMP_UTIL(font, "helveticarounded_22")) {
        return &lv_font_helveticarounded_22;
    }
#endif
#if LV_FONT_HELVETICAROUNDED_24
    else if(!STRNCMP_UTIL(font, "helveticarounded_24")) {
        return &lv_font_helveticarounded_24;
    }
#endif
#if LV_FONT_HELVETICAROUNDED_26
    else if(!STRNCMP_UTIL(font, "helveticarounded_26")) {
        return &lv_font_helveticarounded_26;
    }
#endif
#if LV_FONT_HELVETICAROUNDED_28
    else if(!STRNCMP_UTIL(font, "helveticarounded_28")) {
        return &lv_font_helveticarounded_28;
    }
#endif
#if LV_FONT_HELVETICAROUNDED_30
    else if(!STRNCMP_UTIL(font, "helveticarounded_30")) {
        return &lv_font_helveticarounded_30;
    }
#endif
#if LV_FONT_HELVETICAROUNDED_32
    else if(!STRNCMP_UTIL(font, "helveticarounded_32")) {
        return &lv_font_helveticarounded_32;
    }
#endif
#if LV_FONT_MONTSERRAT_8
    else if(!STRNCMP_UTIL(font, "montserrat_8")) {
        return &lv_font_montserrat_8;
    }
#endif
#if LV_FONT_MONTSERRAT_10
    else if(!STRNCMP_UTIL(font, "montserrat_10")) {
        return &lv_font_montserrat_10;
    }
#endif
#if LV_FONT_MONTSERRAT_12
    else if(!STRNCMP_UTIL(font, "montserrat_12")) {
        return &lv_font_montserrat_12;
    }
#endif
#if LV_FONT_MONTSERRAT_14
    else if(!STRNCMP_UTIL(font, "montserrat_14")) {
        return &lv_font_montserrat_14;
    }
#endif
#if LV_FONT_MONTSERRAT_16
    else if(!STRNCMP_UTIL(font, "montserrat_16")) {
        return &lv_font_montserrat_16;
    }
#endif
#if LV_FONT_MONTSERRAT_18
    else if(!STRNCMP_UTIL(font, "montserrat_18")) {
        return &lv_font_montserrat_18;
    }
#endif
#if LV_FONT_MONTSERRAT_20
    else if(!STRNCMP_UTIL(font, "montserrat_20")) {
        return &lv_font_montserrat_20;
    }
#endif
#if LV_FONT_MONTSERRAT_22
    else if(!STRNCMP_UTIL(font, "montserrat_22")) {
        return &lv_font_montserrat_22;
    }
#endif
#if LV_FONT_MONTSERRAT_24
    else if(!STRNCMP_UTIL(font, "montserrat_24")) {
        return &lv_font_montserrat_24;
    }
#endif
#if LV_FONT_MONTSERRAT_26
    else if(!STRNCMP_UTIL(font, "montserrat_26")) {
        return &lv_font_montserrat_26;
    }
#endif
#if LV_FONT_MONTSERRAT_28
    else if(!STRNCMP_UTIL(font, "montserrat_28")) {
        return &lv_font_montserrat_28;
    }
#endif
#if LV_FONT_MONTSERRAT_30
    else if(!STRNCMP_UTIL(font, "montserrat_30")) {
        return &lv_font_montserrat_30;
    }
#endif
#if LV_FONT_MONTSERRAT_32
    else if(!STRNCMP_UTIL(font, "montserrat_32")) {
        return &lv_font_montserrat_32;
    }
#endif
#if LV_FONT_MONTSERRAT_34
    else if(!STRNCMP_UTIL(font, "montserrat_34")) {
        return &lv_font_montserrat_34;
    }
#endif
#if LV_FONT_MONTSERRAT_36
    else if(!STRNCMP_UTIL(font, "montserrat_36")) {
        return &lv_font_montserrat_36;
    }
#endif
#if LV_FONT_MONTSERRAT_38
    else if(!STRNCMP_UTIL(font, "montserrat_38")) {
        return &lv_font_montserrat_38;
    }
#endif
#if LV_FONT_MONTSERRAT_40
    else if(!STRNCMP_UTIL(font, "montserrat_40")) {
        return &lv_font_montserrat_40;
    }
#endif
#if LV_FONT_MONTSERRAT_42
    else if(!STRNCMP_UTIL(font, "montserrat_42")) {
        return &lv_font_montserrat_42;
    }
#endif
#if LV_FONT_MONTSERRAT_44
    else if(!STRNCMP_UTIL(font, "montserrat_44")) {
        return &lv_font_montserrat_44;
    }
#endif
#if LV_FONT_MONTSERRAT_46
    else if(!STRNCMP_UTIL(font, "montserrat_46")) {
        return &lv_font_montserrat_46;
    }
#endif
#if LV_FONT_MONTSERRAT_48
    else if(!STRNCMP_UTIL(font, "montserrat_48")) {
        return &lv_font_montserrat_48;
    }
#endif
#if LV_FONT_MONTSERRAT_28_COMPRESSED
    else if(!STRNCMP_UTIL(font, "montserrat_28_compressed")) {
        return &lv_font_montserrat_28_compressed;
    }
#endif
#if LV_FONT_DEJAVU_16_PERSIAN_HEBREW
    if(!STRNCMP_UTIL(font, "dejavu_16_persian_hebrew")) {
        return &lv_font_dejavu_16_persian_hebrew;
    }
#endif
#if LV_FONT_SIMSUN_14_CJK
    else if(!STRNCMP_UTIL(font, "simsun_14_cjk")) {
        return &lv_font_simsun_14_cjk;
    }
#endif
#if LV_FONT_SIMSUN_16_CJK
    else if(!STRNCMP_UTIL(font, "simsun_16_cjk")) {
        return &lv_font_simsun_16_cjk;
    }
#endif
#if LV_FONT_UNSCII_8
    else if(!STRNCMP_UTIL(font, "unscii_8")) {
        return &lv_font_unscii_8;
    }
#endif
#if LV_FONT_UNSCII_16
    else if(!STRNCMP_UTIL(font, "unscii_16")) {
        return &lv_font_unscii_16;
    }
#endif
    else {
        return default_font;
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

    /* base */
    cJSON * cjson_base = NULL;
    cjson_base         = cJSON_GetObjectItem(cjson_root, "base");
    if(cjson_base && cJSON_IsString(cjson_base)) {
        params->base = cjson_base->valuestring;
    } else {
        params->base = "/usr/share/pgs/themes/default";
    }

    /* states */
    cJSON * cjson_states = NULL;
    cjson_states         = cJSON_GetObjectItem(cjson_root, "states");
    if(cjson_states && cJSON_IsArray(cjson_states)) {
        int count            = cJSON_GetArraySize(cjson_states);
        params->states_count = 0;
        params->states       = lv_malloc(sizeof(struct pgs_kbd_state) * count);
        if(!params->states) {
            goto err3;
        }

        CJSON_PARSE_ARRAY_BEG(0, count)
        {
            cJSON * cjson_state = cJSON_GetArrayItem(cjson_states, i);
            if(cjson_state) {
                CJSON_PARSE_NAME(cjson_state);
                CJSON_PARSE_ENABLE(cjson_state);
                CJSON_PARSE_ALIGN(cjson_state);
                CJSON_PARSE_ANIM(cjson_state);
                CJSON_PARSE_COLOR(cjson_state);
                CJSON_PARSE_X(cjson_state);
                CJSON_PARSE_Y(cjson_state);

                params->states[params->states_count].type   = pgs_parse_state_type(cjson_name->valuestring);
                params->states[params->states_count].enable = cjson_enable->valueint;
                params->states[params->states_count].align  = pgs_parse_align(cjson_align->valuestring);
                params->states[params->states_count].anim = pgs_parse_anim(cjson_anim->valuestring, PGS_KBD_ANIM_FADE);
                params->states[params->states_count].opa  = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
                params->states[params->states_count].x    = cjson_x->valueint;
                params->states[params->states_count].y    = cjson_y->valueint;
                params->states_count++;
            }
        }
        CJSON_PARSE_ARRAY_END;
    }

    /* keyroll */
    cJSON * cjson_keyroll = NULL;
    cjson_keyroll         = cJSON_GetObjectItem(cjson_root, "keyroll");
    if(cjson_keyroll && cJSON_IsObject(cjson_keyroll)) {
        CJSON_PARSE_BEG
        {
            CJSON_PARSE_ENABLE(cjson_keyroll);
            CJSON_PARSE_ALIGN(cjson_keyroll);
            CJSON_PARSE_ANIM(cjson_keyroll);
            CJSON_PARSE_COLOR(cjson_keyroll);
            CJSON_PARSE_COLORATION(cjson_keyroll);
            CJSON_PARSE_X(cjson_keyroll);
            CJSON_PARSE_Y(cjson_keyroll);
            CJSON_PARSE_N(cjson_keyroll);

            params->keyroll = lv_malloc(sizeof(struct pgs_kbd_keyroll));
            if(!params->keyroll) {
                goto err4;
            }

            params->keyroll->enable     = cjson_enable->valueint;
            params->keyroll->align      = pgs_parse_align(cjson_align->valuestring);
            params->keyroll->anim       = pgs_parse_anim(cjson_anim->valuestring, PGS_KBD_ANIM_FADE);
            params->keyroll->opa        = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
            params->keyroll->coloration = cjson_coloration->valuestring;
            params->keyroll->x          = cjson_x->valueint;
            params->keyroll->y          = cjson_y->valueint;
            params->keyroll->n          = cjson_n->valueint;
        }
        CSJON_PARSE_END;
    }

    /* apmchart */
    cJSON * cjson_apmchart = NULL;
    cjson_apmchart         = cJSON_GetObjectItem(cjson_root, "apmchart");
    if(cjson_apmchart && cJSON_IsObject(cjson_apmchart)) {
        CJSON_PARSE_BEG
        {
            CJSON_PARSE_ENABLE(cjson_apmchart);
            CJSON_PARSE_ALIGN(cjson_apmchart);
            CJSON_PARSE_ANIM(cjson_apmchart);
            CJSON_PARSE_COLOR(cjson_apmchart);
            CJSON_PARSE_X(cjson_apmchart);
            CJSON_PARSE_Y(cjson_apmchart);
            CJSON_PARSE_W(cjson_apmchart);
            CJSON_PARSE_H(cjson_apmchart);

            params->apmchart = lv_malloc(sizeof(struct pgs_kbd_apmchart));
            if(!params->apmchart) {
                goto err5;
            }

            params->apmchart->enable = cjson_enable->valueint;
            params->apmchart->align  = pgs_parse_align(cjson_align->valuestring);
            params->apmchart->anim   = pgs_parse_anim(cjson_anim->valuestring, PGS_KBD_ANIM_FADE);
            params->apmchart->color  = pgs_parse_color(cjson_color->valuestring, 0xFF6086);
            params->apmchart->opa    = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
            params->apmchart->x      = cjson_x->valueint;
            params->apmchart->y      = cjson_y->valueint;
            params->apmchart->w      = cjson_w->valueint;
            params->apmchart->h      = cjson_h->valueint;
        }
        CSJON_PARSE_END;
    }

    /* apmlabel */
    cJSON * cjson_apmlabel = NULL;
    cjson_apmlabel         = cJSON_GetObjectItem(cjson_root, "apmlabel");
    if(cjson_apmlabel && cJSON_IsObject(cjson_apmlabel)) {
        CJSON_PARSE_BEG
        {
            CJSON_PARSE_ENABLE(cjson_apmlabel);
            CJSON_PARSE_ALIGN(cjson_apmlabel);
            CJSON_PARSE_TEXT_ALIGN(cjson_apmlabel);
            CJSON_PARSE_ANIM(cjson_apmlabel);
            CJSON_PARSE_COLOR(cjson_apmlabel);
            CJSON_PARSE_FONT(cjson_apmlabel);
            CJSON_PARSE_X(cjson_apmlabel);
            CJSON_PARSE_Y(cjson_apmlabel);
            CJSON_PARSE_W(cjson_apmlabel);
            CJSON_PARSE_H(cjson_apmlabel);

            params->apmlabel = lv_malloc(sizeof(struct pgs_kbd_label));
            if(!params->apmlabel) {
                goto err6;
            }

            params->apmlabel->enable     = cjson_enable->valueint;
            params->apmlabel->align      = pgs_parse_align(cjson_align->valuestring);
            params->apmlabel->text_align = pgs_parse_align(cjson_text_align->valuestring);
            params->apmlabel->anim       = pgs_parse_anim(cjson_anim->valuestring, PGS_KBD_ANIM_FADE);
            params->apmlabel->color      = pgs_parse_color(cjson_color->valuestring, 0xFF6086);
            params->apmlabel->opa        = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
            params->apmlabel->font       = pgs_parse_font(cjson_font->valuestring, LV_FONT_DEFAULT);
            params->apmlabel->x          = cjson_x->valueint;
            params->apmlabel->y          = cjson_y->valueint;
            params->apmlabel->w          = cjson_w->valueint;
            params->apmlabel->h          = cjson_h->valueint;
        }
        CSJON_PARSE_END;
    }

    cJSON * cjson_labels = NULL;
    cjson_labels         = cJSON_GetObjectItem(cjson_root, "labels");
    if(cjson_labels && cJSON_IsArray(cjson_labels)) {
        int count            = cJSON_GetArraySize(cjson_labels);
        params->labels_count = 0;
        params->labels       = lv_malloc(sizeof(struct pgs_kbd_label) * count);
        if(!params->labels) {
            goto err7;
        }

        CJSON_PARSE_ARRAY_BEG(0, count)
        {
            cJSON * cjson_label = cJSON_GetArrayItem(cjson_labels, i);
            if(cjson_label) {
                CJSON_PARSE_ENABLE(cjson_label);
                CJSON_PARSE_TEXT(cjson_label);
                CJSON_PARSE_ALIGN(cjson_label);
                CJSON_PARSE_TEXT_ALIGN(cjson_label);
                CJSON_PARSE_ANIM(cjson_label);
                CJSON_PARSE_COLOR(cjson_label);
                CJSON_PARSE_FONT(cjson_label);
                CJSON_PARSE_X(cjson_label);
                CJSON_PARSE_Y(cjson_label);
                CJSON_PARSE_W(cjson_label);
                CJSON_PARSE_H(cjson_label);

                params->labels[params->labels_count].text       = cjson_text->valuestring;
                params->labels[params->labels_count].enable     = cjson_enable->valueint;
                params->labels[params->labels_count].align      = pgs_parse_align(cjson_align->valuestring);
                params->labels[params->labels_count].text_align = pgs_parse_align(cjson_text_align->valuestring);
                params->labels[params->labels_count].anim  = pgs_parse_anim(cjson_anim->valuestring, PGS_KBD_ANIM_FADE);
                params->labels[params->labels_count].color = pgs_parse_color(cjson_color->valuestring, 0xF0F0F0);
                params->labels[params->labels_count].opa   = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
                params->labels[params->labels_count].font  = pgs_parse_font(cjson_font->valuestring, LV_FONT_DEFAULT);
                params->labels[params->labels_count].x     = cjson_x->valueint;
                params->labels[params->labels_count].y     = cjson_y->valueint;
                params->labels[params->labels_count].w     = cjson_w->valueint;
                params->labels[params->labels_count].h     = cjson_h->valueint;
                params->labels_count++;
            }
        }
        CJSON_PARSE_ARRAY_END;
    }

    cJSON * cjson_images = NULL;
    cjson_images         = cJSON_GetObjectItem(cjson_root, "images");
    if(cjson_images && cJSON_IsArray(cjson_images)) {
        int count            = cJSON_GetArraySize(cjson_images);
        params->images_count = 0;
        params->images       = lv_malloc(sizeof(struct pgs_kbd_image) * count);
        if(!params->images) {
            goto err8;
        }

        CJSON_PARSE_ARRAY_BEG(0, count)
        {
            cJSON * cjson_image = cJSON_GetArrayItem(cjson_images, i);
            if(cjson_image) {
                CJSON_PARSE_ENABLE(cjson_image);
                CJSON_PARSE_PATH(cjson_image);
                CJSON_PARSE_ALIGN(cjson_image);
                CJSON_PARSE_ANIM(cjson_image);
                CJSON_PARSE_COLOR(cjson_image);
                CJSON_PARSE_X(cjson_image);
                CJSON_PARSE_Y(cjson_image);

                params->images[params->images_count].enable = cjson_enable->valueint;
                params->images[params->images_count].align  = pgs_parse_align(cjson_align->valuestring);
                params->images[params->images_count].anim = pgs_parse_anim(cjson_anim->valuestring, PGS_KBD_ANIM_FADE);
                params->images[params->images_count].opa  = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
                params->images[params->images_count].x    = cjson_x->valueint;
                params->images[params->images_count].y    = cjson_y->valueint;
                params->images_count++;
            }
        }
        CJSON_PARSE_ARRAY_END;
    }

    lv_free(message);

    return params;

err8:
    if(params->labels) lv_free(params->labels);

err7:
    if(params->apmlabel) lv_free(params->apmlabel);

err6:
    if(params->apmchart) lv_free(params->apmchart);

err5:
    if(params->keyroll) lv_free(params->keyroll);

err4:
    if(params->states) lv_free(params->states);

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

struct pgs_kbd_state * pgs_kbd_params_state_get(struct pgs_kbd_params * params, uint8_t type)
{
    struct pgs_kbd_state * state = NULL;

    for(uint8_t i = 0; i < params->states_count; i++) {
        if(params->states[i].type == type) {
            state = &params->states[i];
            break;
        }
    }

    return state;
}
