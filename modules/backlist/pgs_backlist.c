#include "pgs_modules.h"

#include <stdio.h>
#include <signal.h>
#include "pgs_utils.h"
#include "pgs_backlist.h"

static lv_obj_t * ui_container_backlist;
static lv_obj_t * ui_container_list;
static lv_group_t * ui_group;
static void (*ui_key_cb)(uint32_t keycode);

static struct _pgs_list global_list;

static void back_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if(code == LV_EVENT_CLICKED) {
        lv_obj_add_flag(ui_container_backlist, LV_OBJ_FLAG_HIDDEN);
        if(ui_key_cb) {
            ui_key_cb(LV_KEY_ESC);
        }
    }
}

static void menu_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if(code == LV_EVENT_CLICKED) {
        pgs_cleanup();
    }
}

static void apps_event_cb(lv_event_t * event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if(code == LV_EVENT_FOCUSED) {
    } else if(code == LV_EVENT_CLICKED) {
        void (*cb)(lv_event_t * event) = lv_event_get_user_data(event);
        if(cb) {
            cb(event);
        }
        if(ui_key_cb) {
            ui_key_cb(LV_KEY_ESC);
        }
        pgs_backlist_hidden(true, false);
    } else if(code == LV_EVENT_KEY) {
        lv_group_t * g   = lv_indev_get_group(lv_indev_active());
        uint32_t keycode = lv_indev_get_key(lv_indev_active());

        if(g == NULL) return;

        switch(keycode) {
            case LV_KEY_DOWN:
                lv_group_set_editing(g, false);
                lv_group_focus_next(g);
                break;
            case LV_KEY_UP:
                lv_group_set_editing(g, false);
                lv_group_focus_prev(g);
                break;
            default:
                if(ui_key_cb) {
                    ui_key_cb(keycode);
                }
                if(keycode == LV_KEY_ESC) {
                    pgs_backlist_hidden(true, false);
                }
                break;
        }
    }
}

void pgs_backlist_add_item(const char * text, void * icon, lv_event_cb_t event_cb, void * user_data)
{
    struct pgs_backlist_item * item = lv_malloc(sizeof(struct pgs_backlist_item));

    if(!item) {
        perror("pgs_backlist_add_item");
        return;
    }

    item->text_src = text;
    item->icon_src = icon;
    item->event_cb = event_cb;

    pgs_dlist_init(&(item->list));

    item->container = lv_button_create(ui_container_list);
    lv_obj_remove_style_all(item->container);
    lv_obj_set_user_data(item->container, user_data);
    lv_obj_add_event_cb(item->container, apps_event_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(item->container, apps_event_cb, LV_EVENT_CLICKED, item->event_cb);
    lv_obj_add_event_cb(item->container, apps_event_cb, LV_EVENT_KEY, NULL);
    lv_obj_set_size(item->container, lv_pct(75), 36);
    lv_obj_set_style_bg_opa(item->container, LV_OPA_0, LV_PART_MAIN);
    lv_obj_align(item->container, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_set_style_radius(item->container, 6, LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(item->container, lv_color_hex(0x000000),
                              LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(item->container, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(item->container, lv_color_hex(0x505050),
                                   LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_opa(item->container, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(item->container, 1, LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(item->container, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED);

    item->icon = lv_img_create(item->container);
    lv_img_set_src(item->icon, item->icon_src);
    lv_obj_align(item->icon, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_x(item->icon, 6);

    item->text = lv_label_create(item->container);
    lv_obj_set_width(item->text, LV_SIZE_CONTENT);
    lv_obj_set_height(item->text, LV_SIZE_CONTENT);
    lv_obj_align(item->text, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_x(item->text, 30);
    lv_label_set_text(item->text, item->text_src);
    lv_obj_set_style_text_color(item->text, lv_color_hex(0xF0F0F0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(item->text, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(item->text, base_font_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(item->text, LV_LABEL_LONG_SCROLL_CIRCULAR);

    lv_group_add_obj(ui_group, item->container);

    pgs_dlist_insert_before(&global_list, &(item->list));

    printf("Add item %s\n", item->text_src);
}

static lv_obj_t * _pgs_backlist_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode), bool nomenu)
{
    pgs_dlist_init(&global_list);

    ui_group  = group;
    ui_key_cb = key_cb;

    lv_screen_active();

    ui_container_backlist = lv_obj_create(obj);
    lv_obj_remove_style_all(ui_container_backlist);
    lv_obj_set_width(ui_container_backlist, lv_disp_get_hor_res(lv_disp_get_default()));
    lv_obj_set_height(ui_container_backlist, lv_disp_get_ver_res(lv_disp_get_default()) * 2);
    lv_obj_set_align(ui_container_backlist, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_container_backlist, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_container_backlist, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_container_backlist, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_blend_mode(ui_container_backlist, LV_BLEND_MODE_NORMAL, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_container_list = lv_obj_create(ui_container_backlist);
    lv_obj_set_width(ui_container_list, lv_pct(100));
    lv_obj_set_height(ui_container_list, lv_pct(100));
    lv_obj_set_y(ui_container_list, lv_disp_get_ver_res(lv_disp_get_default()) * 178 / 255);
    lv_obj_set_scrollbar_mode(ui_container_list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(ui_container_list, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_container_list, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(ui_container_list, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(ui_container_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui_container_list, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(ui_container_list, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scroll_snap_y(ui_container_list, LV_SCROLL_SNAP_START);

    LV_IMG_DECLARE(icont_back);
    pgs_backlist_add_item("BACK", &icont_back, back_event_cb, NULL);

    LV_IMG_DECLARE(icont_menu);
    if(!nomenu) {
        pgs_backlist_add_item("MENU", &icont_menu, menu_event_cb, NULL);
    }

    return ui_container_backlist;
}

lv_obj_t * pgs_backlist_init(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode))
{
    return _pgs_backlist_init(obj, group, key_cb, false);
}

lv_obj_t * pgs_backlist_init_nomenu(lv_obj_t * obj, lv_group_t * group, void (*key_cb)(uint32_t keycode))
{
    return _pgs_backlist_init(obj, group, key_cb, true);
}

static void anim_y_cb(void * var, int32_t v)
{
    lv_obj_set_y(var, v);
}

static void anim_opa_cb(void * var, int32_t v)
{
    lv_obj_set_style_opa(var, v, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void anim_in_start_cb(lv_anim_t * anim)
{
    lv_obj_clear_flag(ui_container_backlist, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t * first = lv_obj_get_child(ui_container_list, 0);
    if(first) {
        lv_group_focus_obj(first);
        lv_obj_add_state(first, LV_STATE_FOCUS_KEY);
    }
}

static void anim_out_done_cb(lv_anim_t * anim)
{
    lv_obj_t * last = lv_obj_get_child(ui_container_list, lv_obj_get_child_count(ui_container_list) - 1);
    if(last) {
        lv_group_focus_obj(last);
        lv_obj_add_state(last, LV_STATE_FOCUS_KEY);
    }
    lv_obj_add_flag(ui_container_backlist, LV_OBJ_FLAG_HIDDEN);
}

void pgs_backlist_hidden(bool hidden, bool noanim)
{
    if(noanim) {
        if(hidden) {
            lv_obj_set_style_opa(ui_container_backlist, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(ui_container_backlist, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(ui_container_backlist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_opa(ui_container_backlist, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_t * first = lv_obj_get_child(ui_container_list, 0);
            if(first) {
                lv_group_focus_obj(first);
            }
        }
    }

    lv_anim_t a1;
    lv_anim_init(&a1);
    lv_anim_set_var(&a1, ui_container_backlist);
    lv_anim_t a2;
    lv_anim_init(&a2);
    lv_anim_set_var(&a2, ui_container_backlist);

    if(hidden) {
        lv_anim_set_values(&a1, -lv_obj_get_height(ui_container_backlist) / 4, 0);
        lv_anim_set_duration(&a1, 300);
        lv_anim_set_exec_cb(&a1, anim_y_cb);
        lv_anim_set_path_cb(&a1, lv_anim_path_ease_in);
        lv_anim_set_completed_cb(&a1, anim_out_done_cb);

        lv_anim_set_values(&a2, 255, 0);
        lv_anim_set_duration(&a2, 250);
        lv_anim_set_exec_cb(&a2, anim_opa_cb);
        lv_anim_set_path_cb(&a2, lv_anim_path_ease_in_out);

        lv_anim_start(&a1);
        lv_anim_start(&a2);
    } else {
        lv_anim_set_values(&a1, -lv_obj_get_height(ui_container_backlist) / 2,
                           -lv_obj_get_height(ui_container_backlist) / 4);
        lv_anim_set_duration(&a1, 300);
        lv_anim_set_exec_cb(&a1, anim_y_cb);
        lv_anim_set_path_cb(&a1, lv_anim_path_ease_out);
        lv_anim_set_start_cb(&a1, anim_in_start_cb);

        lv_anim_set_values(&a2, 0, 255);
        lv_anim_set_duration(&a2, 250);
        lv_anim_set_exec_cb(&a2, anim_opa_cb);
        lv_anim_set_path_cb(&a2, lv_anim_path_ease_in_out);

        lv_anim_start(&a1);
        lv_anim_start(&a2);
    }
}
