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
#include "test.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define EPD_REFLUSH(_reflush_func)           \
    do                                       \
    {                                        \
        epd_reset();                         \
        led_set(led_pwr, true, K_MSEC(500)); \
        _reflush_func;                       \
        epd_sleep();                         \
        led_set(led_pwr, true, K_FOREVER);   \
    } while (0)

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
    
    {
        epd_fill_color(EPD_COLOR_WHITE);
        k_sleep(K_SECONDS(1));
        epd_sleep();
        return 0;
    }

    /* 这样能正常刷，为什么下面的不行 */
    {
        uint8_t *data_epd = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, EPD_SIZE_BYTE);
        if (data_epd == NULL)
        {
            LOG_ERR("Failed to allocate memory for data_epd");
            return -1;
        }
        bmp_decode_to_epd(bmp, data_epd, false);
        epd_fill_image(data_epd);
        k_sleep(K_SECONDS(5));
        epd_sleep();
        shared_multi_heap_free(data_epd);
        return 0;
    }

    if (wake_cause == WAKEUP_TIMER)
    {
        /* 遍历tf卡，生成双链表 */
        /* 读取config.txt */
        /* 检查retained mem记录的路径和文件是否还存在 */
        /* 轮播时间和循环与否根据配置文件，若路径和文件存在，则读取并刷图下一张 */
        /* 保存新的路径和文件 */
        /* 休眠 */
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
                    uint8_t *data_epd = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, 192000);
                    if (data_epd == NULL)
                    {
                        LOG_ERR("Failed to allocate memory for data_epd");
                        continue;
                    }
                    bmp_decode_to_epd(bmp, data_epd, false);
                    EPD_REFLUSH(epd_fill_image(data_epd));
                    shared_multi_heap_free(data_epd);
                    LOG_DBG("Basic mode finished");
                }
                else if (flags & BTN_BIT_MODE_SERVER)
                {
                    LOG_DBG("Server mode selected");

                    wifi_init();
                    http_server_start();

                    extern void wifi_deinit1(void);
                    extern void http_server_stop(void);
                    extern void http_set_revc_buf(uint8_t *bmp_buf);
                    extern struct k_sem sem_http_data_uping;
                    uint8_t *data_bmp = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, 1152054);
                    if (data_bmp == NULL)
                    {
                        LOG_ERR("Failed to allocate memory for data_bmp");
                        continue;
                    }
                    http_set_revc_buf(data_bmp);
                    uint8_t *data_epd = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, 192000);
                    if (data_epd == NULL)
                    {
                        LOG_ERR("Failed to allocate memory for data_epd");
                        continue;
                    }
                    LOG_DBG("Waiting for data upload to complete...");
                    k_sem_take(&sem_http_data_uping, K_FOREVER);
                    k_sem_give(&sem_http_data_uping);
                    bmp_decode_to_epd(data_bmp, data_epd, true);
                    EPD_REFLUSH(epd_fill_image(data_epd));

                    shared_multi_heap_free(data_bmp);
                    shared_multi_heap_free(data_epd);

                    LOG_DBG("Server mode finished, stopping HTTP server and deinitializing Wi-Fi...");
                    http_server_stop();
                    wifi_deinit1();

                    LOG_DBG("Server mode finished");
                }
            }
            else
            {
                if (flags & BTN_BIT_MODE_BASIC)
                {
                    LOG_DBG("Basic mode led invoked");
                    led_set(led_mode, true, K_MSEC(200));
                }
                else if (flags & BTN_BIT_MODE_SERVER)
                {
                    LOG_DBG("Server mode led invoked");
                    led_set(led_mode, true, K_MSEC(1000));
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
