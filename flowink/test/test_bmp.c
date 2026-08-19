#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include "epd.h"
#include "svc_bmp.h"
#include "rgb_strip.h"

LOG_MODULE_REGISTER(test_bmp, LOG_LEVEL_DBG);

// static const uint8_t bmp[] = {
// #include "test.bmp.inc"
// };

void test_bmp(void)
{
    epd_init();
    k_sleep(K_SECONDS(1));

    uint8_t *bmp_decoded = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, EPD_SIZE_BYTE);
    if (bmp_decoded == NULL)
    {
        LOG_ERR("Failed to allocate memory for bmp_decoded");
        return;
    }
    memset(bmp_decoded, 0xFF, EPD_SIZE_BYTE); 

    // bmp_decode_to_epd(bmp, bmp_decoded, false);

    epd_fill_image(bmp_decoded);
    k_sleep(K_SECONDS(5));
    epd_sleep();

    k_sleep(K_MINUTES(3));

    epd_reset();
    epd_fill_color(EPD_COLOR_WHITE);
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