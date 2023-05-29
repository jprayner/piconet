#include "adlc.h"

#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pinctl.pio.h"
#include "util.h"

const uint GPIO_CLK_OUT = 21;
const uint GPIO_TMP = 15; // used to test sampling clock out pin

const uint GPIO_DATA_0 = 9;
const uint GPIO_DATA_1 = 8;
const uint GPIO_DATA_2 = 7;
const uint GPIO_DATA_3 = 6;
const uint GPIO_DATA_4 = 5;
const uint GPIO_DATA_5 = 4;
const uint GPIO_DATA_6 = 3;
const uint GPIO_DATA_7 = 2;

const uint GPIO_BUFF_A0 = 11;
const uint GPIO_BUFF_A1 = 12;
const uint GPIO_BUFF_CS = 13;
const uint GPIO_BUFF_RnW = 14;
const uint GPIO_BUFF_nRST = 22;

const uint CMD_READ = 0x000;
const uint CMD_WRITE = 0x100;

static PIO pio;
static uint sm;

static unsigned char lookup[16] = {
    0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
    0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf
};

static uint8_t reverse(uint8_t n) {
   return (lookup[n&0b1111] << 4) | lookup[n>>4];
}

uint adlc_read(uint reg) {
    gpio_put(GPIO_BUFF_A0, reg & 0x01);
    gpio_put(GPIO_BUFF_A1, reg & 0x02);

    pio_sm_put_blocking(pio, sm, CMD_READ);
    uint result = pio_sm_get_blocking(pio, sm);

    // note that pin bit order is reversed for board layout reasons
    return reverse(result);
}

void adlc_write(uint reg, uint data_val) {
    gpio_put(GPIO_BUFF_A0, reg & 0x01);
    gpio_put(GPIO_BUFF_A1, reg & 0x02);

    // note that pin bit order is reversed for board layout reasons
    pio_sm_put_blocking(pio, sm, CMD_WRITE | reverse(data_val));
    pio_sm_get_blocking(pio, sm);
}

void adlc_write_cr1(uint data_val) {
    adlc_write(0, data_val);
}

void adlc_write_cr2(uint data_val) {
    adlc_write(0, 0b00000000); // Select CR2
    adlc_write(1, data_val);
}

void adlc_write_cr3(uint data_val) {
    adlc_write(0, 0b00000001); // Select CR3/4
    adlc_write(1, data_val);
}

void adlc_write_cr4(uint data_val) {
    adlc_write(0, 0b00000001); // Select CR3/4
    adlc_write(3, data_val);
}

void adlc_write_fifo(uint data_val) {
    adlc_write(REG_FIFO, data_val);
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
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_init(GPIO_BUFF_A0);
    gpio_set_dir(GPIO_BUFF_A0, GPIO_OUT);
    gpio_init(GPIO_BUFF_A1);
    gpio_set_dir(GPIO_BUFF_A1, GPIO_OUT);
    gpio_init(GPIO_BUFF_nRST);
    gpio_set_dir(GPIO_BUFF_nRST, GPIO_OUT);

    gpio_init(GPIO_TMP);
    gpio_set_dir(GPIO_TMP, GPIO_OUT);

    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    adlc_reset();

    // todo get free sm
    pio = pio0;
    sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &pinctl_program);

    // state machine frequency of 32x 2MHz = 64MHz == 15.625ns period
	// => can sample upto 16 points in each of low and high clock states
    pinctl_program_init(pio, sm, offset, GPIO_DATA_7, GPIO_BUFF_CS, 64000000);

    // Init Control Register 1 (CR1)
    adlc_write_cr1(CR1_TX_RESET | CR1_RX_RESET);
    adlc_write_cr3(0);
    adlc_write_cr4(CR4_TX_WORD_LEN_1 | CR4_TX_WORD_LEN_2 | CR4_RX_WORD_LEN_1 | CR4_RX_WORD_LEN_2);
}

void adlc_irq_reset(void) {
  adlc_write_cr1(CR1_RIE);
  adlc_write(1, CR2_CLEAR_TX_STATUS | CR2_CLEAR_RX_STATUS | CR2_PRIO_STATUS_ENABLE);
}

void adlc_flag_fill(void) {
  adlc_write(REG_CONTROL_2, 0b11100100); // Set CR2 to RTS, TX Status Clear, RX Status clear, Flag fill on idle)
}

void adlc_update_data_led(bool is_on) {
    gpio_put(PICO_DEFAULT_LED_PIN, is_on ? 1 : 0);
}
