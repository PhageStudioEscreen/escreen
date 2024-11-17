#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include "menu.h"
#include "pgs_modules.h"
#include "pgs_utils.h"

#define PGS_SHARE_PATH "/usr/share/pgs/"
#define PGS_SHARE_MENU_PATH PGS_SHARE_PATH "menu/"
#define PGS_SHARE_APP_PATH PGS_SHARE_PATH "apps"

static lv_style_t ui_line_style;
static lv_obj_t * ui_container;
static lv_obj_t * ui_icon_menu;
static lv_obj_t * ui_icon_menu_label;
static lv_obj_t * ui_escreen_label;
static lv_obj_t * ui_line_top;
static lv_obj_t * ui_icon_prev;
static lv_obj_t * ui_icon_prev_label;
static lv_obj_t * ui_icon_next;
static lv_obj_t * ui_icon_next_label;
static lv_obj_t * ui_icon_open;
static lv_obj_t * ui_icon_open_lable;
static lv_obj_t * ui_line_bottom;
static lv_obj_t * ui_container_apps;
static lv_obj_t * ui_lable_app;
static lv_group_t * ui_group;
static void (*ui_key_cb)(uint32_t keycode);

static lv_point_precise_t top_line_points[]    = {{10, 32}, {310, 32}};
static lv_point_precise_t bottom_line_points[] = {{10, 141}, {310, 141}};

static struct _pgs_list global_list;

static char full_path[PATH_MAX];
static char icon_path[PATH_MAX];

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

        lv_label_set_text(ui_lable_app, lv_event_get_user_data(event));
    } else if(code == LV_EVENT_CLICKED) {
        char dbus_name[128] = {0};
        char dbus_path[128] = {0};

        struct pgs_application * app = lv_event_get_user_data(event);

        lv_snprintf(dbus_name, sizeof(dbus_name), PGS_DBUS_NAME_PREFIX "%s", app->name);
        lv_snprintf(dbus_path, sizeof(dbus_path), PGS_DBUS_PATH_PREFIX "%s", app->name);

        pgs_dbus_method_call(dbus_name, dbus_path, 1, 0);
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

static void apps_register(const char * path)
{
    struct pgs_application * app = lv_malloc(sizeof(struct pgs_application));

    if(!app) {
        perror("apps_register");
        return;
    }

    const char * last_slash = strrchr(path, '/');
    if(last_slash != NULL) {
        app->name = strdup(last_slash + 1);
    } else {
        app->name = strdup(path);
    }
    if(!app->name) {
        perror("apps_register");
        return;
    }
    app->path = strdup(path);

    // to_uppercase(app->name);

    lv_snprintf(icon_path, sizeof(icon_path), "%s/icon.png", path);

    pgs_dlist_init(&(app->list));

    app->button = lv_button_create(ui_container_apps);
    lv_obj_remove_style_all(app->button);
    lv_obj_add_event_cb(app->button, apps_event_cb, LV_EVENT_FOCUSED, app->name);
    lv_obj_add_event_cb(app->button, apps_event_cb, LV_EVENT_CLICKED, app);
    lv_obj_add_event_cb(app->button, apps_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_set_size(app->button, 64, 64);
    lv_obj_set_style_bg_opa(app->button, LV_OPA_0, LV_PART_MAIN);

    LV_IMG_DECLARE(icon_app);

    app->icon = lv_image_create(app->button);
    lv_image_set_src(app->icon, &icon_app);
    lv_obj_align(app->icon, LV_ALIGN_CENTER, 0, 0);

    lv_group_add_obj(ui_group, app->button);

    pgs_dlist_insert_before(&global_list, &(app->list));

    printf("Found application %s\n", app->name);
}

lv_obj_t * pgs_apps_menu_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode))
{
    pgs_dlist_init(&global_list);

    ui_group  = group;
    ui_key_cb = key_cb;

    lv_style_init(&ui_line_style);
    lv_style_set_line_width(&ui_line_style, 1);
    lv_style_set_line_color(&ui_line_style, lv_color_hex(0x282828));

    ui_container = lv_obj_create(obj);
    lv_obj_remove_style_all(ui_container);
    lv_obj_set_width(ui_container, 320);
    lv_obj_set_height(ui_container, 172);
    lv_obj_set_align(ui_container, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_container, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    LV_IMG_DECLARE(icon_menu);
    ui_icon_menu = lv_image_create(ui_container);
    lv_image_set_src(ui_icon_menu, &icon_menu);
    lv_obj_set_width(ui_icon_menu, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_icon_menu, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_icon_menu, 5);
    lv_obj_set_y(ui_icon_menu, 5);

    ui_icon_menu_label = lv_label_create(ui_container);
    lv_obj_set_width(ui_icon_menu_label, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_icon_menu_label, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_icon_menu_label, 32);
    lv_obj_set_y(ui_icon_menu_label, 8);
    lv_label_set_text(ui_icon_menu_label, "MENU");
    lv_obj_set_style_text_color(ui_icon_menu_label, lv_color_hex(0x282828), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_icon_menu_label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_escreen_label = lv_label_create(ui_container);
    lv_obj_set_width(ui_escreen_label, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_escreen_label, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_escreen_label, 232);
    lv_obj_set_y(ui_escreen_label, 8);
    lv_label_set_text(ui_escreen_label, "ESCREEN");
    lv_obj_set_style_text_color(ui_escreen_label, lv_color_hex(0x282828), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_escreen_label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_line_top = lv_line_create(ui_container);
    lv_line_set_points(ui_line_top, top_line_points, 2);
    lv_obj_add_style(ui_line_top, &ui_line_style, 0);

    LV_IMG_DECLARE(icon_prev);
    ui_icon_prev = lv_image_create(ui_container);
    lv_image_set_src(ui_icon_prev, &icon_prev);
    lv_obj_set_width(ui_icon_prev, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_icon_prev, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_icon_prev, 10);
    lv_obj_set_y(ui_icon_prev, 148);

    ui_icon_prev_label = lv_label_create(ui_container);
    lv_obj_set_width(ui_icon_prev_label, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_icon_prev_label, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_icon_prev_label, 33);
    lv_obj_set_y(ui_icon_prev_label, 150);
    lv_label_set_text(ui_icon_prev_label, "PREV");
    lv_obj_set_style_text_color(ui_icon_prev_label, lv_color_hex(0x282828), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_icon_prev_label, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    LV_IMG_DECLARE(icon_next);
    ui_icon_next = lv_image_create(ui_container);
    lv_image_set_src(ui_icon_next, &icon_next);
    lv_obj_set_width(ui_icon_next, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_icon_next, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_icon_next, 84);
    lv_obj_set_y(ui_icon_next, 148);

    ui_icon_next_label = lv_label_create(ui_container);
    lv_obj_set_width(ui_icon_next_label, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_icon_next_label, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_icon_next_label, 107);
    lv_obj_set_y(ui_icon_next_label, 150);
    lv_label_set_text(ui_icon_next_label, "NEXT");
    lv_obj_set_style_text_color(ui_icon_next_label, lv_color_hex(0x282828), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_icon_next_label, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    LV_IMG_DECLARE(icon_open);
    ui_icon_open = lv_image_create(ui_container);
    lv_image_set_src(ui_icon_open, &icon_open);
    lv_obj_set_width(ui_icon_open, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_icon_open, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_icon_open, 246);
    lv_obj_set_y(ui_icon_open, 148);

    ui_icon_open_lable = lv_label_create(ui_container);
    lv_obj_set_width(ui_icon_open_lable, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_icon_open_lable, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_icon_open_lable, 269);
    lv_obj_set_y(ui_icon_open_lable, 150);
    lv_label_set_text(ui_icon_open_lable, "OPEN");
    lv_obj_set_style_text_color(ui_icon_open_lable, lv_color_hex(0x282828), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_icon_open_lable, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_line_bottom = lv_line_create(ui_container);
    lv_line_set_points(ui_line_bottom, bottom_line_points, 2);
    lv_obj_add_style(ui_line_bottom, &ui_line_style, 0);

    ui_lable_app = lv_label_create(ui_container);
    lv_obj_set_width(ui_lable_app, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_lable_app, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_lable_app, 0);
    lv_obj_set_y(ui_lable_app, 119);
    lv_obj_set_align(ui_lable_app, LV_ALIGN_TOP_MID);
    lv_label_set_text(ui_lable_app, "???");
    lv_obj_set_style_text_color(ui_lable_app, lv_color_hex(0x606060), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_lable_app, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_lable_app, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_container_apps = lv_obj_create(ui_container);
    lv_obj_set_width(ui_container_apps, 320);
    lv_obj_set_height(ui_container_apps, 64);
    lv_obj_set_scrollbar_mode(ui_container_apps, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_x(ui_container_apps, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_style_bg_opa(ui_container_apps, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_container_apps, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(ui_container_apps, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(ui_container_apps, 32, LV_PART_MAIN);
    lv_obj_set_x(ui_container_apps, 0);
    lv_obj_set_y(ui_container_apps, 51);
    lv_obj_set_flex_flow(ui_container_apps, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_container_apps, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

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
            apps_register(full_path);
        }
    }

    closedir(dir);

    uint32_t mid_btn_index = (lv_obj_get_child_cnt(ui_container_apps) - 1) / 2;
    for(uint32_t i = 0; i < mid_btn_index; i++) {
        lv_obj_move_to_index(lv_obj_get_child(ui_container_apps, -1), 0);
    }
    lv_obj_scroll_to_view(lv_obj_get_child(ui_container_apps, mid_btn_index), LV_ANIM_OFF);

    struct pgs_application * app = PGS_DLIST_ENTRY_FIRST(&global_list, struct pgs_application, list);
    lv_label_set_text(ui_lable_app, app->name);

    return ui_container;
}
