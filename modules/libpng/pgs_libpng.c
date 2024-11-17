#include "pgs_modules.h"

#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <png.h>

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

const lv_image_dsc_t * pgs_libpng_decode(const char * path)
{
    int ret;
    uint8_t * png_data;
    uint32_t png_data_size;
    png_image image;
    lv_image_dsc_t * img;

    lv_memzero(&image, sizeof(image));
    image.version = PNG_IMAGE_VERSION;

    img = lv_malloc(sizeof(lv_image_dsc_t));
    if(!img) {
        LV_LOG_ERROR("png alloc dsc failed");
        return NULL;
    }
    lv_memzero(img, sizeof(lv_image_dsc_t));

    png_data = alloc_file(path, &png_data_size);
    if(png_data == NULL) {
        goto error1;
    }

    ret = png_image_begin_read_from_memory(&image, png_data, png_data_size);
    if(!ret) {
        LV_LOG_ERROR("png read failed: %d", ret);
        goto error2;
    }

    image.format = PNG_FORMAT_BGRA;

    png_bytep buffer = lv_malloc(PNG_IMAGE_SIZE(image));
    if(!buffer) {
        LV_LOG_ERROR("png alloc buffer failed");
        goto error3;
    }

    if(!png_image_finish_read(&image, NULL, buffer, 0, NULL)) {
        goto error4;
    }

    img->header.cf    = LV_COLOR_FORMAT_ARGB8888;
    img->header.magic = LV_IMAGE_HEADER_MAGIC;
    img->header.w     = image.width;
    img->header.h     = image.height;
    img->data_size    = PNG_IMAGE_SIZE(image);
    img->data         = buffer;

    png_image_free(&image);
    lv_free(png_data);

    return img;

error4:
    lv_free(buffer);

error3:
    png_image_free(&image);

error2:
    lv_free(png_data);

error1:
    lv_free(img);

    return NULL;
}

void pgs_libpng_free(const lv_image_dsc_t * img)
{
    if(!img) {
        return;
    }

    if(img->data) {
        lv_free(img->data);
    }

    lv_free(img);
}
