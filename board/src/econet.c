#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "econet.h"

#include "adlc.h"
#include "util.h"

#define TIMEOUT_READ_FIRST_FRAME_MS 2000
#define TIMEOUT_DATA_FRAME_MS 100
#define TIMEOUT_WAIT_ACK_MS 200
#define TIMEOUT_WRITE_READY_MS 10000
#define TIMEOUT_WRITE_COMPLETE_MS 10000
#define TIMEOUT_READ_DATA_MS 10000

typedef struct {
    uint8_t     dest_station;
    uint8_t     dest_net;
    uint8_t     src_station;
    uint8_t     src_net;
    uint8_t     ctrl;
    uint8_t     port;
    uint8_t*    frame;
    size_t      frame_len;
    uint8_t*    data;
    size_t      data_len;
} econet_frame_t;

typedef enum {
    FRAME_TYPE_UNKNOWN = 0L,
    FRAME_TYPE_TRANSMIT,
    FRAME_TYPE_ACK,
    FRAME_TYPE_IMMEDIATE,
    FRAME_TYPE_BROADCAST,
    FRAME_TYPE_DATA
} frame_type_t;

typedef struct {
    frame_type_t type;
    econet_frame_t frame;
} t_frame_parse_result;

typedef enum e_frame_read_status {
    FRAME_READ_OK = 0L,
    FRAME_READ_NO_ADDR_MATCH,
    FRAME_READ_ERROR_CRC,
    FRAME_READ_ERROR_OVERRUN,
    FRAME_READ_ERROR_ABORT,
    FRAME_READ_ERROR_TIMEOUT,
    FRAME_READ_ERROR_OVERFLOW,
    FRAME_READ_ERROR_UNEXPECTED,
} t_frame_read_status;

typedef struct {
    t_frame_read_status status;
    size_t bytes_read;
} t_frame_read_result;

typedef enum eFrameWriteStatus {
    FRAME_WRITE_OK = 0L,
    FRAME_WRITE_UNDERRUN,
    FRAME_WRITE_COLLISION,
    FRAME_WRITE_READY_TIMEOUT,
    FRAME_WRITE_UNEXPECTED,
    FRAME_WRITE_OVERFLOW
} tFrameWriteStatus;

typedef struct {
    bool        valid;
    uint32_t    expiry;
    uint32_t    reply_id;
    uint8_t     station;
    uint8_t     net;
} pending_reply_t;

static t_frame_read_result      _read_frame(uint8_t* buffer, size_t buffer_len, uint8_t* addr, size_t addr_len, uint timeout_ms, bool flag_fill);
static t_frame_parse_result     _parse_frame(uint8_t* buffer, size_t len, bool is_opening_frame);
static econet_rx_result_t       _handle_first_frame();
static econet_rx_result_t       _rx_data_for_scout(t_frame_parse_result* scout_frame);
static econet_rx_result_t       _handle_immediate_scout(t_frame_parse_result* immediate_scout_frame);
static econet_rx_result_t       _handle_transmit_scout(t_frame_parse_result* transmit_scout_frame);
static econet_rx_result_t       _handle_broadcast(t_frame_parse_result* broadcast_frame);
static tFrameWriteStatus        _tx_frame(uint8_t* buffer, size_t len, bool flag_fill);
static tFrameWriteStatus        _send_ack(t_frame_parse_result* incoming_frame, const uint8_t* extra_data, size_t extra_data_len, bool flag_fill);
static bool                     _wait_ack(uint8_t from_station, uint8_t from_network, uint8_t to_station, uint8_t to_network);
static econet_rx_result_t       _rx_result_for_error(econet_rx_error_t error);
static econet_tx_result_t       _tx_result_for_frame_status(tFrameWriteStatus status);
static void                     _check_reply_timeout(void);
static econet_rx_result_t       _map_read_frame_result(t_frame_read_status status);
static void                     _abort_read(void);
static void                     _clear_rx(bool flag_fill);
static void                     _finish_tx(bool flag_fill);


static bool                     _initialised;
uint8_t                         _listen_addresses[] = { 0x02, 0xFF };
pending_reply_t                 _pending_reply;

static uint8_t* _rx_scout_buffer;
static size_t   _rx_scout_buffer_sz;
static uint8_t* _rx_data_buffer;
static size_t   _rx_data_buffer_sz;
static uint8_t* _tx_scout_buffer;
static size_t   _tx_scout_buffer_sz;
static uint8_t* _tx_data_buffer;
static size_t   _tx_data_buffer_sz;
static uint8_t* _ack_buffer;
static size_t   _ack_buffer_sz;

bool econet_init(void) {
    if (_initialised) {
        return false;
    }

    adlc_init();
    adlc_irq_reset();

    _initialised = true;

    return true;
}

econet_tx_result_t broadcast(
                            const uint8_t*  data,
                            size_t          data_len) {
    if (!_initialised) {
        return PICONET_TX_RESULT_ERROR_UNINITIALISED;
    }

    size_t data_frame_len = data_len + 4;
    if (data_frame_len > _tx_data_buffer_sz) {
        return PICONET_TX_RESULT_ERROR_OVERFLOW;
    }
    _tx_data_buffer[0] = 0xff;
    _tx_data_buffer[1] = 0xff;
    _tx_data_buffer[2] = _listen_addresses[0];
    _tx_data_buffer[3] = 0x00;
    memcpy(_tx_data_buffer + 4, data, data_len);

    return _tx_result_for_frame_status(_tx_frame(_tx_data_buffer, data_frame_len, false));
}

econet_tx_result_t transmit(
        uint8_t         station,
        uint8_t         network,
        uint8_t         control,
        uint8_t         port,
        const uint8_t*  data,
        size_t          data_len,
        const uint8_t*  scout_extra_data,
        size_t          scout_extra_data_len) {
    if (!_initialised) {
        return PICONET_TX_RESULT_ERROR_UNINITIALISED;
    }

    size_t data_frame_len = data_len + 4;
    if (data_frame_len > _tx_data_buffer_sz) {
        return PICONET_TX_RESULT_ERROR_OVERFLOW;
    }
    _tx_data_buffer[0] = station;
    _tx_data_buffer[1] = network;
    _tx_data_buffer[2] = _listen_addresses[0];
    _tx_data_buffer[3] = 0x00;
    memcpy(_tx_data_buffer + 4, data, data_len);

    size_t scout_frame_len = scout_extra_data_len + 6;
    if (scout_frame_len > _tx_scout_buffer_sz) {
        return PICONET_TX_RESULT_ERROR_OVERFLOW;
    }
    _tx_scout_buffer[0] = station;
    _tx_scout_buffer[1] = 0x00;
    _tx_scout_buffer[2] = _listen_addresses[0];
    _tx_scout_buffer[3] = 0x00;
    _tx_scout_buffer[4] = control;
    _tx_scout_buffer[5] = port;
    memcpy(_tx_scout_buffer + 6, scout_extra_data, scout_extra_data_len);

    adlc_update_data_led(true);
    econet_tx_result_t scout_result = _tx_result_for_frame_status(_tx_frame(_tx_scout_buffer, scout_frame_len, true));
    if (scout_result != PICONET_TX_RESULT_OK) {
        adlc_update_data_led(false);
        return scout_result;
    }

    if (!_wait_ack(station, network, _listen_addresses[0], 0x00)) {
        adlc_update_data_led(false);
        return PICONET_TX_RESULT_ERROR_NO_SCOUT_ACK;
    }

    econet_tx_result_t data_result = _tx_result_for_frame_status(_tx_frame(_tx_data_buffer, data_frame_len, true));
    if (data_result != PICONET_TX_RESULT_OK) {
        adlc_update_data_led(false);
        return data_result;
    }

    if (!_wait_ack(station, network, _listen_addresses[0], 0x00)) {
        adlc_update_data_led(false);
        return PICONET_TX_RESULT_ERROR_NO_DATA_ACK;
    }

    adlc_update_data_led(false);
    return PICONET_TX_RESULT_OK;
}

econet_tx_result_t reply(uint16_t reply_id, const uint8_t* data, size_t data_len) {
    if (!_initialised) {
        return PICONET_TX_RESULT_ERROR_UNINITIALISED;
    }

    if (!_pending_reply.valid || _pending_reply.reply_id != reply_id) {
        return PICONET_TX_RESULT_ERROR_INVALID_RECEIVE_ID;
    }

    size_t data_frame_len = data_len + 2;
    if (data_frame_len > _tx_data_buffer_sz) {
        return PICONET_TX_RESULT_ERROR_OVERFLOW;
    }
    _tx_data_buffer[0] = _pending_reply.station;
    _tx_data_buffer[1] = _pending_reply.net;
    memcpy(_tx_data_buffer + 2, data, data_len);

    _pending_reply.valid = false;

    econet_tx_result_t data_result = _tx_result_for_frame_status(_tx_frame(_tx_data_buffer, data_frame_len, false));
    if (data_result != PICONET_TX_RESULT_OK) {
        return data_result;
    }

    if (!_wait_ack(_pending_reply.station, _pending_reply.net, _listen_addresses[0], 0x00)) {
        return PICONET_TX_RESULT_ERROR_NO_DATA_ACK;
    }

    return PICONET_TX_RESULT_OK;
}

econet_rx_result_t receive() {
    if (!_initialised) {
        return _rx_result_for_error(ECONET_RX_ERROR_UNINITIALISED);
    }

    _check_reply_timeout();

    econet_rx_result_t result;
    result.type = PICONET_RX_RESULT_NONE;
    result.error = ECONET_RX_ERROR_NONE;

    uint status_reg_1 = adlc_read(REG_STATUS_1);

    if (status_reg_1 & STATUS_1_S2_RD_REQ) {
        uint status_reg_2 = adlc_read(REG_STATUS_2);

        if (status_reg_2 & STATUS_2_ADDR_PRESENT) {
            adlc_update_data_led(true);
            result = _handle_first_frame();
            adlc_update_data_led(false);
        }

        adlc_irq_reset();
    } else if (status_reg_1 & STATUS_1_RDA) {
        _abort_read();
        adlc_irq_reset();
    }

    return result;
}

econet_rx_result_t monitor() {
    if (!_initialised) {
        return _rx_result_for_error(ECONET_RX_ERROR_UNINITIALISED);
    }

    econet_rx_result_t result;
    result.type = PICONET_RX_RESULT_NONE;
    result.detail.data = NULL;
    result.detail.data_len = 0;
    result.detail.scout = NULL;
    result.detail.scout_len = 0;

    uint status_reg_1 = adlc_read(REG_STATUS_1);

    if (status_reg_1 & STATUS_1_S2_RD_REQ) {
        uint status_reg_2 = adlc_read(REG_STATUS_2);

        if (status_reg_2 & STATUS_2_ADDR_PRESENT) {
            adlc_update_data_led(true);
            t_frame_read_result read_frame_result = _read_frame(_rx_data_buffer, _rx_data_buffer_sz, _listen_addresses, 0, 2000, false);

            adlc_update_data_led(false);

            if (read_frame_result.status != FRAME_READ_OK) {
                return _map_read_frame_result(read_frame_result.status);
            }

            result.type = PICONET_RX_RESULT_MONITOR;
            result.detail.data = _rx_data_buffer;
            result.detail.data_len = read_frame_result.bytes_read;
        }

        adlc_irq_reset();
    } else if (status_reg_1 & STATUS_1_RDA) {
        _abort_read();
        adlc_irq_reset();
    }

    return result;
}

uint8_t get_station() {
    return _listen_addresses[0];
}

void set_station(uint8_t station) {
    _listen_addresses[0] = station;
}

void set_tx_scout_buffer(
        uint8_t*    tx_scout_buffer,
        size_t      tx_scout_buffer_sz) {
    _tx_scout_buffer    = tx_scout_buffer;
    _tx_scout_buffer_sz = tx_scout_buffer_sz;
}

void set_tx_data_buffer(
        uint8_t*    tx_data_buffer,
        size_t      tx_data_buffer_sz) {
    _tx_data_buffer     = tx_data_buffer;
    _tx_data_buffer_sz  = tx_data_buffer_sz;
}

void set_rx_scout_buffer(
        uint8_t*    rx_scout_buffer,
        size_t      rx_scout_buffer_sz) {
    _rx_scout_buffer    = rx_scout_buffer;
    _rx_scout_buffer_sz = rx_scout_buffer_sz;
}

void set_rx_data_buffer(
        uint8_t*    rx_data_buffer,
        size_t      rx_data_buffer_sz) {
    _rx_data_buffer     = rx_data_buffer;
    _rx_data_buffer_sz  = rx_data_buffer_sz;
}

void set_ack_buffer(
        uint8_t*    ack_buffer,
        size_t      ack_buffer_sz) {
    _ack_buffer = ack_buffer;
    _ack_buffer_sz = ack_buffer_sz;
}

static t_frame_parse_result _parse_frame(uint8_t* buffer, size_t len, bool is_opening_frame) {
    t_frame_parse_result result;

    if (len < 4) {
        result.type = FRAME_TYPE_UNKNOWN;
        return result;
    }

    result.frame.dest_station = *buffer;
    result.frame.dest_net = *(buffer + 1);
    result.frame.src_station = *(buffer + 2);
    result.frame.src_net = *(buffer + 3);
    result.frame.frame = buffer;
    result.frame.frame_len = len;
    result.frame.data = buffer + 4;
    result.frame.data_len = len - 4;

    if (!is_opening_frame) {
        if (len == 4) {
            result.type = FRAME_TYPE_ACK;
            return result;
        }

        result.type = FRAME_TYPE_DATA;
        result.frame.data = buffer + 4;
        result.frame.data_len = len - 4;
        return result;
    }

    if (len < 6) {
        result.type = FRAME_TYPE_UNKNOWN;
        return result;
    }

    result.frame.ctrl = *(buffer + 4);
    result.frame.port = *(buffer + 5);
    result.frame.data = buffer + 6;
    result.frame.data_len = len - 6;

    if ((result.frame.dest_station == 0x00) || (result.frame.dest_station == 0xff)) {
        result.type = FRAME_TYPE_BROADCAST;
    } else if (len == 6) {
        result.type = FRAME_TYPE_TRANSMIT;
    } else if (result.frame.port == 0 && len > 6) {
        result.type = FRAME_TYPE_IMMEDIATE;
    } else {
        result.type = FRAME_TYPE_UNKNOWN;
    }

    return result;
}

static tFrameWriteStatus _tx_frame(uint8_t* buffer, size_t len, bool flag_fill) {
    uint sr1;

    uint32_t time_start_ms = time_ms();

    adlc_write_cr1(0b00000000); // Disable RX interrupts

    while (!(adlc_read(REG_STATUS_1) & STATUS_1_FRAME_COMPLETE)) {
        if (time_ms() > time_start_ms + TIMEOUT_WRITE_READY_MS) {
            return FRAME_WRITE_READY_TIMEOUT;
        }

        adlc_write_cr2(CR2_RTS_CONTROL | CR2_CLEAR_TX_STATUS | CR2_CLEAR_RX_STATUS | CR2_FLAG_FILL | CR2_PRIO_STATUS_ENABLE);
    };

    for (uint ptr = 0; ptr < len; ptr++) {
        // While not FC/TDRA set, loop until it is - or we get an error
        while (true) {
            uint sr1 = adlc_read(REG_STATUS_1);
            if (sr1 & STATUS_1_TX_UNDERRUN) {
                _finish_tx(false);
                return FRAME_WRITE_UNDERRUN;
            }
            if (sr1 & STATUS_1_FRAME_COMPLETE) {
                break; // We have TDRA
            }

            if (time_ms() > time_start_ms + TIMEOUT_WRITE_READY_MS) {
                _finish_tx(false);
                return FRAME_WRITE_READY_TIMEOUT;
            }
        }

        adlc_write_fifo(buffer[ptr]);
    }

    adlc_write_cr2(CR2_TX_LAST_DATA | CR2_FRAME_COMPLETE | CR2_FLAG_FILL | CR2_PRIO_STATUS_ENABLE); 
    adlc_write_cr1(CR1_TIE);

    // wait for IRQ
    while (true) {
        sr1 = adlc_read(REG_STATUS_1);
        if (sr1 & STATUS_1_IRQ) {
            break;
        }
        if (time_ms() > time_start_ms + TIMEOUT_WRITE_COMPLETE_MS) {
            _finish_tx(false);
            return FRAME_WRITE_READY_TIMEOUT;
        }
    };

    _finish_tx(false);

    if (!(sr1 & STATUS_1_FRAME_COMPLETE)) {
        return FRAME_WRITE_UNEXPECTED;
    }

    return FRAME_WRITE_OK;
}

static tFrameWriteStatus _send_ack(t_frame_parse_result* incoming_frame, const uint8_t* extra_data, size_t extra_data_len, bool flag_fill) {
    adlc_write_cr2(CR2_RTS_CONTROL | CR2_CLEAR_TX_STATUS | CR2_CLEAR_RX_STATUS | CR2_FLAG_FILL);
    adlc_write_cr1(CR1_TX_RESET | CR1_RX_RESET | CR1_RX_FRAME_DISCONTINUE | CR1_TIE);

    size_t frame_len = 4 + extra_data_len;
    if ( frame_len > _ack_buffer_sz) {
        return FRAME_WRITE_OVERFLOW;
    }

    _ack_buffer[0] = incoming_frame->frame.src_station;
    _ack_buffer[1] = incoming_frame->frame.src_net;
    _ack_buffer[2] = incoming_frame->frame.dest_station;
    _ack_buffer[3] = incoming_frame->frame.dest_net;

    memcpy(_ack_buffer + 4, extra_data, extra_data_len);

    return _tx_frame(_ack_buffer, frame_len, flag_fill);
}

static bool _wait_frame_start(uint32_t timeout_ms) {
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

static econet_rx_result_t _rx_result_for_error(econet_rx_error_t error) {
    econet_rx_result_t result;
    result.type = PICONET_RX_RESULT_ERROR;
    result.error = error;
    return result;
}

static econet_tx_result_t _tx_result_for_frame_status(tFrameWriteStatus status) {
    switch (status) {
        case FRAME_WRITE_OK:
            return PICONET_TX_RESULT_OK;
        case FRAME_WRITE_UNDERRUN:
            return PICONET_TX_RESULT_ERROR_UNDERRUN;
        case FRAME_WRITE_READY_TIMEOUT:
            return PICONET_TX_RESULT_ERROR_LINE_JAMMED;
        default:
            return PICONET_TX_RESULT_ERROR_MISC;
    }
}

static bool _wait_ack(uint8_t from_station, uint8_t from_network, uint8_t to_station, uint8_t to_network) {
    t_frame_read_result ack_frame_result;
    while (true) {
        adlc_irq_reset();

        if (!_wait_frame_start(TIMEOUT_WAIT_ACK_MS)) {
            return false;
        }

        ack_frame_result = _read_frame(_ack_buffer, _ack_buffer_sz, _listen_addresses, 1, 2000, false);
        if (ack_frame_result.status == FRAME_READ_OK) {
            break;
        }

        _abort_read();
    }

    t_frame_parse_result ack_frame = _parse_frame(_ack_buffer, ack_frame_result.bytes_read, false);
    if (ack_frame.type != FRAME_TYPE_ACK
            || ack_frame.frame.src_station != from_station || ack_frame.frame.src_net != from_network
            || ack_frame.frame.dest_station != to_station || ack_frame.frame.dest_net != to_network) {
        printf("ERROR [_wait_ack] unexpected frame! type %d from station %u to station %u\n",
            ack_frame.type,
            ack_frame.frame.src_station,
            ack_frame.frame.dest_station);
        return false;
    }

    return true;
}

static econet_rx_result_t _rx_data_for_scout(t_frame_parse_result* scout_frame) {
    tFrameWriteStatus scout_ack_result = _send_ack(scout_frame, NULL, 0, true);
    if (scout_ack_result != FRAME_WRITE_OK) {
        printf("ERROR [_rx_data_for_scout] scout ack failed code=%u\n", scout_ack_result);
        return _rx_result_for_error(ECONET_RX_ERROR_SCOUT_ACK);
    }

    adlc_irq_reset();

    if (!_wait_frame_start(TIMEOUT_DATA_FRAME_MS)) {
        printf("ERROR [_rx_data_for_scout] timed out waiting for data following scout ack\n");
        return _rx_result_for_error(ECONET_RX_ERROR_TIMEOUT);
    }

    t_frame_read_result data_frame_result = _read_frame(
        _rx_data_buffer,
        _rx_data_buffer_sz,
        _listen_addresses,
        sizeof(_listen_addresses),
        TIMEOUT_READ_DATA_MS,
        false);
    if (data_frame_result.status != FRAME_READ_OK) {
        printf("ERROR [_rx_data_for_scout] error reading data following scout ack, error code=%u\n", data_frame_result.status);
        return _rx_result_for_error(data_frame_result.status);
    }

    t_frame_parse_result data_frame = _parse_frame(_rx_data_buffer, data_frame_result.bytes_read, false);
    if (data_frame.type != FRAME_TYPE_DATA) {
        printf("ERROR [_rx_data_for_scout] parse failed type=%u len=%u - aborting\n", data_frame.type, data_frame_result.bytes_read);
        _abort_read();
        return _rx_result_for_error(ECONET_RX_ERROR_MISC);
    }

    tFrameWriteStatus data_ack_result = _send_ack(&data_frame, NULL, 0, false);
    if (data_ack_result != FRAME_WRITE_OK) {
        printf("ERROR [_rx_data_for_scout] data ack failed code=%u\n", data_ack_result);
        return _rx_result_for_error(ECONET_RX_ERROR_DATA_ACK);
    }

    econet_rx_result_t result;
    result.type = PICONET_RX_RESULT_TRANSMIT;
    result.detail.scout = scout_frame->frame.frame;
    result.detail.scout_len = scout_frame->frame.frame_len;
    result.detail.data = data_frame.frame.frame;
    result.detail.data_len = data_frame.frame.frame_len;

    return result;
}

static econet_rx_result_t _handle_immediate_scout(t_frame_parse_result* immediate_scout_frame) {
    if (immediate_scout_frame->frame.ctrl == 0x88) {
        // Machine peek - see https://stardot.org.uk/forums/viewtopic.php?p=390596#p390596
        uint8_t machine_type = 0x55;    // U = undefined
        uint8_t manufacturer_id = 0x4a; // J = JPR
        uint8_t version_minor = 0x00;   // first release
        uint8_t version_major = 0x05;   // normal 32-bit client

        uint8_t ack_extra_data[4];
        ack_extra_data[0] = machine_type;
        ack_extra_data[1] = manufacturer_id;
        ack_extra_data[2] = version_minor;
        ack_extra_data[3] = version_major;

        tFrameWriteStatus scout_ack_result = _send_ack(immediate_scout_frame, ack_extra_data, sizeof(ack_extra_data), false);
        if (scout_ack_result != FRAME_WRITE_OK) {
            printf("ERROR [_handle_immediate_scout] scout ack failed code=%u\n", scout_ack_result);
            return _rx_result_for_error(ECONET_RX_ERROR_SCOUT_ACK);
        }

        _abort_read();

        econet_rx_result_t result;
        result.type = PICONET_RX_RESULT_NONE;
        return result;
    }

    return _rx_data_for_scout(immediate_scout_frame);
}

static econet_rx_result_t _handle_transmit_scout(t_frame_parse_result* transmit_scout_frame) {
    econet_rx_result_t result = _rx_data_for_scout(transmit_scout_frame);
    if (result.type == PICONET_RX_RESULT_TRANSMIT) {
        // set flag fill while we wait a reply
        // adlc_write_cr2(CR2_CLEAR_TX_STATUS | CR2_CLEAR_RX_STATUS | CR2_FLAG_FILL | CR2_PRIO_STATUS_ENABLE);

        // _pending_reply.reply_id++;
        // _pending_reply.expiry = time_ms() + 250; // TODO constant
        // _pending_reply.station = transmit_scout_frame->frame.src_station;
        // _pending_reply.net = transmit_scout_frame->frame.src_net;
        // _pending_reply.valid = true;

        // result.detail.needs_reply = true;
        // result.detail.reply_id = _pending_reply.reply_id;
    } else {
        printf("ERROR [_handle_transmit_scout] unexpected result type=%u\n", result.type);
    }

    return result;
}

static econet_rx_result_t _handle_broadcast(t_frame_parse_result* broadcast_frame) {
    econet_rx_result_t result;
    result.type = PICONET_RX_RESULT_BROADCAST;
    result.detail.scout = NULL;
    result.detail.scout_len = 0;
    result.detail.data = broadcast_frame->frame.frame;
    result.detail.data_len = broadcast_frame->frame.frame_len;
    return result;
}

static econet_rx_result_t _handle_first_frame() {
    t_frame_read_result read_frame_result = _read_frame(
        _rx_scout_buffer,
        _rx_scout_buffer_sz,
        _listen_addresses,
        sizeof(_listen_addresses),
        TIMEOUT_READ_FIRST_FRAME_MS,
        true);

    if (read_frame_result.status != FRAME_READ_OK) {
        printf("ERROR [_handle_first_frame] read failed code=%u - aborting\n", read_frame_result.status);
        _abort_read();
        return _map_read_frame_result(read_frame_result.status);
    }

    t_frame_parse_result result = _parse_frame(_rx_scout_buffer, read_frame_result.bytes_read, true);
    switch (result.type) {
        case FRAME_TYPE_TRANSMIT :
            return _handle_transmit_scout(&result);
        case FRAME_TYPE_IMMEDIATE :
            return _handle_immediate_scout(&result);
        case FRAME_TYPE_BROADCAST :
            _abort_read();
            return _handle_broadcast(&result);
        default :
            printf("ERROR [_handle_first_frame] unexpected type=%u bytes_read=%u - aborting\n", result.type, read_frame_result.bytes_read);
            _abort_read();
            break;
    }

    econet_rx_result_t retval;
    retval.type = PICONET_RX_RESULT_NONE;
    return retval;
}

static t_frame_read_result _read_frame(uint8_t* buffer, size_t buffer_len, uint8_t* addr, size_t addr_len, uint timeout_ms, bool flag_fill) {
    t_frame_read_result result = {
        FRAME_READ_ERROR_UNEXPECTED,
        0
    };
    uint stat = 0;

    if (buffer_len == 0) {
        _abort_read();
        result.status = FRAME_READ_ERROR_OVERFLOW;
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
            _abort_read();
            result.status = FRAME_READ_NO_ADDR_MATCH;
            return result;
        }
    }

    uint32_t time_start_ms = time_ms();

    bool frame_valid = false;
    while (!frame_valid) {
        do {
            if (time_ms() > time_start_ms + timeout_ms) {
                _abort_read();
                result.status = FRAME_READ_ERROR_TIMEOUT;
                return result;
            }

            stat = adlc_read(REG_STATUS_2);
        } while (!(stat & (STATUS_2_RDA | STATUS_2_FRAME_VALID | STATUS_2_ABORT_RX | STATUS_2_FCS_ERROR | STATUS_2_RX_OVERRUN)));

        if (stat & (STATUS_2_ABORT_RX | STATUS_2_FCS_ERROR | STATUS_2_RX_OVERRUN)) {
            if (stat & STATUS_2_ABORT_RX) {
                result.status = FRAME_READ_ERROR_ABORT;
            } else if (stat & STATUS_2_FCS_ERROR) {
                result.status = FRAME_READ_ERROR_CRC;
            } else if (stat & STATUS_2_RX_OVERRUN) {
                result.status = FRAME_READ_ERROR_OVERRUN;
            }
            _abort_read();
            return result;
        }

        if (stat & (STATUS_2_FRAME_VALID | STATUS_2_RDA)) {
            if (result.bytes_read >= buffer_len) {
                _abort_read();
                result.status = FRAME_READ_ERROR_OVERFLOW;
                return result;
            }

            buffer[result.bytes_read++] = adlc_read(REG_FIFO);
        }

        frame_valid = (stat & STATUS_2_FRAME_VALID);
    }

    _clear_rx(flag_fill);

    result.status = FRAME_READ_OK;
    return result;
}

static void _check_reply_timeout(void) {
    if (_pending_reply.valid && (time_ms() < _pending_reply.expiry)) {
        return;
    }

    // client failed to reply in time - stop idle flag to avoid jamming network
    _pending_reply.valid = false;
    adlc_write_cr2(CR2_CLEAR_TX_STATUS | CR2_CLEAR_RX_STATUS | CR2_PRIO_STATUS_ENABLE);
}

static econet_rx_result_t _map_read_frame_result(t_frame_read_status status) {
    econet_rx_result_t result;
    switch (status) {
        case FRAME_READ_NO_ADDR_MATCH :
            result.type = PICONET_RX_RESULT_NONE;
            break;
        case FRAME_READ_ERROR_CRC :
            result.type = PICONET_RX_RESULT_ERROR;
            result.error = ECONET_RX_ERROR_CRC;
            break;
        case FRAME_READ_ERROR_OVERRUN :
            result.type = PICONET_RX_RESULT_ERROR;
            result.error = ECONET_RX_ERROR_OVERRUN;
            break;
        case FRAME_READ_ERROR_ABORT :
            result.type = PICONET_RX_RESULT_ERROR;
            result.error = ECONET_RX_ERROR_ABORT;
            break;
        case FRAME_READ_ERROR_TIMEOUT :
            result.type = PICONET_RX_RESULT_ERROR;
            result.error = ECONET_RX_ERROR_TIMEOUT;
            break;
        case FRAME_READ_ERROR_OVERFLOW :
            result.type = PICONET_RX_RESULT_ERROR;
            result.error = ECONET_RX_ERROR_OVERFLOW;
            break;
        case FRAME_READ_ERROR_UNEXPECTED :
            result.type = PICONET_RX_RESULT_ERROR;
            result.error = ECONET_RX_ERROR_MISC;
            break;
        default :
            result.type = PICONET_RX_RESULT_ERROR;
            result.error = ECONET_RX_ERROR_MISC;
            break;
    }
    return result;
}

static void _abort_read(void) {
    adlc_write_cr2(CR2_PRIO_STATUS_ENABLE | CR2_CLEAR_RX_STATUS | CR2_CLEAR_TX_STATUS | CR2_FLAG_FILL | CR2_2_BYTE_TRANSFER);
    adlc_write_cr1(CR1_RX_FRAME_DISCONTINUE | CR1_RIE | CR1_RX_RESET | CR1_TX_RESET);
    adlc_write_cr1(CR1_RIE | CR1_TX_RESET);
}

static void _clear_rx(bool flag_fill) {
    adlc_write_cr2(CR2_PRIO_STATUS_ENABLE | CR2_CLEAR_RX_STATUS | CR2_CLEAR_TX_STATUS | (flag_fill ? CR2_FLAG_FILL : 0));
    adlc_write_cr1(CR1_RIE | CR1_RX_RESET);
}

static void _finish_tx(bool flag_fill) {
    adlc_write_cr2(CR2_CLEAR_TX_STATUS | CR2_CLEAR_RX_STATUS | CR2_PRIO_STATUS_ENABLE | (flag_fill ? CR2_FLAG_FILL : 0));
    adlc_write_cr1(CR1_RIE | CR1_RX_RESET);
}

/*
void adlc_irq_reset(void) {
  adlc_write_cr1(CR1_RIE);
  adlc_write(1, CR2_CLEAR_TX_STATUS | CR2_CLEAR_RX_STATUS | CR2_PRIO_STATUS_ENABLE);
}
*/