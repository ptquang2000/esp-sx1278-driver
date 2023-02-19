#ifndef SX1278_H
#define SX1278_H

#include "SX1278Def.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MAX_FIFO_BUFFER    256


typedef struct FIFO_struct
{
    uint8_t expected_size;
    uint8_t size;
    uint8_t buffer[MAX_FIFO_BUFFER];
} FIFO;

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

typedef struct SX1278_struct
{
    FIFO fifo;
    TaskHandle_t tx_done_handle;
    TaskHandle_t rx_done_handle;
    SX1278Settings settings;
} SX1278;

SX1278* SX1278_create(SX1278Settings* settings);
void SX1278_destroy(SX1278* dev);
void SX1278_fill_fifo(SX1278* dev, uint8_t* data, uint8_t len);
void SX1278_start_tx(SX1278* dev);
void SX1278_start_rx(SX1278* dev, OperationMode rx_mode, HeaderMode header_mode);
uint8_t SX1278_get_fifo(SX1278* dev, uint8_t* data);

#endif