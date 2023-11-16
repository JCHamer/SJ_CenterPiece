#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/binary_info.h"

#include "SJ_CenterPiece.pio.h"

#define LED_PIN PICO_DEFAULT_LED_PIN
#define BTN_PIN 8
#define LVL_SHIFT_OE_PIN 7
#define NEOPIXEL_PIN 6

#define ENABLE_LVL_SHIFT 0

unsigned freq = 3;

void gpio_btn_isr(uint gpio, uint32_t events);

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq);

int main() {

#ifndef PICO_DEFAULT_LED_PIN
#warning blink example requires a board with a regular LED
#else

	bi_decl(bi_program_description("Test project for Shannon & Jonathan Wedding centerpieces."));
	bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

	// Config Pico On-Board LED as an output
    //gpio_init(LED_PIN);
    //gpio_set_dir(LED_PIN, GPIO_OUT);

	// Config Pin to command NeoPixel channel as an output
    //gpio_init(NEOPIXEL_PIN);
    //gpio_set_dir(NEOPIXEL_PIN, GPIO_OUT);

	// Config CenterPiece On-Board Button as a pull-down button input looking for 3.3V
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_down(BTN_PIN);

	// Config Level Shifter Output Enable pin
    gpio_init(LVL_SHIFT_OE_PIN);
    gpio_set_dir(LVL_SHIFT_OE_PIN, GPIO_OUT);
	sleep_ms(1);
	gpio_put(LVL_SHIFT_OE_PIN, ENABLE_LVL_SHIFT);

    gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_btn_isr);

    // get free sm
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &SJ_CenterPiece_program);

	blink_pin_forever(pio, 0, offset, LED_PIN, 3);
	blink_pin_forever(pio, 1, offset, NEOPIXEL_PIN, 4);

	while (true) {

		// PIO counter program takes 3 more cycles in total than we pass as
		// input (wait for n + 1; mov; jmp)
		pio->txf[0] = (clock_get_hz(clk_sys) / (2 * freq)) - 3;
		pio->txf[1] = (clock_get_hz(clk_sys) / (2 * freq)) - 3;

		/*
		gpio_put(LED_PIN, 1);
		//gpio_put(NEOPIXEL_PIN, 1);
        sleep_ms(dly);
		gpio_put(LED_PIN, 0);
		//gpio_put(NEOPIXEL_PIN, 0);
        sleep_ms(dly);
		*/
    }
#endif
}

void gpio_btn_isr(uint gpio, uint32_t events) {
	// NOTE: events is bit-encoded
	
	// Level low -- Not activated -2023Nov15jhamermesh
	//if (events & 0x1) { }

	// Level high -- Not activated -2023Nov15jhamermesh
	//if (events & 0x2) { }

	// Falling edge
	if (events & 0x4) {
		freq = 1;
		events &= ~0x4;
	}

	// Rising edge
	if (events & 0x8) {
		freq = 5;
		events &= ~0x8;
	}
}

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    SJ_CenterPiece_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (clock_get_hz(clk_sys) / (2 * freq)) - 3;
}

