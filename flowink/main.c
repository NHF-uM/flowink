#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <rgb_strip.h>

#define DC_PIN 12
#define RST_PIN 13
#define BUSY_PIN 14
#define PIN_SET(pin) gpio_pin_set(epd_port, pin, 1)
#define PIN_CLR(pin) gpio_pin_set(epd_port, pin, 0)

#define EPD_7IN3E_WIDTH       800
#define EPD_7IN3E_HEIGHT      480

const struct device *epd_port = DEVICE_DT_GET(DT_NODELABEL(gpio0));
const struct device *epd_spi = DEVICE_DT_GET(DT_BUS(DT_NODELABEL(epd)));
// const struct device *epd_spi = DEVICE_DT_GET((DT_NODELABEL(spi2)));

// #define EPD_SPI_FLASS (SPI_OP_MODE_MASTER | )
// const struct spi_dt_spec epd_spi_spec = SPI_DT_SPEC_GET(DT_NODELABEL(epd), 0);

const struct spi_config epd_spi_cfg = {
    .frequency = 1125000,
    .cs = {
	.cs_is_gpio = true,
	.gpio = SPI_CS_GPIOS_DT_SPEC_GET(DT_NODELABEL(epd)),
	.delay = 0,
    },
    .operation = SPI_WORD_SET(8) | SPI_OP_MODE_MASTER,
    .slave = 0,
};

static void epd_reset(void)
{
    PIN_SET(RST_PIN);
    k_sleep(K_MSEC(20));
    PIN_CLR(RST_PIN);
    k_sleep(K_MSEC(2));
    PIN_SET(RST_PIN);
    k_sleep(K_MSEC(20));
}

static void epd_wait_idle(void)
{
    LOG_INF("e-Paper busy H\r\n");
    while (!gpio_pin_get(epd_port, BUSY_PIN))
    {
        k_sleep(K_MSEC(1));
    } 
    LOG_INF("e-Paper busy H release\r\n");
}

static void epd_send_command(uint8_t cmd)
{
    PIN_CLR(DC_PIN);
    struct spi_buf_set tx_buf = {
	.buffers = &(struct spi_buf){ .buf = &cmd, .len = 1 },
	.count = 1,
    };
    spi_write(epd_spi, &epd_spi_cfg, &tx_buf);
}

static void epd_send_data(uint8_t data)
{
    PIN_SET(DC_PIN);
    struct spi_buf_set tx_buf = {
	.buffers = &(struct spi_buf){ .buf = &data, .len = 1 },
	.count = 1,
    };
    spi_write(epd_spi, &epd_spi_cfg, &tx_buf);
}

void epd_init(void)
{
    epd_reset();
    epd_wait_idle();
    k_sleep(K_MSEC(30));

    epd_send_command(0xAA);    // CMDH
    epd_send_data(0x49);
    epd_send_data(0x55);
    epd_send_data(0x20);
    epd_send_data(0x08);
    epd_send_data(0x09);
    epd_send_data(0x18);

    epd_send_command(0x01);//
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

    epd_send_command(0x04);     //PWR on  
    epd_wait_idle();          //waiting for the electronic paper IC to release the idle signal
}

static void epd_refresh(void)
{
    epd_send_command(0x04); // POWER_ON
    epd_wait_idle();

    //Second setting 
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

void epd_fill_color(uint8_t color)
{
    uint16_t Width, Height;
    Width = (EPD_7IN3E_WIDTH % 2 == 0)? (EPD_7IN3E_WIDTH / 2 ): (EPD_7IN3E_WIDTH / 2 + 1);
    Height = EPD_7IN3E_HEIGHT;

    epd_send_command(0x10);
    for (uint16_t j = 0; j < Height; j++) 
    {
        for (uint16_t i = 0; i < Width; i++) 
	{
            epd_send_data((color<<4)|color);
        }
    }

    epd_refresh();
}

void epd_fill_image(uint8_t *Image)
{
    uint16_t Width, Height;
    Width = (EPD_7IN3E_WIDTH % 2 == 0)? (EPD_7IN3E_WIDTH / 2 ): (EPD_7IN3E_WIDTH / 2 + 1);
    Height = EPD_7IN3E_HEIGHT;

    epd_send_command(0x10);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            epd_send_data(Image[i + j * Width]);
        }
    }
    epd_refresh();
}

int main(void)
{
    gpio_pin_configure(epd_port, DC_PIN, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure(epd_port, RST_PIN, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure(epd_port, BUSY_PIN, GPIO_OUTPUT_INACTIVE);


    rgb_strip_on(BLUE);
    
    while (1) 
    {
        
    }
    return 0;
}
