#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include "pwr_manage.h"

#define LED_PWR_NODE DT_NODELABEL(led_pwr)
#define LED_MODE_NODE DT_NODELABEL(led_mode)
static const struct gpio_dt_spec led_pwr = GPIO_DT_SPEC_GET(LED_PWR_NODE, gpios);
static const struct gpio_dt_spec led_mode = GPIO_DT_SPEC_GET(LED_MODE_NODE, gpios);

static void led_timer_callback(struct k_timer *timer);
static K_TIMER_DEFINE(timer_pwr, led_timer_callback, NULL);
static K_TIMER_DEFINE(timer_mode, led_timer_callback, NULL);

struct led_ctx
{
	const struct gpio_dt_spec *gpio;
	struct k_timer timer;
	bool blinky_en;
};

static struct led_ctx ctx_pwr = {
	.gpio = &led_pwr,
	.timer = &timer_pwr,
	.blinky_en = false,
};

static struct led_ctx ctx_mode = {
	.gpio = &led_mode,
	.timer = &timer_mode,
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

/* 通用LED控制接口：设置常亮/闪烁/关闭
 * @param ctx:    LED上下文
 * @param enable: true开启，false关闭
 * @param period: 闪烁半周期，K_NO_WAIT表示常亮不闪烁
 */
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


// static void btn_work_handler(struct k_work *work);
// static K_WORK_DELAYABLE_DEFINE(dwork_btn, btn_work_handler);

// static void btn_work_handler(struct k_work *work)
// {
// 	if (gpio_pin_get_dt(&btn_wakeup))
// 	{
// 		k_sleep(K_MSEC(CONFIG_BTN_LONG_PRESSED_INTERVAL));

// 		if (gpio_pin_get_dt(&btn_wakeup))
// 		{

// 		}
// 	}
// }

int main(void)
{
	int ret;
	pwr_init();

	if (!gpio_is_ready_dt(&led_pwr) || !gpio_is_ready_dt(&led_mode))
	{
		printk("leds is not ready\n");
	}

	ret = gpio_pin_configure_dt(&led_pwr, GPIO_OUTPUT_INACTIVE);
	if (ret != 0)
	{
		printk("Failed to configure pwr led: %d", ret);
		return -1;
	}

	ret = gpio_pin_configure_dt(&led_mode, GPIO_OUTPUT_INACTIVE);
	if (ret != 0)
	{
		printk("Failed to configure mode led: %d", ret);
		return -1;
	}

	led_set(&ctx_pwr, true, K_NO_WAIT);
	led_set(&ctx_mode, true, K_MSEC(500));

	printk("LED driver initialized");




	// led_set(&ctx_pwr, true, K_NO_WAIT);
	// wakeup_source_t cause = pwr_get_wakeup_cause();
	// if (cause == WAKEIP_TIMER)
	// {

	// }
	// else
	// {
	// 	/* 等待按键 */
	// 	k_sleep(K_FOREVER);
	// }

	return 0;
}