#include "led.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led);

#define LED_PWR_NODE DT_NODELABEL(led_pwr)
#define LED_MODE_NODE DT_NODELABEL(led_mode)

static void led_timer_callback(struct k_timer *timer);
static const struct gpio_dt_spec led_pwr_spec = GPIO_DT_SPEC_GET(LED_PWR_NODE, gpios);
static const struct gpio_dt_spec led_mode_spec = GPIO_DT_SPEC_GET(LED_MODE_NODE, gpios);

/* 其余成员被隐式初始化为0 */
struct led_ctx led_pwr_struct = {
    .gpio = &led_pwr_spec,
};

struct led_ctx led_mode_struct = {
    .gpio = &led_mode_spec,
};

struct led_ctx *const led_pwr = &led_pwr_struct;
struct led_ctx *const led_mode = &led_mode_struct;

static void led_timer_callback(struct k_timer *timer)
{
    struct led_ctx *ctx = CONTAINER_OF(timer, struct led_ctx, timer);

    if (ctx->blinky_en)
    {
        gpio_pin_toggle_dt(ctx->gpio);
    }
}

void led_init(void)
{
    int ret;
    if (!gpio_is_ready_dt(led_pwr->gpio) || !gpio_is_ready_dt(led_mode->gpio))
    {
        LOG_ERR("leds is not ready\n");
    }

    ret = gpio_pin_configure_dt(led_pwr->gpio, GPIO_OUTPUT_INACTIVE);
    if (ret != 0)
    {
        LOG_ERR("Failed to configure pwr led: %d", ret);
        return;
    }

    ret = gpio_pin_configure_dt(led_mode->gpio, GPIO_OUTPUT_INACTIVE);
    if (ret != 0)
    {
        LOG_ERR("Failed to configure mode led: %d", ret);
        return;
    }

    k_timer_init(&led_pwr->timer, led_timer_callback, NULL);
    k_timer_init(&led_mode->timer, led_timer_callback, NULL);
}

void led_set(struct led_ctx *ctx, bool enable, k_timeout_t period)
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
