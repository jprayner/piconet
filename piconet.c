#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"

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

data_dir_type data_dir = UNSPECIFIED;


static void _set_data_dir(data_dir_type new_data_dir) {
    if (new_data_dir == data_dir) {
        return;
    }

    switch (new_data_dir) {
        case (UNSPECIFIED):
            printf("Cannot set data direction to UNSPECIFIED\n");
            break;
        case (INPUT):
            printf("Setting data dir to INPUT\n");
            gpio_put(GPIO_BUFF_RnW, 1);
            gpio_set_dir_in_masked(0xff << 2);
            data_dir = INPUT;
            break;
        case (OUTPUT):
            printf("Setting data dir to OUTPUT\n");
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

    uint result = (gpio_get_all() && (0xff << 2)) >> 2;

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

    while (true) {
        uint volatile result = adlc_read(0);
        //printf("Read value: %u\n", result);
        //sleep_ms(500);
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
