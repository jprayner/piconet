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

typedef enum ePiconetCommandType {
    PICONET_CMD_ACK = 0L
} tPiconetCommandType;

typedef struct AckCommand
{
    uint senderStation;
    uint senderNetwork;
    uint controlByte;
    uint port;
} tAckCommand;

typedef struct
{
    tPiconetCommandType type;
    union {
        tAckCommand ack;
    };
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

static void frame_dump(tEconetRxResultDetail detail) {
    int i;
    for (i = 0; i < detail.length; i++) {
        printf("%02x ", detail.buffer[i]);
    }
    printf("\n");
    print_status1(detail.sr1);
    print_status2(detail.sr2);

}


void core1_entry() {
    command_t cmd;

    while (1) {
        if (queue_try_remove(&command_queue, &cmd)) {
            switch (cmd.type) {
                case PICONET_CMD_ACK: {
                    tEconetTxResult result = ack_scout(
                        cmd.ack.senderStation,
                        cmd.ack.senderNetwork,
                        cmd.ack.controlByte,
                        cmd.ack.port);
                    event_t txEvent;
                    txEvent.type = PICONET_TX_EVENT;
                    txEvent.txEvent = result;
                    queue_add_blocking(&event_queue, &result);
                    break;
                }
            }
        }

        tEconetRxResult rxResult = receive();

        if (rxResult.type != PICONET_RX_RESULT_NONE) {
            event_t result;
            result.type = PICONET_RX_EVENT;
            result.rxEvent = rxResult;
            queue_add_blocking(&event_queue, &result);
        }
    }
}

void core0_loop() {
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
                case PICONET_TX_EVENT: {
                    switch (event.txEvent) {
                        case PICONET_TX_RESULT_OK:
                            printf("TRANS: OK");
                            break;
                        case PICONET_TX_RESULT_ERROR_MISC:
                            printf("TRANS: ERROR_MISC");
                            break;
                        case PICONET_TX_RESULT_ERROR_NO_ACK:
                            printf("TRANS: ERROR_NO_ACK");
                            break;
                        case PICONET_TX_RESULT_ERROR_TIMEOUT:
                            printf("TRANS: ERROR_TIMEOUT");
                            break;
                        default:
                            printf("TRANS: WTF");
                            break;
                    }
                    break;
                }
                case PICONET_RX_EVENT: {
                    switch (event.rxEvent.type) {
                        case PICONET_RX_RESULT_ERROR :
                            printf("Error %u\n", event.rxEvent.error.type);
                            print_status1(event.rxEvent.error.sr1);
                            print_status2(event.rxEvent.error.sr2);
                            printf("\n");
                            break;
                        case PICONET_RX_RESULT_SCOUT :
                            printf("SCOUT: ");
                            frame_dump(event.rxEvent.detail);
                            
                            // command_t cmd;
                            // cmd.type = PICONET_CMD_ACK;
                            // cmd.ack.senderStation = event.rxEvent.detail.buffer[2];
                            // cmd.ack.senderNetwork = event.rxEvent.detail.buffer[3];
                            // cmd.ack.controlByte = event.rxEvent.detail.buffer[4];
                            // cmd.ack.port = event.rxEvent.detail.buffer[5];
                            // queue_add_blocking(&command_queue, &cmd);

                            break;
                        case PICONET_RX_RESULT_BROADCAST :
                            printf("BCAST: ");
                            frame_dump(event.rxEvent.detail);
                            break;
                        case PICONET_RX_RESULT_IMMEDIATE_OP :
                            printf("IMMED: ");
                            frame_dump(event.rxEvent.detail);
                            break;
                        case PICONET_RX_RESULT_FRAME :
                            printf("FRAME: ");
                            frame_dump(event.rxEvent.detail);
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
    }
}

int main() {
    stdio_init_all();

    sleep_ms(2000); // give client a chance to reconnect
    printf("Hello world!\n");

    // core0_loop();
    simple_sniff();
}

void abort_read(void) {
    adlc_write_cr2(CR2_PRIO_STATUS_ENABLE | CR2_CLEAR_RX_STATUS | CR2_CLEAR_TX_STATUS | CR2_FLAG_IDLE);
    adlc_write_cr1(CR1_RX_FRAME_DISCONTINUE | CR1_RIE | CR1_TX_RESET);
}

void simple_read_frame(void) {
    const uint BUFFSIZE = 16384;
    uint buffer[BUFFSIZE];
    uint ptr = 0;
    uint stat = 0;
    bool frame = true;
    bool error = false;

    // First byte should be address
    buffer[ptr++] = adlc_read(REG_FIFO);

    while (frame) {
        // Now check the status register
        do {
            stat = adlc_read(REG_STATUS_2);
        } while (!(stat & (STATUS_2_RDA | STATUS_2_ADDR_PRESENT | STATUS_2_FRAME_VALID | STATUS_2_ABORT_RX | STATUS_2_FCS_ERROR | STATUS_2_RX_OVERRUN)));

/*
#define STATUS_2_ADDR_PRESENT     1
#define STATUS_2_FRAME_VALID      2
#define STATUS_2_INACTIVE_IDLE_RX 4
#define STATUS_2_ABORT_RX         8
#define STATUS_2_FCS_ERROR        16
#define STATUS_2_NOT_DCD          32
#define STATUS_2_RX_OVERRUN       64
#define STATUS_2_RDA              128
*/

        if (stat & (STATUS_2_ABORT_RX | STATUS_2_FCS_ERROR | STATUS_2_RX_OVERRUN)) {
            frame = false;
            error = true;
            continue;
        }

        if (stat & (STATUS_2_RDA | STATUS_2_ADDR_PRESENT)) {
            buffer[ptr++] = adlc_read(REG_FIFO);
        } else if (stat & STATUS_2_FRAME_VALID) {
            buffer[ptr++] = adlc_read(REG_FIFO);

            // Error or end of frame bits set so drop out
            frame = false;
            continue;
        }
    } // End of while in frame - Data Available with no error bits or end set

    if (error) {
        printf("Receive error:\n");
        print_status2(stat);
        abort_read();
        return;
    }

    int i;
    for (i = 0; i < ptr; i++) {
        printf("%02x ", buffer[i]);
    }
    printf("\n");
}

void simple_sniff(void) {
    econet_init();
    while (true) {
        uint status_reg_1 = adlc_read(REG_STATUS_1);

        if (!(status_reg_1 & (STATUS_1_S2_RD_REQ | STATUS_1_RDA))) {
            continue;
        }

        uint status_reg_2 = adlc_read(REG_STATUS_2);
        // print_status1(status_reg_1);
        // print_status2(status_reg_2);

        if (status_reg_2 & STATUS_2_ADDR_PRESENT) {
            //uint addr = adlc_read(REG_FIFO);
            //printf("Address: %02x\n", addr);
            simple_read_frame();
        }

        if (status_reg_2 & STATUS_2_RDA) {
            //simple_read_frame();
        }

        if (status_reg_1 & STATUS_1_IRQ) {
            adlc_irq_reset();
        }
    }
}

void test_read(void) {
    adlc_init();
    while (true) {
        for (uint i = 0; i < 1; i++) {
            uint val = adlc_read(i);
            printf("ADLC %u: %u\n", i, val);
            //adlc_write(i, i);
            sleep_ms(500);
        }
    }
}

void test_write(void) {
    adlc_init();
    while (true) {
        for (uint i = 0; i < 1; i++) {
            adlc_write(i, i);
        }
    }
}
