#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"

#define STATUS_1_RDA 1
#define STATUS_1_S2_RD_REQ 2
#define STATUS_1_LOOP 4
#define STATUS_1_FLAG_DET 8
#define STATUS_1_NOT_CTS 16
#define STATUS_1_TX_UNDERRUN 32
#define STATUS_1_FRAME_COMPLETE 64
#define STATUS_1_IRQ 128

#define STATUS_2_ADDR_PRESENT 1
#define STATUS_2_FRAME_VALID 2
#define STATUS_2_INACTIVE_IDLE_RX 4
#define STATUS_2_ABORT_RX 8
#define STATUS_2_FCS_ERROR 16
#define STATUS_2_NOT_DCD 32
#define STATUS_2_RX_OVERRUN 64
#define STATUS_2_RDA 128

typedef enum { UNSPECIFIED, INPUT, OUTPUT } data_dir_type;

const uint LED_PIN = 25;

const uint GPIO_CLK_OUT = 21;
const uint GPIO_TMP = 18; // used to test sampling clock out pin

const uint GPIO_DATA_0 = 2;
const uint GPIO_DATA_1 = 3;
const uint GPIO_DATA_2 = 4;
const uint GPIO_DATA_3 = 5;
const uint GPIO_DATA_4 = 6;
const uint GPIO_DATA_5 = 7;
const uint GPIO_DATA_6 = 8;
const uint GPIO_DATA_7 = 9;

const uint GPIO_BUFF_RnW = 10;
const uint GPIO_BUFF_A0 = 11;
const uint GPIO_BUFF_A1 = 12;
const uint GPIO_BUFF_CS = 13;
const uint GPIO_BUFF_IRQ = 14;

const uint REG_STATUS_1 = 0;
const uint REG_STATUS_2 = 1;

data_dir_type data_dir = UNSPECIFIED;

const uint CMD_ADLC_READ = 1;
const uint CMD_ADLC_WRITE = 2;

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

typedef struct
{
    uint command;
    uint reg;
    uint data_val;
} command_t;

queue_t call_queue;
queue_t results_queue;

static void _set_data_dir(data_dir_type new_data_dir) {
    if (new_data_dir == data_dir) {
        return;
    }

    switch (new_data_dir) {
        case (UNSPECIFIED):
            break;
        case (INPUT):
            gpio_put(GPIO_BUFF_RnW, 1);
            gpio_set_dir_in_masked(0xff << 2);
            data_dir = INPUT;
            break;
        case (OUTPUT):
            gpio_put(GPIO_BUFF_RnW, 0);
            gpio_set_dir_out_masked(0xff << 2);
            data_dir = OUTPUT;
            break;
    }
}


static uint adlc_read(uint reg) {
    gpio_put(GPIO_BUFF_A0, reg & 0x01);
    gpio_put(GPIO_BUFF_A1, reg & 0x02);

    _set_data_dir(INPUT);

    // wait for high to low clock transition
    while (gpio_get(GPIO_CLK_OUT) == 0);
    while (gpio_get(GPIO_CLK_OUT) == 1);
    
    // assert chip select
    gpio_put(GPIO_BUFF_CS, 0);

    // wait for low to high clock transition
    while (gpio_get(GPIO_CLK_OUT) == 0);

    // wait for data to appear (max 150ns ~= 19x 8ns Pico clock ticks @ 125MHz)
    asm volatile(
         "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
         "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
    );

    gpio_put(GPIO_TMP, 1);

    uint result = (gpio_get_all() & (0xff << 2)) >> 2;
    
    // wait for high to low clock transition
    while (gpio_get(GPIO_CLK_OUT) == 1);

    // wait 8ns (CS hold time is 10ns but there will be safely >2ns to detect clock transition)
    asm volatile("nop");

    // release chip select
    gpio_put(GPIO_BUFF_CS, 1);

    gpio_put(GPIO_TMP, 0);

    return result;
}

static void adlc_write(uint reg, uint data_val) {
    gpio_put(GPIO_BUFF_A0, reg & 0x01);
    gpio_put(GPIO_BUFF_A1, reg & 0x02);

    _set_data_dir(OUTPUT);
    gpio_put_masked(0xff << 2, data_val << 2);

    // wait for high to low clock transition
    while (gpio_get(GPIO_CLK_OUT) == 0);
    while (gpio_get(GPIO_CLK_OUT) == 1);
    
    // assert chip select
    gpio_put(GPIO_BUFF_CS, 0);

    // wait for high to low clock transition
    while (gpio_get(GPIO_CLK_OUT) == 0);
    while (gpio_get(GPIO_CLK_OUT) == 1);

    // wait 8ns (CS hold time is 10ns but there will be safely >2ns to detect clock transition)
    asm volatile("nop");

    // release chip select
    gpio_put(GPIO_BUFF_CS, 1);
}

void initADLC(){
  // Init Control Register 1 (CR1)
  adlc_write(0, 0b11000001); // Put TX and RX into reset, select address 2 for CR3 and 4
//   digitalWriteDirect(PIN_D0,0); // No logical control byte
//   digitalWriteDirect(PIN_D1,0); // 8 bit control field
//   digitalWriteDirect(PIN_D2,0); // No auto address extend
//   digitalWriteDirect(PIN_D3,0); // Idle mode all ones
//   digitalWriteDirect(PIN_D4,0); // Disable Flag detect (not used in Econet)
//   digitalWriteDirect(PIN_D5,0); // Disable Loop mode (not used in Econet)
//   digitalWriteDirect(PIN_D6,0); // Disable active on poll (not used in Econet)
//   digitalWriteDirect(PIN_D7,0); // Disable Loop on-line (not used in Econet)
  adlc_write(1, 0b00000000); // Put TX and RX into reset, select address 2 for CR3 and 4

  // init Control Register 4 (CR4)
//   digitalWriteDirect(PIN_D0,0); // Flag interframe control (not important in Econet) 
//   digitalWriteDirect(PIN_D1,1); // Set TX word length to 8 bit
//   digitalWriteDirect(PIN_D2,1); 
//   digitalWriteDirect(PIN_D3,1); // Set RX word length to 8 bit
//   digitalWriteDirect(PIN_D4,1); 
//   digitalWriteDirect(PIN_D5,0); // No transmit abort 
//   digitalWriteDirect(PIN_D6,0); // No Extended abort (not used in Econet)
//   digitalWriteDirect(PIN_D7,0); // Disable NRZI encoding (not used in Econet)
  adlc_write(3, 0b00011110); // Put TX and RX into reset, select address 2 for CR3 and 4
}

void print_status1(unsigned value) {
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

void print_status2(unsigned value) {
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
    while (1) {
        command_t entry;
        queue_remove_blocking(&call_queue, &entry);

        switch (entry.command) {
            case (CMD_ADLC_READ): {
                uint result = adlc_read(entry.reg);
                queue_add_blocking(&results_queue, &result);
                break;
            }
            case (CMD_ADLC_WRITE): {
                uint result = 0;
                adlc_write(entry.reg, entry.data_val);
                queue_add_blocking(&results_queue, &result);
                break;
            }
        }
    }
}

int main() {

    stdio_init_all();

    sleep_ms(5000); // give client a chance to reconnect
    printf("Hello, world!\n");

    // use USB clock to get 1 MHz for ADLC clock
    clock_gpio_init(21, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_USB, 48);

    // init GPIO outputs
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(GPIO_BUFF_RnW);
    gpio_set_dir(GPIO_BUFF_RnW, GPIO_OUT);
    gpio_init(GPIO_BUFF_A0);
    gpio_set_dir(GPIO_BUFF_A0, GPIO_OUT);
    gpio_init(GPIO_BUFF_A1);
    gpio_set_dir(GPIO_BUFF_A1, GPIO_OUT);
    gpio_init(GPIO_BUFF_CS);
    gpio_set_dir(GPIO_BUFF_CS, GPIO_OUT);
    gpio_init(GPIO_BUFF_IRQ);
    gpio_set_dir(GPIO_BUFF_IRQ, GPIO_OUT);

    gpio_init(GPIO_TMP);
    gpio_set_dir(GPIO_TMP, GPIO_OUT);

    // init GPIO inputs    
    gpio_init(GPIO_BUFF_IRQ);
    gpio_set_dir(GPIO_BUFF_IRQ, GPIO_IN);

    // init GPIO for input/outputs data bus
    gpio_init_mask(0xff << 2);

    gpio_put(LED_PIN, 1);

    queue_init(&call_queue, sizeof(command_t), 2);
    queue_init(&results_queue, sizeof(uint32_t), 2);
    multicore_launch_core1(core1_entry);

    initADLC();

    uint32_t call_result;
    command_t cmd;
    cmd.data_val = 0;
    cmd.command = CMD_ADLC_READ;
    while (true) {
        // cmd.data_val = 0;
        // queue_add_blocking(&call_queue, &cmd);
        // queue_remove_blocking(&results_queue, &call_result);
        // cmd.data_val = 1;
        // queue_add_blocking(&call_queue, &cmd);
        // queue_remove_blocking(&results_queue, &call_result);
        // cmd.data_val = 2;
        // queue_add_blocking(&call_queue, &cmd);
        // queue_remove_blocking(&results_queue, &call_result);
        // cmd.data_val = 3;

        //
        //
        //

        cmd.reg = REG_STATUS_1;
        queue_add_blocking(&call_queue, &cmd);
        queue_remove_blocking(&results_queue, &call_result);
        print_status1(call_result);

        cmd.reg = REG_STATUS_2;
        queue_add_blocking(&call_queue, &cmd);
        queue_remove_blocking(&results_queue, &call_result);
        print_status2(call_result);

        //
        //
        //

        // cmd.reg = 1;
        // queue_add_blocking(&call_queue, &cmd);
        // queue_remove_blocking(&results_queue, &call_result);

        // cmd.reg = 2;
        // queue_add_blocking(&call_queue, &cmd);
        // queue_remove_blocking(&results_queue, &call_result);

        // cmd.reg = 3;
        // queue_add_blocking(&call_queue, &cmd);
        // queue_remove_blocking(&results_queue, &call_result);

        // sleep_ms(500);

        //
        //
        //

        // cmd.reg = REG_STATUS_1;
        // queue_add_blocking(&call_queue, &cmd);
        // queue_remove_blocking(&results_queue, &call_result);
        // printf("Read value: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(call_result));
        // printf("\n");
        // print_status2(call_result);
        sleep_ms(500);
    }

    // // make GPIO_TMP follow clock (around 150ns lag)
    // gpio_init(GPIO_TMP);
    // gpio_set_dir(GPIO_TMP, GPIO_OUT);
    // while (true) {
    //     gpio_put(GPIO_TMP, gpio_get(21));
    // }

    // TODO:
    // * GP2-GP9 connected to A1-A8 of TXS0108E
    // * GP10 connected to OE (active HIGH) of TXS0108E
    // * OE needs 1k (brown-black-red) pull-down
    // * Let's try driving some LEDs!
}
