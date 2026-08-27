#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "multi_button.h"
#include "btn.h"

LOG_MODULE_REGISTER(btn, LOG_LEVEL_DBG);

#define BTN_MODE_NODE DT_NODELABEL(btn_mode)

enum
{
	BTN_WAKEUP_ID = 1,
	BTN_MODE_ID
};

static struct gpio_dt_spec btn_mode_spec = GPIO_DT_SPEC_GET(BTN_MODE_NODE, gpios);

static bool is_server_mode = true; /* 0 为基础模式，1 为服务器模式 */
static Button btn_wakeup;
static Button btn_mode;
K_EVENT_DEFINE(btn_mode_event);
static btn_callback btn_wakeup_double_clicked_callback;

uint8_t read_btn(uint8_t button_id)
{
	extern int get_btn_wakeup_status(void);

	switch (button_id)
	{
	case BTN_WAKEUP_ID:
		return get_btn_wakeup_status();
	case BTN_MODE_ID:
		return gpio_pin_get_dt(&btn_mode_spec);
	default:
		break;
	}

	return 0;
}

static void btn_mode_clicked_cb(Button *btn, void *user_data)
{
	LOG_DBG("Button mode clicked");
	is_server_mode = !is_server_mode;
	k_event_set_masked(&btn_mode_event, is_server_mode ? BTN_BIT_MODE_SERVER : BTN_BIT_MODE_BASIC,
					   BTN_BIT_MODE_ALL);
}

static void btn_wakeup_double_clicked_cb(Button *btn, void *user_data)
{
	LOG_DBG("Button wakeup double clicked");

	btn_wakeup_double_clicked_callback();
}

static void btn_mode_long_press_cb(Button *btn, void *user_data)
{
	LOG_DBG("Button mode long pressed");
	if (is_server_mode)
	{
		k_event_set_masked(&btn_mode_event, BTN_BIT_MODE_SERVER | BTN_BIT_MODE_SELECTED, BTN_BIT_MODE_ALL);
	}
	else
	{
		k_event_set_masked(&btn_mode_event, BTN_BIT_MODE_BASIC | BTN_BIT_MODE_SELECTED, BTN_BIT_MODE_ALL);
	}
}

static void btn_mode_timer_callback(struct k_timer *timer)
{
	button_ticks();
}

K_TIMER_DEFINE(btn_mode_timer, btn_mode_timer_callback, NULL);

void btn_init(void)
{
	if (!device_is_ready(btn_mode_spec.port))
	{
		LOG_ERR("Button device not ready");
		return;
	}

	int ret = gpio_pin_configure_dt(&btn_mode_spec, GPIO_INPUT);
	if (ret != 0)
	{
		LOG_ERR("Error configuring button pin: %d", ret);
		return;
	}
	button_init(&btn_wakeup, read_btn, 1, 1);
	button_attach(&btn_wakeup, BTN_DOUBLE_CLICK, btn_wakeup_double_clicked_cb, NULL);
	button_start(&btn_wakeup);

	button_init(&btn_mode, read_btn, 1, 2);
	button_attach(&btn_mode, BTN_SINGLE_CLICK, btn_mode_clicked_cb, NULL);
	button_attach(&btn_mode, BTN_LONG_PRESS_START, btn_mode_long_press_cb, NULL);
	button_start(&btn_mode);

	k_timer_start(&btn_mode_timer, K_NO_WAIT, K_MSEC(5));
}

void btn_set_double_clicked_callback(btn_callback callback)
{
	btn_wakeup_double_clicked_callback = callback;
}