#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "pwr_manage.h"
#include "rgb_strip.h"
#include <esp_sleep.h>
#include <zephyr/drivers/retained_mem.h>

static const struct device *retained_mem_device = DEVICE_DT_GET(DT_NODELABEL(retained_mem0));

int main(void)
{
    pwr_init();

    k_sleep(K_SECONDS(30));
    esp_sleep_enable_timer_wakeup(10 * 1000 * 1000);

    if (!device_is_ready(retained_mem_device)) {
		LOG_ERR("retained_mem device is not ready!\n");
		return 0;
    }

    uint16_t offset = 100;
    uint8_t aux = 0;
    int err;
    err = retained_mem_read(retained_mem_device, offset, (uint8_t *)&aux, sizeof(aux));
    if (err < 0)
    {
        LOG_ERR("retained_mem_read() failed: %d\n", err);
        return 0;
    }   LOG_INF("wakeup_times: %u", aux);

    aux++;
    err = retained_mem_write(retained_mem_device, offset, (uint8_t *)&aux, sizeof(aux));
    if (err < 0)
    {
		LOG_ERR("retained_mem_write() failed: %d\n", err);
		return 0;
	}   
    
    offset += sizeof(aux);
    wakeup_source_t wa = WAKEUP_UNKNOWN;

    err = retained_mem_read(retained_mem_device, offset, (uint8_t *)&wa, sizeof(wa));
    if (err < 0)
    {
        LOG_ERR("retained_mem_read() failed: %d\n", err);
        return 0;
    }    LOG_INF("cause: %d", wa);

    wa = pwr_get_wakeup_cause();
    
    err = retained_mem_write(retained_mem_device, offset, (uint8_t *)&wa, sizeof(wa));
    if (err < 0)
    {
		LOG_ERR("retained_mem_write() failed: %d\n", err);
		return 0;
	}

    pwr_enter_sleep();

    while (1) {

    }
    return 0;
}

