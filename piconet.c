/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Output PWM signals on pins 0 and 1

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"

uint32_t pwm_set_freq_duty(uint slice_num, uint chan, uint32_t f, int d) {
    uint32_t clock = 125000000;
    uint32_t divider16 = clock/ f/ 4096 + (clock% (f * 4096) != 0);
    if (divider16/ 16 == 0)
        divider16 = 16;
    
    uint32_t wrap = clock * 16/ divider16/ f - 1;
    pwm_set_clkdiv_int_frac(slice_num, divider16/16, divider16 & 0xF);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan, wrap * d/ 100);
    return wrap;
}

int main() {
    const uint PIN_CLK_OUT = 16;
    const uint PIN_TMP = 18;

    // PWM @ 1MHz
    gpio_set_function(PIN_CLK_OUT, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PIN_CLK_OUT);
    uint channel_num = pwm_gpio_to_channel(PIN_CLK_OUT);
    pwm_set_freq_duty(slice_num, channel_num, 1000000, 50);
    pwm_set_enabled(slice_num, true);

    gpio_init(PIN_TMP);
    gpio_set_dir(PIN_TMP, GPIO_OUT);

    while (true) {
        gpio_put(PIN_TMP, gpio_get(PIN_CLK_OUT));
    }
}
