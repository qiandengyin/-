#include "esp_log.h"
#include "esp_check.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "esp32s3/rom/ets_sys.h"

#include "bsp_74hc165.h"


#define _74HC165D_HOST      SPI2_HOST
#define _74HC165D_MISO_PIN  13
#define _74HC165D_SCLK_PIN  12
#define _74HC165D_PL_PIN    3 // PL数据读取控制
#define _74HC165D_CE_PIN    7 // 保持低电平
#define GPIO_OUTPUT_PIN_SEL ((1ULL << _74HC165D_PL_PIN) | (1ULL << _74HC165D_CE_PIN))

static spi_device_handle_t spi_74hc165d;
static bool _74hc165d_inited = false;

void bsp_74hc165d_init(void)
{
    esp_err_t ret;

    gpio_config_t io_conf = {0};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(_74HC165D_PL_PIN, 0);
    gpio_set_level(_74HC165D_CE_PIN, 0);

    spi_bus_config_t buscfg = {
        .miso_io_num = _74HC165D_MISO_PIN,
        .mosi_io_num = -1,
        .sclk_io_num = _74HC165D_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        // .max_transfer_sz = 16 * 320 * 2 + 8, // 开启DMA, 默认4096字节
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000, // Clock out at 1 MHz
        .mode = 2,                         // SPI mode 2
        .spics_io_num = -1,                // CS pin
        .queue_size = 7,                   // We want to be able to queue 7 transactions at a time
    };
    ret = spi_bus_initialize(_74HC165D_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    ret = spi_bus_add_device(_74HC165D_HOST, &devcfg, &spi_74hc165d);
    ESP_ERROR_CHECK(ret);

    _74hc165d_inited = true;
}

static esp_err_t bsp_spi_transfer_bytes(const uint8_t *data_out, uint8_t *data_in, uint32_t data_len)
{
    esp_err_t ret;
    spi_transaction_t trans = {
        .length = data_len * 8,
        .tx_buffer = NULL,
        .rx_buffer = NULL,
    };

    if (data_out)
    {
        trans.tx_buffer = data_out;
    }

    if (data_in)
    {
        trans.rx_buffer = data_in;
    }

    ret = spi_device_polling_transmit(spi_74hc165d, &trans);

    return ret;
}

/// @brief 读取74HC165D数据
/// @param buffer 
/// @param len 
/// @return 
void bsp_74hc165d_read(uint8_t *buffer, int len)
{
    if (!_74hc165d_inited)
        return;
    gpio_set_level(_74HC165D_PL_PIN, 1);
    ets_delay_us(10);
    bsp_spi_transfer_bytes(NULL, buffer, len);
    gpio_set_level(_74HC165D_PL_PIN, 0);
}