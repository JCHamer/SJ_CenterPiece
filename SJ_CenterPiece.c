#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/binary_info.h"

#include "SJ_CenterPiece.pio.h"

#define LED_PIN PICO_DEFAULT_LED_PIN
#define BTN_PIN 8
#define LVL_SHIFT_OE_PIN 7

#define IS_RGBW false
#define NUM_PIXELS 12
#define WS2812_PIN 6

#define ENABLE_LVL_SHIFT 0

unsigned dly = 250;
unsigned color_idx = 0;

const uint8_t colors[2][3] = {
	{0x1F,0x30,0x50},
	{0x30,0x50,0x50}
};
const uint max_color = sizeof(colors)/sizeof(colors[0]);

void gpio_btn_isr(uint gpio, uint32_t events);

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);
}

void put_ws2812(uint pstn, uint len, uint color) {
	uint color_;
	if (pstn > (len / 2)) {
		color_ = (color == max_color) ? 0 : (color + 1);
	}
	else {
		color_ = color;
	}

	put_pixel(urgb_u32(colors[color_][0], colors[color_][1], colors[color_][2]));
}

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

    // todo get free sm
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    while (true) {
		for (int i = 0; i < NUM_PIXELS; i++) {
			put_ws2812(i, NUM_PIXELS, color_idx);
		}

		gpio_put(LED_PIN, 1);
		//gpio_put(NEOPIXEL_PIN, 1);
        sleep_ms(dly);
		gpio_put(LED_PIN, 0);
		//gpio_put(NEOPIXEL_PIN, 0);
        sleep_ms(dly);
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
		dly = 100;
		color_idx = 0;
		events &= ~0x4;
	}

	// Rising edge
	if (events & 0x8) {
		dly = 500;
		color_idx = 1;
		events &= ~0x8;
	}
}

