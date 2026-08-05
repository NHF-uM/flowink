#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwr_m);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/poweroff.h>
#include "pwr_manage.h"
#include <esp_sleep.h>

#define WAKEUP_IO_NODE DT_ALIAS(pwr_wakeup_io)
static const struct gpio_dt_spec wakeup_io_spec = GPIO_DT_SPEC_GET(WAKEUP_IO_NODE, gpios);
static wakeup_source_t wakeup_cause; 

/// @brief 更新并保存深度休眠唤醒原因
/// @param  无
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
    }

    wakeup_cause = WAKEUP_UNKNOWN;
}

/// @brief 配置唤醒引脚，更新记录唤醒源
/// @param  
void pwr_init(void)
{
    if (!gpio_is_ready_dt(&wakeup_io_spec))
    {
        LOG_ERR("pwr_wakeup pin not ready \n");
        return;
    }

    // if (!device_is_ready(retained_mem_device)) {
	// 	LOG_ERR("retained_mem device is not ready!\n");
	// 	return 0;
    // }

    /* 在dts已经配置为中断唤醒引脚了，只需要再配置一下输入和中断触发 */
    if (gpio_pin_configure_dt(&wakeup_io_spec, GPIO_INPUT) || gpio_pin_interrupt_configure_dt(&wakeup_io_spec, GPIO_INT_EDGE_TO_INACTIVE);)
    {
        LOG_ERR("failed to configure pwr_wakeup pin \n");
        return;
    }

    if (!pm_device_wakeup_enable(wakeup_io_spec.port, true))
    {
        LOG_ERR("failed to enable wakeup pin \n");
        return;
    }

    pwr_update_wakeup_cause();
}

inline wakeup_source_t pwr_get_wakeup_cause(void)
{
    return wakeup_cause;
}

/// @brief 设置轮播唤醒时间
/// @param time_s 单位/秒，范围300~86400
void pwr_set_sleep_timer_wakeup(int time_s)
{
    if (time_s < 300 || time_s > (3600 * 24))
    {
        LOG_ERR("invalid wakeup time %ds", time_s);
        time_s = CONFIG_WAKEUP_TIME_SEC;
    }

    esp_sleep_enable_timer_wakeup(time_s * 1000 * 1000);
}

/// @brief 进入深度休眠，每次唤醒后都需要重新配置唤醒源
/// @param  无
inline void pwr_enter_sleep(void)
{
    sys_poweroff();
}
