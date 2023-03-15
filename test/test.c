#include "unity.h"
#include "test_utils.h"
#include "SX1278.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static SX1278Settings settings = {
    .channel_freq = DEFAULT_SX1278_FREQUENCY,
    .pa_config.val = DEFAULT_PA_CONFIG,
    .preamble_len = DEFAULT_PREAMBLE_LENGTH,
    .modem_config1.val = DEFAULT_MODEM_CONFIG1,
    .modem_config2.val = DEFAULT_MODEM_CONFIG2,
    .sync_word = DEFAULT_SYNC_WORD,
    .invert_iq.val = DEFAULT_NORMAL_IQ,
};
static uint8_t expected[] = {'s', 'x', '1', '2', '7', '8'};
SX1278* dev;
static uint8_t task_done = 1;

void on_tx_done(void* p)
{
    SX1278* dev = p;
    while (1)
    {
        if (ulTaskNotifyTake(pdTRUE, (TickType_t) portMAX_DELAY) > 0)
        {
            task_done = 0;
            vTaskDelete(dev->tx_done_handle);
        }
    }
}

void on_rx_done(void* p)
{
    SX1278* dev = p;
    while (1)
    {
        if (ulTaskNotifyTake(pdTRUE, (TickType_t) portMAX_DELAY) > 0)
        {
            task_done = 0;
            vTaskDelete(dev->rx_done_handle);
        }
    }
}

TEST_CASE("create device", "[init]")
{
    dev = SX1278_create();
}

TEST_CASE("destroy device", "[init]")
{
    SX1278_destroy(dev);
}

static void sender()
{
    xTaskCreate(on_tx_done, "tx_done", 1024, (void*)dev, tskIDLE_PRIORITY, &dev->tx_done_handle);
    dev->fifo.size = 0;
    SX1278_fill_fifo(dev, expected, sizeof(expected));
    SX1278_start_tx(dev);
    
    while (task_done) { vTaskDelay(100 / portTICK_PERIOD_MS); };
    task_done = 1;
    unity_send_signal("Sender sent");
}

static void receiver()
{
    xTaskCreate(on_rx_done, "rx_done", 1024, (void*)dev, tskIDLE_PRIORITY, &dev->rx_done_handle);
    SX1278_start_rx(dev, RxContinuous, ExplicitHeaderMode);

    unity_send_signal("Receiver ready");
    long start = xTaskGetTickCount();
    while (task_done) 
    { 
        long time = (xTaskGetTickCount() - start) * portTICK_PERIOD_MS;
        if (time > 5000) 
        { 
            TEST_FAIL_MESSAGE("Timeout 5 seconds");
            SX1278_switch_mode(dev, Sleep);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS); 
    };
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, dev->fifo.buffer, sizeof(expected));
    task_done = 1;
}

/////////////////////////////   CRC    /////////////////////////////////////////

static void crc_presence_sender()
{
    settings.modem_config2.bits.rx_payload_crc_on = 1;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

static void crc_presence_receiver()
{
    settings.modem_config2.bits.rx_payload_crc_on = 1;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

TEST_CASE_MULTIPLE_DEVICES("Test CRC presence", "[sx1278][CRC]", crc_presence_sender, crc_presence_receiver);

static void crc_empty_sender()
{
    settings.modem_config2.bits.rx_payload_crc_on = 0;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

static void crc_empty_receiver()
{
    settings.modem_config2.bits.rx_payload_crc_on = 0;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

TEST_CASE_MULTIPLE_DEVICES("Test CRC empty", "[sx1278][CRC]", crc_empty_sender, crc_empty_receiver);

///////////////////////////// Lora IQ  /////////////////////////////////////////

static void iq_normal_sender()
{
    settings.invert_iq.bits.mode = 0;
    SX1278_initialize(dev, &settings);
    sender();
    settings.invert_iq.val = DEFAULT_INVERT_IQ;
}

static void iq_normal_receiver()
{
    settings.invert_iq.bits.mode = 0;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.invert_iq.val = DEFAULT_INVERT_IQ;
}

TEST_CASE_MULTIPLE_DEVICES("Test Lora IQ normal", "[sx1278][IQ]", iq_normal_sender, iq_normal_receiver);

static void iq_inverted_sender()
{
    settings.invert_iq.val = 0x66;
    SX1278_initialize(dev, &settings);
    sender();
    settings.invert_iq.val = DEFAULT_INVERT_IQ;
}

static void iq_inverted_receiver()
{
    settings.invert_iq.val = 0x66;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.invert_iq.val = DEFAULT_INVERT_IQ;
}

TEST_CASE_MULTIPLE_DEVICES("Test Lora IQ inverted", "[sx1278][IQ]", iq_inverted_sender, iq_inverted_receiver);

/////////////////////////////    SF    /////////////////////////////////////////

static void SF7_sender()
{
    settings.modem_config2.bits.spreading_factor = SF7;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

static void SF7_receiver()
{
    settings.modem_config2.bits.spreading_factor = SF7;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

TEST_CASE_MULTIPLE_DEVICES("Test SpreadingFactor SF7", "[sx1278][SF]", SF7_sender, SF7_receiver);

static void SF8_sender()
{
    settings.modem_config2.bits.spreading_factor = SF8;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

static void SF8_receiver()
{
    settings.modem_config2.bits.spreading_factor = SF8;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

TEST_CASE_MULTIPLE_DEVICES("Test SpreadingFactor SF8", "[sx1278][SF]", SF8_sender, SF8_receiver);

static void SF9_sender()
{
    settings.modem_config2.bits.spreading_factor = SF9;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

static void SF9_receiver()
{
    settings.modem_config2.bits.spreading_factor = SF9;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

TEST_CASE_MULTIPLE_DEVICES("Test SpreadingFactor SF9", "[sx1278][SF]", SF9_sender, SF9_receiver);

static void SF10_sender()
{
    settings.modem_config2.bits.spreading_factor = SF10;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

static void SF10_receiver()
{
    settings.modem_config2.bits.spreading_factor = SF10;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

TEST_CASE_MULTIPLE_DEVICES("Test SpreadingFactor SF10", "[sx1278][SF]", SF10_sender, SF10_receiver);

static void SF11_sender()
{
    settings.modem_config2.bits.spreading_factor = SF11;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

static void SF11_receiver()
{
    settings.modem_config2.bits.spreading_factor = SF11;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

TEST_CASE_MULTIPLE_DEVICES("Test SpreadingFactor SF11", "[sx1278][SF]", SF11_sender, SF11_receiver);

static void SF12_sender()
{
    settings.modem_config2.bits.spreading_factor = SF12;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

static void SF12_receiver()
{
    settings.modem_config2.bits.spreading_factor = SF12;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config2.val = DEFAULT_MODEM_CONFIG2;
}

TEST_CASE_MULTIPLE_DEVICES("Test SpreadingFactor SF12", "[sx1278][SF]", SF12_sender, SF12_receiver);

/////////////////////////////    BW    /////////////////////////////////////////

static void Bw7_8kHz_sender()
{
    settings.modem_config1.bits.bandwidth = Bw7_8kHz;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

static void Bw7_8kHz_receiver()
{
    settings.modem_config1.bits.bandwidth = Bw7_8kHz;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

TEST_CASE_MULTIPLE_DEVICES("Test Bandwidth Bw7_8kHz", "[sx1278][BW]", Bw7_8kHz_sender, Bw7_8kHz_receiver);

static void Bw10_4kHz_sender()
{
    settings.modem_config1.bits.bandwidth = Bw10_4kHz;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

static void Bw10_4kHz_receiver()
{
    settings.modem_config1.bits.bandwidth = Bw10_4kHz;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

TEST_CASE_MULTIPLE_DEVICES("Test Bandwidth Bw10_4kHz", "[sx1278][BW]", Bw10_4kHz_sender, Bw10_4kHz_receiver);

static void Bw15_6kHz_sender()
{
    settings.modem_config1.bits.bandwidth = Bw15_6kHz;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

static void Bw15_6kHz_receiver()
{
    settings.modem_config1.bits.bandwidth = Bw15_6kHz;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

TEST_CASE_MULTIPLE_DEVICES("Test Bandwidth Bw15_6kHz", "[sx1278][BW]", Bw15_6kHz_sender, Bw15_6kHz_receiver);

static void Bw20_8kHz_sender()
{
    settings.modem_config1.bits.bandwidth = Bw20_8kHz;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

static void Bw20_8kHz_receiver()
{
    settings.modem_config1.bits.bandwidth = Bw20_8kHz;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

TEST_CASE_MULTIPLE_DEVICES("Test Bandwidth Bw20_8kHz", "[sx1278][BW]", Bw20_8kHz_sender, Bw20_8kHz_receiver);

static void Bw31_25kHz_sender()
{
    settings.modem_config1.bits.bandwidth = Bw31_25kHz;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

static void Bw31_25kHz_receiver()
{
    settings.modem_config1.bits.bandwidth = Bw31_25kHz;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

TEST_CASE_MULTIPLE_DEVICES("Test Bandwidth Bw31_25kHz", "[sx1278][BW]", Bw31_25kHz_sender, Bw31_25kHz_receiver);

static void Bw41_7kHz_sender()
{
    settings.modem_config1.bits.bandwidth = Bw41_7kHz;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

static void Bw41_7kHz_receiver()
{
    settings.modem_config1.bits.bandwidth = Bw41_7kHz;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

TEST_CASE_MULTIPLE_DEVICES("Test Bandwidth Bw41_7kHz", "[sx1278][BW]", Bw41_7kHz_sender, Bw41_7kHz_receiver);

static void Bw62_5kHz_sender()
{
    settings.modem_config1.bits.bandwidth = Bw62_5kHz;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

static void Bw62_5kHz_receiver()
{
    settings.modem_config1.bits.bandwidth = Bw62_5kHz;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

TEST_CASE_MULTIPLE_DEVICES("Test Bandwidth Bw62_5kHz", "[sx1278][BW]", Bw62_5kHz_sender, Bw62_5kHz_receiver);

static void Bw125kHz_sender()
{
    settings.modem_config1.bits.bandwidth = Bw125kHz;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

static void Bw125kHz_receiver()
{
    settings.modem_config1.bits.bandwidth = Bw125kHz;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

TEST_CASE_MULTIPLE_DEVICES("Test Bandwidth Bw125kHz", "[sx1278][BW]", Bw125kHz_sender, Bw125kHz_receiver);

static void Bw250kHz_sender()
{
    settings.modem_config1.bits.bandwidth = Bw250kHz;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

static void Bw250kHz_receiver()
{
    settings.modem_config1.bits.bandwidth = Bw250kHz;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

TEST_CASE_MULTIPLE_DEVICES("Test Bandwidth Bw250kHz", "[sx1278][BW]", Bw250kHz_sender, Bw250kHz_receiver);

static void Bw500kHz_sender()
{
    settings.modem_config1.bits.bandwidth = Bw500kHz;
    SX1278_initialize(dev, &settings);
    sender();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

static void Bw500kHz_receiver()
{
    settings.modem_config1.bits.bandwidth = Bw500kHz;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.modem_config1.val = DEFAULT_MODEM_CONFIG1;
}

TEST_CASE_MULTIPLE_DEVICES("Test Bandwidth Bw500kHz", "[sx1278][BW]", Bw500kHz_sender, Bw500kHz_receiver);

/////////////////////////////   Freq   /////////////////////////////////////////

static void RF433_175MHZ_sender()
{
    settings.channel_freq = RF433_175MHZ;
    SX1278_initialize(dev, &settings);
    sender();
    settings.channel_freq = DEFAULT_SX1278_FREQUENCY;
}

static void RF433_175MHZ_receiver()
{
    settings.channel_freq = RF433_175MHZ;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.channel_freq = DEFAULT_SX1278_FREQUENCY;
}

TEST_CASE_MULTIPLE_DEVICES("Test Frequency RF433_175MHZ", "[sx1278][Freq]", RF433_175MHZ_sender, RF433_175MHZ_receiver);

static void RF433_375MHZ_sender()
{
    settings.channel_freq = RF433_375MHZ;
    SX1278_initialize(dev, &settings);
    sender();
    settings.channel_freq = DEFAULT_SX1278_FREQUENCY;
}

static void RF433_375MHZ_receiver()
{
    settings.channel_freq = RF433_375MHZ;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.channel_freq = DEFAULT_SX1278_FREQUENCY;
}

TEST_CASE_MULTIPLE_DEVICES("Test Frequency RF433_375MHZ", "[sx1278][Freq]", RF433_375MHZ_sender, RF433_375MHZ_receiver);

static void RF433_575MHZ_sender()
{
    settings.channel_freq = RF433_575MHZ;
    SX1278_initialize(dev, &settings);
    sender();
    settings.channel_freq = DEFAULT_SX1278_FREQUENCY;
}

static void RF433_575MHZ_receiver()
{
    settings.channel_freq = RF433_575MHZ;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.channel_freq = DEFAULT_SX1278_FREQUENCY;
}

TEST_CASE_MULTIPLE_DEVICES("Test Frequency RF433_575MHZ", "[sx1278][Freq]", RF433_575MHZ_sender, RF433_575MHZ_receiver);


/////////////////////////////  TxPower /////////////////////////////////////////

static void TxPower0_sender()
{
    settings.pa_config.bits.output_power = TxPower0;
    SX1278_initialize(dev, &settings);
    sender();
    settings.pa_config.bits.output_power = TxPower0;
}

static void TxPower0_receiver()
{
    settings.pa_config.bits.output_power = TxPower0;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.pa_config.bits.output_power = TxPower0;
}

TEST_CASE_MULTIPLE_DEVICES("Test Max EIRP", "[sx1278][TxPower]", TxPower0_sender, TxPower0_receiver);

static void TxPower1_sender()
{
    settings.pa_config.bits.output_power = TxPower1;
    SX1278_initialize(dev, &settings);
    sender();
    settings.pa_config.bits.output_power = TxPower0;
}

static void TxPower1_receiver()
{
    settings.pa_config.bits.output_power = TxPower1;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.pa_config.bits.output_power = TxPower0;
}

TEST_CASE_MULTIPLE_DEVICES("Test Max EIRP - 2dbm", "[sx1278][TxPower]", TxPower1_sender, TxPower1_receiver);

static void TxPower2_sender()
{
    settings.pa_config.bits.output_power = TxPower2;
    SX1278_initialize(dev, &settings);
    sender();
    settings.pa_config.bits.output_power = TxPower0;
}

static void TxPower2_receiver()
{
    settings.pa_config.bits.output_power = TxPower2;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.pa_config.bits.output_power = TxPower0;
}

TEST_CASE_MULTIPLE_DEVICES("Test Max EIRP - 4dbm", "[sx1278][TxPower]", TxPower2_sender, TxPower2_receiver);


static void TxPower3_sender()
{
    settings.pa_config.bits.output_power = TxPower3;
    SX1278_initialize(dev, &settings);
    sender();
    settings.pa_config.bits.output_power = TxPower0;
}

static void TxPower3_receiver()
{
    settings.pa_config.bits.output_power = TxPower3;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.pa_config.bits.output_power = TxPower0;
}

TEST_CASE_MULTIPLE_DEVICES("Test Max EIRP - 6dbm", "[sx1278][TxPower]", TxPower3_sender, TxPower3_receiver);

static void TxPower4_sender()
{
    settings.pa_config.bits.output_power = TxPower4;
    SX1278_initialize(dev, &settings);
    sender();
    settings.pa_config.bits.output_power = TxPower0;
}

static void TxPower4_receiver()
{
    settings.pa_config.bits.output_power = TxPower4;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.pa_config.bits.output_power = TxPower0;
}

TEST_CASE_MULTIPLE_DEVICES("Test Max EIRP - 8dbm", "[sx1278][TxPower]", TxPower4_sender, TxPower4_receiver);

static void TxPower5_sender()
{
    settings.pa_config.bits.output_power = TxPower5;
    SX1278_initialize(dev, &settings);
    sender();
    settings.pa_config.bits.output_power = TxPower0;
}

static void TxPower5_receiver()
{
    settings.pa_config.bits.output_power = TxPower5;
    SX1278_initialize(dev, &settings);
    receiver();
    settings.pa_config.bits.output_power = TxPower0;
}

TEST_CASE_MULTIPLE_DEVICES("Test Max EIRP - 10dbm", "[sx1278][TxPower]", TxPower5_sender, TxPower5_receiver);