#include <stdio.h>
#include <sys/time.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"

#include "econet.h"
#include "adlc.h"
#include "util.h"

//define DEBUG

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

void abort_read(void) {
    adlc_write_cr2(CR2_PRIO_STATUS_ENABLE | CR2_CLEAR_RX_STATUS | CR2_CLEAR_TX_STATUS | CR2_FLAG_IDLE);
    adlc_write_cr1(CR1_RX_FRAME_DISCONTINUE | CR1_RIE | CR1_TX_RESET);
}

void clear_rx(void) {
    adlc_write_cr2(CR2_PRIO_STATUS_ENABLE | CR2_CLEAR_RX_STATUS | CR2_CLEAR_TX_STATUS | CR2_FLAG_IDLE);
}

typedef enum eFrameReadStatus {
    ECO_FRAME_READ_OK = 0L,
    ECO_FRAME_READ_NO_ADDR_MATCH,
    ECO_FRAME_READ_ERROR_CRC,
    ECO_FRAME_READ_ERROR_OVERRUN,
    ECO_FRAME_READ_ERROR_ABORT,
    ECO_FRAME_READ_ERROR_TIMEOUT,
    ECO_FRAME_READ_ERROR_OVERFLOW,
    ECO_FRAME_READ_ERROR_UNEXPECTED,
} tFrameReadStatus;

typedef struct
{
    tFrameReadStatus status;
    size_t bytes_read;
} tFrameReadResult;

tFrameReadResult simple_read_frame(uint8_t *buffer, size_t buffer_len, uint8_t *addr, size_t addr_len, uint timeout_ms) {
    tFrameReadResult result = {
        ECO_FRAME_READ_ERROR_UNEXPECTED,
        0
    };
    uint stat = 0;

    if (buffer_len == 0) {
        result.status = ECO_FRAME_READ_ERROR_OVERFLOW;
        return result;
    }

    // First byte should be address
    buffer[result.bytes_read++] = adlc_read(REG_FIFO);
    if (addr_len > 0) {
        bool discard = true;
        for (uint i = 0; i < addr_len; i++) {
            if (buffer[result.bytes_read - 1] == addr[i]) {
                discard = false;
                break;
            }
        }

        if (discard) {
            result.status = ECO_FRAME_READ_NO_ADDR_MATCH;
            return result;
        }
    }

    uint32_t time_start_ms = time_ms();

    bool frame_valid = false;
    while (!frame_valid) {
        do {
            if (time_ms() > time_start_ms + timeout_ms) {
                result.status = ECO_FRAME_READ_ERROR_TIMEOUT;
                return result;
            }

            stat = adlc_read(REG_STATUS_2);
        } while (!(stat & (STATUS_2_RDA | STATUS_2_FRAME_VALID | STATUS_2_ABORT_RX | STATUS_2_FCS_ERROR | STATUS_2_RX_OVERRUN)));

        if (stat & (STATUS_2_ABORT_RX | STATUS_2_FCS_ERROR | STATUS_2_RX_OVERRUN)) {
            if (stat & STATUS_2_ABORT_RX) {
                result.status = ECO_FRAME_READ_ERROR_ABORT;
            } else if (stat & STATUS_2_FCS_ERROR) {
                result.status = ECO_FRAME_READ_ERROR_CRC;
            } else if (stat & STATUS_2_RX_OVERRUN) {
                result.status = ECO_FRAME_READ_ERROR_OVERRUN;
            }
            return result;
        }

        if (stat & (STATUS_2_FRAME_VALID | STATUS_2_RDA)) {
            if (result.bytes_read >= buffer_len) {
                result.status = ECO_FRAME_READ_ERROR_OVERFLOW;
                return result;
            }

            buffer[result.bytes_read++] = adlc_read(REG_FIFO);
        }

        frame_valid = (stat & STATUS_2_FRAME_VALID);
    }

    result.status = ECO_FRAME_READ_OK;
    return result;
}

void simple_sniff(void) {
    const uint NUM_BUFFS = 20;
    const uint BUFFSIZE = 8000;
    const uint MAX_LISTEN_ADDRS = 16;
    uint8_t buffers[NUM_BUFFS][BUFFSIZE];
    size_t bytes_read[NUM_BUFFS];
    uint8_t listen_addrs[MAX_LISTEN_ADDRS];
    uint buffer_index = 0;
    econet_init();

    uint32_t start_time = 0;

    while (start_time == 0 || time_ms() < start_time + 3000) {
        uint status_reg_1 = adlc_read(REG_STATUS_1);

        if (!(status_reg_1 & (STATUS_1_S2_RD_REQ | STATUS_1_RDA))) {
            continue;
        }

        uint status_reg_2 = adlc_read(REG_STATUS_2);

        if (status_reg_2 & STATUS_2_ADDR_PRESENT) {
            if (buffer_index >= NUM_BUFFS - 1) {
                break;
            }
            tFrameReadResult read_result = simple_read_frame(buffers[buffer_index], sizeof(buffers[buffer_index]), listen_addrs, 0, 100000);

            switch (read_result.status) {
                case ECO_FRAME_READ_OK: {
                    bytes_read[buffer_index] = read_result.bytes_read;
                    buffer_index++;

                    if (start_time == 0) {
                        start_time = time_ms();
                    }

                    clear_rx();
                    break;
                }
                case ECO_FRAME_READ_ERROR_TIMEOUT:
                    printf("ECO_FRAME_READ_ERROR_TIMEOUT\n");
                    abort_read();
                    break;
                case ECO_FRAME_READ_NO_ADDR_MATCH:
                    printf("ECO_FRAME_READ_NO_ADDR_MATCH\n");
                    abort_read();
                    break;
                case ECO_FRAME_READ_ERROR_OVERRUN:
                    printf("ECO_FRAME_READ_ERROR_OVERRUN\n");
                    abort_read();
                    break;
                case ECO_FRAME_READ_ERROR_ABORT:
                    printf("ECO_FRAME_READ_ERROR_ABORT\n");
                    abort_read();
                    break;
                case ECO_FRAME_READ_ERROR_CRC:
                    printf("ECO_FRAME_READ_ERROR_CRC\n");
                    abort_read();
                    break;
                case ECO_FRAME_READ_ERROR_OVERFLOW:
                    printf("ECO_FRAME_READ_ERROR_OVERFLOW\n");
                    abort_read();
                    break;
                case ECO_FRAME_READ_ERROR_UNEXPECTED:
                    printf("ECO_FRAME_READ_ERROR_OVERFLOW\n");
                    abort_read();
                    break;
                default:
                    printf("ERRR\n");
                    abort_read();
                    break;
            }
        }

        if (status_reg_1 & STATUS_1_IRQ) {
            adlc_irq_reset();
        }
    }

    for (uint i = 0; i < (buffer_index - 1); i++) {
        printf("Frame %u: ", i);
        for (uint j = 0; j < bytes_read[i]; j++) {
            printf("%02x ", buffers[i][j]);
        }
        printf(".\n");
    }
}

void test_read(void) {
    adlc_init();
    while (true) {
        for (uint i = 0; i < 4; i++) {
            uint val = adlc_read(i);
            //printf("ADLC %u: %u\n", i, val);
            //adlc_write(i, i);
            //sleep_ms(500);
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

int main() {
    stdio_init_all();

    sleep_ms(2000); // give client a chance to reconnect
    printf("Hello world 31!\n");

    // core0_loop();
    // test_read();
    simple_sniff();
}
