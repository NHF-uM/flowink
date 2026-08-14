#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/drivers/retained_mem.h>
#include "pwr_manage.h"
#include "rgb_strip.h"
#include <esp_sleep.h>
#include "test.h"


int main(void)
{
	test_pwr();
	return 0;
}