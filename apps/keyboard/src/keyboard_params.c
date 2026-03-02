#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include "pgs_utils.h"
#include "keyboard_params.h"
#include "cJSON.h"

#define STRNCMP_UTIL(str1, str2) strncmp((str1), (str2), sizeof(str2))

#define CJSON_PARSE_BEG do
#define CJSON_PARSE_END while(0)

#define CJSON_PARSE_ARRAY_BEG(start, item, end) for(uint32_t(item) = (start); (item) < (end); (item)++)
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

#define CJSON_PARSE_RADIUS(cjson_obj)                                                                                  \
    cJSON * cjson_radius = cJSON_GetObjectItem(cjson_obj, "radius");                                                   \
    if(!(cjson_radius && cJSON_IsNumber(cjson_radius))) {                                                              \
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

#define CJSON_PARSE_SUFFIX(cjson_obj)                                                                                  \
    cJSON * cjson_suffix = cJSON_GetObjectItem(cjson_obj, "suffix");                                                   \
    if(!(cjson_suffix && cJSON_IsString(cjson_suffix))) {                                                              \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_GROUP(cjson_obj)                                                                                   \
    cJSON * cjson_group = cJSON_GetObjectItem(cjson_obj, "group");                                                     \
    if(!(cjson_group && cJSON_IsNumber(cjson_group))) {                                                                \
        continue;                                                                                                      \
    }

#define CJSON_PARSE_ZINDEX(cjson_obj) cJSON * cjson_zindex = cJSON_GetObjectItem(cjson_obj, "zindex");
#define CJSON_CHECK_ZINDEX(cjson_obj) (cjson_obj && cJSON_IsNumber(cjson_obj))

#define CJSON_PARSE_SINGLE(cjson_obj) cJSON * cjson_single = cJSON_GetObjectItem(cjson_obj, "single");
#define CJSON_CHECK_SINGLE(cjson_obj) (cjson_obj && cJSON_IsBool(cjson_obj))

#define CJSON_PARSE_KEEP(cjson_obj) cJSON * cjson_keep = cJSON_GetObjectItem(cjson_obj, "keep");
#define CJSON_CHECK_KEEP(cjson_obj) (cjson_obj && cJSON_IsBool(cjson_obj))

#define CJSON_PARSE_DURATION(cjson_obj) cJSON * cjson_duration = cJSON_GetObjectItem(cjson_obj, "duration");
#define CJSON_CHECK_DURATION(cjson_obj) (cjson_obj && cJSON_IsNumber(cjson_obj))

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
        return PGS_WIDGETS_TYPE_MACRO;
    } else if(!STRNCMP_UTIL(name, "macro1")) {
        return PGS_WIDGETS_TYPE_MACRO1;
    } else if(!STRNCMP_UTIL(name, "macro2")) {
        return PGS_WIDGETS_TYPE_MACRO2;
    } else if(!STRNCMP_UTIL(name, "layer")) {
        return PGS_WIDGETS_TYPE_LAYER;
    } else if(!STRNCMP_UTIL(name, "capslock")) {
        return PGS_WIDGETS_TYPE_CAPSLOCK;
    } else if(!STRNCMP_UTIL(name, "numlock")) {
        return PGS_WIDGETS_TYPE_NUMLOCK;
    } else if(!STRNCMP_UTIL(name, "ble1")) {
        return PGS_WIDGETS_TYPE_BLE1;
    } else if(!STRNCMP_UTIL(name, "ble2")) {
        return PGS_WIDGETS_TYPE_BLE2;
    } else if(!STRNCMP_UTIL(name, "ble3")) {
        return PGS_WIDGETS_TYPE_BLE3;
    } else if(!STRNCMP_UTIL(name, "2g4")) {
        return PGS_WIDGETS_TYPE_2G4;
    } else if(!STRNCMP_UTIL(name, "usb")) {
        return PGS_WIDGETS_TYPE_USB;
    } else if(!STRNCMP_UTIL(name, "scr")) {
        return PGS_WIDGETS_TYPE_SCR;
    } else if(!STRNCMP_UTIL(name, "bat")) {
        return PGS_WIDGETS_TYPE_BAT;
    } else {
        return PGS_WIDGETS_TYPE_UNKNOWN;
    }
}

static uint8_t pgs_parse_anim(const char * anim, uint8_t default_anim)
{
    if(!STRNCMP_UTIL(anim, "fade")) {
        return PGS_WIDGETS_ANIM_FADE;
    } else if(!STRNCMP_UTIL(anim, "left_right")) {
        return PGS_WIDGETS_ANIM_LEFT_RIGHT;
    } else if(!STRNCMP_UTIL(anim, "right_left")) {
        return PGS_WIDGETS_ANIM_RIGHT_LEFT;
    } else if(!STRNCMP_UTIL(anim, "up_down")) {
        return PGS_WIDGETS_ANIM_UP_DOWN;
    } else if(!STRNCMP_UTIL(anim, "down_up")) {
        return PGS_WIDGETS_ANIM_DOWN_UP;
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

static int32_t pgs_parse_text_align(const char * align)
{
    if(!STRNCMP_UTIL(align, "auto")) {
        return LV_TEXT_ALIGN_AUTO;
    } else if(!STRNCMP_UTIL(align, "left")) {
        return LV_TEXT_ALIGN_LEFT;
    } else if(!STRNCMP_UTIL(align, "center")) {
        return LV_TEXT_ALIGN_CENTER;
    } else if(!STRNCMP_UTIL(align, "right")) {
        return LV_TEXT_ALIGN_RIGHT;
    } else {
        return LV_TEXT_ALIGN_AUTO;
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

struct keyboard_params * keyboard_params_parse(const char * json_path)
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
        goto err_message;
    }

    struct keyboard_params * params = lv_malloc(sizeof(struct keyboard_params));
    if(!cjson_root) {
        LV_LOG_WARN("malloc failed\n");
        goto err_parse;
    }
    lv_memzero(params, sizeof(struct keyboard_params));
    params->cjson = cjson_root;

    /* base */
    params->base_dynamic = 0;
    cJSON * cjson_base   = NULL;
    cjson_base           = cJSON_GetObjectItem(cjson_root, "base");
    if(cjson_base && cJSON_IsString(cjson_base)) {
        params->base = cjson_base->valuestring;
    } else {
        const char * lastSlash = strrchr(json_path, '/');
        if(lastSlash != NULL) {
            size_t dirPathLength = lastSlash - json_path;
            char * dirPath       = strdup(json_path);
            if(!dirPath) {
                params->base = "/usr/share/pgs/themes/default";
            } else {
                dirPath[dirPathLength] = '\0';
                params->base           = dirPath;
                params->base_dynamic   = 1;
            }
        } else {
            params->base = "/usr/share/pgs/themes/default";
        }
    }

    /* states */
    cJSON * cjson_states = NULL;
    cjson_states         = cJSON_GetObjectItem(cjson_root, "states");
    if(cjson_states && cJSON_IsArray(cjson_states)) {
        int count            = cJSON_GetArraySize(cjson_states);
        params->states_count = 0;
        params->states       = lv_malloc(sizeof(struct pgs_widgets_params_state) * count);
        if(!params->states) {
            goto err_malloc;
        }

        CJSON_PARSE_ARRAY_BEG(0, i, count)
        {
            cJSON * cjson_state = cJSON_GetArrayItem(cjson_states, i);
            if(cjson_state) {
                CJSON_PARSE_NAME(cjson_state);
                CJSON_PARSE_ENABLE(cjson_state);
                CJSON_PARSE_ALIGN(cjson_state);
                CJSON_PARSE_ANIM(cjson_state);
                CJSON_PARSE_COLOR(cjson_state);
                CJSON_PARSE_ZINDEX(cjson_state);
                CJSON_PARSE_X(cjson_state);
                CJSON_PARSE_Y(cjson_state);

                params->states[params->states_count].zindex =
                    CJSON_CHECK_ZINDEX(cjson_zindex) ? cjson_zindex->valueint : PGS_WIDGETS_ZINDEX_DEFAULT;

                params->states[params->states_count].type   = pgs_parse_state_type(cjson_name->valuestring);
                params->states[params->states_count].enable = cjson_enable->valueint;
                params->states[params->states_count].align  = pgs_parse_align(cjson_align->valuestring);
                params->states[params->states_count].anim =
                    pgs_parse_anim(cjson_anim->valuestring, PGS_WIDGETS_ANIM_FADE);
                params->states[params->states_count].opa = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
                params->states[params->states_count].x   = cjson_x->valueint;
                params->states[params->states_count].y   = cjson_y->valueint;
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
            CJSON_PARSE_ZINDEX(cjson_keyroll);
            CJSON_PARSE_COLORATION(cjson_keyroll);
            CJSON_PARSE_X(cjson_keyroll);
            CJSON_PARSE_Y(cjson_keyroll);
            CJSON_PARSE_N(cjson_keyroll);

            params->keyroll = lv_malloc(sizeof(struct pgs_widgets_params_keyroll));
            if(!params->keyroll) {
                goto err_p_states;
            }

            params->keyroll->zindex =
                CJSON_CHECK_ZINDEX(cjson_zindex) ? cjson_zindex->valueint : PGS_WIDGETS_ZINDEX_DEFAULT;

            params->keyroll->enable     = cjson_enable->valueint;
            params->keyroll->align      = pgs_parse_align(cjson_align->valuestring);
            params->keyroll->anim       = pgs_parse_anim(cjson_anim->valuestring, PGS_WIDGETS_ANIM_FADE);
            params->keyroll->opa        = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
            params->keyroll->coloration = cjson_coloration->valuestring;
            params->keyroll->x          = cjson_x->valueint;
            params->keyroll->y          = cjson_y->valueint;
            params->keyroll->n          = cjson_n->valueint;
        }
        CJSON_PARSE_END;
    }

    /* keyanim */
    cJSON * cjson_keyanim = NULL;
    cjson_keyanim         = cJSON_GetObjectItem(cjson_root, "keyanim");
    if(cjson_keyanim && cJSON_IsObject(cjson_keyanim)) {
        CJSON_PARSE_BEG
        {
            CJSON_PARSE_ENABLE(cjson_keyanim);
            CJSON_PARSE_ALIGN(cjson_keyanim);
            CJSON_PARSE_ANIM(cjson_keyanim);
            CJSON_PARSE_COLOR(cjson_keyanim);
            CJSON_PARSE_ZINDEX(cjson_keyanim);
            CJSON_PARSE_X(cjson_keyanim);
            CJSON_PARSE_Y(cjson_keyanim);
            CJSON_PARSE_RADIUS(cjson_keyanim);
            CJSON_PARSE_PATH(cjson_keyanim);
            CJSON_PARSE_SUFFIX(cjson_keyanim);
            CJSON_PARSE_SINGLE(cjson_keyanim);
            CJSON_PARSE_KEEP(cjson_keyanim);
            CJSON_PARSE_DURATION(cjson_keyanim);

            params->keyanim = lv_malloc(sizeof(struct pgs_widgets_params_keyanim));
            if(!params->keyanim) {
                goto err_p_keyroll;
            }
            lv_memzero(params->keyanim, sizeof(struct pgs_widgets_params_keyanim));

            params->keyanim->zindex =
                CJSON_CHECK_ZINDEX(cjson_zindex) ? cjson_zindex->valueint : PGS_WIDGETS_ZINDEX_DEFAULT;

            params->keyanim->single   = CJSON_CHECK_SINGLE(cjson_single) ? cjson_single->valueint : 0;
            params->keyanim->keep     = CJSON_CHECK_KEEP(cjson_keep) ? cjson_keep->valueint : 0;
            params->keyanim->duration = CJSON_CHECK_DURATION(cjson_duration) ? cjson_duration->valueint : 0;

            params->keyanim->radius = cjson_radius->valueint;
            params->keyanim->path   = cjson_path->valuestring;
            params->keyanim->suffix = cjson_suffix->valuestring;
            params->keyanim->enable = cjson_enable->valueint;
            params->keyanim->align  = pgs_parse_align(cjson_align->valuestring);
            params->keyanim->anim   = pgs_parse_anim(cjson_anim->valuestring, PGS_WIDGETS_ANIM_FADE);
            params->keyanim->opa    = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
            params->keyanim->x      = cjson_x->valueint;
            params->keyanim->y      = cjson_y->valueint;
        }
        CJSON_PARSE_END;

        /* keyanim-waitanims */
        cJSON * cjson_waitanims = NULL;
        cjson_waitanims         = cJSON_GetObjectItem(cjson_keyanim, "waitanims");
        if(cjson_waitanims && cJSON_IsArray(cjson_waitanims)) {
            int count                  = cJSON_GetArraySize(cjson_waitanims);
            params->keyanim->waitcount = 0;
            params->keyanim->waitanims = lv_malloc(sizeof(struct pgs_widgets_params_waitanim) * count);
            if(!params->keyanim->waitanims) {
                goto err_p_keyanim;
            }
            lv_memzero(params->keyanim->waitanims, sizeof(struct pgs_widgets_params_waitanim) * count);

            CJSON_PARSE_ARRAY_BEG(0, i, count)
            {
                cJSON * cjson_waitanim = cJSON_GetArrayItem(cjson_waitanims, i);
                if(cjson_waitanim) {
                    CJSON_PARSE_ENABLE(cjson_waitanim);
                    CJSON_PARSE_X(cjson_waitanim);
                    CJSON_PARSE_Y(cjson_waitanim);
                    CJSON_PARSE_PATH(cjson_waitanim);
                    CJSON_PARSE_RADIUS(cjson_waitanim);
                    CJSON_PARSE_GROUP(cjson_waitanim);

                    params->keyanim->waitanims[params->keyanim->waitcount].enable = cjson_enable->valueint;
                    params->keyanim->waitanims[params->keyanim->waitcount].x      = cjson_x->valueint;
                    params->keyanim->waitanims[params->keyanim->waitcount].y      = cjson_y->valueint;
                    params->keyanim->waitanims[params->keyanim->waitcount].path   = cjson_path->valuestring;
                    params->keyanim->waitanims[params->keyanim->waitcount].radius = cjson_radius->valueint;
                    params->keyanim->waitanims[params->keyanim->waitcount].group  = cjson_group->valueint;
                    params->keyanim->waitcount++;
                }
            }
            CJSON_PARSE_ARRAY_END;
        }
    }

    /* keysnd */
    cJSON * cjson_keysnd = NULL;
    cjson_keysnd         = cJSON_GetObjectItem(cjson_root, "keysnd");
    if(cjson_keysnd && cJSON_IsObject(cjson_keysnd)) {
        CJSON_PARSE_BEG
        {
            CJSON_PARSE_ENABLE(cjson_keysnd);
            CJSON_PARSE_ALIGN(cjson_keysnd);
            CJSON_PARSE_ANIM(cjson_keysnd);
            CJSON_PARSE_COLOR(cjson_keysnd);
            CJSON_PARSE_ZINDEX(cjson_keysnd);
            CJSON_PARSE_X(cjson_keysnd);
            CJSON_PARSE_Y(cjson_keysnd);
            CJSON_PARSE_RADIUS(cjson_keysnd);
            CJSON_PARSE_PATH(cjson_keysnd);
            CJSON_PARSE_SUFFIX(cjson_keysnd);
            CJSON_PARSE_SINGLE(cjson_keysnd);
            CJSON_PARSE_KEEP(cjson_keysnd);
            CJSON_PARSE_DURATION(cjson_keysnd);

            params->keysnd = lv_malloc(sizeof(struct pgs_widgets_params_keysnd));
            if(!params->keysnd) {
                goto err_p_waitanim;
            }

            params->keysnd->zindex =
                CJSON_CHECK_ZINDEX(cjson_zindex) ? cjson_zindex->valueint : PGS_WIDGETS_ZINDEX_DEFAULT;

            params->keysnd->single   = CJSON_CHECK_SINGLE(cjson_single) ? cjson_single->valueint : 0;
            params->keysnd->keep     = CJSON_CHECK_KEEP(cjson_keep) ? cjson_keep->valueint : 0;
            params->keysnd->duration = CJSON_CHECK_DURATION(cjson_duration) ? cjson_duration->valueint : 0;

            params->keysnd->path   = cjson_path->valuestring;
            params->keysnd->suffix = cjson_suffix->valuestring;
            params->keysnd->enable = cjson_enable->valueint;
            params->keysnd->align  = pgs_parse_align(cjson_align->valuestring);
            params->keysnd->anim   = pgs_parse_anim(cjson_anim->valuestring, PGS_WIDGETS_ANIM_FADE);
            params->keysnd->opa    = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
            params->keysnd->x      = cjson_x->valueint;
            params->keysnd->y      = cjson_y->valueint;
            params->keysnd->radius = cjson_radius->valueint;
        }
        CJSON_PARSE_END;
    }

    /* wpmchart */
    cJSON * cjson_wpmchart = NULL;
    cjson_wpmchart         = cJSON_GetObjectItem(cjson_root, "wpmchart");
    if(cjson_wpmchart && cJSON_IsObject(cjson_wpmchart)) {
        CJSON_PARSE_BEG
        {
            CJSON_PARSE_ENABLE(cjson_wpmchart);
            CJSON_PARSE_ALIGN(cjson_wpmchart);
            CJSON_PARSE_ANIM(cjson_wpmchart);
            CJSON_PARSE_COLOR(cjson_wpmchart);
            CJSON_PARSE_ZINDEX(cjson_wpmchart);
            CJSON_PARSE_X(cjson_wpmchart);
            CJSON_PARSE_Y(cjson_wpmchart);
            CJSON_PARSE_W(cjson_wpmchart);
            CJSON_PARSE_H(cjson_wpmchart);

            params->wpmchart = lv_malloc(sizeof(struct pgs_widgets_params_wpmchart));
            if(!params->wpmchart) {
                goto err_p_keysnd;
            }

            params->wpmchart->zindex =
                CJSON_CHECK_ZINDEX(cjson_zindex) ? cjson_zindex->valueint : PGS_WIDGETS_ZINDEX_DEFAULT;

            params->wpmchart->enable = cjson_enable->valueint;
            params->wpmchart->align  = pgs_parse_align(cjson_align->valuestring);
            params->wpmchart->anim   = pgs_parse_anim(cjson_anim->valuestring, PGS_WIDGETS_ANIM_FADE);
            params->wpmchart->color  = pgs_parse_color(cjson_color->valuestring, 0xFF6086);
            params->wpmchart->opa    = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
            params->wpmchart->x      = cjson_x->valueint;
            params->wpmchart->y      = cjson_y->valueint;
            params->wpmchart->w      = cjson_w->valueint;
            params->wpmchart->h      = cjson_h->valueint;
        }
        CJSON_PARSE_END;
    }

    /* wpmlabel */
    cJSON * cjson_wpmlabel = NULL;
    cjson_wpmlabel         = cJSON_GetObjectItem(cjson_root, "wpmlabel");
    if(cjson_wpmlabel && cJSON_IsObject(cjson_wpmlabel)) {
        CJSON_PARSE_BEG
        {
            CJSON_PARSE_ENABLE(cjson_wpmlabel);
            CJSON_PARSE_ALIGN(cjson_wpmlabel);
            CJSON_PARSE_TEXT_ALIGN(cjson_wpmlabel);
            CJSON_PARSE_ANIM(cjson_wpmlabel);
            CJSON_PARSE_COLOR(cjson_wpmlabel);
            CJSON_PARSE_ZINDEX(cjson_wpmlabel);
            CJSON_PARSE_FONT(cjson_wpmlabel);
            CJSON_PARSE_X(cjson_wpmlabel);
            CJSON_PARSE_Y(cjson_wpmlabel);
            CJSON_PARSE_W(cjson_wpmlabel);
            CJSON_PARSE_H(cjson_wpmlabel);

            params->wpmlabel = lv_malloc(sizeof(struct pgs_widgets_params_label));
            if(!params->wpmlabel) {
                goto err_p_wpmchart;
            }

            params->wpmlabel->zindex =
                CJSON_CHECK_ZINDEX(cjson_zindex) ? cjson_zindex->valueint : PGS_WIDGETS_ZINDEX_DEFAULT;

            params->wpmlabel->enable     = cjson_enable->valueint;
            params->wpmlabel->align      = pgs_parse_align(cjson_align->valuestring);
            params->wpmlabel->text_align = pgs_parse_text_align(cjson_text_align->valuestring);
            params->wpmlabel->anim       = pgs_parse_anim(cjson_anim->valuestring, PGS_WIDGETS_ANIM_FADE);
            params->wpmlabel->color      = pgs_parse_color(cjson_color->valuestring, 0xFF6086);
            params->wpmlabel->opa        = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
            params->wpmlabel->font       = pgs_parse_font(cjson_font->valuestring, LV_FONT_DEFAULT);
            params->wpmlabel->x          = cjson_x->valueint;
            params->wpmlabel->y          = cjson_y->valueint;
            params->wpmlabel->w          = cjson_w->valueint;
            params->wpmlabel->h          = cjson_h->valueint;
        }
        CJSON_PARSE_END;
    }

    /* labels */
    cJSON * cjson_labels = NULL;
    cjson_labels         = cJSON_GetObjectItem(cjson_root, "labels");
    if(cjson_labels && cJSON_IsArray(cjson_labels)) {
        int count            = cJSON_GetArraySize(cjson_labels);
        params->labels_count = 0;
        params->labels       = lv_malloc(sizeof(struct pgs_widgets_params_label) * count);
        if(!params->labels) {
            goto err_p_wpmlabel;
        }

        CJSON_PARSE_ARRAY_BEG(0, i, count)
        {
            cJSON * cjson_label = cJSON_GetArrayItem(cjson_labels, i);
            if(cjson_label) {
                CJSON_PARSE_ENABLE(cjson_label);
                CJSON_PARSE_TEXT(cjson_label);
                CJSON_PARSE_ALIGN(cjson_label);
                CJSON_PARSE_TEXT_ALIGN(cjson_label);
                CJSON_PARSE_ANIM(cjson_label);
                CJSON_PARSE_COLOR(cjson_label);
                CJSON_PARSE_ZINDEX(cjson_label);
                CJSON_PARSE_FONT(cjson_label);
                CJSON_PARSE_X(cjson_label);
                CJSON_PARSE_Y(cjson_label);
                CJSON_PARSE_W(cjson_label);
                CJSON_PARSE_H(cjson_label);

                params->labels[params->labels_count].zindex =
                    CJSON_CHECK_ZINDEX(cjson_zindex) ? cjson_zindex->valueint : PGS_WIDGETS_ZINDEX_DEFAULT;

                params->labels[params->labels_count].text       = cjson_text->valuestring;
                params->labels[params->labels_count].enable     = cjson_enable->valueint;
                params->labels[params->labels_count].align      = pgs_parse_align(cjson_align->valuestring);
                params->labels[params->labels_count].text_align = pgs_parse_text_align(cjson_text_align->valuestring);
                params->labels[params->labels_count].anim =
                    pgs_parse_anim(cjson_anim->valuestring, PGS_WIDGETS_ANIM_FADE);
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

    /* images */
    cJSON * cjson_images = NULL;
    cjson_images         = cJSON_GetObjectItem(cjson_root, "images");
    if(cjson_images && cJSON_IsArray(cjson_images)) {
        int count            = cJSON_GetArraySize(cjson_images);
        params->images_count = 0;
        params->images       = lv_malloc(sizeof(struct pgs_widgets_params_image) * count);
        if(!params->images) {
            goto err_p_labels;
        }

        CJSON_PARSE_ARRAY_BEG(0, i, count)
        {
            cJSON * cjson_image = cJSON_GetArrayItem(cjson_images, i);
            if(cjson_image) {
                CJSON_PARSE_ENABLE(cjson_image);
                CJSON_PARSE_PATH(cjson_image);
                CJSON_PARSE_ALIGN(cjson_image);
                CJSON_PARSE_ANIM(cjson_image);
                CJSON_PARSE_COLOR(cjson_image);
                CJSON_PARSE_ZINDEX(cjson_image);
                CJSON_PARSE_X(cjson_image);
                CJSON_PARSE_Y(cjson_image);
                CJSON_PARSE_RADIUS(cjson_image);

                params->images[params->images_count].zindex =
                    CJSON_CHECK_ZINDEX(cjson_zindex) ? cjson_zindex->valueint : PGS_WIDGETS_ZINDEX_DEFAULT;

                params->images[params->images_count].enable = cjson_enable->valueint;
                params->images[params->images_count].align  = pgs_parse_align(cjson_align->valuestring);
                params->images[params->images_count].anim =
                    pgs_parse_anim(cjson_anim->valuestring, PGS_WIDGETS_ANIM_FADE);
                params->images[params->images_count].opa    = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
                params->images[params->images_count].x      = cjson_x->valueint;
                params->images[params->images_count].y      = cjson_y->valueint;
                params->images[params->images_count].path   = cjson_path->valuestring;
                params->images[params->images_count].radius = cjson_radius->valueint;
                params->images_count++;
            }
        }
        CJSON_PARSE_ARRAY_END;
    }

    /* gifs */
    cJSON * cjson_gifs = NULL;
    cjson_gifs         = cJSON_GetObjectItem(cjson_root, "gifs");
    if(cjson_gifs && cJSON_IsArray(cjson_gifs)) {
        int count          = cJSON_GetArraySize(cjson_gifs);
        params->gifs_count = 0;
        params->gifs       = lv_malloc(sizeof(struct pgs_widgets_params_gif) * count);
        if(!params->gifs) {
            goto err_p_images;
        }

        CJSON_PARSE_ARRAY_BEG(0, i, count)
        {
            cJSON * cjson_gif = cJSON_GetArrayItem(cjson_gifs, i);
            if(cjson_gif) {
                CJSON_PARSE_ENABLE(cjson_gif);
                CJSON_PARSE_PATH(cjson_gif);
                CJSON_PARSE_ALIGN(cjson_gif);
                CJSON_PARSE_ANIM(cjson_gif);
                CJSON_PARSE_COLOR(cjson_gif);
                CJSON_PARSE_ZINDEX(cjson_gif);
                CJSON_PARSE_X(cjson_gif);
                CJSON_PARSE_Y(cjson_gif);
                CJSON_PARSE_RADIUS(cjson_gif);

                params->gifs[params->gifs_count].zindex =
                    CJSON_CHECK_ZINDEX(cjson_zindex) ? cjson_zindex->valueint : PGS_WIDGETS_ZINDEX_DEFAULT;

                params->gifs[params->gifs_count].enable = cjson_enable->valueint;
                params->gifs[params->gifs_count].align  = pgs_parse_align(cjson_align->valuestring);
                params->gifs[params->gifs_count].anim = pgs_parse_anim(cjson_anim->valuestring, PGS_WIDGETS_ANIM_FADE);
                params->gifs[params->gifs_count].opa  = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
                params->gifs[params->gifs_count].x    = cjson_x->valueint;
                params->gifs[params->gifs_count].y    = cjson_y->valueint;
                params->gifs[params->gifs_count].path = cjson_path->valuestring;
                params->gifs[params->gifs_count].radius = cjson_radius->valueint;
                params->gifs_count++;
            }
        }
        CJSON_PARSE_ARRAY_END;
    }

    /* videos */
    cJSON * cjson_videos = NULL;
    cjson_videos         = cJSON_GetObjectItem(cjson_root, "videos");
    if(cjson_videos && cJSON_IsArray(cjson_videos)) {
        int count            = cJSON_GetArraySize(cjson_videos);
        params->videos_count = 0;
        params->videos       = lv_malloc(sizeof(struct pgs_widgets_params_video) * count);
        if(!params->videos) {
            goto err_p_gifs;
        }

        CJSON_PARSE_ARRAY_BEG(0, i, count)
        {
            cJSON * cjson_video = cJSON_GetArrayItem(cjson_videos, i);
            if(cjson_video) {
                CJSON_PARSE_ENABLE(cjson_video);
                CJSON_PARSE_ALIGN(cjson_video);
                CJSON_PARSE_ANIM(cjson_video);
                CJSON_PARSE_COLOR(cjson_video);
                CJSON_PARSE_ZINDEX(cjson_video);
                CJSON_PARSE_X(cjson_video);
                CJSON_PARSE_Y(cjson_video);
                CJSON_PARSE_RADIUS(cjson_video);

                params->videos[params->videos_count].zindex =
                    CJSON_CHECK_ZINDEX(cjson_zindex) ? cjson_zindex->valueint : PGS_WIDGETS_ZINDEX_DEFAULT;

                params->videos[params->videos_count].enable = cjson_enable->valueint;
                params->videos[params->videos_count].align  = pgs_parse_align(cjson_align->valuestring);
                params->videos[params->videos_count].anim =
                    pgs_parse_anim(cjson_anim->valuestring, PGS_WIDGETS_ANIM_FADE);
                params->videos[params->videos_count].opa    = pgs_parse_opa(cjson_color->valuestring, LV_OPA_100);
                params->videos[params->videos_count].x      = cjson_x->valueint;
                params->videos[params->videos_count].y      = cjson_y->valueint;
                params->videos[params->videos_count].radius = cjson_radius->valueint;
                params->videos[params->videos_count].count  = 0;

                cJSON * cjson_paths = NULL;
                cjson_paths         = cJSON_GetObjectItem(cjson_video, "paths");
                if(cjson_paths && cJSON_IsArray(cjson_paths)) {
                    int paths_count                            = cJSON_GetArraySize(cjson_video);
                    params->videos[params->videos_count].count = 0;
                    params->videos[params->videos_count].paths = lv_malloc(sizeof(const char *) * paths_count);
                    if(!params->videos[params->videos_count].paths) {
                        for(uint32_t k = 0; k < params->videos_count; k++) {
                            if(params->videos[k].paths) {
                                lv_free(params->videos[k].paths);
                            }
                        }
                        goto err_p_videos;
                    }

                    CJSON_PARSE_ARRAY_BEG(0, j, paths_count)
                    {
                        cJSON * cjson_path = cJSON_GetArrayItem(cjson_paths, j);
                        if(!(cjson_path && cJSON_IsString(cjson_path))) {
                            continue;
                        }

                        params->videos[params->videos_count].paths[params->videos[params->videos_count].count] =
                            cjson_path->valuestring;
                        params->videos[params->videos_count].count++;
                    }
                    CJSON_PARSE_ARRAY_END;
                }
                params->videos_count++;
            }
        }
        CJSON_PARSE_ARRAY_END;
    }

    lv_free(message);

    return params;

err_p_videos:
    if(params->videos) lv_free(params->videos);

err_p_gifs:
    if(params->gifs) lv_free(params->gifs);

err_p_images:
    if(params->images) lv_free(params->images);

err_p_labels:
    if(params->labels) lv_free(params->labels);

err_p_wpmlabel:
    if(params->wpmlabel) lv_free(params->wpmlabel);

err_p_wpmchart:
    if(params->wpmchart) lv_free(params->wpmchart);

err_p_keysnd:
    if(params->keysnd) lv_free(params->keysnd);

err_p_waitanim:
    if(params->keyanim && params->keyanim->waitanims) {
        lv_free(params->keyanim->waitanims);
    }

err_p_keyanim:
    if(params->keyanim) lv_free(params->keyanim);

err_p_keyroll:
    if(params->keyroll) lv_free(params->keyroll);

err_p_states:
    if(params->states) lv_free(params->states);

err_malloc:
    if(params->base_dynamic && params->base) {
        free((void *)params->base);
    }
    lv_free(params);

err_parse:
    cJSON_Delete(cjson_root);

err_message:
    lv_free(message);

    return NULL;
}

void keyboard_params_delete(struct keyboard_params * params)
{
    if(!params) {
        return;
    }
    if(params->states) lv_free(params->states);
    if(params->keyroll) lv_free(params->keyroll);
    if(params->keyanim && params->keyanim->waitanims) lv_free(params->keyanim->waitanims);
    if(params->keyanim) lv_free(params->keyanim);
    if(params->keysnd) lv_free(params->keysnd);
    if(params->wpmchart) lv_free(params->wpmchart);
    if(params->wpmlabel) lv_free(params->wpmlabel);
    if(params->labels) lv_free(params->labels);
    if(params->images) lv_free(params->images);
    if(params->gifs) lv_free(params->gifs);
    if(params->videos) {
        for(uint32_t i = 0; i < params->videos_count; i++) {
            if(params->videos[i].paths) {
                lv_free(params->videos[i].paths);
            }
        }
        lv_free(params->videos);
    }
    if(params->cjson) cJSON_Delete(params->cjson);
    if(params->base_dynamic && params->base) {
        free((void *)params->base);
    }
    lv_free(params);
}

struct pgs_widgets_params_state * keyboard_params_state_get(struct keyboard_params * params, uint8_t type)
{
    struct pgs_widgets_params_state * state = NULL;

    for(uint8_t i = 0; i < params->states_count; i++) {
        if(params->states[i].type == type) {
            state = &params->states[i];
            break;
        }
    }

    return state;
}
