#ifndef __PGS_LIBPNG_H__
#define __PGS_LIBPNG_H__

#include <stdbool.h>
#include "pgs_modules.h"
#include "lvgl/lvgl.h"

const lv_image_dsc_t * pgs_libpng_decode(const char * path);
void pgs_libpng_free(const lv_image_dsc_t * image);

#endif
