#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "multi_button.h" 

LOG_MODULE_REGISTER(app_btn, LOG_LEVEL_DBG);

#define BTN_MODE_NODE DT_NODELABEL(btn_mode)
static struct gpio_dt_spec btn_mode_spec = GPIO_DT_SPEC_GET(BTN_MODE_NODE, gpios); 

Button btn_mode;

uint8_t read_btn(uint8_t button_id)
{
	return gpio_pin_get_dt(&btn_mode_spec);
}

void btn_mode_callback(Button* btn, void* user_data)
{
	LOG_INF("Button mode pressed");
}

void btn_mode_timer_callback(struct k_timer *timer)
{
	button_ticks();
}

K_TIMER_DEFINE(btn_mode_timer, btn_mode_timer_callback, NULL);

void btn_init(void)
{
	if (!device_is_ready(btn_mode_spec.port)) {
		LOG_ERR("Button device not ready");
		return;
	}

	int ret = gpio_pin_configure_dt(&btn_mode_spec, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error configuring button pin: %d", ret);
		return;
	}

	button_init(&btn_mode, read_btn, 1, 1);
	button_attach(&btn_mode, BTN_SINGLE_CLICK, btn_mode_callback, NULL);
	button_start(&btn_mode);

	k_timer_start(&btn_mode_timer, K_NO_WAIT, K_MSEC(5));
}
