#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/logging/log.h>
#include "pwr_manage.h"
#include "led.h"
#include "app_btn.h"
#include "rgb_strip.h"
#include "svc_bmp.h"
#include "epd.h"
#include "net.h"
#include "app_tf.h"
#include "app_nv.h"

LOG_MODULE_REGISTER(test_bmp, LOG_LEVEL_DBG);

static const uint8_t bmp[] = {
#include "test.bmp.inc"
};

void test_bmp(void)
{
    epd_init();
    k_sleep(K_SECONDS(1));

    uint8_t *bmp_decoded = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, CONFIG_EPD_SEND_BUF_SIZE);
    if (bmp_decoded == NULL)
    {
        LOG_ERR("Failed to allocate memory for bmp_decoded");
        return;
    }
    memset(bmp_decoded, 0xFF, CONFIG_EPD_SEND_BUF_SIZE);

    bmp_decode_to_epd(bmp, bmp_decoded, false);

    epd_show_image(bmp_decoded);

    /* 测试休眠和唤醒 */
    k_sleep(K_SECONDS(5));
    epd_sleep();

    k_sleep(K_MINUTES(3));
    epd_reset();

    epd_show_color(EPD_COLOR_WHITE);
    epd_sleep();

    shared_multi_heap_free(bmp_decoded);

    while (1)
    {
        k_sleep(K_SECONDS(1));
        rgb_strip_on(BLUE);
        k_sleep(K_SECONDS(1));
        rgb_strip_on(RED);
        k_sleep(K_SECONDS(1));
        rgb_strip_on(GREEN);
        k_sleep(K_SECONDS(1));
    }
    return;
}

int test_bmp_01(void)
{
    epd_show_color(EPD_COLOR_WHITE);
    k_sleep(K_SECONDS(1));
    epd_sleep();
    return 0;
}

int test_bmp_02(void)
{
    uint8_t *data_bmp = NULL;
    uint8_t *data_epd = NULL;

    data_bmp = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, CONFIG_BMP_ORIGINAL_SIZE);
    data_epd = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, CONFIG_EPD_SEND_BUF_SIZE);

    if (data_bmp == NULL || data_epd == NULL)
    {
        LOG_ERR("alloc data_bmp or data_epd failed");
        if (data_bmp)
        {
            shared_multi_heap_free(data_bmp);
        }
        if (data_epd)
        {
            shared_multi_heap_free(data_epd);
        }
        return -1;
    }

    /* 内联原 bmp_decode_and_show_with_led 全部逻辑 */
    int ret = bmp_decode_to_epd(bmp, data_epd, true);
    if (ret != 0)
    {
        LOG_ERR("Failed to decode bmp");
        shared_multi_heap_free(data_bmp);
        shared_multi_heap_free(data_epd);
        return -1;
    }

    led_set(led_pwr, true, K_MSEC(500));
    epd_show_image(data_epd);
    led_set(led_pwr, true, K_FOREVER);

    k_sleep(K_SECONDS(1));
    epd_sleep();

    shared_multi_heap_free(data_bmp);
    shared_multi_heap_free(data_epd);
    return 0;
}

int test_bmp_03(void)
{
    uint8_t *data_bmp = NULL;
    uint8_t *data_epd = NULL;

    data_bmp = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, CONFIG_BMP_ORIGINAL_SIZE);
    data_epd = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, CONFIG_EPD_SEND_BUF_SIZE);

    if (data_bmp == NULL || data_epd == NULL)
    {
        LOG_ERR("Failed to allocate memory for data_epd");
        if (data_bmp)
        {
            shared_multi_heap_free(data_bmp);
        }
        if (data_epd)
        {
            shared_multi_heap_free(data_epd);
        }
        return -1;
    }

    memset(data_bmp, 0xFF, CONFIG_BMP_ORIGINAL_SIZE);
    memset(data_epd, 0xFF, CONFIG_EPD_SEND_BUF_SIZE);

    /* 第一张 1b5.bmp true */
    tf_read_bmp("/SD:/1b5.bmp", data_bmp);
    bmp_decode_to_epd(data_bmp, data_epd, true);
    epd_show_image(data_epd);
    k_sleep(K_MINUTES(3));

    /* 第一张 1b5.bmp false */
    tf_read_bmp("/SD:/1b5.bmp", data_bmp);
    bmp_decode_to_epd(data_bmp, data_epd, false);
    epd_show_image(data_epd);
    k_sleep(K_MINUTES(3));

    /* 第二张 45ff.bmp true */
    tf_read_bmp("/SD:/bbb/45ff.bmp", data_bmp);
    bmp_decode_to_epd(data_bmp, data_epd, true);
    epd_show_image(data_epd);
    k_sleep(K_MINUTES(3));

    /* 第二张 45ff.bmp false */
    tf_read_bmp("/SD:/bbb/45ff.bmp", data_bmp);
    bmp_decode_to_epd(data_bmp, data_epd, false);
    epd_show_image(data_epd);
    k_sleep(K_MINUTES(3));

    tf_deinit();

    shared_multi_heap_free(data_bmp);
    shared_multi_heap_free(data_epd);
    data_bmp = NULL;
    data_epd = NULL;

    epd_sleep();
    return 0;
}
