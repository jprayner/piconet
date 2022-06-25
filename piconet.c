#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"


int main() {
    const uint LED_PIN = 25;
    const uint PIN_CLK_OUT = 21;
    const uint PIN_TMP = 18; // used to test sampling clock out pin

    stdio_init_all();
    sleep_ms(5000); // give client a chance to reconnect
    printf("Hello, world!\n");

    // use USB clock to get 1 MHz for ADLC clock
    clock_gpio_init(21, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_USB, 48);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    // make PIN_TMP follow clock (around 150ns lag)
    gpio_init(PIN_TMP);
    gpio_set_dir(PIN_TMP, GPIO_OUT);
    while (true) {
        gpio_put(PIN_TMP, gpio_get(21));
    }

    // TODO:
    // * GP2-GP9 connected to A1-A8 of TXS0108E
    // * GP10 connected to OE (active HIGH) of TXS0108E
    // * OE needs 1k (brown-black-red) pull-down
    // * Let's try driving some LEDs!
}
