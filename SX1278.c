#include "SX1278.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "esp_system.h"
#include "driver/spi.h"
#include "driver/gpio.h"


#define SLEEP_MODE_DEFAULT          0b10000000
#define STANDBY_MODE_DEFAULT        0b10000001
#define LORA_MODE                   0b10000000
#define LORA_TX_MODE                0b10000011
#define LORA_RX_CONTINUOUS_MODE     0b10000101
#define LORA_RX_SINGLE_MODE         0b10000110

#define BASE_FIFO_ADDR              0b00000000
#define RX_TIMEOUT_MASK             0b10000000
#define RX_DONE_MASK                0b01000000
#define PAYLOAD_CRC_ERROR_MASK      0b00100000
#define VALID_HEADER_MASK           0b00010000
#define TX_DONE_MASK                0b00001000
#define CRC_ON_PAYLOAD_MASK         0b01000000
#define RX_PAYLOAD_CRC_ON_MASK      0b00000100

#define LNA_DEFAULT                 0b00100000
#define HEADER_MODE_MASK            0b00000001
#define OPERATION_MODE_MASK         0b00000111

static TaskHandle_t tx_done_handle = NULL; 
static TaskHandle_t rx_done_handle = NULL; 


uint8_t read_single_access(uint8_t addr)
{
    uint32_t miso;
    spi_trans_t trans = {0};
    uint16_t cmd = ((uint16_t) 0 << 7) | ((uint16_t)addr);

    trans.cmd = &cmd;
    trans.bits.cmd = 8;
    trans.miso = &miso;
    trans.bits.miso = 8;

    spi_trans(HSPI_HOST, &trans);
    return miso;
}

void write_single_access(uint8_t addr, uint8_t data)
{
    uint32_t mosi = data;
    spi_trans_t trans = {0};
    uint16_t cmd = ((uint16_t) 1 << 7) | ((uint16_t)addr);

    trans.cmd = &cmd;
    trans.bits.cmd = 8;
    trans.mosi = &mosi;
    trans.bits.mosi = 8;

    spi_trans(HSPI_HOST, &trans);
}

void write_burst_access(uint8_t addr, uint8_t* data, uint8_t len)
{
    uint8_t size = len / 4 + 1;
    uint32_t* mosi = malloc(sizeof(uint32_t) * size);
    spi_trans_t trans = {0};
    uint16_t cmd = ((uint16_t) 1 << 7) | ((uint16_t)addr);

    uint32_t* pmosi = mosi;
    for (uint8_t i = 0; i < size; i++)
    {
        if (i == size - 1)
        {
            memcpy(pmosi, data, sizeof(uint8_t) * (len % 4));
        }
        else
        {
            memcpy(pmosi, data, sizeof(uint32_t));
        }
        pmosi += 4;
        data += 4;
    }

    trans.cmd = &cmd;
    trans.bits.cmd = 8;
    trans.mosi = mosi;
    trans.bits.mosi = 8 * len;

    spi_trans(HSPI_HOST, &trans);
}

void SX1278_reset()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL<<DEFAULT_SX1278_RESET_PIN;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    
    gpio_config(&io_conf);
    gpio_set_level(DEFAULT_SX1278_RESET_PIN, 1);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    gpio_set_level(DEFAULT_SX1278_RESET_PIN, 0);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    gpio_set_level(DEFAULT_SX1278_RESET_PIN, 1);
    vTaskDelay(5 / portTICK_PERIOD_MS);
}

void SX1278_fill_fifo(SX1278* dev, uint8_t* data, uint8_t len)
{
    uint16_t size = dev->fifo.size + len;
    ESP_ERROR_CHECK(size >= MAX_FIFO_BUFFER);
    memcpy(dev->fifo.buffer + dev->fifo.size, data, len);
    dev->fifo.size = size;
}

void SX1278_prepare_fifo(SX1278* dev, uint8_t len)
{
    dev->fifo.expected_size = len;
}

static void debug()
{
    printf("-----------------------------------------------------------------\n");
    printf("mode: %02x\n", read_single_access(REG_OPMODE));
    printf("config1: %02x\n", read_single_access(REG_MODEM_CONFIG1));
    printf("config2: %02x\n", read_single_access(REG_MODEM_CONFIG2));
    printf("freq: %02x%02x%02x\n", read_single_access(REG_FR_MSB), read_single_access(REG_FR_MID), read_single_access(REG_FR_LSB));
    printf("inv iq: %02x\n", read_single_access(REG_INVERT_IQ));
    printf("pa: %02x\n", read_single_access(REG_PA_CONFIG));
    printf("sync: %02x\n", read_single_access(REG_SYNC_WORD));
    printf("-----------------------------------------------------------------\n");
}

void SX1278_wait_for_tx_done(void* p)
{
    SX1278* dev = p;
    uint8_t flags;
    while (1)
    {
        vTaskDelay(300 / portTICK_PERIOD_MS);
        flags = read_single_access(REG_IRQ_FLAGS);
        if ((flags & TX_DONE_MASK) != 0)
        {
            xTaskNotifyGive(dev->tx_done_handle);
            write_single_access(REG_IRQ_FLAGS, flags & (TX_DONE_MASK ^ 1));
            vTaskDelete(tx_done_handle);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void SX1278_start_tx(SX1278* dev)
{
    write_single_access(REG_OPMODE, STANDBY_MODE_DEFAULT);
    write_single_access(REG_FIFO_TX_BASE_ADDR, BASE_FIFO_ADDR);
    write_single_access(REG_FIFO_ADDR_PTR, BASE_FIFO_ADDR);
    for (uint8_t i = 0; i < dev->fifo.size; i++)
    {
        write_single_access(REG_FIFO, dev->fifo.buffer[i]);
    }
    write_single_access(REG_PAYLOAD_LENGTH, dev->fifo.size);
    write_single_access(REG_OPMODE, LORA_TX_MODE);
    // debug();
    xTaskCreate(SX1278_wait_for_tx_done, "tx_done", 1024, (void*)dev, tskIDLE_PRIORITY, &tx_done_handle);
}

void SX1278_wait_for_rx_done(void* p)
{
    SX1278* dev = p;
    uint8_t flags, valid_crc, required_crc, pfifo;
    uint8_t hmode = read_single_access(REG_MODEM_CONFIG1) & HEADER_MODE_MASK;
    uint8_t rxmode = read_single_access(REG_OPMODE) & OPERATION_MODE_MASK; 
    while (1)
    {
        flags = read_single_access(REG_IRQ_FLAGS);
        if ((flags & RX_DONE_MASK) != 0)
        {
            required_crc = hmode == 0 ? (read_single_access(REG_HOP_CHANNEL) & CRC_ON_PAYLOAD_MASK) : (read_single_access(REG_MODEM_CONFIG2) & RX_PAYLOAD_CRC_ON_MASK);
            valid_crc = (flags & PAYLOAD_CRC_ERROR_MASK) & required_crc;
            if ((flags & VALID_HEADER_MASK) != 0 && valid_crc == 0)
            {
                pfifo = read_single_access(REG_FIFO_RX_CURRENT_ADDR);
                dev->fifo.size = hmode == 0 ? read_single_access(REG_RX_NB_BYTES) : read_single_access(REG_PAYLOAD_LENGTH);
                write_single_access(REG_FIFO_ADDR_PTR, pfifo);
                for (uint8_t i = 0; i < dev->fifo.size; i++)
                {
                    dev->fifo.buffer[i] = read_single_access(REG_FIFO);
                }
                if (rxmode == RxContinuous)
                {
                    write_single_access(REG_FIFO_ADDR_PTR, BASE_FIFO_ADDR);
                }

                uint8_t rssi = read_single_access(REG_PKT_RSSI_VALUE);
                if (dev->settings.channel_freq > MID_RANGE_FREQ_THRESHOLD)
                {
                    dev->pkt_status.rssi = RSSI_OFFSET_HF + rssi + (rssi >> 4);
                }
                else
                {
                    dev->pkt_status.rssi = RSSI_OFFSET_LF + rssi + (rssi >> 4);
                }
                dev->pkt_status.snr = (int8_t)read_single_access(REG_PKT_SNR_VALUE) / 4;
            }

            // debug();
            write_single_access(REG_IRQ_FLAGS, flags & (RX_DONE_MASK ^ 1));
            write_single_access(REG_IRQ_FLAGS, flags & (VALID_HEADER_MASK ^ 1));
            write_single_access(REG_IRQ_FLAGS, flags & (PAYLOAD_CRC_ERROR_MASK ^ 1));
            xTaskNotifyGive(dev->rx_done_handle);
            if (rxmode == RxSingle)
            {
                vTaskDelete(rx_done_handle);
            }
        }
        else if ((flags & RX_TIMEOUT_MASK) != 0)
        {
            dev->fifo.size = 0;
            write_single_access(REG_IRQ_FLAGS, flags & (RX_TIMEOUT_MASK ^ 1));
            xTaskNotifyGive(dev->rx_done_handle);
            vTaskDelete(rx_done_handle);
        }
        else
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}

void SX1278_start_rx(SX1278* dev, OperationMode rx_mode, HeaderMode header_mode)
{
    write_single_access(REG_OPMODE, STANDBY_MODE_DEFAULT);
    write_single_access(REG_FIFO_RX_BASE_ADDR, BASE_FIFO_ADDR);
    write_single_access(REG_FIFO_ADDR_PTR, BASE_FIFO_ADDR);
    if (header_mode == ImplicitHeaderMode)
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(dev->fifo.size == 0);
        write_single_access(REG_PAYLOAD_LENGTH, dev->fifo.size);
    }
    switch (rx_mode)
    {
    case RxContinuous: write_single_access(REG_OPMODE, LORA_RX_CONTINUOUS_MODE); break;
    case RxSingle: write_single_access(REG_OPMODE, LORA_RX_SINGLE_MODE); break;
    default: ESP_ERROR_CHECK(1); break;
    }
    xTaskCreate(SX1278_wait_for_rx_done, "rx_done", 1024, (void*)dev, tskIDLE_PRIORITY, &rx_done_handle);
}

void SX1278_switch_mode(SX1278* dev, OperationMode mode)
{
    if ((read_single_access(REG_OPMODE) & OPERATION_MODE_MASK) == RxContinuous)
    {
        xTaskNotifyGive(dev->rx_done_handle);
        vTaskDelete(dev->rx_done_handle);
    }
    write_single_access(REG_OPMODE, LORA_MODE | mode);
}

SX1278* SX1278_create()
{
    SX1278_reset();

    SX1278* device = malloc(sizeof(SX1278));
    device->fifo.size = 0;
    device->fifo.expected_size = 0;
    device->rx_done_handle = NULL;
    device->tx_done_handle = NULL;

    spi_config_t spi_config = {0};
    spi_config.interface.val = SPI_DEFAULT_INTERFACE;
    spi_config.intr_enable.val = SPI_MASTER_DEFAULT_INTR_ENABLE;
    spi_config.mode = SPI_MASTER_MODE;
    spi_config.clk_div = SPI_10MHz_DIV;
    spi_init(HSPI_HOST, &spi_config);

    vTaskDelay(200 / portTICK_PERIOD_MS);

    return device;
}

void SX1278_destroy(SX1278* device)
{
    free(device);
    spi_deinit(HSPI_HOST);
    
    vTaskDelay(200 / portTICK_PERIOD_MS);
}


void SX1278_initialize(SX1278* device, SX1278Settings* settings)
{
    write_single_access(REG_OPMODE, SLEEP_MODE_DEFAULT);
    write_single_access(REG_OPMODE, LORA_MODE);

    SX1278_set_frequency(device, settings->channel_freq);
    write_single_access(REG_PA_CONFIG, settings->pa_config.val);
    write_single_access(REG_MODEM_CONFIG1, settings->modem_config1.val);
    write_single_access(REG_MODEM_CONFIG2, settings->modem_config2.val);
    write_single_access(REG_SYNC_WORD, settings->sync_word);
    write_single_access(REG_INVERT_IQ, settings->invert_iq.val);

    vTaskDelay(200 / portTICK_PERIOD_MS);
    debug();
}

void SX1278_set_txpower(SX1278* device, TxPower txpower)
{
    uint8_t mode = read_single_access(REG_OPMODE);
    write_single_access(REG_OPMODE, STANDBY_MODE_DEFAULT);

    uint8_t pa_config = (DEFAULT_PA_CONFIG & 0b1111) | txpower;
    write_single_access(REG_PA_CONFIG, pa_config);

    write_single_access(REG_OPMODE, mode);
}

void SX1278_set_frequency(SX1278* device, ChannelFrequency freq)
{
    uint8_t mode = read_single_access(REG_OPMODE);
    write_single_access(REG_OPMODE, STANDBY_MODE_DEFAULT);

    write_single_access(REG_FR_LSB, freq & 0xff);
    freq = freq >> 8;
    write_single_access(REG_FR_MID, freq & 0xff);
    freq = freq >> 8;
    write_single_access(REG_FR_MSB, freq & 0xff);

    write_single_access(REG_OPMODE, mode);
}

void SX1278_set_iq(SX1278* device, uint8_t value)
{
    write_single_access(REG_INVERT_IQ, value);
}