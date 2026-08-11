#include "app_led.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_led);

#define LED_PWR_NODE DT_NODELABEL(led_pwr)
#define LED_MODE_NODE DT_NODELABEL(led_mode)
static const struct gpio_dt_spec led_pwr_spec = GPIO_DT_SPEC_GET(LED_PWR_NODE, gpios);
static const struct gpio_dt_spec led_mode_spec = GPIO_DT_SPEC_GET(LED_MODE_NODE, gpios);

static void led_timer_callback(struct k_timer *timer);

/* timer 隐式初始化为0 */
struct led_ctx led_pwr = {
    .gpio = &led_pwr_spec,
    .blinky_en = false,
};

struct led_ctx led_mode = {
    .gpio = &led_mode_spec,
    .blinky_en = false,
};

static void led_timer_callback(struct k_timer *timer)
{
    struct led_ctx *ctx = CONTAINER_OF(timer, struct led_ctx, timer);

    if (ctx->blinky_en)
    {
        gpio_pin_toggle_dt(ctx->gpio);
    }
}

void app_led_init(void)
{
    int ret;
    if (!gpio_is_ready_dt(led_pwr.gpio) || !gpio_is_ready_dt(led_mode.gpio))
    {
        LOG_ERR("leds is not ready\n");
    }

    ret = gpio_pin_configure_dt(led_pwr.gpio, GPIO_OUTPUT_INACTIVE);
    if (ret != 0)
    {
        LOG_ERR("Failed to configure pwr led: %d", ret);
        return;
    }

    ret = gpio_pin_configure_dt(led_mode.gpio, GPIO_OUTPUT_INACTIVE);
    if (ret != 0)
    {
        LOG_ERR("Failed to configure mode led: %d", ret);
        return;
    }

    k_timer_init(&led_pwr.timer, led_timer_callback, NULL);
    k_timer_init(&led_mode.timer, led_timer_callback, NULL);
}

/* 通用LED控制接口：设置常亮/闪烁/关闭
 * @param ctx:    LED上下文
 * @param enable: true开启，false关闭
 * @param period: 闪烁半周期，K_NO_WAIT表示常亮不闪烁
 */
void app_led_set(struct led_ctx *ctx, bool enable, k_timeout_t period)
{
    if (!enable)
    {
        ctx->blinky_en = false;
        gpio_pin_set_dt(ctx->gpio, 0);
        k_timer_stop(&ctx->timer);
        return;
    }

    if (K_TIMEOUT_EQ(period, K_NO_WAIT) || K_TIMEOUT_EQ(period, K_FOREVER))
    {
        ctx->blinky_en = false;
        gpio_pin_set_dt(ctx->gpio, 1);
        k_timer_stop(&ctx->timer);
    }
    else
    {
        ctx->blinky_en = true;
        k_timer_start(&ctx->timer, K_NO_WAIT, period);
    }
}
