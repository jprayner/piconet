#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"

#include "econet.h"
#include "adlc.h"
#include "util.h"
#include "buffer_pool.h"
#include "./lib/b64/cencode.h"
#include "./lib/b64/cdecode.h"

#define VERSION_MAJOR           2
#define VERSION_MINOR           0
#define VERSION_REV             20
#define VERSION_STR_MAXLEN      17

#define TX_DATA_BUFFER_SZ       3500
#define RX_DATA_BUFFER_SZ       16536
#define TX_SCOUT_BUFFER_SZ      32
#define RX_SCOUT_BUFFER_SZ      32
#define ACK_BUFFER_SZ           32
#define B64_SCOUT_BUFFER_SZ     RX_SCOUT_BUFFER_SZ * 2
#define B64_DATA_BUFFER_SZ      RX_DATA_BUFFER_SZ * 2
#define CMD_BUFFER_SZ           TX_DATA_BUFFER_SZ * 2

#define QUEUE_SZ_CMD            1
#define QUEUE_SZ_EVENT          6

#define CMD_STATUS              "STATUS"
#define CMD_RESTART             "RESTART"
#define CMD_SET_MODE            "SET_MODE"
#define CMD_SET_STATION         "SET_STATION"
#define CMD_TX                  "TX"
#define CMD_REPLY               "REPLY"
#define CMD_BCAST               "BCAST"
#define CMD_TEST                "TEST"

#define CMD_PARAM_MODE_STOP     "STOP"
#define CMD_PARAM_MODE_LISTEN   "LISTEN"
#define CMD_PARAM_MODE_MONITOR  "MONITOR"

typedef enum ePiconetEventType {
    PICONET_STATUS_EVENT = 0L,
    PICONET_RX_EVENT,
    PICONET_TX_EVENT,
    PICONET_REPLY_EVENT
} tPiconetEventType;

typedef enum {
    PICONET_CMD_SET_MODE_STOP = 0L,
    PICONET_CMD_SET_MODE_LISTEN,
    PICONET_CMD_SET_MODE_MONITOR
} piconet_mode_t;

typedef struct {
    econet_rx_result_type_t type;
    econet_rx_error_t       error;
    uint8_t                 scout[RX_SCOUT_BUFFER_SZ];
    size_t                  scout_len;
    uint                    data_buffer_handle;
    size_t                  data_len;
    uint16_t                reply_id;
} econet_rx_event_t;

typedef struct {
    econet_tx_result_t      type;
} econet_tx_event_t;

typedef struct {
    char                    version[VERSION_STR_MAXLEN];
    uint8_t                 station;
    uint8_t                 status_register_1;
    piconet_mode_t          mode;
} event_status_t;

typedef struct
{
    tPiconetEventType type;
    union {
        econet_rx_event_t   rx_event_detail;    // if type == PICONET_RX_EVENT
        econet_tx_event_t   tx_event_detail;    // if type == PICONET_TX_EVENT
        econet_tx_event_t   reply_event_detail;    // if type == PICONET_REPLY_EVENT
        event_status_t      status;             // if type == PICONET_STATUS_EVENT
    };
} event_t;

typedef enum {
    PICONET_CMD_STATUS = 0L,
    PICONET_CMD_RESTART,
    PICONET_CMD_SET_MODE,
    PICONET_CMD_SET_STATION,
    PICONET_CMD_TX,
    PICONET_CMD_REPLY,
    PICONET_CMD_BCAST,
    PICONET_CMD_TEST,
} cmd_type_t;

typedef struct {
    uint8_t                 dest_station;
    uint8_t                 dest_network;
    uint8_t                 control_byte;
    uint8_t                 port;
    uint8_t                 data[TX_DATA_BUFFER_SZ];
    size_t                  data_len;
    uint8_t                 scout_extra_data[TX_DATA_BUFFER_SZ]; // TODO: really this long?
    size_t                  scout_extra_data_len;
} cmd_tx_t;

typedef struct {
    uint8_t                 data[TX_DATA_BUFFER_SZ];
    size_t                  data_len;
} cmd_bcast_t;

typedef struct {
    uint16_t                reply_id;
    uint8_t                 data[TX_DATA_BUFFER_SZ];
    size_t                  data_len;
} cmd_reply_t;

typedef struct {
    cmd_type_t type;
    union {
        piconet_mode_t      set_mode;   // if type == PICONET_CMD_SET_MODE
        cmd_tx_t            tx;         // if type == PICONET_CMD_TX
        cmd_reply_t         reply;      // if type == PICONET_CMD_REPLY
        cmd_bcast_t         bcast;      // if type == PICONET_CMD_BCAST
        uint8_t             station;    // if type == PICONET_CMD_ACK
    };
} command_t;

queue_t     command_queue;
queue_t     event_queue;
command_t   cmd;
char*       b64_scout_buffer;
char*       b64_data_buffer;
pool_t      rx_buffer_pool;

void    _core0_loop(void);
void    _core1_loop(void);
char*   _tx_error_to_str(econet_tx_result_t error);
char*   _rx_error_to_str(econet_rx_error_t error);
void    _read_command_input(void);
char*   _encode_base64(char* output_buffer, const uint8_t* input, size_t len);
size_t  _decode_base64(const char* input, uint8_t* output_buffer);
void    _test_board(void);

int main() {
    stdio_init_all();

    if (!pool_init(&rx_buffer_pool, RX_DATA_BUFFER_SZ, QUEUE_SZ_EVENT)) {
        printf("ERROR Failed to allocate memory for RX data buffers\n");
        return 1;
    }

    b64_scout_buffer = malloc(B64_SCOUT_BUFFER_SZ);
    if (b64_scout_buffer == NULL) {
        printf("ERROR Failed to allocate memory for base64 scout buffer\n");
        return 1;
    }

    b64_data_buffer = malloc(B64_DATA_BUFFER_SZ);
    if (b64_data_buffer == NULL) {
        printf("ERROR Failed to allocate memory for base64 data buffer\n");
        return 1;
    }

    queue_init(&command_queue, sizeof(command_t), QUEUE_SZ_CMD);
    queue_init(&event_queue, sizeof(event_t), QUEUE_SZ_EVENT);
    multicore_launch_core1(_core1_loop);

    _core0_loop();
}

void _core0_loop(void) {
    event_t event;
    while (true) {
        _read_command_input();

        if (!queue_try_remove(&event_queue, &event)) {
            continue;
        }
        switch (event.type) {
            case PICONET_STATUS_EVENT: {
                printf(
                    "STATUS %s %d %02x %d\n",
                    event.status.version,
                    event.status.station,
                    event.status.status_register_1,
                    event.status.mode);
                break;
            }

            case PICONET_TX_EVENT: {
                printf("TX_RESULT %s\n", _tx_error_to_str(event.tx_event_detail.type));
                break;
            }

            case PICONET_REPLY_EVENT: {
                printf("REPLY_RESULT %s\n", _tx_error_to_str(event.reply_event_detail.type));
                break;
            }

            case PICONET_RX_EVENT: {
                if (event.rx_event_detail.type == PICONET_RX_RESULT_ERROR) {
                    printf("ERROR %s\n", _rx_error_to_str(event.rx_event_detail.error));
                    break;
                }

                buffer_t* buffer = pool_buffer_get(&rx_buffer_pool, event.rx_event_detail.data_buffer_handle);
                if (buffer == NULL) {
                    printf("ERROR Failed to get RX data buffer - logic error\n");
                    break;
                }
 
                switch (event.rx_event_detail.type) {
                    case PICONET_RX_RESULT_MONITOR :
                        printf("MONITOR %s\n", _encode_base64(
                            b64_data_buffer,
                            buffer->data,
                            event.rx_event_detail.data_len));
                        break;
                    case PICONET_RX_RESULT_BROADCAST :
                        printf("RX_BROADCAST %s\n", _encode_base64(
                            b64_data_buffer,
                            buffer->data,
                            event.rx_event_detail.data_len));
                        break;
                    case PICONET_RX_RESULT_IMMEDIATE_OP :
                        printf(
                            "RX_IMMEDIATE %s %s\n",
                            _encode_base64(
                                b64_scout_buffer,
                                event.rx_event_detail.scout,
                                event.rx_event_detail.scout_len),
                            _encode_base64(
                                b64_data_buffer,
                                buffer->data,
                                event.rx_event_detail.data_len));
                        break;
                    case PICONET_RX_RESULT_TRANSMIT :
                        printf(
                            "RX_TRANSMIT %s %s\n",
                            _encode_base64(
                                b64_scout_buffer,
                                event.rx_event_detail.scout,
                                event.rx_event_detail.scout_len),
                            _encode_base64(
                                b64_data_buffer,
                                buffer->data,
                                event.rx_event_detail.data_len));
                        break;
                    default :
                        // do nothing if no data or error (latter handled above)
                        break;
                }

                pool_buffer_release(
                    &rx_buffer_pool,
                    event.rx_event_detail.data_buffer_handle);

                break;
            }

            default: {
                printf("ERROR Unexpected event type %u\n", event.type);
                break;
            }
        }
    }
}

void _core1_loop(void) {
    command_t       received_command;
    event_t         event;
    uint8_t         scout_buffer[TX_SCOUT_BUFFER_SZ];
    uint8_t         tx_buffer[TX_DATA_BUFFER_SZ];
    uint8_t         ack_buffer[ACK_BUFFER_SZ];
    piconet_mode_t  mode = PICONET_CMD_SET_MODE_STOP;

    if (!econet_init()) {
        printf("ERROR Failed to init econet module. Game over, man.\n");
        return;
    }

    set_tx_scout_buffer(scout_buffer, TX_SCOUT_BUFFER_SZ);
    set_tx_data_buffer(tx_buffer, TX_DATA_BUFFER_SZ);
    set_rx_scout_buffer(event.rx_event_detail.scout, RX_SCOUT_BUFFER_SZ);
    set_ack_buffer(ack_buffer, ACK_BUFFER_SZ);

    while (true) {
        if (queue_try_remove(&command_queue, &received_command)) {
            switch (received_command.type) {
                case PICONET_CMD_STATUS:
                    event.type = PICONET_STATUS_EVENT;
                    sprintf(event.status.version, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_REV);
                    event.status.station = get_station();
                    event.status.status_register_1 = adlc_read(REG_STATUS_2);
                    event.status.mode = mode;
                    queue_add_blocking(&event_queue, &event);
                    break;
                case PICONET_CMD_RESTART:
                    adlc_reset();
                    break;
                case PICONET_CMD_SET_MODE:
                    mode = received_command.set_mode;
                    break;
                case PICONET_CMD_SET_STATION:
                    set_station(received_command.station);
                    break;
                case PICONET_CMD_TX: {
                    econet_tx_result_t result = transmit(
                        received_command.tx.dest_station,
                        received_command.tx.dest_network,
                        received_command.tx.control_byte,
                        received_command.tx.port,
                        received_command.tx.data,
                        received_command.tx.data_len,
                        received_command.tx.scout_extra_data,
                        received_command.tx.scout_extra_data_len);
                    event.type = PICONET_TX_EVENT;
                    event.tx_event_detail.type = result;
                    queue_add_blocking(&event_queue, &event);
                    break;
                }
                case PICONET_CMD_REPLY: {
                    econet_tx_result_t result = reply(
                        received_command.reply.reply_id,
                        received_command.reply.data,
                        received_command.reply.data_len);
                    event.type = PICONET_REPLY_EVENT;
                    event.reply_event_detail.type = result;
                    queue_add_blocking(&event_queue, &event);
                    break;
                }
                case PICONET_CMD_BCAST: {
                    econet_tx_result_t result = broadcast(
                        received_command.bcast.data,
                        received_command.bcast.data_len);
                    event.type = PICONET_TX_EVENT;
                    event.tx_event_detail.type = result;
                    queue_add_blocking(&event_queue, &event);
                    break;
                }
                case PICONET_CMD_TEST: {
                    _test_board();
                    break;
                }
            }
        }

        if (mode == PICONET_CMD_SET_MODE_STOP) {
            continue;
        }

        buffer_t* rx_data_buffer = pool_buffer_claim(&rx_buffer_pool);
        if (rx_data_buffer == NULL) {
            printf("ERROR Packet rate too high, rx buffers exhausted. Increase buffer count/reduce buffer size.\n");
            continue;
        }
        set_rx_data_buffer(rx_data_buffer->data, rx_data_buffer->size);

        econet_rx_result_t rx_result = (mode == PICONET_CMD_SET_MODE_MONITOR) ? monitor() : receive();

        switch (rx_result.type) {
            case PICONET_RX_RESULT_NONE:
                pool_buffer_release(&rx_buffer_pool, rx_data_buffer->handle);
                break;
            case PICONET_RX_RESULT_ERROR:
                event.type = PICONET_RX_EVENT;
                event.rx_event_detail.type = rx_result.type;
                event.rx_event_detail.error = rx_result.error;
                queue_add_blocking(&event_queue, &event);
                pool_buffer_release(&rx_buffer_pool, rx_data_buffer->handle);
                break;
            default:
                event.type = PICONET_RX_EVENT;
                event.rx_event_detail.type = rx_result.type;
                event.rx_event_detail.scout_len = rx_result.detail.scout_len;       // scout itself populated by econet module
                event.rx_event_detail.data_len = rx_result.detail.data_len;
                event.rx_event_detail.data_buffer_handle = rx_data_buffer->handle;
                queue_add_blocking(&event_queue, &event);
                break;
        }
    }
}

char* _encode_base64(char* output_buffer, const uint8_t* input, size_t len) {
    // TODO: check for buffer overflow
    char* c = output_buffer;
	size_t cnt = 0;
	base64_encodestate s;

	base64_init_encodestate(&s);
	cnt = base64_encode_block((const char *) input, len, c, &s);
	c += cnt;
	cnt = base64_encode_blockend(c, &s);
	c += cnt;
	*c = 0;

    return output_buffer;
}

size_t _decode_base64(const char* input, uint8_t* output_buffer) {
    if (input == NULL) {
        return 0;
    }

    // TODO: check for buffer overflow
    uint8_t* c = output_buffer;
    base64_decodestate s;    
    base64_init_decodestate(&s);
    return base64_decode_block(input, strlen(input), c, &s);
}

void _read_command_input(void) {
    static char buffer[CMD_BUFFER_SZ];
    static int buffer_pos = 0;

    int c = getchar_timeout_us(0);
    if (c == PICO_ERROR_TIMEOUT) {
        return;
    }

    if (c == '\r') {
        buffer[buffer_pos] = 0;
        buffer_pos = 0;

        char delim[] = " ";
        char *ptr = strtok(buffer, delim);

        if (ptr == NULL) {
            return;
        }

        bool error = false;
        if (strcmp(ptr, CMD_STATUS) == 0) {
            cmd.type = PICONET_CMD_STATUS;
        } else if (strcmp(ptr, CMD_RESTART) == 0) {
            cmd.type = PICONET_CMD_RESTART;
        } else if (strcmp(ptr, CMD_SET_MODE) == 0) {
            cmd.type = PICONET_CMD_SET_MODE;
            const char *mode = strtok(NULL, delim);
            if (mode == NULL) {
                error = true;
            } else if (strcmp(mode, CMD_PARAM_MODE_STOP) == 0) {
                cmd.set_mode = PICONET_CMD_SET_MODE_STOP;
            } else if (strcmp(mode, CMD_PARAM_MODE_LISTEN) == 0) {
                cmd.set_mode = PICONET_CMD_SET_MODE_LISTEN;
            } else if (strcmp(mode, CMD_PARAM_MODE_MONITOR) == 0) {
                cmd.set_mode = PICONET_CMD_SET_MODE_MONITOR;
            } else {
                error = true;
            }
        } else if (strcmp(ptr, CMD_SET_STATION) == 0) {
            cmd.type = PICONET_CMD_SET_STATION;
            const char *station_str = strtok(NULL, delim);
            if (station_str == NULL) {
                error = true;
            } else {
                long station = strtol(station_str, NULL, 10);
                if (station < 0 || station > 0xff) {
                    error = true;
                } else {
                    cmd.station = station;
                }
            }
        } else if (strcmp(ptr, CMD_TX) == 0) {
            cmd.type = PICONET_CMD_TX;
            cmd.tx.dest_station = strtol(strtok(NULL, delim), NULL, 10);
            cmd.tx.dest_network = strtol(strtok(NULL, delim), NULL, 10);
            cmd.tx.control_byte = strtol(strtok(NULL, delim), NULL, 10);
            cmd.tx.port = strtol(strtok(NULL, delim), NULL, 10);
            cmd.tx.data_len = _decode_base64(strtok(NULL, delim), cmd.tx.data);
            cmd.tx.scout_extra_data_len = _decode_base64(strtok(NULL, delim), cmd.tx.scout_extra_data);
        } else if (strcmp(ptr, CMD_BCAST) == 0) {
            cmd.type = PICONET_CMD_BCAST;
            cmd.tx.data_len = _decode_base64(strtok(NULL, delim), cmd.tx.data);
        } else if (strcmp(ptr, CMD_REPLY) == 0) {
            cmd.type = PICONET_CMD_REPLY;
            cmd.reply.reply_id = strtol(strtok(NULL, delim), NULL, 10);
            cmd.reply.data_len = _decode_base64(strtok(NULL, delim), cmd.reply.data);
        } else if (strcmp(ptr, CMD_TEST) == 0) {
            cmd.type = PICONET_CMD_TEST;
        } else {
            error = true;
        }

        if (error) {
            printf("ERROR WHAT??\n");
        } else {
            queue_add_blocking(&command_queue, &cmd);
        }
    } else {
        if (buffer_pos >= sizeof(buffer)) {
            return;
        }
        buffer[buffer_pos++] = c;
    }
}

char* _rx_error_to_str(econet_rx_error_t error) {
    switch (error) {
        case ECONET_RX_ERROR_MISC:
            return "ECONET_RX_ERROR_MISC";
        case ECONET_RX_ERROR_UNINITIALISED:
            return "ECONET_RX_ERROR_UNINITIALISED";
        case ECONET_RX_ERROR_CRC:
            return "ECONET_RX_ERROR_CRC";
        case ECONET_RX_ERROR_OVERRUN:
            return "ECONET_RX_ERROR_OVERRUN";
        case ECONET_RX_ERROR_ABORT:
            return "ECONET_RX_ERROR_ABORT";
        case ECONET_RX_ERROR_TIMEOUT:
            return "ECONET_RX_ERROR_TIMEOUT";
        case ECONET_RX_ERROR_OVERFLOW:
            return "ECONET_RX_ERROR_OVERFLOW";
        case ECONET_RX_ERROR_SCOUT_ACK:
            return "ECONET_RX_ERROR_SCOUT_ACK";
        case ECONET_RX_ERROR_DATA_ACK:
            return "ECONET_RX_ERROR_DATA_ACK";
        default:
            return "UNEXPECTED";
    }
}

char* _tx_error_to_str(econet_tx_result_t error) {
    switch (error) {
        case PICONET_TX_RESULT_OK:
            return "OK";
        case PICONET_TX_RESULT_ERROR_UNINITIALISED:
            return "UNINITIALISED";
        case PICONET_TX_RESULT_ERROR_OVERFLOW:
            return "OVERFLOW";
        case PICONET_TX_RESULT_ERROR_UNDERRUN:
            return "UNDERRUN";
        case PICONET_TX_RESULT_ERROR_LINE_JAMMED:
            return "LINE_JAMMED";
        case PICONET_TX_RESULT_ERROR_NO_SCOUT_ACK:
            return "NO_SCOUT_ACK";
        case PICONET_TX_RESULT_ERROR_NO_DATA_ACK:
            return "NO_DATA_ACK";
        case PICONET_TX_RESULT_ERROR_TIMEOUT:
            return "TIMEOUT";
        case PICONET_TX_RESULT_ERROR_INVALID_RECEIVE_ID:
            return "INVALID_RECEIVE_ID";
        case PICONET_TX_RESULT_ERROR_MISC:
            return "MISC";
        default:
            return "UNEXPECTED";
    }
}

void _test_board() {
    printf("TESTING BOARD (REBOOT TO STOP)");
    while (true) {
        for (uint adlc_register = 0; adlc_register < 4; adlc_register++) {
            for (uint data_bit = 0; data_bit < 8; data_bit++) {
                adlc_write(adlc_register, 1 << data_bit);
            }
        }

        adlc_reset();
    }
}
