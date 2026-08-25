#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(app_nv, LOG_LEVEL_DBG);

#define RETAIN_MAGIC 0xA55AA55AU
#define RETAIN_OFFSET_MAGIC 0
#define RETAIN_OFFSET_PATH 4

static char path[128];  /* 128 字节的路径缓冲区，读取和写入要完整，在路径结束和缓冲区末尾有'\0' */

static const struct device *retained_mem_device = DEVICE_DT_GET(DT_NODELABEL(retained_mem0));

char *nv_read_path(void)
{
    if (!device_is_ready(retained_mem_device))
    {
        LOG_DBG("retained_mem device is not ready!\n");
        return NULL;
    }

    uint32_t magic = 0;
    int err = retained_mem_read(retained_mem_device, RETAIN_OFFSET_MAGIC, (uint8_t *)&magic, sizeof(magic));
    if (err != 0)
    {
        LOG_DBG("read magic fail, err=%d", err);
        goto init_retained;
    }

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

    size_t valid_len = strnlen(path, sizeof(path));
    if (valid_len < sizeof(path))
    {
        path[valid_len] = '\0';
    }
    else
    {
        /* 整个缓冲区都没有'\0' */
        path[sizeof(path) - 1] = '\0';
    }

    LOG_DBG("nv retained path: %s\n", path);
    return path;
}

void nv_write_path(const char *path_buf)
{
    if (!device_is_ready(retained_mem_device))
    {
        LOG_WRN("retained_mem device is not ready!\n");
        return;
    }

    if (path_buf == NULL)
    {
        LOG_WRN("path_buf is NULL!\n");
        return;
    }

    strncpy(path, path_buf, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';

    if (retained_mem_write(retained_mem_device, RETAIN_OFFSET_PATH, (uint8_t *)path, sizeof(path)))
    {
        LOG_WRN("retained_mem write path failed!\n");
    }

    LOG_DBG("nv retained path written: %s\n", path);
}

void nv_break_magic(void)
{
    if (!device_is_ready(retained_mem_device))
    {
        LOG_DBG("retained_mem device is not ready!\n");
        return;
    }

    uint32_t magic = 0;
    if (retained_mem_write(retained_mem_device, RETAIN_OFFSET_MAGIC, (uint8_t *)&magic, sizeof(magic)))
    {
        LOG_WRN("retained_mem write magic failed!\n");
    }
}