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

#define RETAIN_MAGIC 0xA55AA55A // 魔法校验值，区分冷启动/休眠唤醒
#define RETAIN_OFFSET_MAGIC 0
#define RETAIN_OFFSET_WAKE_CNT 4

#define EPD_REFLUSH(_reflush_func)            \
    do                                        \
    {                                         \
        epd_reset();                          \
        led_set(led_pwr, true, K_MSEC(500));  \
        _reflush_func;                        \
        epd_sleep();                          \
        led_set(led_pwr, true, K_FORVERY); \
    } while (0)

// static const uint8_t bmp[] = {
// #include "test.bmp.inc"
// };

/*
 * [00:01:06.785,000] <dbg> http_server: data_up_handler: data has been received (1138786 bytes)
[00:01:06.810,000] <dbg> http_server: data_up_handler: data has been received (1147034 bytes)
[00:01:06.838,000] <dbg> http_server: data_up_handler: picture received succ (1152054 bytes).
[00:01:06.839,000] <dbg> http_server: data_up_handler: Transmission completed, including response
[00:01:06.839,000] <dbg> svc_bmp: bmp_decode_to_epd: BMP header: type=0x4D42, bitcount=24, offset=54
[00:01:06.949,000] <dbg> svc_bmp: bmp_decode_to_epd: bmp decode to epd finish, rotate_180=0
[00:01:06.949,000] <err> os_heap: heap corruption (buffer overflow?) at 0x3c1bc2b8
[00:01:06.949,000] <err> os:  ** FATAL EXCEPTION
[00:01:06.949,000] <err> os:  ** CPU 0 EXCCAUSE 63 (zephyr exception)
[00:01:06.949,000] <err> os:  **  PC 0x403783db VADDR 0
[00:01:06.949,000] <err> os:  **  PS 0x60a20
[00:01:06.949,000] <err> os:  **    (INTLEVEL:0 EXCM: 0 UM:1 RING:0 WOE:1 OWB:10 CALLINC:2)
[00:01:06.949,000] <err> os:  **  A0 0x820086a8  SP 0x3fc9dff0  A2 0x4  A3 0x1840
[00:01:06.949,000] <err> os:  **  A4 0x3fc9dff0  A5 0  A6 0x3fcac680  A7 0x3fc9df80
[00:01:06.949,000] <err> os:  **  A8 0x8037c478  A9 0x3fc9df60 A10 0x3fcac680 A11 0x3fc9923c
[00:01:06.949,000] <err> os:  ** A12 0x1840 A13 0 A14 0xc A15 0x3fc9def0
[00:01:06.949,000] <err> os:  ** LBEG 0x40056f5c LEND 0x40056f72 LCOUNT 0xffffffff
[00:01:06.949,000] <err> os:  ** SAR 0x4
[00:01:06.949,000] <err> os:  **  THREADPTR 0x10
[00:01:06.949,000] <err> os: >>> ZEPHYR FATAL ERROR 4: Kernel panic on CPU 0
[00:01:06.949,000] <err> os: Current thread: 0x3fcaf138 (unknown)
[00:01:07.134,000] <err> os: Halting system
 */
int main(void)
{
    pwr_init();
    led_init();
    led_set(led_pwr, true, K_FOREVER);

    wakeup_source_t wake_cause = pwr_get_wakeup_cause();

    epd_init();

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
                }
                else if (flags & BTN_BIT_MODE_SERVER)
                {
                    LOG_DBG("Server mode selected");

                    wifi_init();
                    http_server_start();
                    k_sleep(K_SECONDS(2));

                    extern void wifi_deinit(void);
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
                        shared_multi_heap_free(data_bmp);
                        continue;
                    }
                    k_sem_take(&sem_http_data_uping, K_FOREVER);
                    k_sem_give(&sem_http_data_uping);
                    bmp_decode_to_epd(data_bmp, data_epd, false);
                    EPD_REFLUSH(epd_fill_image(data_epd));

                    shared_multi_heap_free(data_bmp);
                    shared_multi_heap_free(data_epd);

                    http_server_stop();
                    wifi_deinit();
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
