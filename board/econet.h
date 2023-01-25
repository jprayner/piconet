#ifndef _PICONET_ECONET_H_
#define _PICONET_ECONET_H_

#include "pico/stdlib.h"

typedef enum {
    PICONET_TX_RESULT_OK = 0L,
    PICONET_TX_RESULT_ERROR_MISC,
    PICONET_TX_RESULT_ERROR_NO_ACK,
    PICONET_TX_RESULT_ERROR_TIMEOUT
} econet_tx_result_t;

typedef enum {
    PICONET_RX_RESULT_NONE = 0L,
    PICONET_RX_RESULT_ERROR,
    PICONET_RX_RESULT_BROADCAST,
    PICONET_RX_RESULT_IMMEDIATE_OP,
    PICONET_RX_RESULT_TRANSMIT,
    PICONET_RX_RESULT_MONITOR
} econet_rx_result_type_t;

typedef enum {
    ECONET_RX_ERROR_NONE = 0L,
    ECONET_RX_ERROR_MISC,
    ECONET_RX_ERROR_UNINITIALISED,
    ECONET_RX_ERROR_CRC,
    ECONET_RX_ERROR_OVERRUN,
    ECONET_RX_ERROR_ABORT,
    ECONET_RX_ERROR_TIMEOUT,
    ECONET_RX_ERROR_OVERFLOW,
    ECONET_RX_ERROR_SCOUT_ACK,
    ECONET_RX_ERROR_DATA_ACK
} econet_rx_error_t;

typedef struct {
    uint8_t*    scout;
    size_t      scout_len;
    uint8_t*    data;
    size_t      data_len;
} econet_rx_result_detail_t;

typedef struct
{
    econet_rx_result_type_t         type;
    union {
        econet_rx_error_t           error;      /* if type is PICONET_RX_RESULT_ERROR */
        econet_rx_result_detail_t   detail;     /* if type not PICONET_RX_RESULT_NONE or PICONET_RX_RESULT_ERROR */
    };
} econet_rx_result_t;

bool                    econet_init(
                            uint8_t*    rx_scout_buffer,
                            size_t      rx_scout_buffer_sz,
                            uint8_t*    rx_data_buffer,
                            size_t      rx_data_buffer_sz,
                            uint8_t*    ack_buffer,
                            size_t      ack_buffer_sz);
econet_tx_result_t      broadcast(uint8_t* buff, int bytes);
econet_tx_result_t      transmit(uint8_t station, uint8_t network, uint8_t control, uint8_t port, uint* data, size_t data_len);
econet_rx_result_t      receive();
econet_rx_result_t      monitor();
uint8_t                 get_station();
void                    set_station(uint8_t station);

#endif