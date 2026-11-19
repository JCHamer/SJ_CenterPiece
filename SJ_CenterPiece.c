#include <stdio.h>	// printf
#include <stdlib.h>	// rand()

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "math.h"

#include "SJ_CenterPiece.pio.h"

// Keyboard Inputs
#define COL1_PIN 14

#define ROW1_PIN 16
#define ROW2_PIN 17

#define ROW2COL 0
#define COL2ROW 1
#define DIODE_DIRECTION COL2ROW

// WS2812 Config
#define IS_RGBW false
#define NUM_PIXELS 3

#define TIMER_DLY_MS 20	// Pixels Update Prd
struct repeating_timer timer;

#define NUM_TIMER_HITS_NEW_DIR 500
unsigned NumTimerHits = 0;

// LED Pattern Settings
int PstnInBreathing = 0;
int CurrBreathingDir = 1;
const float Breathing_SmoothnessPts = 500;
const float Breathing_Gamma = 0.14;
const float Breathing_Beta = 0.5;

unsigned pattern_idx = 0;

#define MIN_IRQ_DEBOUNCE_DLY_MS 50
// uint32_t PrevBtnTime_ms;

void scan_matrix();
bool timer_isr(struct repeating_timer *t);

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint8_t get_breathe_scalar() {
	float pwm_val = 255.0 * (exp(-(pow(((PstnInBreathing/Breathing_SmoothnessPts)-Breathing_Beta)/Breathing_Gamma,2.0))/2.0));

	return (uint8_t)pwm_val;
}

void pattern_breathing(uint NumPixels) {
	uint32_t breathe_val = 0x10101 * get_breathe_scalar(PstnInBreathing);

    for (int i = 0; i < NumPixels; ++i) {
		// Pixel: as u32 _grb
        put_pixel(breathe_val);
    }
}

void pattern_breathing_color_r(uint NumPixels) {
	uint32_t breathe_val = 0x00100 * get_breathe_scalar(PstnInBreathing);

    for (int i = 0; i < NumPixels; ++i) {
		// Pixel: as u32 _grb
        put_pixel(breathe_val);
    }
}

void pattern_breathing_color_y(uint NumPixels) {
	uint32_t breathe_val = 0x10100 * get_breathe_scalar(PstnInBreathing);

    for (int i = 0; i < NumPixels; ++i) {
		// Pixel: as u32 _grb
        put_pixel(breathe_val);
    }
}

void pattern_breathing_color_g(uint NumPixels) {
	uint32_t breathe_val = 0x10000 * get_breathe_scalar(PstnInBreathing);

    for (int i = 0; i < NumPixels; ++i) {
		// Pixel: as u32 _grb
        put_pixel(breathe_val);
    }
}

void pattern_breathing_color_c(uint NumPixels) {
	uint32_t breathe_val = 0x10001 * get_breathe_scalar(PstnInBreathing);

    for (int i = 0; i < NumPixels; ++i) {
		// Pixel: as u32 _grb
        put_pixel(breathe_val);
    }
}

void pattern_breathing_color_b(uint NumPixels) {
	uint32_t breathe_val = 0x00001 * get_breathe_scalar(PstnInBreathing);

    for (int i = 0; i < NumPixels; ++i) {
		// Pixel: as u32 _grb
        put_pixel(breathe_val);
    }
}

void pattern_breathing_color_m(uint NumPixels) {
	uint32_t breathe_val = 0x00101 * get_breathe_scalar(PstnInBreathing);

    for (int i = 0; i < NumPixels; ++i) {
		// Pixel: as u32 _grb
        put_pixel(breathe_val);
    }
}

typedef void (*pattern)(uint NumPixels);
const pattern pattern_table[] = {
		pattern_breathing,
		pattern_breathing_color_r,
		pattern_breathing_color_y,
		pattern_breathing_color_g,
		pattern_breathing_color_c,
		pattern_breathing_color_b,
		pattern_breathing_color_m
};
const unsigned pattern_cnt = sizeof(pattern_table)/sizeof(pattern);

int main() {
	// Initialize USB StdIO
    stdio_init_all();
    printf("Startup...\n");
    //uart_default_tx_wait_blocking();

#if DIODE_DIRECTION == ROW2COL
	gpio_init(COL1_PIN);
	gpio_set_dir(COL1_PIN, GPIO_OUT);
	gpio_put(COL1_PIN, 1);

	gpio_init(ROW1_PIN);
	gpio_init(ROW2_PIN);
	gpio_set_dir(ROW1_PIN, GPIO_IN);
	gpio_set_dir(ROW2_PIN, GPIO_IN);
	gpio_pull_up(ROW1_PIN);
	gpio_pull_up(ROW2_PIN);
#elif DIODE_DIRECTION == COL2ROW
	gpio_init(COL1_PIN);
	gpio_set_dir(COL1_PIN, GPIO_IN);
	gpio_pull_up(COL1_PIN);

	gpio_init(ROW1_PIN);
	gpio_init(ROW2_PIN);
	gpio_set_dir(ROW1_PIN, GPIO_OUT);
	gpio_set_dir(ROW2_PIN, GPIO_OUT);
	gpio_put(ROW1_PIN, 1);
	gpio_put(ROW2_PIN, 1);
#endif

	// Connect button handler to receive button presses and advance the light pattern
	// PrevBtnTime_ms = to_ms_since_boot(get_absolute_time());
    // gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_btn_isr);

    // get free sm
    PIO pio = pio0;

	int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, PICO_DEFAULT_WS2812_PIN, 800000, IS_RGBW);

	add_repeating_timer_ms(TIMER_DLY_MS, &timer_isr, NULL, &timer);

	while (true) {
		sleep_ms(248);
		scan_matrix();
    }
}

void scan_matrix() {
	// // NOTE: Math with CurrTime in ms stored in u32 could have troubles at overflow,
	// // ~50 days of ms ticks should fit into u32, no worries on battery powerd system running for a few hours. -2023Nov23jhamermesh
	// uint32_t CurrTime_ms = to_ms_since_boot(get_absolute_time());
	// int32_t DeltaISR_ms = CurrTime_ms - PrevBtnTime_ms;
	//printf("Button ISR: events[%X], PrevTime[%u], CurrTime[%u]\n", events, PrevBtnTime_ms, CurrTime_ms);
	
	// if (DeltaISR_ms < MIN_IRQ_DEBOUNCE_DLY_MS) {
	// 	return;
	// }
	// PrevBtnTime_ms = CurrTime_ms;

#if DIODE_DIRECTION == ROW2COL
	gpio_put(COL1_PIN, 0);
	sleep_ms(2);
	if (gpio_get(!ROW1_PIN)) {
		pattern_idx = ((pattern_idx + 1) == pattern_cnt) ? 0 : pattern_idx + 1;
	}
	if (gpio_get(!ROW2_PIN)) {
		pattern_idx = (pattern_idx == 0) ? pattern_cnt : pattern_idx - 1;
	}
	gpio_put(COL1_PIN, 1);
#elif DIODE_DIRECTION == COL2ROW
	gpio_put(ROW1_PIN, 0);
	sleep_ms(1);
	if (!gpio_get(COL1_PIN)) {
		pattern_idx = ((pattern_idx + 1) == pattern_cnt) ? 0 : pattern_idx + 1;
	}
	gpio_put(ROW1_PIN, 1);
	gpio_put(ROW2_PIN, 0);
	sleep_ms(1);
	if (!gpio_get(COL1_PIN)) {
		pattern_idx = (pattern_idx == 0) ? pattern_cnt : pattern_idx - 1;
	}
	gpio_put(ROW2_PIN, 1);
#endif
}

bool timer_isr(struct repeating_timer *t) {
	// TODO: #ifdef DEBUG? -2023Nov22jhamermesh
	//printf("Hello, world from a timer ISR!\n");

	if (NumTimerHits == NUM_TIMER_HITS_NEW_DIR) {
		CurrBreathingDir = (CurrBreathingDir == 1) ? -1 : 1;	// Always toggle
		NumTimerHits = 0;
	}
	else {
		NumTimerHits++;
	}

	pattern_table[pattern_idx](NUM_PIXELS);

	PstnInBreathing += CurrBreathingDir;	// Incr or Decr PstnInBreathing by 1

	return true;
}

