#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/drivers/retained_mem.h>
#include "pwr_manage.h"
#include "rgb_strip.h"
#include <esp_sleep.h>

#define RETAIN_MAGIC 0xA55AA55AU // 魔法校验值，区分冷启动/休眠唤醒
#define RETAIN_OFFSET_MAGIC 0
#define RETAIN_OFFSET_WAKE_CNT 4
#define RETAIN_OFFSET_WAKE_CAUSE 8


int main(void)
{
	pwr_init();
	printk("board init\n");

	uint64_t time = 86400ULL * 1000 * 1000; // 24 小时，单位微秒
	uint64_t time2 = 10ULL * 1000 * 1000; // 10 秒，单位微秒
	int ret = esp_sleep_enable_timer_wakeup(time);
	if (ret == ESP_OK)
	{
		printk("set wakeup timer success\n");
	}
	else if (ret == ESP_ERR_INVALID_ARG)
	{
		printk("set wakeup timer failed, invalid argument\n");
	}
	k_sleep(K_SECONDS(15)); // 休眠 120 秒让日志稳定输出
	pwr_enter_sleep(false); // 进入深度休眠，定时唤醒

	// 休眠之后芯片断电，永远不会执行到这里
	return 0;
}