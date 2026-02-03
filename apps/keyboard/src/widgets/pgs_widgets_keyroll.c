#include "pgs_widgets.h"
#include "../../../lvgl/src/core/lv_obj_private.h"

#define SCROLL_ANIM_TIME_MIN 50  /*ms*/
#define SCROLL_ANIM_TIME_MAX 200 /*ms*/

static lv_result_t pgs_lv_obj_scroll_by_raw(lv_obj_t * obj, int32_t x, int32_t y)
{
    if(x == 0 && y == 0) return LV_RESULT_OK;

    lv_obj_allocate_spec_attr(obj);

    obj->spec_attr->scroll.x += x;
    obj->spec_attr->scroll.y += y;

    lv_obj_move_children_by(obj, x, y, true);
    lv_result_t res = lv_obj_send_event(obj, LV_EVENT_SCROLL, NULL);
    if(res != LV_RESULT_OK) return res;
    lv_obj_invalidate(obj);
    return LV_RESULT_OK;
}

static void scroll_x_anim(void * obj, int32_t v)
{
    pgs_lv_obj_scroll_by_raw(obj, v + lv_obj_get_scroll_x(obj), 0);
}

static void scroll_y_anim(void * obj, int32_t v)
{
    pgs_lv_obj_scroll_by_raw(obj, 0, v + lv_obj_get_scroll_y(obj));
}

static void scroll_end_cb(lv_anim_t * a)
{
    /*Do not sent END event if there wasn't a BEGIN*/
    if(a->start_cb_called) lv_obj_send_event(a->var, LV_EVENT_SCROLL_END, NULL);
}

static void pgs_lv_obj_scroll_by(lv_obj_t * obj, int32_t dx, int32_t dy, lv_anim_enable_t anim_en)
{
    if(dx == 0 && dy == 0) return;
    if(anim_en == LV_ANIM_ON) {
        lv_display_t * d = lv_obj_get_display(obj);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, obj);
        lv_anim_set_deleted_cb(&a, scroll_end_cb);

        if(dx) {
            uint32_t t = lv_anim_speed_clamped((lv_display_get_horizontal_resolution(d)) >> 1, SCROLL_ANIM_TIME_MIN,
                                               SCROLL_ANIM_TIME_MAX);
            lv_anim_set_duration(&a, t);
            int32_t sx = lv_obj_get_scroll_x(obj);
            lv_anim_set_values(&a, -sx, -sx + dx);
            lv_anim_set_exec_cb(&a, scroll_x_anim);
            lv_anim_set_path_cb(&a, lv_anim_path_ease_out);

            lv_result_t res;
            res = lv_obj_send_event(obj, LV_EVENT_SCROLL_BEGIN, &a);
            if(res != LV_RESULT_OK) return;
            lv_anim_start(&a);
        }

        if(dy) {
            uint32_t t = lv_anim_speed_clamped((lv_display_get_vertical_resolution(d)) >> 1, SCROLL_ANIM_TIME_MIN,
                                               SCROLL_ANIM_TIME_MAX);
            lv_anim_set_duration(&a, t);
            int32_t sy = lv_obj_get_scroll_y(obj);
            lv_anim_set_values(&a, -sy, -sy + dy);
            lv_anim_set_exec_cb(&a, scroll_y_anim);
            lv_anim_set_path_cb(&a, lv_anim_path_ease_out);

            lv_result_t res;
            res = lv_obj_send_event(obj, LV_EVENT_SCROLL_BEGIN, &a);
            if(res != LV_RESULT_OK) return;
            lv_anim_start(&a);
        }
    } else {
        /*Remove pending animations*/
        lv_anim_delete(obj, scroll_y_anim);
        lv_anim_delete(obj, scroll_x_anim);

        lv_result_t res;
        res = lv_obj_send_event(obj, LV_EVENT_SCROLL_BEGIN, NULL);
        if(res != LV_RESULT_OK) return;

        res = pgs_lv_obj_scroll_by_raw(obj, dx, dy);
        if(res != LV_RESULT_OK) return;

        res = lv_obj_send_event(obj, LV_EVENT_SCROLL_END, NULL);
        if(res != LV_RESULT_OK) return;
    }
}

static void pgs_scroll_area_into_view(const lv_area_t * area, lv_obj_t * child, lv_point_t * scroll_value,
                                      lv_anim_enable_t anim_en)
{
    lv_obj_t * parent = lv_obj_get_parent(child);
    if(!lv_obj_has_flag(parent, LV_OBJ_FLAG_SCROLLABLE)) return;

    lv_dir_t scroll_dir = lv_obj_get_scroll_dir(parent);
    int32_t snap_goal   = 0;
    int32_t act         = 0;
    const lv_area_t * area_tmp;

    int32_t y_scroll        = 0;
    lv_scroll_snap_t snap_y = lv_obj_get_scroll_snap_y(parent);
    if(snap_y != LV_SCROLL_SNAP_NONE)
        area_tmp = &child->coords;
    else
        area_tmp = area;

    int32_t stop        = lv_obj_get_style_space_top(parent, LV_PART_MAIN);
    int32_t sbottom     = lv_obj_get_style_space_bottom(parent, LV_PART_MAIN);
    int32_t top_diff    = parent->coords.y1 + stop - area_tmp->y1 - scroll_value->y;
    int32_t bottom_diff = -(parent->coords.y2 - sbottom - area_tmp->y2 - scroll_value->y);
    int32_t parent_h    = lv_obj_get_height(parent) - stop - sbottom;
    if((top_diff >= 0 && bottom_diff >= 0))
        y_scroll = 0;
    else if(top_diff > 0) {
        y_scroll = top_diff;
        /*Do not let scrolling in*/
        int32_t st = lv_obj_get_scroll_top(parent);
        if(st - y_scroll < 0) y_scroll = 0;
    } else if(bottom_diff > 0) {
        y_scroll = -bottom_diff;
        /*Do not let scrolling in*/
        int32_t sb = lv_obj_get_scroll_bottom(parent);
        if(sb + y_scroll < 0) y_scroll = 0;
    }

    switch(snap_y) {
        case LV_SCROLL_SNAP_START:
            snap_goal = parent->coords.y1 + stop;
            act       = area_tmp->y1 + y_scroll;
            y_scroll += snap_goal - act;
            break;
        case LV_SCROLL_SNAP_END:
            snap_goal = parent->coords.y2 - sbottom;
            act       = area_tmp->y2 + y_scroll;
            y_scroll += snap_goal - act;
            break;
        case LV_SCROLL_SNAP_CENTER:
            snap_goal = parent->coords.y1 + stop + parent_h / 2;
            act       = lv_area_get_height(area_tmp) / 2 + area_tmp->y1 + y_scroll;
            y_scroll += snap_goal - act;
            break;
        case LV_SCROLL_SNAP_NONE: break;
    }

    int32_t x_scroll        = 0;
    lv_scroll_snap_t snap_x = lv_obj_get_scroll_snap_x(parent);
    if(snap_x != LV_SCROLL_SNAP_NONE)
        area_tmp = &child->coords;
    else
        area_tmp = area;

    int32_t sleft      = lv_obj_get_style_space_left(parent, LV_PART_MAIN);
    int32_t sright     = lv_obj_get_style_space_right(parent, LV_PART_MAIN);
    int32_t left_diff  = parent->coords.x1 + sleft - area_tmp->x1 - scroll_value->x;
    int32_t right_diff = -(parent->coords.x2 - sright - area_tmp->x2 - scroll_value->x);
    if((left_diff >= 0 && right_diff >= 0))
        x_scroll = 0;
    else if(left_diff > 0) {
        x_scroll = left_diff;
        /*Do not let scrolling in*/
        int32_t sl = lv_obj_get_scroll_left(parent);
        if(sl - x_scroll < 0) x_scroll = 0;
    } else if(right_diff > 0) {
        x_scroll = -right_diff;
        /*Do not let scrolling in*/
        int32_t sr = lv_obj_get_scroll_right(parent);
        if(sr + x_scroll < 0) x_scroll = 0;
    }

    int32_t parent_w = lv_obj_get_width(parent) - sleft - sright;
    switch(snap_x) {
        case LV_SCROLL_SNAP_START:
            snap_goal = parent->coords.x1 + sleft;
            act       = area_tmp->x1 + x_scroll;
            x_scroll += snap_goal - act;
            break;
        case LV_SCROLL_SNAP_END:
            snap_goal = parent->coords.x2 - sright;
            act       = area_tmp->x2 + x_scroll;
            x_scroll += snap_goal - act;
            break;
        case LV_SCROLL_SNAP_CENTER:
            snap_goal = parent->coords.x1 + sleft + parent_w / 2;
            act       = lv_area_get_width(area_tmp) / 2 + area_tmp->x1 + x_scroll;
            x_scroll += snap_goal - act;
            break;
        case LV_SCROLL_SNAP_NONE: break;
    }

    /*Remove any pending scroll animations.*/
    lv_anim_delete(parent, scroll_y_anim);
    lv_anim_delete(parent, scroll_x_anim);

    if((scroll_dir & LV_DIR_LEFT) == 0 && x_scroll < 0) x_scroll = 0;
    if((scroll_dir & LV_DIR_RIGHT) == 0 && x_scroll > 0) x_scroll = 0;
    if((scroll_dir & LV_DIR_TOP) == 0 && y_scroll < 0) y_scroll = 0;
    if((scroll_dir & LV_DIR_BOTTOM) == 0 && y_scroll > 0) y_scroll = 0;

    scroll_value->x += anim_en == LV_ANIM_OFF ? 0 : x_scroll;
    scroll_value->y += anim_en == LV_ANIM_OFF ? 0 : y_scroll;
    pgs_lv_obj_scroll_by(parent, x_scroll, y_scroll, anim_en);
}

static void pgs_lv_obj_scroll_to_view(lv_obj_t * obj, lv_anim_enable_t anim_en)
{
    /*Be sure the screens layout is correct*/
    lv_obj_update_layout(obj);

    lv_point_t p = {0, 0};
    pgs_scroll_area_into_view(&obj->coords, obj, &p, anim_en);
}

static void keyroll_key_focused_event_cb(lv_event_t * event)
{
    lv_obj_t * cur_key   = lv_event_get_current_target(event);
    lv_obj_t * container = lv_obj_get_parent(cur_key);
    uint32_t keycount    = lv_obj_get_child_cnt(container);
    uint32_t mid_key_idx = (keycount - 1) / 2;
    uint32_t cur_key_idx = lv_obj_get_index(cur_key);

    if(cur_key_idx > mid_key_idx) {
        pgs_lv_obj_scroll_to_view(lv_obj_get_child(container, mid_key_idx), LV_ANIM_OFF);
        pgs_lv_obj_scroll_to_view(lv_obj_get_child(container, mid_key_idx + 1), LV_ANIM_ON);
        lv_obj_move_to_index(lv_obj_get_child(container, 0), -1);
    }
}

struct pgs_widgets_keyroll * pgs_widgets_keyroll_create(lv_obj_t * obj, const char * base,
                                                        struct pgs_widgets_params_keyroll * keyroll)
{
    if(!base) {
        return NULL;
    }
    if(!keyroll || !keyroll->enable) {
        return NULL;
    }

    struct pgs_widgets_keyroll * target = lv_malloc(sizeof(struct pgs_widgets_keyroll));
    if(!target) {
        return NULL;
    }

    lv_memzero(target, sizeof(struct pgs_widgets_keyroll));

    target->_keyroll = keyroll;

    /* keycap */
    lv_snprintf(keyboard_path_buffer, sizeof(keyboard_path_buffer), "%s/keycap.png", base);
    const lv_image_dsc_t * png_keycap = pgs_libpng_decode(keyboard_path_buffer);

    target->group = lv_group_create();

    target->container = lv_obj_create(obj);
    lv_obj_remove_style_all(target->container);
    lv_obj_set_width(target->container, 300);
    lv_obj_set_height(target->container, 144);
    lv_obj_set_align(target->container, keyroll->align);
    lv_obj_set_style_opa(target->container, keyroll->opa, 0);
    lv_obj_set_scrollbar_mode(target->container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_x(target->container, LV_SCROLL_SNAP_START);
    lv_obj_set_style_bg_opa(target->container, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(target->container, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(target->container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(target->container, 12, LV_PART_MAIN);
    lv_obj_set_x(target->container, keyroll->x);
    lv_obj_set_y(target->container, keyroll->y);
    lv_obj_set_flex_flow(target->container, LV_FLEX_FLOW_ROW_REVERSE);
    lv_obj_set_flex_align(target->container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END);

    for(uint8_t i = 0; i < (sizeof(target->keys) / sizeof(lv_obj_t *)); i++) {
        target->keys[i] = lv_button_create(target->container);
        lv_obj_remove_style_all(target->keys[i]);
        lv_obj_set_width(target->keys[i], 144);
        lv_obj_set_height(target->keys[i], 144);
        lv_obj_set_style_opa(target->keys[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(target->keys[i], lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(target->keys[i], 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(target->keys[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(target->keys[i], 12, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t * label_keycap_top = lv_label_create(target->keys[i]);
        lv_obj_set_width(label_keycap_top, 36);
        lv_obj_set_height(label_keycap_top, 36);
        lv_obj_set_x(label_keycap_top, 33);
        lv_obj_set_y(label_keycap_top, 25);
        lv_label_set_text_fmt(label_keycap_top, " ");
        lv_obj_set_style_text_color(label_keycap_top, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_keycap_top, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(label_keycap_top, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_keycap_top, &lv_font_helveticarounded_32, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t * label_keycap_mid = lv_label_create(target->keys[i]);
        lv_obj_set_width(label_keycap_mid, 82);
        lv_obj_set_height(label_keycap_mid, 24);
        lv_obj_set_x(label_keycap_mid, 30);
        lv_obj_set_y(label_keycap_mid, 55);
        lv_label_set_long_mode(label_keycap_mid, LV_LABEL_LONG_CLIP);
        lv_label_set_text(label_keycap_mid, " ");
        lv_obj_set_style_text_color(label_keycap_mid, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_keycap_mid, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(label_keycap_mid, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_keycap_mid, &lv_font_helveticarounded_28, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t * label_keycap_btm = lv_label_create(target->keys[i]);
        lv_obj_set_width(label_keycap_btm, 36);
        lv_obj_set_height(label_keycap_btm, 36);
        lv_obj_set_x(label_keycap_btm, 33);
        lv_obj_set_y(label_keycap_btm, 76);
        lv_label_set_text_fmt(label_keycap_btm, " ");
        lv_obj_set_style_text_color(label_keycap_btm, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_keycap_btm, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(label_keycap_btm, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_keycap_btm, &lv_font_helveticarounded_32, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t * label_keycap_sgl = lv_label_create(target->keys[i]);
        lv_obj_set_width(label_keycap_sgl, 36);
        lv_obj_set_height(label_keycap_sgl, 36);
        lv_obj_set_x(label_keycap_sgl, 33);
        lv_obj_set_y(label_keycap_sgl, 25);
        lv_label_set_text_fmt(label_keycap_sgl, " ");
        lv_obj_set_style_text_color(label_keycap_sgl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_keycap_sgl, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(label_keycap_sgl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_keycap_sgl, &lv_font_helveticarounded_32, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t * image_keycap = lv_image_create(target->keys[i]);
        if(png_keycap) {
            lv_image_set_src(image_keycap, png_keycap);
        }
        lv_obj_set_width(image_keycap, LV_SIZE_CONTENT);
        lv_obj_set_height(image_keycap, LV_SIZE_CONTENT);
        lv_obj_set_align(image_keycap, LV_ALIGN_CENTER);
        lv_obj_remove_flag(image_keycap, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }

    for(uint8_t i = 0; i < (sizeof(target->keys) / sizeof(lv_obj_t *)); i++) {
        lv_obj_clear_state(target->keys[i], LV_STATE_FOCUS_KEY);
        lv_group_add_obj(target->group, target->keys[i]);
    }

    lv_group_focus_obj(target->keys[2]);
    lv_obj_add_state(target->keys[2], LV_STATE_FOCUS_KEY);

    for(uint8_t i = 0; i < (sizeof(target->keys) / sizeof(lv_obj_t *)); i++) {
        lv_obj_add_event_cb(target->keys[i], keyroll_key_focused_event_cb, LV_EVENT_FOCUSED, NULL);
    }

    for(uint8_t i = 0; i < (sizeof(target->keys) / sizeof(lv_obj_t *)) + 1; i++) {
        lv_group_set_editing(target->group, false);
        lv_group_focus_next(target->group);
    }

    return target;
}

void pgs_widgets_keyroll_push(struct pgs_widgets_keyroll * keyroll, uint32_t keycode, uint32_t keycolor)
{
    if(!keyroll || !keyroll->_keyroll->enable) {
        return;
    }

    struct keycode_param * _keycode = keycode_param_get(keycode);
    struct keycap_color * _keycolor = keycap_color_get_by_index(keycolor);

    uint32_t keycount     = lv_obj_get_child_cnt(keyroll->container);
    lv_obj_t * key        = lv_obj_get_child(keyroll->container, 0);
    lv_obj_t * keycap_top = lv_obj_get_child(key, 0);
    lv_obj_t * keycap_mid = lv_obj_get_child(key, 1);
    lv_obj_t * keycap_btm = lv_obj_get_child(key, 2);
    lv_obj_t * keycap_sgl = lv_obj_get_child(key, 3);

    lv_label_set_text_fmt(keycap_top, "%s", _keycode->top);
    lv_label_set_text_fmt(keycap_mid, "%s", _keycode->mid);
    lv_label_set_text_fmt(keycap_btm, "%s", _keycode->btm);
    lv_label_set_text_fmt(keycap_sgl, "%s", _keycode->sgl);

    lv_obj_set_style_bg_color(key, lv_color_hex(_keycolor->color), LV_PART_MAIN | LV_STATE_DEFAULT);

    for(uint8_t i = 0; i < keycount; i++) {
        lv_obj_t * ckey = lv_obj_get_child(keyroll->container, i);
        if((255 != lv_obj_get_style_opa(ckey, LV_PART_MAIN | LV_STATE_DEFAULT)) &&
           !lv_color_eq(lv_obj_get_style_bg_color(ckey, LV_PART_MAIN | LV_STATE_DEFAULT), lv_color_hex(0x000000))) {
            lv_obj_set_style_opa(ckey, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    lv_group_set_editing(keyroll->group, false);
    lv_group_focus_next(keyroll->group);
}
