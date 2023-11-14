#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

#define LED_PIN PICO_DEFAULT_LED_PIN
#define BTN_PIN 8
#define LVL_SHIFT_OE_PIN 7
#define NEOPIXEL_PIN 6

#define ENABLE_LVL_SHIFT 0

int main() {

#ifndef PICO_DEFAULT_LED_PIN
#warning blink example requires a board with a regular LED
#else

	bi_decl(bi_program_description("Test project for Shannon & Jonathan Wedding centerpieces."));
	bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

	// Config Pico On-Board LED as an output
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

	// Config Pin to command NeoPixel channel as an output
    gpio_init(NEOPIXEL_PIN);
    gpio_set_dir(NEOPIXEL_PIN, GPIO_OUT);

	// Config CenterPiece On-Board Button as a pull-down button input looking for 3.3V
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_down(BTN_PIN);

	// Config Level Shifter Output Enable pin
    gpio_init(LVL_SHIFT_OE_PIN);
    gpio_set_dir(LVL_SHIFT_OE_PIN, GPIO_OUT);
	sleep_ms(1);
	gpio_put(LVL_SHIFT_OE_PIN, ENABLE_LVL_SHIFT);

    while (true) {
		unsigned dly = 250;
        if (!gpio_get(BTN_PIN)) {
			dly = 100;
		}

		gpio_put(LED_PIN, 1);
		gpio_put(NEOPIXEL_PIN, 1);
        sleep_ms(dly);
		gpio_put(LED_PIN, 0);
		gpio_put(NEOPIXEL_PIN, 0);
        sleep_ms(dly);
    }
#endif
}
