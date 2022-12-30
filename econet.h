#ifndef _PICONET_ECONET_H_
#define _PICONET_ECONET_H_

#include "pico/stdlib.h"

typedef enum eEconetError {
    PICONET_ERROR_MISC = 0L,
    PICONET_ERROR_INVALID_FRAME
} tEconetError;

typedef struct EconetErrorDetail
{
    tEconetError type;
    uint sr1;
    uint sr2;
} tEconetErrorDetail;

typedef enum eEconetRxResultType {
    PICONET_RX_RESULT_NONE = 0L,
    PICONET_RX_RESULT_ERROR,
    PICONET_RX_RESULT_SCOUT,
    PICONET_RX_RESULT_BROADCAST,
    PICONET_RX_RESULT_IMMEDIATE_OP,
    PICONET_RX_RESULT_FRAME
} tEconetRxResultType;

typedef struct EconetRxResultDetail
{
    uint *buffer; // TODO: make byte!
    size_t length;
    uint sr1;
    uint sr2;
} tEconetRxResultDetail;

typedef enum eEconetTxResult {
    PICONET_TX_RESULT_OK = 0L,
    PICONET_TX_RESULT_ERROR_MISC,
    PICONET_TX_RESULT_ERROR_NO_ACK,
    PICONET_TX_RESULT_ERROR_TIMEOUT
} tEconetTxResult;

typedef struct EconetRxResult
{
    tEconetRxResultType type;
    union {
        tEconetErrorDetail error;      /* if type is PICONET_RX_RESULT_ERROR */
        tEconetRxResultDetail detail;   /* if type not PICONET_RX_RESULT_NONE or PICONET_RX_RESULT_ERROR */
    };
} tEconetRxResult;

void econet_init(void);
tEconetTxResult transmit(uint *buff, int bytes, bool getAck, bool scout, bool immediate);
tEconetRxResult receive(void);
tEconetTxResult ack_scout(uint sender_station, uint sender_network, uint control_byte, uint port);

#endif