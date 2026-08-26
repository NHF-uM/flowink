#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/logging/log.h>
#include "pwr_manage.h"
#include "led.h"
#include "btn.h"
#include "rgb_strip.h"
#include "svc_bmp.h"
#include "epd.h"
#include "net.h"
#include "tf.h"
#include "nv.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static void timer_enter_sleep_fn(struct k_timer *timer);
K_TIMER_DEFINE(timer_enter_sleep, timer_enter_sleep_fn, NULL);

static const uint8_t bmp[] = {
#include "build_in.bmp.inc"
};

static uint8_t *data_bmp;
static uint8_t *data_epd;

/**
 * @brief 解码失败就退出了，不刷内置图？ 
 * @param bmp_buf 
 */
void bmp_decode_and_show_with_led(const uint8_t *bmp_buf)
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

/**
 * @brief 刷传入路径图片——>刷 start_file_path ——>刷能找到的第一张图片——>刷内置图片
 * @param path_target 图片路径，传入 NULL 则直接从 start_file_path 开始流程
 */
static void play_bmp(const char *path_target)
{
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

show_build_in_bmp:
    bmp_decode_and_show_with_led(bmp);
    nv_break_magic();
    return;

bmp_ok:
    bmp_decode_and_show_with_led(data_bmp);
    nv_write_path(path);
    return;
}

static void mode_basic_handler(bool is_tf_init_failure);
static void mode_server_handler(void);

/* 提供两个exe：一个用来乱序并且排序号，一个用来展示和拖拽图片，最后排序号  */
/* 刷完图之后马上进入休眠（未实现）不然两个data 会和后续操作冲突 */
/* 深度休眠会清空所有 RAM 数据和 PSRAM 数据*/
int main(void)
{
    pwr_init();
    led_init();
    led_set(led_pwr, true, K_FOREVER);
    epd_init();
    int ret = tf_init(false);
    if (ret)
    {
        LOG_WRN("Failed to initialize TF, related functions will be disabled.");
    }

    /* 两个唤醒模式只能运行一个，且运行完就会进入深度休眠，唤醒后从 main 函数重新开始运行*/
    wakeup_source_t wake_cause = pwr_get_wakeup_cause();
    if (wake_cause == WAKEUP_TIMER)
    {
        data_bmp = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, CONFIG_BMP_ORIGINAL_SIZE);
        data_epd = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, CONFIG_EPD_SEND_BUF_SIZE);
        if (data_bmp == NULL || data_epd == NULL)
        {
            LOG_ERR("Failed to allocate memory for data_epd");
            if (data_bmp)
                shared_multi_heap_free(data_bmp);
            if (data_epd)
                shared_multi_heap_free(data_epd);
            return -1;
        }
        memset(data_bmp, 0xFF, CONFIG_BMP_ORIGINAL_SIZE);
        memset(data_epd, 0xFF, CONFIG_EPD_SEND_BUF_SIZE);

        if (ret)
        {
            bmp_decode_and_show_with_led(bmp)

            goto sleep;
        }

        /* 读取nv的记录，播放下一张，如果找不到，就刷start_file，start_file 找不到就刷第一张，第一张没有就刷内置图片*/
        char *path = nv_read_path();
        if (path != NULL)
        {
            /* 每次唤醒都会刷新链表，只有在上次休眠之后又删除了图片才会返回 NULL，复用 path 为 next_path */
            char *next_path = tf_find_next_bmp(path); /* real_path 可能不存在*/

            play_bmp(next_path);
            nv_free_path(path);
        }
        else
        {
            /* nv读取失败或者路径无效，播放 start */
            play_bmp(NULL);
        }

    sleep:
        shared_multi_heap_free(data_bmp);
        shared_multi_heap_free(data_epd);

        /* 立即休眠 */
        pwr_set_sleep_timer_wakeup(tf_get_carousel_interval());
        pwr_enter_sleep();
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
                k_timer_stop(&timer_enter_sleep);
                led_set(led_mode, false, K_NO_WAIT);

                data_bmp = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, CONFIG_BMP_ORIGINAL_SIZE);
                data_epd = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, CONFIG_EPD_SEND_BUF_SIZE);
                if (data_bmp == NULL || data_epd == NULL)
                {
                    LOG_ERR("Failed to allocate memory for data_epd");
                    if (data_bmp)
                        shared_multi_heap_free(data_bmp);
                    if (data_epd)
                        shared_multi_heap_free(data_epd);
                    return -1;
                }
                memset(data_bmp, 0xFF, CONFIG_BMP_ORIGINAL_SIZE);
                memset(data_epd, 0xFF, CONFIG_EPD_SEND_BUF_SIZE);

                if (flags & BTN_BIT_MODE_BASIC)
                {
                    mode_basic_handler(ret);
                }
                else if (flags & BTN_BIT_MODE_SERVER)
                {
                    mode_server_handler();
                }

                shared_multi_heap_free(data_bmp);
                shared_multi_heap_free(data_epd);
                data_bmp = NULL;
                data_epd = NULL;
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

static void timer_enter_sleep_fn(struct k_timer *timer)
{
    tf_deinit();
    pwr_enter_sleep();
}

static void mode_basic_handler(bool is_tf_init_failure)
{
    LOG_DBG("Basic mode selected");

    if (is_tf_init_failure)
    {
        bmp_decode_and_show_with_led(bmp)
        return;
    }

    play_bmp(NULL);

    pwr_set_sleep_timer_wakeup(180);
    // pwr_set_sleep_timer_wakeup(tf_get_carousel_interval());
    // k_timer_start(&timer_enter_sleep, K_MINUTES(5), K_NO_WAIT);
    pwr_enter_sleep();
    LOG_DBG("Basic mode finished, enter deep sleep after 5 minutes");
}

static void mode_server_handler(void)
{
    LOG_DBG("Server mode selected");

    wifi_init();
    http_server_start();
    http_set_revc_buf(data_bmp);

    /* 死等就好了，用户如果不想再上传就进深休再唤醒 */
    LOG_DBG("Waiting for data upload to complete...");
    k_sem_take(&sem_http_data_uping, K_FOREVER);

    bmp_decode_and_show_with_led(data_bmp);

    LOG_DBG("Server mode finished, stopping HTTP server and deinitializing Wi-Fi...");
    http_server_stop();
    wifi_deinit1();

    pwr_set_sleep_timer_wakeup(180);
    // pwr_set_sleep_timer_wakeup(24 * 3600);
    // k_timer_start(&timer_enter_sleep, K_MINUTES(5), K_NO_WAIT);
    pwr_enter_sleep();
    LOG_DBG("Server mode finished, enter deep sleep after 5 minutes");
}
