#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/logging/log.h>
#include "pwr_manage.h"
#include "rgb_strip.h"
#include <esp_sleep.h>

LOG_MODULE_REGISTER(test_pwr, LOG_LEVEL_DBG);

#define RETAIN_MAGIC 0xA55AA55AU // 魔法校验值，区分冷启动/休眠唤醒
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

	// 1. 获取本次上电真实唤醒原因
	wakeup_source_t curr_wake_cause = pwr_get_wakeup_cause();

	uint32_t magic = 0;
	uint32_t wake_cnt = 0;
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
		wake_cnt = 1;
		// 写入魔法值
		retained_mem_write(retained_mem_device, RETAIN_OFFSET_MAGIC, (uint8_t *)&magic, sizeof(magic));
		// 唤醒次数初始化为1
		retained_mem_write(retained_mem_device, RETAIN_OFFSET_WAKE_CNT, (uint8_t *)&wake_cnt, sizeof(wake_cnt));
		// 唤醒原因初始化为未知
		wakeup_source_t init_cause = WAKEUP_UNKNOWN;
		retained_mem_write(retained_mem_device, RETAIN_OFFSET_WAKE_CAUSE, (uint8_t *)&init_cause, sizeof(init_cause));
		LOG_DBG("Cold boot, init retained memory\n");
	}
	else
	{
		// 休眠唤醒：读取唤醒次数并自增
		retained_mem_read(retained_mem_device, RETAIN_OFFSET_WAKE_CNT, (uint8_t *)&wake_cnt, sizeof(wake_cnt));
		LOG_DBG("Wakeup times: %u\n", wake_cnt);
		wake_cnt++;
		retained_mem_write(retained_mem_device, RETAIN_OFFSET_WAKE_CNT, (uint8_t *)&wake_cnt, sizeof(wake_cnt));
	}

	// 读取【上一次休眠保存的唤醒原因】并打印
	wakeup_source_t last_wake_cause;
	retained_mem_read(retained_mem_device, RETAIN_OFFSET_WAKE_CAUSE, (uint8_t *)&last_wake_cause, sizeof(last_wake_cause));
	LOG_DBG("Last wake cause: %d\n", last_wake_cause);

	// 业务逻辑：闪烁LED 2秒
	for (int i = 0; i < 2; i++)
	{
		rgb_strip_on(0x01);
		k_sleep(K_SECONDS(1));
		rgb_strip_off();
		k_sleep(K_SECONDS(1));
	}

	// 2. 休眠前：把【本次真实唤醒原因】存入保留内存，留给下次上电读取
	retained_mem_write(retained_mem_device, RETAIN_OFFSET_WAKE_CAUSE, (uint8_t *)&curr_wake_cause, sizeof(curr_wake_cause));

	// 3. 休眠配置（放在断电前）
	esp_sleep_enable_timer_wakeup(10 * 1000 * 1000);
	LOG_DBG("Enter deep sleep after 10s timer wakeup\n");
	k_sleep(K_SECONDS(2));
	// 真正进入深度断电休眠
	pwr_enter_sleep();

	// 休眠之后芯片断电，永远不会执行到这里
	return;
}