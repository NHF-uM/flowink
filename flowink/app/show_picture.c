#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include "led.h"
#include "tf.h"
#include "epd.h"
#include "nv.h"
#include "svc_bmp.h"

LOG_MODULE_REGISTER(show_picture, LOG_LEVEL_DBG);

static const uint8_t buildin_bmp[] = {
#include "build_in.bmp.inc"
};

static bool is_heap_ready;
static uint8_t *data_bmp;
static uint8_t *data_epd;

static void decode_show_with_led(const uint8_t *bmp_buf)
{
    int ret = bmp_decode_to_epd(bmp_buf, data_epd, true);
    if (ret != 0)
    {
        LOG_ERR("Failed to decode bmp");
        return;
    }

    led_set(led_pwr, true, K_MSEC(500));
    epd_show_image(data_epd);
    led_set(led_pwr, true, K_FOREVER);
}

int show_pic_malloc(uint8_t **ptr_bmp_buf)
{
    data_bmp = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, CONFIG_BMP_ORIGINAL_SIZE);
    data_epd = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, CONFIG_EPD_SEND_BUF_SIZE);
    if (data_bmp == NULL || data_epd == NULL)
    {
        LOG_ERR("Failed to allocate memory for show picture");
        if (data_bmp)
            shared_multi_heap_free(data_bmp);
        if (data_epd)
            shared_multi_heap_free(data_epd);
        return -1;
    }
    memset(data_bmp, 0xFF, CONFIG_BMP_ORIGINAL_SIZE);
    memset(data_epd, 0xFF, CONFIG_EPD_SEND_BUF_SIZE);

    if (ptr_bmp_buf != NULL)
    {
        *ptr_bmp_buf = data_bmp;
    }
    is_heap_ready = true;
    return 0;
}

void show_pic_free(void)
{
    shared_multi_heap_free(data_bmp);
    shared_multi_heap_free(data_epd);

    data_bmp = NULL;
    data_epd = NULL;

    is_heap_ready = false;
}

void show_pic_tf_bmp(const char *path_target)
{
    if (!is_heap_ready)
    {
        LOG_ERR("Heap of picture_show is not ready");
        return;
    }

    const char *path = NULL;
    int ret = -1;

    if (path_target != NULL)
    {
        ret = tf_read_bmp(path_target, data_bmp);
        if (ret == 0)
        {
            path = path_target;
            goto bmp_ok;
        }
    }

    path = tf_get_start_file_path();
    if (path != NULL)
    {
        ret = tf_read_bmp(path, data_bmp);
        if (ret == 0)
        {
            goto bmp_ok;
        }
    }

    path = tf_find_first_bmp();
    if (path != NULL)
    {
        ret = tf_read_bmp(path, data_bmp);
        if (ret == 0)
        {
            goto bmp_ok;
        }
    }

    decode_show_with_led(buildin_bmp);
    nv_break_magic();
    return;

bmp_ok:
    decode_show_with_led(data_bmp);
    nv_write_path(path);
    return;
}

void show_pic_server_bmp(void)
{
    if (!is_heap_ready)
    {
        LOG_ERR("Heap of picture_show is not ready");
        return;
    }

    decode_show_with_led(data_bmp);
}

void show_pic_buildin_bmp(void)
{
    if (!is_heap_ready)
    {
        LOG_ERR("Heap of picture_show is not ready");
        return;
    }

    decode_show_with_led(buildin_bmp);
    nv_break_magic();
}
