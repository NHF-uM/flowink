#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include "epd.h"
#include "svc_bmp.h"

LOG_MODULE_REGISTER(svc_bmp);

static const uint8_t bmp[] = {
#include "test.bmp.inc"
}

void test_bmp(void)
{
    epd_init();

    uint8_t *bmp_decoded = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, EPD_SIZE_BYTE);

    bmp_decode_to_epd(bmp, bmp_decoded, false);

    epd_fill_image(bmp_decoded);

    epd_sleep();

    k_sleep(K_MINUTES(3));

    epd_reset();
    epd_fill_color(EPD_COLOR_WHITE);
    epd_sleep();

    return 0;
}