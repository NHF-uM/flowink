#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_nv, LOG_LEVEL_DBG);

#define RETAIN_MAGIC 0xA55AA55AU /* 魔幻数，默认是有符号数，后面加 'U' 能防止一些情况下出错 */
#define RETAIN_OFFSET_MAGIC 0
#define RETAIN_OFFSET_PATH 4

static char path[128];

static const struct device *retained_mem_device = DEVICE_DT_GET(DT_NODELABEL(retained_mem0));

char *nv_read_path(void)
{
    if (!device_is_ready(retained_mem_device))
    {
        LOG_DBG("retained_mem device is not ready!\n");
        return;
    }

    uint32_t magic = 0;
    int err = retained_mem_read(retained_mem_device, RETAIN_OFFSET_MAGIC, (uint8_t *)&magic, sizeof(magic));
    if (err != 0)
    {
        LOG_DBG("read magic fail, err=%d", err);
        goto init_retained;
    }

    // 魔法校验失败
    if (magic != RETAIN_MAGIC)
    {
    init_retained:
        magic = RETAIN_MAGIC;
        err = retained_mem_write(retained_mem_device, RETAIN_OFFSET_MAGIC, (uint8_t *)&magic, sizeof(magic));
        if (err != 0)
        {
            LOG_DBG("retained_mem write magic failed!\n");
            return NULL;
        }

        memset(path, 0, sizeof(path));
        err = retained_mem_write(retained_mem_device, RETAIN_OFFSET_PATH, (uint8_t *)path, sizeof(path));
        if (err != 0)
        {
            LOG_ERR("init path area fail err=%d", err);
            return NULL;
        }
        return NULL;
    }

    err = retained_mem_read(retained_mem_device, RETAIN_OFFSET_PATH, (uint8_t *)path, sizeof(path));
    if (err != 0)
    {
        LOG_DBG("retained_mem read path failed!\n");
        return NULL;
    }

    path[127] = '\0';

    LOG_DBG("nv retained path: %s\n", path);
    return path;
}

int nv_write_path(const char *path)
{
    if (!device_is_ready(retained_mem_device))
    {
        LOG_DBG("retained_mem device is not ready!\n");
        return -1;
    }

    int err = retained_mem_write(retained_mem_device, RETAIN_OFFSET_PATH, (uint8_t *)path, sizeof(path));
    if (err != 0)
    {
        LOG_DBG("retained_mem write path failed!\n");
        return -1;
    }

    return 0;
}
