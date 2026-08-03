#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/input/input.h>
#include <rgb_strip.h>

#define DC_PIN 12
#define RST_PIN 13
#define BUSY_PIN 14
#define PIN_SET(pin) gpio_pin_set(epd_port, pin, 1)
#define PIN_CLR(pin) gpio_pin_set(epd_port, pin, 0)

#define EPD_NODE            DT_NODELABEL(epd)
#define EPD_7IN3E_WIDTH     DT_PROP(EPD_NODE, width)
#define EPD_7IN3E_HEIGHT    DT_PROP(EPD_NODE, height)

const struct spi_dt_spec epd_spi = SPI_DT_SPEC_GET(EPD_NODE, SPI_OP_MODE_MASTER | SPI_WORD_SET(8));
const struct gpio_dt_spec epd_gpio_dc = GPIO_DT_SPEC_GET(EPD_NODE, dc_gpios);
const struct gpio_dt_spec epd_gpio_rst = GPIO_DT_SPEC_GET(EPD_NODE, rst_gpios);
const struct gpio_dt_spec epd_gpio_busy = GPIO_DT_SPEC_GET(EPD_NODE, busy_gpios);

/**
 * @brief 复位屏幕驱动芯片
 * @param  无
 */
static void epd_reset(void)
{
    gpio_pin_set_dt(&epd_gpio_rst, 1);
    k_sleep(K_MSEC(50));
    gpio_pin_set_dt(&epd_gpio_rst, 0);
    k_sleep(K_MSEC(20));
    gpio_pin_set_dt(&epd_gpio_rst, 1);
    k_sleep(K_MSEC(50));
}

/**
 * @brief 等待BUSY引脚拉高
 * @param  无
 */
static void epd_wait_idle(void)
{
    LOG_INF("e-Paper busy H\r\n");
    while (!gpio_pin_get_dt(&epd_gpio_busy))
    {
        k_sleep(K_MSEC(5));
    }
    LOG_INF("e-Paper busy H release\r\n");
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

void epd_init(void)
{
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

/**
 * @brief 开电源-刷新（包含等待）-关电源
 * @param  无
 */
static void epd_refresh(void)
{
    epd_send_command(0x04); // POWER_ON
    epd_wait_idle();

    // Second setting
    epd_send_command(0x06);
    epd_send_data(0x6F);
    epd_send_data(0x1F);
    epd_send_data(0x17);
    epd_send_data(0x49);

    epd_send_command(0x12); // DISPLAY_REFRESH
    epd_send_data(0x00);
    epd_wait_idle();

    epd_send_command(0x02); // POWER_OFF
    epd_send_data(0X00);
    epd_wait_idle();
}

void epd_sleep(void)
{
    epd_send_command(0X02); // DEEP_SLEEP
    epd_send_data(0x00);
    epd_wait_idle();

    epd_send_command(0x07); // DEEP_SLEEP
    epd_send_data(0XA5);
}

/**
 * @brief 填充结束后会等待刷新完成
 * @param color 
 */
void epd_fill_color(uint8_t color)
{
    uint16_t Width, Height;
    Width = (EPD_7IN3E_WIDTH % 2 == 0) ? (EPD_7IN3E_WIDTH / 2) : (EPD_7IN3E_WIDTH / 2 + 1);
    Height = EPD_7IN3E_HEIGHT;

    epd_send_command(0x10);
    for (uint16_t j = 0; j < Height; j++)
    {
        for (uint16_t i = 0; i < Width; i++)
        {
            epd_send_data((color << 4) | color);
        }
    }

    epd_refresh();
}

void epd_fill_image(uint8_t *Image)
{
    uint16_t Width, Height;
    Width = (EPD_7IN3E_WIDTH % 2 == 0) ? (EPD_7IN3E_WIDTH / 2) : (EPD_7IN3E_WIDTH / 2 + 1);
    Height = EPD_7IN3E_HEIGHT;

    epd_send_command(0x10);
    for (uint16_t j = 0; j < Height; j++)
    {
        for (uint16_t i = 0; i < Width; i++)
        {
            epd_send_data(Image[i + j * Width]);
        }
    }
    epd_refresh();
}
uint8_t color = 0;
int main(void)
{
    gpio_pin_configure_dt(&epd_gpio_dc, GPIO_OUTPUT_LOW);
    gpio_pin_configure_dt(&epd_gpio_rst, GPIO_OUTPUT_HIGH);
    gpio_pin_configure_dt(&epd_gpio_busy, GPIO_INPUT);

    if (!gpio_is_ready_dt(&epd_gpio_dc) || !gpio_is_ready_dt(&epd_gpio_rst) || !gpio_is_ready_dt(&epd_gpio_busy))
    {
        LOG_ERR("GPIO device not ready");
        return 0;
    }

    if (!spi_is_ready_dt(&epd_spi))
    {
        LOG_ERR("SPI device not ready");
        return 0;
    }

    rgb_strip_on(BLUE);

    // 先复位再按逻辑分析仪，不然逻辑分析仪会跑飞
    // epd_wait_idle();
    // epd_reset();
    // k_sleep(K_MSEC(25));
    // epd_send_command(0x5a);
    // k_sleep(K_MSEC(25));
    // epd_send_data(0x75);
    // k_sleep(K_MSEC(25));
    // epd_sleep();
    // k_sleep(K_MSEC(25));
    // epd_init();

    epd_init();
    epd_fill_color(0x2);
    k_sleep(K_MSEC(10000));
    epd_fill_color(0x1);
    epd_sleep();

    while (1)
    {
        rgb_strip_on(color);
        k_sleep(K_MSEC(1000));
        rgb_strip_off();
        k_sleep(K_MSEC(1000));
    }
    
    return 0;
}

static void button_input_cb(struct input_event *evt, void *user_data)
{
    if (evt->sync == 0)
    {
        return;
    }

    color = (color + 1) % 3;
}

INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);