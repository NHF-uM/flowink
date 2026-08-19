#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/logging/log.h>
#include "pwr_manage.h"
#include "rgb_strip.h"
#include <esp_sleep.h>

LOG_MODULE_REGISTER(test_pwr, LOG_LEVEL_DBG);

#define RETAIN_MAGIC 0xA55AA55AU /* 魔幻数，默认是有符号数，后面加 'U' 能防止一些情况下出错 */
#define RETAIN_OFFSET_MAGIC 0
#define RETAIN_OFFSET_WAKE_CNT 4
#define RETAIN_OFFSET_WAKE_CAUSE 8

static const struct device *retained_mem_device = DEVICE_DT_GET(DT_NODELABEL(retained_mem0));

void test_pwr(void)
{
	pwr_init();
	LOG_DBG("board init\n");

	if (!device_is_ready(retained_mem_device))
	{
		LOG_DBG("retained_mem device is not ready!\n");
		return;
	}

	wakeup_source_t curr_wake_cause = pwr_get_wakeup_cause();

	uint32_t magic = 0;
	int err;

	// 读取魔法值
	err = retained_mem_read(retained_mem_device, RETAIN_OFFSET_MAGIC, (uint8_t *)&magic, sizeof(magic));
	if (err < 0)
	{
		goto init_retained;
	}

	// 魔法校验失败 = 冷启动上电，初始化保留内存
	if (magic != RETAIN_MAGIC)
	{
	init_retained:
		magic = RETAIN_MAGIC;
		retained_mem_write(retained_mem_device, RETAIN_OFFSET_MAGIC, (uint8_t *)&magic, sizeof(magic));
		LOG_DBG("Cold boot, init retained memory\n");
	}

	wakeup_source_t last_wake_cause;
	retained_mem_read(retained_mem_device, RETAIN_OFFSET_WAKE_CAUSE, (uint8_t *)&last_wake_cause, sizeof(last_wake_cause));
	LOG_DBG("Last wake cause: %d\n", last_wake_cause);

	// 2. 休眠前：把【本次真实唤醒原因】存入保留内存，留给下次上电读取
	retained_mem_write(retained_mem_device, RETAIN_OFFSET_WAKE_CAUSE, (uint8_t *)&curr_wake_cause, sizeof(curr_wake_cause));

	k_sleep(K_SECONDS(15));
	LOG_DBG("Enter deep sleep after 5s timer wakeup\n");

	esp_sleep_enable_timer_wakeup(5 * 1000 * 1000);
	pwr_enter_sleep();

	return;
}