#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/logging/log.h>
#include "pwr_manage.h"
#include <esp_sleep.h>

LOG_MODULE_REGISTER(pwr_manage, LOG_LEVEL_DBG);

#define BTN_WAKEUP_NODE DT_NODELABEL(btn_wakeup)
static const struct gpio_dt_spec btn_wakeup_spec = GPIO_DT_SPEC_GET(BTN_WAKEUP_NODE, gpios);
static wakeup_source_t wakeup_cause;

static uint64_t wakeup_time_sec = CONFIG_WAKEUP_TIME_SEC;

/**
 * @brief 更新并保存深度休眠唤醒原因
 * @param  无
 */
static void pwr_update_wakeup_cause(void)
{
    uint32_t cause = esp_sleep_get_wakeup_causes();

    if (cause & BIT(ESP_SLEEP_WAKEUP_EXT1))
    {
        LOG_INF("CPU woken up by RTC_GPIO");
        wakeup_cause = WAKEUP_IO;
    }
    else if (cause & BIT(ESP_SLEEP_WAKEUP_TIMER))
    {
        LOG_INF("CPU woken up by Timer");
        wakeup_cause = WAKEUP_TIMER;
    }
    else
    {
        LOG_INF("CPU woken up by unknown wakeup source:%d", cause);
        wakeup_cause = WAKEUP_UNKNOWN;
    }
}

void pwr_init(void)
{
    pwr_update_wakeup_cause();

    if (!gpio_is_ready_dt(&btn_wakeup_spec))
    {
        LOG_ERR("btn_wakeup pin not ready \n");
        return;
    }

    // if (!device_is_ready(retained_mem_device)) {
    // 	LOG_ERR("retained_mem device is not ready!\n");
    // 	return 0;
    // }

    /* 在dts已经配置为中断唤醒引脚了，只需要再配置一下输入和中断触发 */
    int ret = gpio_pin_configure_dt(&btn_wakeup_spec, GPIO_INPUT);
    if (ret != 0)
    {
        LOG_ERR("fail to configure btn_wakeup pin: %d", ret);
        return;
    }
}

wakeup_source_t pwr_get_wakeup_cause(void)
{
    return wakeup_cause;
}

void pwr_set_sleep_timer_wakeup(int time_s)
{
    if (time_s < 300 || time_s > (3600 * 24))
    {
        LOG_ERR("invalid wakeup time %ds", time_s);
        return;
    }

    wakeup_time_sec = time_s;
}

void pwr_enter_sleep(void)
{
    /* 经过测试发现 sys_poweroff() 要紧跟 esp_sleep_enable_timer_wakeup()，rtc 唤醒才会生效 */
    esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000 * 1000);
    sys_poweroff();
}
