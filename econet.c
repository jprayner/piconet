#include <stdio.h>

#include "econet.h"

#include "adlc.h"
#include "util.h"

static int BUFFSIZE = 16384;

uint rx_buff[16384]; // TODO should match BUFFSIZE
uint scout_buff[8];
uint ack_buff[8];

uint cfg_scout_timeout = 100;
uint cfg_ack_timeout = 200;
uint cfg_tx_begin_timeout = 5000;

uint8_t listen_addresses[] = { };

typedef enum e_frame_read_status {
    ECO_FRAME_READ_OK = 0L,
    ECO_FRAME_READ_NO_ADDR_MATCH,
    ECO_FRAME_READ_ERROR_CRC,
    ECO_FRAME_READ_ERROR_OVERRUN,
    ECO_FRAME_READ_ERROR_ABORT,
    ECO_FRAME_READ_ERROR_TIMEOUT,
    ECO_FRAME_READ_ERROR_OVERFLOW,
    ECO_FRAME_READ_ERROR_UNEXPECTED,
} t_frame_read_status;

typedef struct
{
    t_frame_read_status status;
    size_t bytes_read;
} t_frame_read_result;

t_frame_read_result _read_frame(uint8_t *buffer, size_t buffer_len, uint8_t *addr, size_t addr_len, uint timeout_ms);
tEconetRxResult _map_read_frame_result(t_frame_read_status status);

void _abort_read(void) {
    adlc_write_cr2(CR2_PRIO_STATUS_ENABLE | CR2_CLEAR_RX_STATUS | CR2_CLEAR_TX_STATUS | CR2_FLAG_IDLE);
    adlc_write_cr1(CR1_RX_FRAME_DISCONTINUE | CR1_RIE | CR1_TX_RESET);
}

void _clear_rx(void) {
    adlc_write_cr2(CR2_PRIO_STATUS_ENABLE | CR2_CLEAR_RX_STATUS | CR2_CLEAR_TX_STATUS | CR2_FLAG_IDLE);
}

void econet_init(void) {
    adlc_init();
    adlc_irq_reset();
}

typedef struct
{
    uint8_t dest_station;
    uint8_t dest_net;
    uint8_t src_station;
    uint8_t src_net;
    uint8_t ctrl;
    uint8_t port;
    uint8_t *frame;
    size_t frame_len;
    uint8_t **data;
    size_t data_len;
} t_econet_frame;

typedef enum e_frame_type {
    ECO_FRAME_TYPE_UNKNOWN = 0L,
    ECO_FRAME_TYPE_TRANSMIT,
    ECO_FRAME_TYPE_ACK,
    ECO_FRAME_TYPE_IMMEDIATE,
    ECO_FRAME_TYPE_BROADCAST,
    ECO_FRAME_TYPE_DATA
} t_frame_type;

typedef struct
{
    t_frame_type type;
    t_econet_frame frame;
} t_frame_parse_result;

t_frame_parse_result _parse_frame(uint8_t *buffer, size_t len, bool is_opening_frame) {
    t_frame_parse_result result;

    if (len < 4) {
        result.type = ECO_FRAME_TYPE_UNKNOWN;
        return result;
    }

    result.frame.dest_station = buffer[0];
    result.frame.dest_net = buffer[1];
    result.frame.src_station = buffer[2];
    result.frame.src_net = buffer[3];
    result.frame.frame = buffer;
    result.frame.frame_len = len;
    result.frame.data = buffer[4];
    result.frame.data_len = len - 4;

    if (!is_opening_frame) {
        if (len == 4) {
            result.type = ECO_FRAME_TYPE_ACK;
            return result;
        }

        result.type = ECO_FRAME_TYPE_DATA;
        result.frame.data = buffer[4];
        result.frame.data_len = len - 4;
        return result;
    }

    if (len < 6) {
        result.type = ECO_FRAME_TYPE_UNKNOWN;
        return result;
    }

    result.frame.ctrl = buffer[4];
    result.frame.port = buffer[5];
    result.frame.data = buffer[6];
    result.frame.data_len = len - 6;

    if ((result.frame.dest_station == 0x00) || (result.frame.dest_station == 0xff)) {
        result.type = ECO_FRAME_TYPE_BROADCAST;
    } else if (len == 6) {
        result.type = ECO_FRAME_TYPE_TRANSMIT;
    } else if (result.frame.port == 0 && len > 6) {
        result.type = ECO_FRAME_TYPE_IMMEDIATE;
    } else {
        result.type = ECO_FRAME_TYPE_UNKNOWN;
    }

    return result;
}

typedef enum eFrameWriteStatus {
    ECO_FRAME_WRITE_OK = 0L,
    ECO_FRAME_WRITE_UNDERRUN,
    ECO_FRAME_WRITE_COLLISION,
    ECO_FRAME_WRITE_READY_TIMEOUT,
    ECO_FRAME_WRITE_UNEXPECTED,
} tFrameWriteStatus;

void _finish_tx(void) {
    adlc_write_cr2(CR2_CLEAR_TX_STATUS | CR2_CLEAR_RX_STATUS | CR2_PRIO_STATUS_ENABLE);
    adlc_write_cr1(CR1_RIE | CR1_RX_RESET);
}

tFrameWriteStatus tx_frame(uint8_t *buffer, size_t len) {
    static uint timeout_ms = 2000;
	char tdra_flag;
	int loopcount = 0;
    int tdra_counter;
    uint sr1;

    uint32_t time_start_ms = time_ms();

    adlc_write_cr1(0b00000000); // Disable RX interrupts

    while (!(adlc_read(REG_STATUS_1) & STATUS_1_FRAME_COMPLETE)) {
        if (time_ms() > time_start_ms + timeout_ms) {
            return ECO_FRAME_WRITE_READY_TIMEOUT;
        }

        adlc_write_cr2(CR2_RTS_CONTROL | CR2_CLEAR_TX_STATUS | CR2_CLEAR_RX_STATUS | CR2_FLAG_IDLE | CR2_PRIO_STATUS_ENABLE);
    };

    for (uint ptr = 0; ptr < len; ptr++) {
        // While not FC/TDRA set, loop until it is - or we get an error
        while (true) {
            uint sr1 = adlc_read(REG_STATUS_1);
            if (sr1 & STATUS_1_TX_UNDERRUN) {
                _finish_tx();
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
                _finish_tx();
                return ECO_FRAME_WRITE_READY_TIMEOUT;
            }
        }

        adlc_write_fifo(buffer[ptr]);
    }

    adlc_write_cr2(CR2_TX_LAST_DATA | CR2_FRAME_COMPLETE | CR2_FLAG_IDLE | CR2_PRIO_STATUS_ENABLE); 
    adlc_write_cr1(CR1_TIE);

    // wait for IRQ
    while (true) {
        sr1 = adlc_read(REG_STATUS_1);
        if (sr1 & STATUS_1_IRQ) {
            break;
        }
        if (time_ms() > time_start_ms + timeout_ms) {
            _finish_tx();
            return ECO_FRAME_WRITE_READY_TIMEOUT;
        }
    };

    _finish_tx();

    if (!(sr1 & STATUS_1_FRAME_COMPLETE)) {
        return ECO_FRAME_WRITE_UNEXPECTED;
    }

    return ECO_FRAME_WRITE_OK;
}

tFrameWriteStatus _send_ack(t_frame_parse_result *incoming_frame) {
    adlc_write_cr2(CR2_RTS_CONTROL | CR2_CLEAR_TX_STATUS | CR2_CLEAR_RX_STATUS | CR2_FLAG_IDLE);
    adlc_write_cr1(CR1_TX_RESET | CR1_RX_RESET | CR1_RX_FRAME_DISCONTINUE | CR1_TIE);

    uint8_t ack_frame[4];
    ack_frame[0] = incoming_frame->frame.src_station;
    ack_frame[1] = incoming_frame->frame.src_net;
    ack_frame[2] = incoming_frame->frame.dest_station;
    ack_frame[3] = incoming_frame->frame.dest_net;

    return tx_frame(ack_frame, 4);
}

bool _wait_frame_start(uint32_t timeout_ms) {
    uint32_t time_start_ms = time_ms();

    while (true) {
        while (!(adlc_read(REG_STATUS_1) & STATUS_1_S2_RD_REQ)) {
            if (time_ms() > time_start_ms + timeout_ms) {
                return false;
            }
        }
        
        if (adlc_read(REG_STATUS_2) & STATUS_2_ADDR_PRESENT) {
            break;
        }

        adlc_irq_reset();
    }
    return true;
}

tEconetRxResult _rx_result_for_error(tEconetError error) {
    tEconetRxResult result;
    result.type = PICONET_RX_RESULT_ERROR;
    result.error.type = error;
    return result;
}

tEconetRxResult _rx_data_for_scout(t_frame_parse_result *scout_frame, uint8_t *buffer, size_t len) {
    t_frame_type frame_type = scout_frame->type;
    tFrameWriteStatus scout_ack_result = _send_ack(scout_frame);
    if (scout_ack_result != ECO_FRAME_WRITE_OK) {
        printf("[_rx_data_for_scout] scout ack failed code=%u\n", scout_ack_result);
        return _rx_result_for_error(PICONET_RX_ERROR_SCOUT_ACK);
    }

    t_frame_read_result data_frame_result;
    while (true) {
        adlc_irq_reset();

        if (!_wait_frame_start(200)) {
            return _rx_result_for_error(PICONET_RX_ERROR_MISC_TIMEOUT);
        }

        data_frame_result = _read_frame(buffer, len, listen_addresses, sizeof(listen_addresses), 2000);
        if (data_frame_result.status == ECO_FRAME_READ_OK) {
            break;
        }
        _abort_read();
    }

    t_frame_parse_result data_frame = _parse_frame(buffer, data_frame_result.bytes_read, false);
    if (data_frame.type != ECO_FRAME_TYPE_DATA) {
        printf("[_rx_data_for_scout] parse failed type=%u len=%u - aborting", data_frame.type, data_frame_result.bytes_read);
        _abort_read();
        return _rx_result_for_error(PICONET_RX_RESULT_ERROR);
    }

    tFrameWriteStatus data_ack_result = _send_ack(&data_frame);
    if (data_ack_result != ECO_FRAME_WRITE_OK) {
        printf("[_rx_data_for_scout] data ack failed code=%u\n", data_ack_result);
        return _rx_result_for_error(PICONET_RX_ERROR_DATA_ACK);
    }

    tEconetRxResult result;
    result.type = frame_type;
    result.detail.buffer = data_frame.frame.frame;
    result.detail.length = data_frame.frame.frame_len;
    return result;
}

tEconetRxResult _handle_immediate_scout(t_frame_parse_result *immediate_scout_frame, uint8_t *buffer, size_t len) {
    return _rx_data_for_scout(immediate_scout_frame, buffer, len);
}

tEconetRxResult _handle_transmit_scout(t_frame_parse_result *transmit_scout_frame, uint8_t *buffer, size_t len) {
    return _rx_data_for_scout(transmit_scout_frame, buffer, len);
}

tEconetRxResult _handle_broadcast(t_frame_parse_result *broadcast_frame, uint8_t *buffer, size_t len) {
    tEconetRxResult result;
    result.type = PICONET_RX_RESULT_BROADCAST;
    result.detail.buffer = broadcast_frame->frame.frame;
    result.detail.length = broadcast_frame->frame.frame_len;
    return result;
}

tEconetRxResult _handle_first_frame(uint8_t *buffer, size_t len) {
    t_frame_read_result read_frame_result = _read_frame(buffer, len, listen_addresses, sizeof(listen_addresses), 2000);

    if (read_frame_result.status != ECO_FRAME_READ_OK) {
        _clear_rx();
        return _map_read_frame_result(read_frame_result.status);
    }

    t_frame_parse_result result = _parse_frame(buffer, read_frame_result.bytes_read, true);
    switch (result.type) {
        case ECO_FRAME_TYPE_TRANSMIT :
            return _handle_transmit_scout(&result, buffer, len);
        case ECO_FRAME_TYPE_IMMEDIATE :
            return _handle_immediate_scout(&result, buffer, len);
        case ECO_FRAME_TYPE_BROADCAST :
            _abort_read();
            return _handle_broadcast(&result, buffer, len);
        default :
            printf("[_handle_first_frame] unexpected type=%u bytes_read=%u - aborting", result.type, read_frame_result.bytes_read);
            _abort_read();
            break;
    }
}

tEconetRxResult receive(uint8_t *buffer, size_t len) {
    tEconetRxResult result;
    result.type = PICONET_RX_RESULT_NONE;

    uint status_reg_1 = adlc_read(REG_STATUS_1);

    if (status_reg_1 & STATUS_1_S2_RD_REQ) {
        uint status_reg_2 = adlc_read(REG_STATUS_2);

        if (status_reg_2 & STATUS_2_ADDR_PRESENT) {
            result = _handle_first_frame(buffer, len);
        }

        adlc_irq_reset();
    } else if (status_reg_1 & STATUS_1_RDA) {
        printf("[receive] unexpected RDA - aborting");
        // Unexpected RDA
        _abort_read();
        adlc_irq_reset();
    }

    return result;
}

t_frame_read_result _read_frame(uint8_t *buffer, size_t buffer_len, uint8_t *addr, size_t addr_len, uint timeout_ms) {
    t_frame_read_result result = {
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

tEconetRxResult _map_read_frame_result(t_frame_read_status status) {
    tEconetRxResult result;
    switch (status) {
        case ECO_FRAME_READ_NO_ADDR_MATCH :
            result.type = PICONET_RX_RESULT_NONE;
            break;
        case ECO_FRAME_READ_ERROR_CRC :
            result.type = PICONET_RX_RESULT_ERROR;
            result.error.type = PICONET_RX_ERROR_MISC_CRC;
            break;
        case ECO_FRAME_READ_ERROR_OVERRUN :
            result.type = PICONET_RX_RESULT_ERROR;
            result.error.type = PICONET_RX_ERROR_MISC_OVERRUN;
            break;
        case ECO_FRAME_READ_ERROR_ABORT :
            result.type = PICONET_RX_RESULT_ERROR;
            result.error.type = PICONET_RX_ERROR_MISC_ABORT;
            break;
        case ECO_FRAME_READ_ERROR_TIMEOUT :
            result.type = PICONET_RX_RESULT_ERROR;
            result.error.type = PICONET_RX_ERROR_MISC_TIMEOUT;
            break;
        case ECO_FRAME_READ_ERROR_OVERFLOW :
            result.type = PICONET_RX_RESULT_ERROR;
            result.error.type = PICONET_RX_ERROR_MISC_OVERFLOW;
            break;
        case ECO_FRAME_READ_ERROR_UNEXPECTED :
            result.type = PICONET_RX_RESULT_ERROR;
            result.error.type = PICONET_RX_ERROR_MISC;
            break;
        default :
            result.type = PICONET_RX_RESULT_ERROR;
            result.error.type = PICONET_RX_ERROR_MISC;
            break;
    }
    return result;
}
