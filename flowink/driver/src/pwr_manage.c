#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwr_m);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/poweroff.h>
#include <esp_sleep.h>

static int time_timer_wakup;

#define WAKEUP_IO_NODE DT_ALIAS(pwr_wakeup_io)
static const struct gpio_dt_spec wakeup_io_spec = GPIO_DT_SPEC_GET(WAKEUP_IO_NODE, gpios);

/// @brief esp32 进入深度休眠
/// @param  无
void pwr_enter_sleep(void)
{
    int ret;

    /* 每次深度休眠前必须重配唤醒源 */
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    /* 在dts已经配置为中断唤醒引脚了，只需要再配置一下输入 */
    ret = gpio_pin_configure_dt(&wakeup_io_spec, GPIO_INPUT);
    if (ret != 0)
    {
        printk("failed to configure pwr_wakeup pin \n");
        return;
    }

    int time = time_timer_wakup ? time_timer_wakup : CONFIG_WAKEUP_TIME_SEC;
    esp_sleep_enable_timer_wakeup(time * 1000 * 1000);

    sys_poweroff();
}

/// @brief 设置轮播唤醒时间
/// @param time_s 
void pwr_set_sleep_timer_wakeup(int time_s)
{
    if (time_s <= 180){
        LOG_ERR("invalid wakeup time %ds", time_s);
        return;
    }
    time_timer_wakup = time_s;
}