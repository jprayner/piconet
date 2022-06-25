#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"

int main() {
    const uint PIN_CLK_OUT = 21;
    const uint PIN_TMP = 18; // used to test sampling clock out pin

    // use USB clock to get 1 MHz for ADLC clock
    clock_gpio_init(21, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_USB, 48);

    gpio_init(PIN_TMP);
    gpio_set_dir(PIN_TMP, GPIO_OUT);

    // make PIN_TMP follow clock (around 150ns lag)
    while (true) {
        gpio_put(PIN_TMP, gpio_get(21));
    }
}
