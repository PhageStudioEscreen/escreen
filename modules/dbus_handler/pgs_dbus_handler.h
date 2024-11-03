#ifndef __PGS_DBUS_HANDLER_H__
#define __PGS_DBUS_HANDLER_H__

#include "pgs_modules.h"

void pgs_dbus_handler_init(const char * name, const char * path);
void pgs_dbus_method_call(const char * name, const char * path, const int state, const int pid);

#endif
