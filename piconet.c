#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"

#include "econet.h"
#include "adlc.h"
#include "util.h"

const uint CMD_RECEIVE = 1;

typedef enum ePiconetEventType {
    PICONET_RX_EVENT = 0L,
    PICONET_TX_EVENT
} tPiconetEventType;

typedef struct
{
    tPiconetEventType type;
    union {
        tEconetRxResult rxEvent; // for PICONET_RX_EVENT
        tEconetTxResult txEvent; // for PICONET_TX_EVENT
    };
} event_t;

typedef struct
{
    uint command;
    // TODO: other stuff
} command_t;

queue_t command_queue;
queue_t event_queue;

void print_status1(uint value) {
    printf("Status register 1: 0x%2x ", value);
    printf("[ ");

    if (value & STATUS_1_RDA) {
        printf(" RDA");
    }
    if (value & STATUS_1_S2_RD_REQ) {
        printf(" RD_REQ");
    }
    if (value & STATUS_1_LOOP) {
        printf(" LOOP");
    }
    if (value & STATUS_1_FLAG_DET) {
        printf(" FLAG");
    }
    if ((value & STATUS_1_NOT_CTS) == 0) {
        printf(" CTS");
    }
    if (value & STATUS_1_TX_UNDERRUN) {
        printf(" URUN");
    }
    if (value & STATUS_1_FRAME_COMPLETE) {
        printf(" FRM_COMP");
    }
    if (value & STATUS_1_IRQ) {
        printf(" IRQ");
    }

    printf(" ]\n");
}

void print_status2(uint value) {
    printf("Status register 2: 0x%2x ", value);
    printf("[ ");

    if (value & STATUS_2_ADDR_PRESENT) {
        printf(" ADDR");
    }
    if (value & STATUS_2_FRAME_VALID) {
        printf(" VALID");
    }
    if (value & STATUS_2_INACTIVE_IDLE_RX) {
        printf(" IDLE");
    }
    if (value & STATUS_2_ABORT_RX) {
        printf(" ABRT");
    }
    if (value & STATUS_2_FCS_ERROR) {
        printf(" FCS_ERR");
    }
    if ((value & STATUS_2_NOT_DCD) == 0) {
        printf(" DCD");
    }
    if (value & STATUS_2_RX_OVERRUN) {
        printf(" OVERRUN");
    }
    if (value & STATUS_2_RDA) {
        printf(" RDA");
    }

    printf(" ]\n");
}

void core1_entry() {
    command_t cmd;

    while (1) {
        /*
        if (queue_try_remove(&command_queue, &cmd)) {
            // TODO: do something with cmd
        }
        */

        tEconetRxResult rxResult = receive();

        if (rxResult.type != PICONET_RX_RESULT_NONE) {
            event_t result;
            result.type = PICONET_RX_EVENT;
            result.rxEvent = rxResult;
            queue_add_blocking(&event_queue, &result);
        }
    }
}

int main() {
    stdio_init_all();

    sleep_ms(5000); // give client a chance to reconnect
    printf("Hello, world!\n");

    queue_init(&command_queue, sizeof(command_t), 1);
    queue_init(&event_queue, sizeof(event_t), 1);
    multicore_launch_core1(core1_entry);

    econet_init();

    event_t event;
    while (true) {
        receive();

        // TODO: this should not be an Rx result - should be more general
        event_t event;
        if (queue_try_remove(&event_queue, &event)) {
            switch (event.type) {
                case PICONET_RX_EVENT: {
                    switch (event.rxEvent.type) {
                        case PICONET_RX_RESULT_ERROR :
                            printf("Error %u\n", event.rxEvent.error.type);
                            print_status1(event.rxEvent.error.sr1);
                            print_status2(event.rxEvent.error.sr2);
                            printf("\n");
                            break;
                        case PICONET_RX_RESULT_BROADCAST :
                            printf("Received broadcast\n");
                            break;
                        case PICONET_RX_RESULT_IMMEDIATE_OP :
                            printf("Received immediate\n");
                            break;
                        case PICONET_RX_RESULT_FRAME :
                            printf("Received frame\n");
                            break;
                        default:
                            printf("WTF2 %u\n", event.rxEvent.type);
                            break;
                    }
                    break;
                }
                default: {
                    printf("WTF1 %u\n", event.type);
                    break;
                }
            }
        }

/*
        printf("IRQ: %u\n", gpio_get(14));
        sleep_ms(500);
*/
        // printf("Read value: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(call_result));
        // printf("\n");
        // sleep_ms(500);
    }
}
