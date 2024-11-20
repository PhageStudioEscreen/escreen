#ifndef __PGS_UTILS_H__
#define __PGS_UTILS_H__

#include "lvgl/lvgl.h"

#define PGS_DBUS_NAME_PREFIX "com.pgsapp."
#define PGS_DBUS_PATH_PREFIX "/com/pgsapp/"

#define PGS_DBUS_MENU "menu"
#define PGS_DBUS_MENU_NAME PGS_DBUS_NAME_PREFIX PGS_DBUS_MENU
#define PGS_DBUS_MENU_PATH PGS_DBUS_PATH_PREFIX PGS_DBUS_MENU

LV_FONT_DECLARE(lv_font_helveticarounded_8);
LV_FONT_DECLARE(lv_font_helveticarounded_10);
LV_FONT_DECLARE(lv_font_helveticarounded_12);
LV_FONT_DECLARE(lv_font_helveticarounded_14);
LV_FONT_DECLARE(lv_font_helveticarounded_16);
LV_FONT_DECLARE(lv_font_helveticarounded_18);
LV_FONT_DECLARE(lv_font_helveticarounded_20);
LV_FONT_DECLARE(lv_font_helveticarounded_22);
LV_FONT_DECLARE(lv_font_helveticarounded_24);
LV_FONT_DECLARE(lv_font_helveticarounded_26);
LV_FONT_DECLARE(lv_font_helveticarounded_28);
LV_FONT_DECLARE(lv_font_helveticarounded_30);
LV_FONT_DECLARE(lv_font_helveticarounded_32);

void pgs_lvgl_init(const char * name);
const char * pgs_get_name(void);
const char * pgs_get_path(void);
lv_display_t * pgs_get_display(void);
lv_indev_t * pgs_get_keyboard(void);
lv_indev_t * pgs_get_mouse(void);
void pgs_lvgl_resume(void);
void pgs_lvgl_suspend(void);
void pgs_cleanup(void);

#endif
