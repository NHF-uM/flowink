#include "epd.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <string.h>

LOG_MODULE_REGISTER(epd, LOG_LEVEL_DBG);

#define EPD_NODE DT_NODELABEL(epd)

const struct spi_dt_spec epd_spi = SPI_DT_SPEC_GET(EPD_NODE, SPI_OP_MODE_MASTER | SPI_WORD_SET(8));
const struct gpio_dt_spec epd_gpio_dc = GPIO_DT_SPEC_GET(EPD_NODE, dc_gpios);
const struct gpio_dt_spec epd_gpio_rst = GPIO_DT_SPEC_GET(EPD_NODE, rst_gpios);
const struct gpio_dt_spec epd_gpio_busy = GPIO_DT_SPEC_GET(EPD_NODE, busy_gpios);

static uint16_t wait_time_cnt;

static void epd_lowlevel_init(void)
{
    gpio_pin_configure_dt(&epd_gpio_dc, GPIO_OUTPUT_LOW);
    gpio_pin_configure_dt(&epd_gpio_rst, GPIO_OUTPUT_HIGH);
    gpio_pin_configure_dt(&epd_gpio_busy, GPIO_INPUT);

    if (!gpio_is_ready_dt(&epd_gpio_dc) || !gpio_is_ready_dt(&epd_gpio_rst) || !gpio_is_ready_dt(&epd_gpio_busy))
    {
        LOG_ERR("GPIO device not ready");
        return;
    }

    if (!spi_is_ready_dt(&epd_spi))
    {
        LOG_ERR("SPI device not ready");
        return;
    }
}

/**
 * @brief 等待BUSY引脚拉高
 * @param  无
 */
static void epd_wait_idle(void)
{
    while (!gpio_pin_get_dt(&epd_gpio_busy))
    {
        k_sleep(K_MSEC(2));
        // wait_time_cnt++;

        // if (wait_time_cnt > 30000)
        // {
        //     LOG_ERR("EPD busy timeout");
        //     return;
        // }
    }
}

static void epd_send_command(uint8_t cmd)
{
    gpio_pin_set_dt(&epd_gpio_dc, 0);
    struct spi_buf_set tx_buf = {
        .buffers = &(struct spi_buf){.buf = &cmd, .len = 1},
        .count = 1,
    };
    spi_write_dt(&epd_spi, &tx_buf);
}

static void epd_send_data(uint8_t data)
{
    gpio_pin_set_dt(&epd_gpio_dc, 1);
    struct spi_buf_set tx_buf = {
        .buffers = &(struct spi_buf){.buf = &data, .len = 1},
        .count = 1,
    };
    spi_write_dt(&epd_spi, &tx_buf);
}

/**
 * @brief 批量发送数据
 * @param data 数据指针
 * @param len 数据长度
 */
static void epd_send_data_bulk(const uint8_t *data, uint32_t len)
{
    gpio_pin_set_dt(&epd_gpio_dc, 1);
    struct spi_buf tx = {.buf = (void *)data, .len = len};
    struct spi_buf_set tx_set = {.buffers = &tx, .count = 1};
    spi_write_dt(&epd_spi, &tx_set);
}

/**
 * @brief 开电源-刷新（包含等待）-关电源
 * @param  无
 */
static void epd_refresh(void)
{
    epd_send_command(0x04); // POWER_ON，执行该指令后，数据条目将持续写入内存，直至写入另一道指令为止
    epd_wait_idle();

    // Second setting
    epd_send_command(0x06);
    epd_send_data(0x6F);
    epd_send_data(0x1F);
    epd_send_data(0x17);
    epd_send_data(0x49);

    epd_send_command(0x12); // Display refresh，执行后需要等待 busy 线拉高
    epd_send_data(0x00);
    epd_wait_idle();

    epd_send_command(0x02); // POWER_OFF
    epd_send_data(0X00);
    epd_wait_idle();
}

void epd_reset(void)
{
    gpio_pin_set_dt(&epd_gpio_rst, 1);
    k_sleep(K_MSEC(50));
    gpio_pin_set_dt(&epd_gpio_rst, 0);
    k_sleep(K_MSEC(20));
    gpio_pin_set_dt(&epd_gpio_rst, 1);
    k_sleep(K_MSEC(50));
}

void epd_init(void)
{
    epd_lowlevel_init();

    epd_reset();
    epd_wait_idle();
    k_sleep(K_MSEC(30));

    epd_send_command(0xAA);
    epd_send_data(0x49);
    epd_send_data(0x55);
    epd_send_data(0x20);
    epd_send_data(0x08);
    epd_send_data(0x09);
    epd_send_data(0x18);

    epd_send_command(0x01);
    epd_send_data(0x3F);

    epd_send_command(0x00);
    epd_send_data(0x5F);
    epd_send_data(0x69);

    epd_send_command(0x03);
    epd_send_data(0x00);
    epd_send_data(0x54);
    epd_send_data(0x00);
    epd_send_data(0x44);

    epd_send_command(0x05);
    epd_send_data(0x40);
    epd_send_data(0x1F);
    epd_send_data(0x1F);
    epd_send_data(0x2C);

    epd_send_command(0x06);
    epd_send_data(0x6F);
    epd_send_data(0x1F);
    epd_send_data(0x17);
    epd_send_data(0x49);

    epd_send_command(0x08);
    epd_send_data(0x6F);
    epd_send_data(0x1F);
    epd_send_data(0x1F);
    epd_send_data(0x22);

    epd_send_command(0x30);
    epd_send_data(0x03);

    epd_send_command(0x50);
    epd_send_data(0x3F);

    epd_send_command(0x60);
    epd_send_data(0x02);
    epd_send_data(0x00);

    epd_send_command(0x61);
    epd_send_data(0x03);
    epd_send_data(0x20);
    epd_send_data(0x01);
    epd_send_data(0xE0);

    epd_send_command(0x84);
    epd_send_data(0x01);

    epd_send_command(0xE3);
    epd_send_data(0x2F);

    /* 打开电源并且等待稳定 */
    epd_send_command(0x04);
    epd_wait_idle();
}

void epd_fill_color(uint8_t color)
{
    uint8_t *buf = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, EPD_SIZE_BYTE);
    memset(buf, (color << 4) | color, EPD_SIZE_BYTE);
    epd_send_command(0x10);
    epd_send_data_bulk(buf, EPD_SIZE_BYTE);
    shared_multi_heap_free(buf);
    epd_refresh();
}

void epd_fill_image(uint8_t *Image)
{
    epd_send_command(0x10);
    epd_send_data_bulk(Image, EPD_SIZE_BYTE);
    epd_refresh();
}

// void epd_fill_color(uint8_t color)
// {
//     epd_send_command(0x10);

//     uint16_t width = EPD_7IN3E_WIDTH / 2;
//     for (uint16_t j = 0; j < EPD_7IN3E_HEIGHT; j++)
//     {
//         for (uint16_t i = 0; i < width; i++)
//         {
//             epd_send_data((color << 4) | color);
//         }
//     }

//     epd_refresh();
// }

// void epd_fill_image(uint8_t *Image)
// {
//     epd_send_command(0x10);

//     for (uint16_t j = 0; j < EPD_7IN3E_HEIGHT; j++)
//     {
//         for (uint16_t i = 0; i < EPD_7IN3E_WIDTH; i++)
//         {
//             epd_send_data(Image[i + j * EPD_7IN3E_WIDTH]);
//         }
//     }
//     epd_refresh();
// }

void epd_sleep(void)
{
    epd_send_command(0X02); // Power off
    epd_send_data(0x00);
    epd_wait_idle();

    epd_send_command(0x07); // Deep sleep
    epd_send_data(0XA5);
}
