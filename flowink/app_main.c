#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
// #include "pwr_manage.h"
#include "app_led.h"

LOG_MODULE_REGISTER(main);

#define BTN_WAKEUP_NODE DT_NODELABEL(btn_wakeup)
static const struct gpio_dt_spec btn_wakeup = GPIO_DT_SPEC_GET(BTN_WAKEUP_NODE, gpios);
static struct gpio_callback button_cb_data;

static void btn_click_work_handler(struct k_work_delayable *work);
static void btn_long_press_work_handler(struct k_work_delayable *work);
static K_WORK_DELAYABLE_DEFINE(dwork_btn_click, btn_click_work_handler);
static K_WORK_DELAYABLE_DEFINE(dwork_btn_long_press, btn_long_press_work_handler);

static bool is_pressed;
static bool long_press_triggered;

static void btn_click_work_handler(struct k_work_delayable *work)
{
	if (gpio_pin_get_dt(&btn_wakeup))
	{
		/* 按键按下（消抖后确认） */
		LOG_INF("button pressed (debounced)");
		is_pressed = true;
		long_press_triggered = false; 
		// 启动长按计时
		k_work_schedule(&dwork_btn_long_press, K_MSEC(CONFIG_BTN_LONG_PRESSED_INTERVAL));
	}
	else
	{
		/* 按键松开（消抖后确认） */
		LOG_INF("button released (debounced)");
		k_work_cancel_delayable(&dwork_btn_long_press);

		if (is_pressed && !long_press_triggered)
		{
			/* 判定为短按 */
			LOG_INF("button short click");
		}

		is_pressed = false;
	}
}

static void btn_long_press_work_handler(struct k_work_delayable *work)
{
	if (gpio_pin_get_dt(&btn_wakeup))
	{
		/* 触发长按 */
		LOG_INF("button long pressed");
		long_press_triggered = true;
	}
}

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_work_reschedule(&dwork_btn_click, K_MSEC(20));
}

int main(void)
{
	int ret;
	// pwr_init();

	app_led_init();

	app_led_set(&led_pwr, true, K_NO_WAIT);
	app_led_set(&led_mode, true, K_MSEC(500));

	if (!gpio_is_ready_dt(&btn_wakeup))
	{
		LOG_ERR("button device is not ready\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&btn_wakeup, GPIO_INPUT);
	if (ret != 0)
	{
		LOG_ERR("failed to configure button pin\n");
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&btn_wakeup, GPIO_INT_EDGE_BOTH);
	if (ret != 0)
	{
		LOG_ERR("failed to configure interrupt btn pin %d\n");
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

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