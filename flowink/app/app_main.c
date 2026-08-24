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
        led_set(led_pwr, true, K_MSEC(500)); \
        _reflush_func;                       \
        led_set(led_pwr, true, K_FOREVER);   \
    } while (0)

int main(void)
{
    pwr_init();
    led_init();
    led_set(led_pwr, true, K_FOREVER);

    wakeup_source_t wake_cause = pwr_get_wakeup_cause();

    epd_init();

    {
        epd_show_color(EPD_COLOR_WHITE);
        k_sleep(K_SECONDS(1));
        epd_sleep();
        return 0;
    }

    /* 这样能正常刷，为什么下面的不行 */
    {
        test_bmp_play_pic();
        k_sleep(K_SECONDS(1));
        epd_sleep();
        return 0;
    }

    if (wake_cause == WAKEUP_TIMER)
    {
        /* 遍历tf卡，生成双链表 */
        /* 读取config.txt，得到轮播时间和循环与否，也读起始路径和起始文件名，不然后面不存在的话怎么办 */
        /* 读取retained mem记录的路径 */
        /* 检查tf卡是否还存在上述文件 */
        /* 若路径和文件存在，则读取并刷图下一张 */ /* 否则，从起始路径和文件名开始 */
        /* 保存新的路径和文件 */
        /* 休眠 */

        /* 提供两个exe：一个用来乱序并且排序号，一个用来展示和拖拽图片，最后排序号  */
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
                    EPD_REFLUSH(test_bmp_play_pic());
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
                    uint8_t *data_bmp = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, BMP_ORIGINAL_SIZE);
                    if (data_bmp == NULL)
                    {
                        LOG_ERR("Failed to allocate memory for data_bmp");
                        continue;
                    }
                    http_set_revc_buf(data_bmp);
                    uint8_t *data_epd = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, EPD_DATA_SIZE);
                    if (data_epd == NULL)
                    {
                        LOG_ERR("Failed to allocate memory for data_epd");
                        continue;
                    }
                    LOG_DBG("Waiting for data upload to complete...");
                    k_sem_take(&sem_http_data_uping, K_FOREVER);
                    k_sem_give(&sem_http_data_uping);
                    bmp_decode_to_epd(data_bmp, data_epd, true);
                    EPD_REFLUSH(epd_show_image(data_epd));

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
