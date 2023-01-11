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
/*
    for (uint j = 0; j < bytes_read[i]; j++) {
        printf("%02x ", buffers[i][j]);
    }
    printf(".\n");
*/
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

bool seize_line(void) {
    uint8_t count, outercount, seized = 0;
    outercount = 0;

    while (outercount++ < 1 && !seized)
    {
        // Attempt to seize line
        adlc_write_cr2(CR2_CLEAR_RX_STATUS | CR2_CLEAR_TX_STATUS | CR2_PRIO_STATUS_ENABLE | CR2_FLAG_IDLE);
        uint sr2 = adlc_read(REG_STATUS_2);

        // No clock
        if (sr2 & STATUS_2_NOT_DCD) {
            printf("[No clock]");
            return false;
        }

        print_status2(sr2);
        count = 0;
        while (count++ < 5 && (!(sr2 & STATUS_2_INACTIVE_IDLE_RX))) {
            adlc_write_cr2(CR2_CLEAR_RX_STATUS | CR2_CLEAR_TX_STATUS | CR2_PRIO_STATUS_ENABLE | CR2_FLAG_IDLE);
            sleep_ms(count << 2);
            sr2 = adlc_read(REG_STATUS_2);
            print_status2(sr2);
        }

        if (sr2 & STATUS_2_INACTIVE_IDLE_RX)
        {
            adlc_write_cr2(CR2_CLEAR_RX_STATUS | CR2_CLEAR_TX_STATUS | CR2_PRIO_STATUS_ENABLE | CR2_FLAG_IDLE | CR2_RTS_CONTROL);
            adlc_write_cr1(CR1_RX_RESET | CR1_TIE);
            uint sr1 = adlc_read(REG_STATUS_1);
            print_status1(sr1);
            if (!(sr1 & STATUS_1_NOT_CTS)) {
                return true;
            }
        }
    }

    printf("[Timeout]");
    return false;
}

typedef enum eFrameWriteStatus {
    ECO_FRAME_WRITE_OK = 0L,
    ECO_FRAME_WRITE_UNDERRUN,
    ECO_FRAME_WRITE_COLLISION,
    ECO_FRAME_WRITE_READY_TIMEOUT,
    ECO_FRAME_WRITE_UNEXPECTED,
} tFrameWriteStatus;

tFrameWriteStatus tx_frame(uint8_t *buffer, size_t len) {
    static uint timeout_ms = 2000;
	char tdra_flag;
	int loopcount = 0;
    int tdra_counter;
    uint sr1;

    uint32_t time_start_ms = time_ms();

    adlc_write(0, 0b00000000); // Disable RX interrupts

    while (!(adlc_read(REG_STATUS_1) & STATUS_1_FRAME_COMPLETE)) {
        if (time_ms() > time_start_ms + timeout_ms) {
            return ECO_FRAME_WRITE_READY_TIMEOUT;
        }

        adlc_write(1, 0b11100101); // CR2: raise RTS, clear TX and RX status, flag fill and Prioritise status
    };
    

    // printf("sending: ");
    // for (uint j = 0; j < len; j++) {
    //     printf("%02x ", buffer[j]);
    // }
    // printf("\n");

    for (uint ptr = 0; ptr < len; ptr++) {
        // While not FC/TDRA set, loop until it is - or we get an error
        while (true) {
            uint sr1 = adlc_read(REG_STATUS_1);
            if (sr1 & STATUS_1_TX_UNDERRUN) {
                return ECO_FRAME_WRITE_UNDERRUN;
            }
            if (sr1 & STATUS_1_FRAME_COMPLETE) {
                break; // We have TDRA
            }
            // if (sr1 & 192) { // Some other error JIM NOTE: looks wrong mask to me
            //     printSR1(sr1); 
            //     resetIRQ(); 
            //     return(false);
            // };

            if (time_ms() > time_start_ms + timeout_ms) {
                // move to reset_irq or similar
                adlc_write(0, 0b00000010); // Enable RX interrupts, select CR2
                adlc_write(1, 0b01100001); // Clear RX and TX status, prioritise status 

                return ECO_FRAME_WRITE_READY_TIMEOUT;
            }
        }

        adlc_write(REG_FIFO, buffer[ptr]);
    }

/*
#define CR2_PRIO_STATUS_ENABLE    1
#define CR2_2_BYTE_TRANSFER       2
#define CR2_FLAG_IDLE             4
#define CR2_FRAME_COMPLETE        8
#define CR2_TX_LAST_DATA          16
#define CR2_CLEAR_RX_STATUS       32
#define CR2_CLEAR_TX_STATUS       64
#define CR2_RTS_CONTROL           128
*/
//econet_write_cr(ECONET_GPIO_CR2, ECONET_GPIO_C2_TXLAST | ECONET_GPIO_C2_FC | ECONET_GPIO_C2_FLAGIDLE | ECONET_GPIO_C2_PSE); // No RX status reset


    // CR2_PRIO_STATUS_ENABLE | CR2_FRAME_COMPLETE | CR2_TX_LAST_DATA | CR2_CLEAR_RX_STATUS
    adlc_write(1, CR2_TX_LAST_DATA | CR2_FRAME_COMPLETE | CR2_FLAG_IDLE | CR2_PRIO_STATUS_ENABLE); 
    // was: 0b00111001); // Tell the ADLC that was the last byte, and clear flag fill modes and RTS. 
    adlc_write(0, 0b00000100); // Tx interrupt enable

    // wait for IRQ
    while (true) {
        sr1 = adlc_read(REG_STATUS_1);
        if (sr1 & STATUS_1_IRQ) {
            break;
        }
        if (time_ms() > time_start_ms + timeout_ms) {
            return ECO_FRAME_WRITE_READY_TIMEOUT;
        }
    };

    if (!(sr1 & STATUS_1_FRAME_COMPLETE)) {
        return ECO_FRAME_WRITE_UNEXPECTED;
    }

    adlc_write(1, 0b01100001); // CR2: Clear any pending status, prioritise status
    adlc_write(0, 0b00000010); //Suppress tx interrupts, Enable RX interrupts

    return ECO_FRAME_WRITE_OK;
}

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


                        adlc_write(0, 0b00000000); // Select CR2
                        adlc_write(1, 0b11100100); // Set CR2 to RTS, TX Status Clear, RX Status clear, Flag fill on idle)
                        adlc_write(1, 0b11100100); // seems to need calling twice to clear everything

                        // printf("parse_notify_scout ok\n");
                        // printf("Received:\n");
                        // for (uint j = 0; j < read_result.bytes_read; j++) {
                        //     printf("%02x ", buffer[j]);
                        // }
                        // printf(".\n\n");


                        uint8_t ack_frame[4];
                        ack_frame[0] = buffer[2];
                        ack_frame[1] = buffer[3];
                        ack_frame[2] = buffer[0];
                        ack_frame[3] = buffer[1];

                        tEconetTxResult tx_result = tx_frame(ack_frame, sizeof(ack_frame));
                        if (tx_result != ECO_FRAME_WRITE_OK) {
                            printf("ack scout tx_frame failed code=%u\n", tx_result);
                            return false;
                        } else {
                            // printf("ack scout tx_frame success!\n", tx_result);
                        }

                        clear_rx();

                        got_scout = true;
                    } else {
                        bool ok = parse_notify_data(buffer, read_result.bytes_read);
                        if (!ok) {
                            printf("parse_notify_data failed");
                            return false;
                        }

                        adlc_write(0, 0b00000000); // Select CR2
                        adlc_write(1, 0b11100100); // Set CR2 to RTS, TX Status Clear, RX Status clear, Flag fill on idle)
                        adlc_write(1, 0b11100100); // seems to need calling twice to clear everything

                        // printf("parse_notify_data ok\n");
                        // printf("Received:\n");
                        // for (uint j = 0; j < read_result.bytes_read; j++) {
                        //     printf("%02x ", buffer[j]);
                        // }
                        // printf(".\n\n");

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
    //simple_sniff();
    rx_notify();
}
