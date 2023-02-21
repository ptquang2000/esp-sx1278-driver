#ifndef SX1278_H
#define SX1278_H

#include "SX1278Def.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MAX_FIFO_BUFFER             256

#define DEFAULT_PREAMBLE_LENGTH     0x08
#define DEFAULT_MODEM_CONFIG1       0x72
#define DEFAULT_MODEM_CONFIG2       0x70
#define DEFAULT_SYNC_WORD           0x24
#define DEFAULT_INVERT_IQ           0x27
#define DEFAULT_PA_CONFIG           0x8f
#define DEFAULT_SX1278_RESET_PIN    0x04

typedef struct FIFO_struct
{
    uint8_t expected_size;
    uint8_t size;
    uint8_t buffer[MAX_FIFO_BUFFER];
} FIFO;

typedef struct SX1278_struct
{
    FIFO fifo;
    TaskHandle_t tx_done_handle;
    TaskHandle_t rx_done_handle;
    SX1278Settings settings;
} SX1278;

SX1278* SX1278_create(SX1278Settings* settings);
void SX1278_destroy(SX1278* dev);
void SX1278_switch_mode(SX1278* dev, OperationMode mode);
void SX1278_fill_fifo(SX1278* dev, uint8_t* data, uint8_t len);
void SX1278_start_tx(SX1278* dev);
void SX1278_start_rx(SX1278* dev, OperationMode rx_mode, HeaderMode header_mode);
uint8_t SX1278_get_fifo(SX1278* dev, uint8_t* data);
void SX1278_reset();


#endif