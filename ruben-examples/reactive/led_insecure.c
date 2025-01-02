#include "led_insecure.h"
#include <stdio.h>
#include <msp430.h>

#include "log.h"

static const unsigned int LED_INDEX = 0x1 << 5;
static int led_initialized = 0;

void led_insecure_init(void) {
    /*
     * Our setup: LD0 permanently grounded, LD123 mapped to P1OUT[5:7]
     *
     * https://store.digilentinc.com/pmod-led-four-high-brightness-leds/
     * http://processors.wiki.ti.com/index.php/Digital_I/O_(MSP430)
     */
    P1SEL = 0x00;
    P1DIR = 0xff;
    P1OUT = 0x00;

    led_initialized = 1;
}

void led_insecure_toggle(int state) {
    LOG_DEBUG("toggling led");

    if (!led_initialized)
        led_insecure_init();

    if (state)
        P1OUT |= LED_INDEX;
    else
        P1OUT &= ~LED_INDEX;
}
