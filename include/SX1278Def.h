#ifndef SX1278DEF_H
#define SX1278DEF_H

#include "stdint.h"

#define REG_FIFO                        0x00
#define REG_OPMODE                      0x01
#define REG_FR_MSB                      0x06
#define REG_FR_MID                      0x07
#define REG_FR_LSB                      0x08
#define REG_PA_CONFIG                   0x09
#define REG_PA_RAMP                     0x0a
#define REG_OCP                         0x0b
#define REG_LNA                         0x0c
#define REG_FIFO_ADDR_PTR               0x0d
#define REG_FIFO_TX_BASE_ADDR           0x0e
#define REG_FIFO_RX_BASE_ADDR           0x0f
#define REG_FIFO_RX_CURRENT_ADDR        0x10
#define REG_IRQ_FLAGS_MASK              0x11
#define REG_IRQ_FLAGS                   0x12
#define REG_RX_NB_BYTES                 0x13
#define REG_RX_HEADER_CNT_VALUE_MSB     0x14
#define REG_RX_HEADER_CNT_VALUE_LSB     0x15
#define REG_RX_PACKET_CNT_VALUE_LSB     0x16
#define REG_RX_PACKET_CNT_VALUE_MSB     0x17
#define REG_MODEM_STAT                  0x18
#define REG_PKT_SNR_VALUE               0x19
#define REG_PKT_RSSI_VALUE              0x1a
#define REG_RSSI_VALUE                  0x1b
#define REG_HOP_CHANNEL                 0x1c
#define REG_MODEM_CONFIG1               0x1d
#define REG_MODEM_CONFIG2               0x1e
#define REG_SYMB_TIMOUT_LSB             0x1f
#define REG_PREAMBLE_MSB                0x20
#define REG_PREAMBLE_LSB                0x21
#define REG_PAYLOAD_LENGTH              0x22
#define REG_MAX_PAYLOAD_LENGTH          0x23
#define REG_HOP_PERIOD                  0x24
#define REG_FIFO_RX_BYTE_ADDR           0x25
#define REG_MODEM_CONFIG3               0x26
#define REG_PPM_CORRECTION              0x27
#define REG_FEI_MSB                     0x28
#define REG_FEI_MID                     0x29
#define REG_FEI_LSB                     0x2a
#define REG_RSSI_WIDEBAND               0x2c
#define REG_DETECT_OPTIMIZE             0x31
#define REG_INVERT_IQ                   0x33
#define REG_DETECTION_THRESHOLD         0x37
#define REG_SYNC_WORD                   0x39
#define REG_VERSION                     0x42


typedef enum HeaderMode_enum {
    ImplicitHeaderMode = 0,
    ExplicitHeaderMode = 1
} HeaderMode;

typedef enum OperationMode_struct {
    Sleep = 0,
    Standby,
    Fstx,
    Tx,
    Fxrx,
    RxContinuous,
    RxSingle,
    Cad
} OperationMode;

typedef enum CodingRate_enum
{
    CR5 = 1,
    CR6,
    CR7,
    CR8
} CodingRate;

typedef enum Bandwidth_enum
{
    Bw7_8kHz = 0,
    Bw10_4kHz,
    Bw15_6kHz,
    Bw20_8kHz,
    Bw31_25kHz,
    Bw41_7kHz,
    Bw62_5kHz,
    Bw125kHz,
    Bw250kHz,
    Bw500kHz
} Bandwidth;

typedef enum SpreadingFactor_enum
{
    SF7 = 7,
    SF8,
    SF9,
    SF10,
    SF11,
    SF12,
} SpreadingFactor;

typedef enum TxPower_enum
{
    TxPower5 = 0,
    TxPower4 = 1,
    TxPower3 = 2,
    TxPower2 = 4,
    TxPower1 = 8,
    TxPower0 = 14,
} TxPower;

typedef enum ChannelFrequency_enum
{
    RF433_175MHZ = 0x6C8B35,
    RF433_375MHZ = 0x6C9800,
    RF433_575MHZ = 0x6CA4CD,
} ChannelFrequency;

typedef struct SX1278Settings_struct
{
    ChannelFrequency channel_freq;
    union {
        struct {
            uint8_t output_power: 4;
            uint8_t max_power: 3;
            uint8_t pa_select: 1;
        } bits;
        uint8_t val;
    } pa_config;
    uint16_t preamble_len;
    union {
        struct {
            uint8_t implicit_header_on: 1;
            uint8_t coding_rate: 3;
            uint8_t bandwidth: 4;
        } bits;
        uint8_t val;
    } modem_config1;
    union {
        struct {
            uint8_t symb_timeout: 2;
            uint8_t rx_payload_crc_on: 1;
            uint8_t tx_continuous_mode: 1;
            uint8_t spreading_factor: 4;
        } bits;
        uint8_t val;
    } modem_config2;
    union {
        struct {
            uint8_t reserverd2: 6;
            uint8_t mode: 1;
            uint8_t reserverd1: 1;
        } bits;
        uint8_t val;
    } invert_iq;
    uint8_t sync_word;
} SX1278Settings;


#endif //SX1278DEF_H