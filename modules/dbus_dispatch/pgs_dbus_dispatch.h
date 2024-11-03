#ifndef __PGS_DBUS_DISPATCH_H__
#define __PGS_DBUS_DISPATCH_H__

#include "pgs_modules.h"

int pgs_get_lvgl_foreground(void);
void pgs_wait_become_foreground(void);
void pgs_dispatch_dbus_message(int data);

#endif
