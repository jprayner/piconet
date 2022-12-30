#include "adlc.h"

#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"

typedef enum { UNSPECIFIED, INPUT, OUTPUT } data_dir_type;

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

const uint GPIO_BUFF_RnW = 10;
const uint GPIO_BUFF_A0 = 11;
const uint GPIO_BUFF_A1 = 12;
const uint GPIO_BUFF_CS = 13;
const uint GPIO_BUFF_IRQ = 14;
const uint GPIO_BUFF_nRST = 22;

data_dir_type data_dir = UNSPECIFIED;

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


uint adlc_read(uint reg) {
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

    // wait for data to appear (max 150ns) — this delay found to achieve this through measurement
    asm volatile(
         "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
         "nop\n"
    );

    // gpio_put(GPIO_TMP, 1);

    uint result = (gpio_get_all() & (0xff << 2)) >> 2;
    
    // wait for high to low clock transition
    while (gpio_get(GPIO_CLK_OUT) == 1);

    // wait 8ns (CS hold time is 10ns but there will be safely >2ns to detect clock transition)
    asm volatile("nop");

    // release chip select
    gpio_put(GPIO_BUFF_CS, 1);

    // gpio_put(GPIO_TMP, 0);

    return result;
}

void adlc_write(uint reg, uint data_val) {
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
    gpio_init(GPIO_BUFF_RnW);
    gpio_set_dir(GPIO_BUFF_RnW, GPIO_OUT);
    gpio_init(GPIO_BUFF_A0);
    gpio_set_dir(GPIO_BUFF_A0, GPIO_OUT);
    gpio_init(GPIO_BUFF_A1);
    gpio_set_dir(GPIO_BUFF_A1, GPIO_OUT);
    gpio_init(GPIO_BUFF_CS);
    gpio_set_dir(GPIO_BUFF_CS, GPIO_OUT);
    gpio_init(GPIO_BUFF_nRST);
    gpio_set_dir(GPIO_BUFF_nRST, GPIO_OUT);

    gpio_init(GPIO_TMP);
    gpio_set_dir(GPIO_TMP, GPIO_OUT);

    // init GPIO inputs
    gpio_init(GPIO_BUFF_IRQ);
    gpio_set_dir(GPIO_BUFF_IRQ, GPIO_IN);
    gpio_init(GPIO_CLK_IN);
    gpio_set_dir(GPIO_CLK_IN, GPIO_IN);

    // init GPIO for input/outputs data bus
    gpio_init_mask(0xff << 2);

    gpio_put(LED_PIN, 1);

    adlc_reset();

    // Init Control Register 1 (CR1)
    adlc_write(0, 0b11000001);
//   digitalWriteDirect(PIN_D0,0); // No logical control byte
//   digitalWriteDirect(PIN_D1,0); // 8 bit control field
//   digitalWriteDirect(PIN_D2,0); // No auto address extend
//   digitalWriteDirect(PIN_D3,0); // Idle mode all ones
//   digitalWriteDirect(PIN_D4,0); // Disable Flag detect (not used in Econet)
//   digitalWriteDirect(PIN_D5,0); // Disable Loop mode (not used in Econet)
//   digitalWriteDirect(PIN_D6,0); // Disable active on poll (not used in Econet)
//   digitalWriteDirect(PIN_D7,0); // Disable Loop on-line (not used in Econet)
    adlc_write(2, 0b00000000);

  // init Control Register 4 (CR4)
//   digitalWriteDirect(PIN_D0,0); // Flag interframe control (not important in Econet) 
//   digitalWriteDirect(PIN_D1,1); // Set TX word length to 8 bit
//   digitalWriteDirect(PIN_D2,1); 
//   digitalWriteDirect(PIN_D3,1); // Set RX word length to 8 bit
//   digitalWriteDirect(PIN_D4,1); 
//   digitalWriteDirect(PIN_D5,0); // No transmit abort 
//   digitalWriteDirect(PIN_D6,0); // No Extended abort (not used in Econet)
//   digitalWriteDirect(PIN_D7,0); // Disable NRZI encoding (not used in Econet)
    adlc_write(3, 0b00011110);
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
