#include <stdio.h>	// printf
#include <stdlib.h>	// rand()

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#include "SJ_CenterPiece.pio.h"

#define LED_PIN PICO_DEFAULT_LED_PIN
#define BTN_PIN 8
#define LVL_SHIFT_OE_PIN 7

#define IS_RGBW false
#define NUM_PIXELS 12
#define WS2812_PIN 6

#define ENABLE_LVL_SHIFT 1

unsigned pattern_idx = 0;

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

void pattern_snakes(uint len, uint t) {
    for (uint i = 0; i < len; ++i) {
        uint x = (i + (t >> 1)) % 64;
        if (x < 10)
            put_pixel(urgb_u32(0xff, 0, 0));
        else if (x >= 15 && x < 25)
            put_pixel(urgb_u32(0, 0xff, 0));
        else if (x >= 30 && x < 40)
            put_pixel(urgb_u32(0, 0, 0xff));
        else
            put_pixel(0);
    }
}

void pattern_random(uint len, uint t) {
    if (t % 8)
        return;
    for (int i = 0; i < len; ++i)
        put_pixel(rand());
}

void pattern_sparkle(uint len, uint t) {
    if (t % 8)
        return;
    for (int i = 0; i < len; ++i)
        put_pixel(rand() % 16 ? 0 : 0xffffffff);
}

void pattern_greys(uint len, uint t) {
    int max = 100; // let's not draw too much current!
    t %= max;
    for (int i = 0; i < len; ++i) {
        put_pixel(t * 0x10101);
        if (++t >= max) t = 0;
    }
}

typedef void (*pattern)(uint len, uint t);
const pattern pattern_table[] = {
        pattern_snakes,
        pattern_random,
        pattern_sparkle,
        pattern_greys
};
const unsigned pattern_cnt = sizeof(pattern_table)/sizeof(pattern);

int main() {

//#ifndef PICO_DEFAULT_LED_PIN
//#warning blink example requires a board with a regular LED
//#else
	// Initialize USB StdIO
    stdio_init_all();

	// Config Pico On-Board LED as an output
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

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

	int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    int t = 0;
	while (true) {
        int dir = (rand() >> 30) & 1 ? 1 : -1;
        for (int i = 0; i < 1000; ++i) {
            pattern_table[pattern_idx](NUM_PIXELS, t);
            sleep_ms(10);
            t += dir;
        }

        printf("Hello, world!\n");
    }
//#endif
}

void gpio_btn_isr(uint gpio, uint32_t events) {
	// NOTE: events is bit-encoded
	
	// Level low -- Not activated -2023Nov15jhamermesh
	//if (events & 0x1) { }

	// Level high -- Not activated -2023Nov15jhamermesh
	//if (events & 0x2) { }

	// Falling edge
	if (events & 0x4) {
		gpio_put(LED_PIN, 0);

		events &= ~0x4;
	}

	// Rising edge
	if (events & 0x8) {
		gpio_put(LED_PIN, 1);
		pattern_idx = ((pattern_idx + 1) == pattern_cnt) ? 0 : pattern_idx + 1;

		events &= ~0x8;
	}
}

