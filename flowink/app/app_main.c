#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <zephyr/logging/log.h>
#include "pwr_manage.h"
#include "led.h"
#include "app_btn.h"
#include "rgb_strip.h"
#include "svc_bmp.h"
#include "epd.h"
#include "net.h"

LOG_MODULE_REGISTER(main);

static const uint8_t bmp[] = {
#include "test.bmp.inc"
};

int main(void)
{
    pwr_init();
    led_init();
    led_set(led_pwr, true, K_FOREVER);

    wakeup_source_t wake_cause = pwr_get_wakeup_cause();

    epd_init();

    if (wake_cause == WAKEUP_TIMER)
    {
    }
    else
    {
        btn_init();

        while (1)
        {
            /* 单击切换模式（仅置位模式），长按确定选择（具体模式和确定选择都置位） */
            uint32_t flags = k_event_wait(&btn_mode_event, BTN_BIT_MODE_ALL, true, K_FOREVER);
            if (flags & BTN_BIT_MODE_SELECTED)
            {
                led_set(led_mode, false, K_NO_WAIT);

                if (flags & BTN_BIT_MODE_BASIC)
                {
                    LOG_DBG("Basic mode selected");
                    uint8_t *data_epd = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, 119200);
                    bmp_decode_to_epd(bmp, data_epd, false);
                    epd_fill_image(data_epd);
                    epd_sleep();
                    shared_multi_heap_free(data_epd);
                }
                else if (flags & BTN_BIT_MODE_SERVER)
                {
                    LOG_DBG("Server mode selected");
                    wifi_init();
                    http_server_start();
                    extern void http_set_revc_buf(uint8_t *bmp_buf);
                    extern struct k_sem sem_http_data_uping;
                    uint8_t *data_bmp = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, 1152054);
                    uint8_t *data_epd = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, 119200);
                    http_set_revc_buf(data_bmp);
                    k_sem_take(sem_http_data_uping, K_FOREVER);
                    k_sem_give(sem_http_data_uping);
                    bmp_decode_to_epd(data_bmp, data_epd, false);
                    epd_fill_image(data_epd);

                    shared_multi_heap_free(data_bmp);
                    shared_multi_heap_free(data_epd);
                    epd_sleep();
                }
            }
            else
            {
                if (flags & BTN_BIT_MODE_BASIC)
                {
                    LOG_DBG("Basic mode led invoked");
                    led_set(led_mode, true, 200);
                }
                else if (flags & BTN_BIT_MODE_SERVER)
                {
                    LOG_DBG("Server mode led invoked");
                    led_set(led_mode, true, 1000);
                }
            }
        }
    }

    return 0;
}

void rgb_strip_thread_entry(void)
{
    while (1)
    {
        rgb_strip_on(BLUE);
        k_sleep(K_SECONDS(1));
        rgb_strip_on(RED);
        k_sleep(K_SECONDS(1));
    }
}

K_THREAD_DEFINE(rgb_strip_thread, 1024, rgb_strip_thread_entry, NULL, NULL, NULL,
                7, 0, 0);
