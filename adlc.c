#include "adlc.h"

#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pinctl.pio.h"

const uint LED_PIN = 25;

const uint GPIO_CLK_OUT = 21;
const uint GPIO_CLK_IN = 20;
const uint GPIO_TMP = 15; // used to test sampling clock out pin

const uint GPIO_DATA_0 = 2;
const uint GPIO_DATA_1 = 3;
const uint GPIO_DATA_2 = 4;
const uint GPIO_DATA_3 = 5;
const uint GPIO_DATA_4 = 6;
const uint GPIO_DATA_5 = 7;
const uint GPIO_DATA_6 = 8;
const uint GPIO_DATA_7 = 9;

// WARNING!!! IRQ and RnW have been swapped - don't forget to modify hardware!!!
//    IRQ used to be 14
//    RnW used to be 10

const uint GPIO_BUFF_IRQ = 10;
const uint GPIO_BUFF_A0 = 11;
const uint GPIO_BUFF_A1 = 12;
const uint GPIO_BUFF_CS = 13;
const uint GPIO_BUFF_RnW = 14;
const uint GPIO_BUFF_nRST = 22;

const uint CMD_READ = 0x000;
const uint CMD_WRITE = 0x100;

static PIO pio;
static uint sm;

uint adlc_read(uint reg) {
    gpio_put(GPIO_BUFF_A0, reg & 0x01);
    gpio_put(GPIO_BUFF_A1, reg & 0x02);

    pio_sm_put_blocking(pio, sm, CMD_READ);
    uint result = pio_sm_get_blocking(pio, sm);
    return result;
}

void adlc_write(uint reg, uint data_val) {
    gpio_put(GPIO_BUFF_A0, reg & 0x01);
    gpio_put(GPIO_BUFF_A1, reg & 0x02);

    pio_sm_put_blocking(pio, sm, CMD_WRITE | data_val);
    pio_sm_get_blocking(pio, sm);
}

void adlc_write_cr1(uint data_val) {
    adlc_write(0, data_val);
}

void adlc_write_cr2(uint data_val) {
    adlc_write(0, 0b11000000); // Select CR2
    adlc_write(1, data_val);
}

void adlc_write_cr3(uint data_val) {
    adlc_write(0, 0b11000001); // Select CR3/4
    adlc_write(1, data_val);
}

void adlc_write_cr4(uint data_val) {
    adlc_write(0, 0b11000001); // Select CR3/4
    adlc_write(3, data_val);
}

void adlc_reset(void) {
  gpio_put(GPIO_BUFF_nRST, 0);
  sleep_ms(100);
  gpio_put(GPIO_BUFF_nRST, 1);
  sleep_ms(100);
}

void adlc_init(void) {
    // use USB clock to get 2 MHz for ADLC clock
    clock_gpio_init(GPIO_CLK_OUT, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_USB, 24);

    // init GPIO outputs
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(GPIO_BUFF_A0);
    gpio_set_dir(GPIO_BUFF_A0, GPIO_OUT);
    gpio_init(GPIO_BUFF_A1);
    gpio_set_dir(GPIO_BUFF_A1, GPIO_OUT);
    gpio_init(GPIO_BUFF_nRST);
    gpio_set_dir(GPIO_BUFF_nRST, GPIO_OUT);

    gpio_init(GPIO_TMP);
    gpio_set_dir(GPIO_TMP, GPIO_OUT);

    // init GPIO inputs
    gpio_init(GPIO_BUFF_IRQ);
    gpio_set_dir(GPIO_BUFF_IRQ, GPIO_IN);
    gpio_init(GPIO_CLK_IN);
    gpio_set_dir(GPIO_CLK_IN, GPIO_IN);

    gpio_put(LED_PIN, 1);

    adlc_reset();

    // Reenable before testing with ADLC (and don't forget pin swap!)
/*
    // Init Control Register 1 (CR1)
    adlc_write_cr1(0b11000001);
    adlc_write_cr3(0b00000000);
    adlc_write_cr4(0b00011110);
*/

    // todo get free sm
    pio = pio0;
    sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &pinctl_program);

    // state machine frequency of 32x 2MHz = 64MHz == 15.625ns period
	// => can sample upto 16 points in low or high clock state
    pinctl_program_init(pio, sm, offset, GPIO_DATA_0, GPIO_BUFF_CS, 64000000);
}

void adlc_irq_reset(void) {
  adlc_write(0, 0b00000010); // Enable RX interrupts, select address 1
  adlc_write(1, 0b01100001); // Clear RX and TX status, prioritise status 
}

void adlc_flag_fill(void) {
  adlc_write(REG_CONTROL_2, 0b11100100); // Set CR2 to RTS, TX Status Clear, RX Status clear, Flag fill on idle)
}

bool adlc_get_irq(void) {
    return !gpio_get(GPIO_BUFF_IRQ); // active low so invert
}
