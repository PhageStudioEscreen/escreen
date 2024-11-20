#include "pgs_widgets.h"

static void keyroll_key_focused_event_cb(lv_event_t * event)
{
    lv_obj_t * cur_key   = lv_event_get_current_target(event);
    lv_obj_t * container = lv_obj_get_parent(cur_key);
    uint32_t keycount    = lv_obj_get_child_cnt(container);
    uint32_t mid_key_idx = (keycount - 1) / 2;
    uint32_t cur_key_idx = lv_obj_get_index(cur_key);

    if(cur_key_idx > mid_key_idx) {
        lv_obj_scroll_to_view(lv_obj_get_child(container, mid_key_idx), LV_ANIM_OFF);
        lv_obj_scroll_to_view(lv_obj_get_child(container, mid_key_idx + 1), LV_ANIM_ON);
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
    lv_obj_set_width(target->container, 197);
    lv_obj_set_height(target->container, 96);
    lv_obj_set_align(target->container, keyroll->align);
    lv_obj_set_style_opa(target->container, keyroll->opa, 0);
    lv_obj_set_scrollbar_mode(target->container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_x(target->container, LV_SCROLL_SNAP_START);
    lv_obj_set_style_bg_opa(target->container, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(target->container, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(target->container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(target->container, 5, LV_PART_MAIN);
    lv_obj_set_x(target->container, keyroll->x);
    lv_obj_set_y(target->container, keyroll->y);
    lv_obj_set_flex_flow(target->container, LV_FLEX_FLOW_ROW_REVERSE);
    lv_obj_set_flex_align(target->container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END);

    for(uint8_t i = 0; i < (sizeof(target->keys) / sizeof(lv_obj_t *)); i++) {
        target->keys[i] = lv_button_create(target->container);
        lv_obj_remove_style_all(target->keys[i]);
        lv_obj_set_width(target->keys[i], 96);
        lv_obj_set_height(target->keys[i], 96);
        lv_obj_set_style_opa(target->keys[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(target->keys[i], lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(target->keys[i], 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(target->keys[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(target->keys[i], 8, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t * label_keycap_top = lv_label_create(target->keys[i]);
        lv_obj_set_width(label_keycap_top, 24);
        lv_obj_set_height(label_keycap_top, 24);
        lv_obj_set_x(label_keycap_top, 23);
        lv_obj_set_y(label_keycap_top, 15);
        lv_label_set_text_fmt(label_keycap_top, " ");
        lv_obj_set_style_text_color(label_keycap_top, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_keycap_top, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(label_keycap_top, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_keycap_top, &lv_font_helveticarounded_24, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t * label_keycap_mid = lv_label_create(target->keys[i]);
        lv_obj_set_width(label_keycap_mid, 58);
        lv_obj_set_height(label_keycap_mid, 20);
        lv_obj_set_x(label_keycap_mid, 20);
        lv_obj_set_y(label_keycap_mid, 34);
        lv_label_set_long_mode(label_keycap_mid, LV_LABEL_LONG_CLIP);
        lv_label_set_text(label_keycap_mid, " ");
        lv_obj_set_style_text_color(label_keycap_mid, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_keycap_mid, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(label_keycap_mid, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_keycap_mid, &lv_font_helveticarounded_24, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t * label_keycap_btm = lv_label_create(target->keys[i]);
        lv_obj_set_width(label_keycap_btm, 24);
        lv_obj_set_height(label_keycap_btm, 24);
        lv_obj_set_x(label_keycap_btm, 23);
        lv_obj_set_y(label_keycap_btm, 50);
        lv_label_set_text_fmt(label_keycap_btm, " ");
        lv_obj_set_style_text_color(label_keycap_btm, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_keycap_btm, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(label_keycap_btm, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_keycap_btm, &lv_font_helveticarounded_24, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t * label_keycap_sgl = lv_label_create(target->keys[i]);
        lv_obj_set_width(label_keycap_sgl, 24);
        lv_obj_set_height(label_keycap_sgl, 24);
        lv_obj_set_x(label_keycap_sgl, 23);
        lv_obj_set_y(label_keycap_sgl, 15);
        lv_label_set_text_fmt(label_keycap_sgl, " ");
        lv_obj_set_style_text_color(label_keycap_sgl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_keycap_sgl, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(label_keycap_sgl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label_keycap_sgl, &lv_font_helveticarounded_28, LV_PART_MAIN | LV_STATE_DEFAULT);

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
    if(!keyroll) {
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
