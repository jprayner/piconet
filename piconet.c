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
        econet_rx_result_t rxEvent; // for PICONET_RX_EVENT
        econet_rx_result_t txEvent; // for PICONET_TX_EVENT
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

static void hexdump(uint8_t *buffer, size_t len) {
    int i;
    for (i = 0; i < len; i++) {
        printf("%02x ", buffer[i]);
    }
    printf("\n");
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

void test_read_poll(void) {
    if (!econet_init()) {
        printf("Failed to init econet\n");
        return;
    }

    while (true) {
        //econet_rx_result_t result = receive();
        econet_rx_result_t result = monitor();

        switch (result.type) {
            case PICONET_RX_RESULT_NONE :
                break;
            case PICONET_RX_RESULT_ERROR :
                printf("Error %u\n", result.error);
                break;
            case PICONET_RX_RESULT_BROADCAST :
                printf("BCAST:\n    ");
                hexdump(result.detail.data, result.detail.data_len);
                break;
            case PICONET_RX_RESULT_MONITOR :
                hexdump(result.detail.data, result.detail.data_len);
                break;
            case PICONET_RX_RESULT_IMMEDIATE_OP :
                printf("IMMED:");
                printf("\n   ");
                hexdump(result.detail.scout, result.detail.scout_len);
                printf("   ");
                hexdump(result.detail.data, result.detail.data_len);
                break;
            case PICONET_RX_RESULT_TRANSMIT :
                printf("TRANS: ");
                printf("\n   ");
                hexdump(result.detail.scout, result.detail.scout_len);
                printf("   ");
                hexdump(result.detail.data, result.detail.data_len);
                break;
            default:
                printf("WTF2 %u\n", result.type);
                break;
        }
    }
}

int main() {
    stdio_init_all();

    sleep_ms(2000); // give client a chance to reconnect
    printf("Hello world 32!\n");

    // core0_loop();
    // test_read();
    // simple_sniff();
    // rx_notify();
    test_read_poll();
}


/*
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
                        case PICONET_RX_RESULT_BROADCAST :
                            printf("BCAST: ");
                            frame_dump(event.rxEvent.detail);
                            break;
                        case PICONET_RX_RESULT_IMMEDIATE_OP :
                            printf("IMMED: ");
                            frame_dump(event.rxEvent.detail);
                            break;
                        case PICONET_RX_RESULT_TRANSMIT :
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

    // TODO: sort harcoded time
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

bool parse_notify_data(uint8_t *buffer, size_t bytes_read) {
    // printf("parse_notify_data\n");
    // for (uint j = 0; j < bytes_read; j++) {
    //     printf("%02x ", buffer[j]);
    // }
    // printf(".\n");

    if (bytes_read < 5) {
        return false;
    }

    uint8_t to_station      = buffer[0];
    uint8_t to_network      = buffer[1];
    uint8_t from_station    = buffer[2];
    uint8_t from_network    = buffer[3];
    uint8_t data            = buffer[4];

    if (to_station != 0x02 | to_network != 0x00) {
        return false;
    }

    printf("%c", data);
    return true;
}

bool parse_notify_scout(uint8_t *buffer, size_t bytes_read) {
    if (bytes_read < 10) {
        return false;
    }

    uint8_t to_station      = buffer[0];
    uint8_t to_network      = buffer[1];
    uint8_t from_station    = buffer[2];
    uint8_t from_network    = buffer[3];
    uint8_t control_byte    = buffer[4];
    uint8_t port            = buffer[5];
    uint8_t data            = buffer[8];

    if (to_station != 0x02 | to_network != 0x00 | control_byte != 0x85 | port != 0x00) {
        return false;
    }

    return true;
}
/*
bool rx_notify(void) {
    const uint BUFFSIZE = 8000;
    uint8_t buffer[BUFFSIZE];
    uint8_t listen_addrs[1];
    bool got_scout = false;

    listen_addrs[0] = 0x02;

    econet_init();

    while (true) {
        uint status_reg_1 = adlc_read(REG_STATUS_1);

        if (!(status_reg_1 & (STATUS_1_S2_RD_REQ | STATUS_1_RDA))) {
            continue;
        }

        uint status_reg_2 = adlc_read(REG_STATUS_2);

        if (status_reg_2 & STATUS_2_ADDR_PRESENT) {
            tFrameReadResult read_result = simple_read_frame(buffer, sizeof(buffer), listen_addrs, sizeof(listen_addrs), 100000);

            switch (read_result.status) {
                case ECO_FRAME_READ_OK: {
                    if (!got_scout) {
                        bool ok = parse_notify_scout(buffer, read_result.bytes_read);
                        if (!ok) {
                            printf("parse_notify_scout failed\n");
                            return false;
                        }

                        adlc_write_cr2(CR2_RTS_CONTROL | CR2_CLEAR_TX_STATUS | CR2_CLEAR_RX_STATUS | CR2_FLAG_IDLE);
                        adlc_write_cr1(CR1_TX_RESET | CR1_RX_RESET | CR1_RX_FRAME_DISCONTINUE | CR1_TIE);

                        uint8_t ack_frame[4];
                        ack_frame[0] = buffer[2];
                        ack_frame[1] = buffer[3];
                        ack_frame[2] = buffer[0];
                        ack_frame[3] = buffer[1];

                        tEconetTxResult tx_result = tx_frame(ack_frame, sizeof(ack_frame));
                        if (tx_result != ECO_FRAME_WRITE_OK) {
                            printf("ack scout tx_frame failed code=%u\n", tx_result);
                            return false;
                        }

                        clear_rx();

                        got_scout = true;
                    } else {
                        bool ok = parse_notify_data(buffer, read_result.bytes_read);
                        if (!ok) {
                            printf("parse_notify_data failed");
                            return false;
                        }

                        adlc_write_cr2(CR2_RTS_CONTROL | CR2_CLEAR_TX_STATUS | CR2_CLEAR_RX_STATUS | CR2_FLAG_IDLE);
                        adlc_write_cr1(CR1_TX_RESET | CR1_RX_RESET | CR1_RX_FRAME_DISCONTINUE | CR1_TIE);

                        uint8_t ack_frame[4];
                        ack_frame[0] = buffer[2];
                        ack_frame[1] = buffer[3];
                        ack_frame[2] = buffer[0];
                        ack_frame[3] = buffer[1];

                        tEconetTxResult tx_result = tx_frame(ack_frame, sizeof(ack_frame));
                        if (tx_result != ECO_FRAME_WRITE_OK) {
                            printf("ack data tx_frame failed code=%u\n", tx_result);
                            return false;
                        } else {
                            // printf("ack data tx_frame success!\n\n", tx_result);
                        }

                        clear_rx();

                        got_scout = false;
                    }

                    break;
                }
                case ECO_FRAME_READ_ERROR_TIMEOUT:
                    printf("ECO_FRAME_READ_ERROR_TIMEOUT\n");
                    abort_read();
                    break;
                case ECO_FRAME_READ_NO_ADDR_MATCH:
                    //printf("ECO_FRAME_READ_NO_ADDR_MATCH\n");
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
}
*/

