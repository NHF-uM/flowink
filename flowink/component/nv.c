#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(nv, LOG_LEVEL_DBG);

#define RETAIN_MAGIC 0xA55AA55AU
#define RETAIN_OFFSET_MAGIC 0
#define RETAIN_OFFSET_PATH 4

static char path[128]; /* 128 字节的路径缓冲区，读取和写入要完整，在路径结束和缓冲区末尾有'\0' */

static const struct device *retained_mem_device = DEVICE_DT_GET(DT_NODELABEL(retained_mem0));

/**
 * @brief 读取magic值
 * @param magic_out 输出读到的magic
 * @return 0 成功
 */
static int nv_read_magic(uint32_t *magic_out)
{
    return retained_mem_read(retained_mem_device, RETAIN_OFFSET_MAGIC, (uint8_t *)magic_out, sizeof(*magic_out));
}

char *nv_read_path(void)
{
    if (!device_is_ready(retained_mem_device))
    {
        LOG_WRN("retained_mem device is not ready!\n");
        return NULL;
    }

    uint32_t magic = 0;
    int err = nv_read_magic(&magic);
    if (err != 0)
    {
        LOG_WRN("read magic fail, err=%d", err);
        goto init_retained;
    }

    if (magic != RETAIN_MAGIC)
    {
init_retained:
        magic = RETAIN_MAGIC;
        err = retained_mem_write(retained_mem_device, RETAIN_OFFSET_MAGIC, (uint8_t *)&magic, sizeof(magic));
        if (err != 0)
        {
            LOG_WRN("retained_mem write magic failed!\n");
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
        LOG_WRN("retained_mem read path failed!\n");
        return NULL;
    }

    size_t valid_len = strnlen(path, sizeof(path));
    if (valid_len < sizeof(path))
    {
        path[valid_len] = '\0';
    }
    else
    {
        path[sizeof(path) - 1] = '\0';
    }

    if (path[0] == '\0')
    {
        LOG_WRN("nv retained path is empty");
        return NULL;
    }

    char *dyn_str = k_malloc(valid_len + 1);
    if (dyn_str == NULL)
    {
        LOG_WRN("nv_read_path k_malloc failed, len=%zu", valid_len + 1);
        return NULL;
    }

    memcpy(dyn_str, path, valid_len);
    dyn_str[valid_len] = '\0';

    LOG_DBG("nv retained path: %s, dyn alloc len=%zu", dyn_str, valid_len);
    return dyn_str;
}

void nv_free_path(char *ptr)
{
    if (ptr != NULL)
    {
        k_free(ptr);
    }
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

    uint32_t magic = 0;
    int err = nv_read_magic(&magic);
    if ((err != 0) || (magic != RETAIN_MAGIC))
    {
        uint32_t valid_magic = RETAIN_MAGIC;
        err = retained_mem_write(retained_mem_device, RETAIN_OFFSET_MAGIC, (uint8_t *)&valid_magic, sizeof(valid_magic));
        if (err != 0)
        {
            LOG_WRN("retained_mem write magic during write_path failed! err=%d", err);
            return;
        }
        LOG_DBG("nv_write_path: magic invalid, refresh magic + path");
    }

    err = retained_mem_write(retained_mem_device, RETAIN_OFFSET_PATH, (uint8_t *)path, sizeof(path));
    if (err != 0)
    {
        LOG_WRN("retained_mem write path failed! err=%d", err);
        return;
    }

    LOG_DBG("nv retained path written: %s\n", path);
}

void nv_break_magic(void)
{
    if (!device_is_ready(retained_mem_device))
    {
        LOG_WRN("retained_mem device is not ready!\n");
        return;
    }

    uint32_t magic = 0;
    if (retained_mem_write(retained_mem_device, RETAIN_OFFSET_MAGIC, (uint8_t *)&magic, sizeof(magic)))
    {
        LOG_WRN("retained_mem write magic failed!\n");
    }

    LOG_DBG("nv retained magic broken\n");
}
