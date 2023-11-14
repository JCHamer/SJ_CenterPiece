#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

#define LED_PIN PICO_DEFAULT_LED_PIN
#define BTN_PIN 8

int main() {

#ifndef PICO_DEFAULT_LED_PIN
#warning blink example requires a board with a regular LED
#else

	bi_decl(bi_program_description("Test project for Shannon & Jonathan Wedding centerpieces."));
	bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    // We are using the button to pull down to 0v when pressed, so ensure that when
    // unpressed, it uses internal pull ups. Otherwise when unpressed, the input will
    // be floating.
    gpio_pull_down(BTN_PIN);

    while (true) {
		unsigned dly = 250;
        if (!gpio_get(BTN_PIN)) {
			dly = 100;
		}

		gpio_put(LED_PIN, 1);
        sleep_ms(dly);
		gpio_put(LED_PIN, 0);
        sleep_ms(dly);
    }
#endif
}
