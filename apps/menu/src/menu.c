#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <linux/limits.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include "cJSON.h"
#include "menu.h"
#include "pgs_modules.h"
#include "pgs_utils.h"

#define PGS_SHARE_PATH "/usr/share/pgs/"
#define PGS_SHARE_MENU_PATH PGS_SHARE_PATH "menu/"
#define PGS_SHARE_APP_PATH PGS_SHARE_PATH "apps"
#define PGS_MENU_CONFIG_PATH PGS_SHARE_MENU_PATH "resources/config.json"
#define PGS_MENU_DEFAULT_LANGUAGE "zh-cn"

// static lv_style_t ui_line_style;
static lv_obj_t * ui_container;
static lv_obj_t * ui_img_bg;
static lv_obj_t * ui_container_apps;
static lv_obj_t * ui_label_app;
static lv_group_t * ui_group;
static void (*ui_key_cb)(uint32_t keycode);

static struct _pgs_list global_list;

static char full_path[PATH_MAX];
static char icon_path[PATH_MAX];
static char full_name[PATH_MAX];

struct pgs_app_meta
{
    char * name;
    char * version;
    char * author;
    int index;
};

struct pgs_app_item
{
    char * id;
    char * path;
    struct pgs_app_meta meta;
};

struct pgs_menu_config
{
    char language[16];
    char current[PATH_MAX];
};

static struct pgs_menu_config global_config;

static void to_uppercase(char * str)
{
    while(*str) {
        *str = toupper((unsigned char)*str);
        str++;
    }
}

static void str_replace_all(char * str, char old_char, char new_char)
{
    int i = 0;

    while(str[i] != '\0') {
        if(str[i] == old_char) {
            str[i] = new_char;
        }
        i++;
    }
}

static void load_menu_config(struct pgs_menu_config * config)
{
    char * content = read_text_file(PGS_MENU_CONFIG_PATH);

    memset(config, 0, sizeof(*config));
    lv_snprintf(config->language, sizeof(config->language), "%s", PGS_MENU_DEFAULT_LANGUAGE);
    if(content == NULL) {
        return;
    }

    cJSON * root = cJSON_Parse(content);
    if(root != NULL) {
        cJSON * cjson_language = cJSON_GetObjectItemCaseSensitive(root, "language");
        cJSON * cjson_current  = cJSON_GetObjectItemCaseSensitive(root, "current");

        if(cJSON_IsString(cjson_language) && cjson_language->valuestring[0] != '\0') {
            lv_snprintf(config->language, sizeof(config->language), "%s", cjson_language->valuestring);
        }
        if(cJSON_IsString(cjson_current) && cjson_current->valuestring[0] != '\0') {
            lv_snprintf(config->current, sizeof(config->current), "%s", cjson_current->valuestring);
        }

        cJSON_Delete(root);
    }

    free(content);
}

static bool save_menu_config(const struct pgs_menu_config * config)
{
    bool result    = false;
    cJSON * root   = cJSON_CreateObject();
    cJSON * item   = NULL;
    char * content = NULL;

    if(root == NULL) {
        return false;
    }

    item = cJSON_CreateString(config->language);
    if(item == NULL) {
        goto exit;
    }
    cJSON_AddItemToObject(root, "language", item);

    item = cJSON_CreateString(config->current);
    if(item == NULL) {
        goto exit;
    }
    cJSON_AddItemToObject(root, "current", item);

    content = cJSON_Print(root);
    if(content == NULL) {
        goto exit;
    }

    result = write_text_file(PGS_MENU_CONFIG_PATH, content);

exit:
    if(content != NULL) {
        cJSON_free(content);
    }
    cJSON_Delete(root);
    return result;
}

static void free_app_meta(struct pgs_app_meta * app)
{
    if(app == NULL) {
        return;
    }

    free(app->name);
    free(app->version);
    free(app->author);
}

static void free_app_item(struct pgs_app_item * app)
{
    if(app == NULL) {
        return;
    }

    free(app->id);
    free(app->path);
    free_app_meta(&app->meta);
}

static int compare_app_item(const void * lhs, const void * rhs)
{
    const struct pgs_app_item * app_lhs = lhs;
    const struct pgs_app_item * app_rhs = rhs;

    if(app_lhs->meta.index != app_rhs->meta.index) {
        return app_lhs->meta.index - app_rhs->meta.index;
    }

    return strcmp(app_lhs->id, app_rhs->id);
}

static bool load_app_meta(const char * path, const char * language, struct pgs_app_meta * app)
{
    char info_path[PATH_MAX];
    char * content      = NULL;
    char * display_name = NULL;
    char * version      = NULL;
    char * author       = NULL;
    cJSON * root        = NULL;

    memset(app, 0, sizeof(*app));

    lv_snprintf(info_path, sizeof(info_path), "%s/info.json", path);
    content = read_text_file(info_path);
    if(content == NULL) {
        return false;
    }

    root = cJSON_Parse(content);
    if(root == NULL) {
        free(content);
        return false;
    }

    cJSON * cjson_name    = cJSON_GetObjectItemCaseSensitive(root, "name");
    cJSON * cjson_display = cJSON_GetObjectItemCaseSensitive(cjson_name, language);
    cJSON * cjson_version = cJSON_GetObjectItemCaseSensitive(root, "version");
    cJSON * cjson_author  = cJSON_GetObjectItemCaseSensitive(root, "author");
    cJSON * cjson_index   = cJSON_GetObjectItemCaseSensitive(root, "index");

    if(!cJSON_IsObject(cjson_name) || !cJSON_IsString(cjson_display) || cjson_display->valuestring[0] == '\0' ||
       !cJSON_IsString(cjson_version) || cjson_version->valuestring[0] == '\0' || !cJSON_IsString(cjson_author) ||
       cjson_author->valuestring[0] == '\0' || !cJSON_IsNumber(cjson_index)) {
        cJSON_Delete(root);
        free(content);
        return false;
    }

    display_name = strdup(cjson_display->valuestring);
    version      = strdup(cjson_version->valuestring);
    author       = strdup(cjson_author->valuestring);
    if(display_name == NULL || version == NULL || author == NULL) {
        free(display_name);
        free(version);
        free(author);
        cJSON_Delete(root);
        free(content);
        return false;
    }

    app->name    = display_name;
    app->version = version;
    app->author  = author;
    app->index   = cjson_index->valueint;

    cJSON_Delete(root);

    free(content);
    return true;
}

static bool load_app_item(const char * path, const char * dir_name, const char * language, struct pgs_app_item * app)
{
    memset(app, 0, sizeof(*app));

    app->id   = strdup(dir_name);
    app->path = strdup(path);
    if(app->id == NULL || app->path == NULL) {
        free_app_item(app);
        return false;
    }

    if(!load_app_meta(path, language, &app->meta)) {
        free_app_item(app);
        return false;
    }

    return true;
}

static void apps_event_cb(lv_event_t * event)
{
    // lv_obj_t * current_btn = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);

    if(code == LV_EVENT_FOCUSED) {
        // uint32_t current_btn_index = lv_obj_get_index(current_btn);
        // uint32_t mid_btn_index     = (lv_obj_get_child_cnt(ui_container_apps) - 1) / 2;

        // if(current_btn_index > mid_btn_index) {
        //     lv_obj_scroll_to_view(lv_obj_get_child(ui_container_apps, mid_btn_index - 1), LV_ANIM_OFF);
        //     lv_obj_scroll_to_view(lv_obj_get_child(ui_container_apps, mid_btn_index), LV_ANIM_ON);
        //     lv_obj_move_to_index(lv_obj_get_child(ui_container_apps, 0), -1);

        // } else if(current_btn_index < mid_btn_index) {
        //     lv_obj_scroll_to_view(lv_obj_get_child(ui_container_apps, mid_btn_index + 1), LV_ANIM_OFF);
        //     lv_obj_scroll_to_view(lv_obj_get_child(ui_container_apps, mid_btn_index), LV_ANIM_ON);
        //     lv_obj_move_to_index(lv_obj_get_child(ui_container_apps, -1), 0);
        // }

        lv_label_set_text(ui_label_app, lv_event_get_user_data(event));
    } else if(code == LV_EVENT_CLICKED) {
        char dbus_name[128] = {0};
        char dbus_path[128] = {0};

        struct pgs_application * app = lv_event_get_user_data(event);

        lv_snprintf(global_config.current, sizeof(global_config.current), "%s", app->id);
        save_menu_config(&global_config);

        lv_snprintf(dbus_name, sizeof(dbus_name), PGS_DBUS_NAME_PREFIX "%s", app->id);
        lv_snprintf(dbus_path, sizeof(dbus_path), PGS_DBUS_PATH_PREFIX "%s", app->id);

        pgs_lvgl_suspend();
        /* wake up app, then kill */
        pgs_dbus_method_call(dbus_name, dbus_path, 1, getpid());
        while(1) {
            /* wait for kill */
            sleep(1);
        }
    } else if(code == LV_EVENT_KEY) {
        lv_group_t * g   = lv_indev_get_group(lv_indev_active());
        uint32_t keycode = lv_indev_get_key(lv_indev_active());

        if(g == NULL) return;

        switch(keycode) {
            case LV_KEY_RIGHT:
                lv_group_set_editing(g, false);
                lv_group_focus_next(g);
                break;
            case LV_KEY_LEFT:
                lv_group_set_editing(g, false);
                lv_group_focus_prev(g);
                break;
            default:
                if(ui_key_cb) {
                    ui_key_cb(keycode);
                }
                break;
        }
    }
}

static void apps_register(const struct pgs_app_item * item)
{
    struct pgs_application * app = lv_malloc(sizeof(struct pgs_application));

    if(!app) {
        perror("apps_register");
        return;
    }

    app->id   = strdup(item->id);
    app->name = strdup(item->meta.name);
    app->path = strdup(item->path);
    if(!app->id || !app->name || !app->path) {
        perror("apps_register");
        return;
    }

    // to_uppercase(app->name);

    lv_snprintf(icon_path, sizeof(icon_path), "%s/icon.png", app->path);

    pgs_dlist_init(&(app->list));

    app->button = lv_button_create(ui_container_apps);
    lv_obj_remove_style_all(app->button);
    lv_obj_add_event_cb(app->button, apps_event_cb, LV_EVENT_FOCUSED, app->name);
    lv_obj_add_event_cb(app->button, apps_event_cb, LV_EVENT_CLICKED, app);
    lv_obj_add_event_cb(app->button, apps_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_set_size(app->button, 96, 96);
    lv_obj_set_style_bg_opa(app->button, LV_OPA_0, LV_PART_MAIN);

    LV_IMG_DECLARE(icon_app);

    app->icon                       = lv_image_create(app->button);
    const lv_image_dsc_t * icon_dyn = pgs_libpng_decode(icon_path);
    if(icon_dyn) {
        lv_image_set_src(app->icon, icon_dyn);
    } else {
        lv_image_set_src(app->icon, &icon_app);
    }

    lv_obj_align(app->icon, LV_ALIGN_CENTER, 0, 0);

    lv_group_add_obj(ui_group, app->button);

    pgs_dlist_insert_before(&global_list, &(app->list));

    printf("Found application %s(%s)\n", app->id, app->name);
}

lv_obj_t * pgs_apps_menu_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode))
{
    pgs_dlist_init(&global_list);

    ui_group  = group;
    ui_key_cb = key_cb;

    ui_container = lv_obj_create(obj);
    lv_obj_remove_style_all(ui_container);
    lv_obj_set_width(ui_container, lv_disp_get_hor_res(lv_disp_get_default()));
    lv_obj_set_height(ui_container, lv_disp_get_ver_res(lv_disp_get_default()));
    lv_obj_set_align(ui_container, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(ui_container, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui_container, LV_OPA_COVER, 0);
    lv_obj_clear_flag(ui_container, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    LV_IMG_DECLARE(img_bg);
    ui_img_bg = lv_image_create(ui_container);
    lv_image_set_src(ui_img_bg, &img_bg);
    lv_obj_set_width(ui_img_bg, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_img_bg, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_img_bg, 0);
    lv_obj_set_y(ui_img_bg, 0);
    lv_obj_set_align(ui_img_bg, LV_ALIGN_CENTER);

    ui_label_app = lv_label_create(ui_container);
    lv_obj_set_width(ui_label_app, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_label_app, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_label_app, 0);
    lv_obj_set_y(ui_label_app, 175);
    lv_obj_set_align(ui_label_app, LV_ALIGN_TOP_MID);
    lv_label_set_text(ui_label_app, "???");
    lv_obj_set_style_text_color(ui_label_app, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_label_app, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_label_app, base_font_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_container_apps = lv_obj_create(ui_container);
    lv_obj_set_width(ui_container_apps, 536);
    lv_obj_set_height(ui_container_apps, 96);
    lv_obj_set_scrollbar_mode(ui_container_apps, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_x(ui_container_apps, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_style_bg_opa(ui_container_apps, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_container_apps, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(ui_container_apps, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(ui_container_apps, 48, LV_PART_MAIN);
    lv_obj_set_x(ui_container_apps, 0);
    lv_obj_set_y(ui_container_apps, 71);
    lv_obj_set_flex_flow(ui_container_apps, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_container_apps, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    bool jump                  = false;
    struct pgs_app_item * apps = NULL;
    size_t app_count           = 0;

    load_menu_config(&global_config);
    lv_snprintf(full_name, sizeof(full_name), "%s", global_config.current);

    DIR * dir;
    struct dirent * entry;
    struct stat statbuf;

    dir = opendir(PGS_SHARE_APP_PATH);
    if(dir == NULL) {
        perror("pgs_apps_menu_init");
        return NULL;
    }

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        lv_snprintf(full_path, sizeof(full_path), "%s/%s", PGS_SHARE_APP_PATH, entry->d_name);

        if(stat(full_path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
            struct pgs_app_item * resized_apps;

            resized_apps = realloc(apps, sizeof(*apps) * (app_count + 1));
            if(resized_apps == NULL) {
                continue;
            }

            apps = resized_apps;
            if(!load_app_item(full_path, entry->d_name, global_config.language, &apps[app_count])) {
                continue;
            }

            if(strcmp(apps[app_count].id, full_name) == 0) {
                jump = true;
            }

            app_count++;
        }
    }

    closedir(dir);

    if(app_count > 1) {
        qsort(apps, app_count, sizeof(*apps), compare_app_item);
    }

    for(size_t i = 0; i < app_count; i++) {
        apps_register(&apps[i]);
        free_app_item(&apps[i]);
    }
    free(apps);

    if(lv_obj_get_child_cnt(ui_container_apps) == 0) {
        lv_label_set_text(ui_label_app, "???");
        return ui_container;
    }

    uint32_t mid_btn_index = (lv_obj_get_child_cnt(ui_container_apps) - 1) / 2;
    for(uint32_t i = 0; i < mid_btn_index; i++) {
        lv_obj_move_to_index(lv_obj_get_child(ui_container_apps, -1), 0);
    }
    lv_obj_scroll_to_view(lv_obj_get_child(ui_container_apps, mid_btn_index), LV_ANIM_OFF);

    struct pgs_application * app = PGS_DLIST_ENTRY_FIRST(&global_list, struct pgs_application, list);
    lv_label_set_text(ui_label_app, app->name);

    if(jump) {
        char dbus_name[128] = {0};
        char dbus_path[128] = {0};

        lv_snprintf(dbus_name, sizeof(dbus_name), PGS_DBUS_NAME_PREFIX "%s", full_name);
        lv_snprintf(dbus_path, sizeof(dbus_path), PGS_DBUS_PATH_PREFIX "%s", full_name);

        pgs_lvgl_suspend();
        /* wake up app, then kill */
        pgs_dbus_method_call(dbus_name, dbus_path, 1, getpid());
        while(1) {
            /* wait for kill */
            sleep(1);
        }
    }

    return ui_container;
}
