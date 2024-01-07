#include <stdio.h>	// printf
#include <stdlib.h>	// rand()

#include "pico/sleep.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
#include "math.h"

#include "SJ_CenterPiece.pio.h"

#define LED_PIN PICO_DEFAULT_LED_PIN
#define BTN_PIN 8
#define LVL_SHIFT_OE_PIN 7

#define IS_RGBW false
#define NUM_PIXELS 8
#define WS2812_PIN 6

#define ENABLE_LVL_SHIFT 1

#define TIMER_DLY_MS 20	// Pixels Update Prd
struct repeating_timer timer;

#define NUM_TIMER_HITS_NEW_DIR 500
unsigned NumTimerHits = 0;

// LED Pattern Settings
#define NUM_PIXELS_IN_RING 3	// Number of pixels making up the loop, the rest are a static element
int PstnInCurrPattern = 0;
int CurrPatternDir = 1;

int PstnInBreathing = 0;
int CurrBreathingDir = 1;
const float Breathing_SmoothnessPts = 500;
const float Breathing_Gamma = 0.14;
const float Breathing_Beta = 0.5;

unsigned pattern_idx = 0;

#define MIN_IRQ_DEBOUNCE_DLY_MS 500
uint32_t PrevBtnTime_ms;

void gpio_btn_isr(uint gpio, uint32_t events);
bool timer_isr(struct repeating_timer *t);

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);
}

static inline uint8_t get_breathe_scalar() {
	float pwm_val = 255.0 * (exp(-(pow(((PstnInBreathing/Breathing_SmoothnessPts)-Breathing_Beta)/Breathing_Gamma,2.0))/2.0));

	return (uint8_t)pwm_val;
}

void pattern_breathing(uint NumPixels, uint) {
	uint32_t breathe_val = 0x10101 * get_breathe_scalar(PstnInBreathing);

    for (int i = 0; i < NumPixels; ++i) {
		// Pixel: as u32 _grb
        put_pixel(breathe_val);
    }
}

void pattern_snakes(uint NumPixels, uint PstnInCurrPattern) {
	uint32_t breathe_scalar = get_breathe_scalar(PstnInBreathing);

    for (uint i = 0; i < NumPixels; ++i) {
		if (i >= NUM_PIXELS_IN_RING) {
			put_pixel(breathe_scalar * 0x10101);
			continue;
		}

		uint x = (i + (PstnInCurrPattern >> 2)) % 64;
		if (x < 15)
			put_pixel(urgb_u32(0xff, 0, 0));
		else if (x >= 20 && x < 35)
			put_pixel(urgb_u32(0, 0xff, 0));
		else if (x >= 40 && x < 55)
			put_pixel(urgb_u32(0, 0, 0xff));
		else
			put_pixel(0);
    }
}

void pattern_breathing_color_r(uint NumPixels, uint) {
	uint32_t breathe_val = 0x00100 * get_breathe_scalar(PstnInBreathing);

    for (int i = 0; i < NumPixels; ++i) {
		// Pixel: as u32 _grb
        put_pixel(breathe_val);
    }
}

void pattern_breathing_color_y(uint NumPixels, uint) {
	uint32_t breathe_val = 0x10100 * get_breathe_scalar(PstnInBreathing);

    for (int i = 0; i < NumPixels; ++i) {
		// Pixel: as u32 _grb
        put_pixel(breathe_val);
    }
}

void pattern_breathing_color_g(uint NumPixels, uint) {
	uint32_t breathe_val = 0x10000 * get_breathe_scalar(PstnInBreathing);

    for (int i = 0; i < NumPixels; ++i) {
		// Pixel: as u32 _grb
        put_pixel(breathe_val);
    }
}

void pattern_breathing_color_c(uint NumPixels, uint) {
	uint32_t breathe_val = 0x10001 * get_breathe_scalar(PstnInBreathing);

    for (int i = 0; i < NumPixels; ++i) {
		// Pixel: as u32 _grb
        put_pixel(breathe_val);
    }
}

void pattern_breathing_color_b(uint NumPixels, uint) {
	uint32_t breathe_val = 0x00001 * get_breathe_scalar(PstnInBreathing);

    for (int i = 0; i < NumPixels; ++i) {
		// Pixel: as u32 _grb
        put_pixel(breathe_val);
    }
}

void pattern_breathing_color_m(uint NumPixels, uint) {
	uint32_t breathe_val = 0x00101 * get_breathe_scalar(PstnInBreathing);

    for (int i = 0; i < NumPixels; ++i) {
		// Pixel: as u32 _grb
        put_pixel(breathe_val);
    }
}

typedef void (*pattern)(uint NumPixels, uint PstnInCurrPattern);
const pattern pattern_table[] = {
		pattern_breathing,
        pattern_snakes,
		pattern_breathing_color_r,
		pattern_breathing_color_y,
		pattern_breathing_color_g,
		pattern_breathing_color_c,
		pattern_breathing_color_b,
		pattern_breathing_color_m
};
const unsigned pattern_cnt = sizeof(pattern_table)/sizeof(pattern);

void recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig){

    //Re-enable ring Oscillator control
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_LSB);

    //reset procs back to default
    scb_hw->scr = scb_orig;
    clocks_hw->sleep_en0 = clock0_orig;
    clocks_hw->sleep_en1 = clock1_orig;

    //reset clocks
    clocks_init();
    stdio_init_all();

    return;
}

int main() {

//#ifndef PICO_DEFAULT_LED_PIN
//#warning blink example requires a board with a regular LED
//#else
	// Initialize USB StdIO
    stdio_init_all();
    printf("Startup...\n");
    //uart_default_tx_wait_blocking();

	// Config Pico On-Board LED as an output
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

	// Config CenterPiece On-Board Button as a pull-down button input looking for 3.3V
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_down(BTN_PIN);

	// Blink to indicate booted
	gpio_put(LED_PIN, 1);
	sleep_ms(1000);
	gpio_put(LED_PIN, 0);

    //Save clocks for restoring from sleep later
    uint scb_orig = scb_hw->scr;
    uint clock0_orig = clocks_hw->sleep_en0;
    uint clock1_orig = clocks_hw->sleep_en1;

	// Go to sleep
    printf("Going to sleep...\n");
    //uart_default_tx_wait_blocking();

	sleep_run_from_xosc();
	sleep_goto_dormant_until_pin(BTN_PIN, true, true);	// gpio_pin, bool edge, bool high
	
	// Recover from sleep
	recover_from_sleep(scb_orig, clock0_orig, clock1_orig);

    printf("Recovered...\n");
    //uart_default_tx_wait_blocking();

	// Config Level Shifter Output Enable pin
    gpio_init(LVL_SHIFT_OE_PIN);
    gpio_set_dir(LVL_SHIFT_OE_PIN, GPIO_OUT);
	sleep_ms(1);
	gpio_put(LVL_SHIFT_OE_PIN, ENABLE_LVL_SHIFT);

	// Connect button handler to receive button presses and advance the light pattern
	PrevBtnTime_ms = to_ms_since_boot(get_absolute_time());
    gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_btn_isr);

    // get free sm
    PIO pio = pio0;

	int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

	add_repeating_timer_ms(TIMER_DLY_MS, &timer_isr, NULL, &timer);

	while (true) {
		sleep_ms(100);
    }
//#endif
}

void gpio_btn_isr(uint gpio, uint32_t events) {
	// NOTE: Math with CurrTime in ms stored in u32 could have troubles at overflow,
	// ~50 days of ms ticks should fit into u32, no worries on battery powerd system running for a few hours. -2023Nov23jhamermesh
	uint32_t CurrTime_ms = to_ms_since_boot(get_absolute_time());
	int32_t DeltaISR_ms = CurrTime_ms - PrevBtnTime_ms;
	//printf("Button ISR: events[%X], PrevTime[%u], CurrTime[%u]\n", events, PrevBtnTime_ms, CurrTime_ms);
	
	if (DeltaISR_ms < MIN_IRQ_DEBOUNCE_DLY_MS) {
		return;
	}
	PrevBtnTime_ms = CurrTime_ms;

	// Rising edge
	if (events & 0x8) {
		printf("Button ISR: Button [Press]\n");

		//gpio_put(LED_PIN, 1);
		pattern_idx = ((pattern_idx + 1) == pattern_cnt) ? 0 : pattern_idx + 1;

		events &= ~0x8;
	}
}

bool timer_isr(struct repeating_timer *t) {
	// TODO: #ifdef DEBUG? -2023Nov22jhamermesh
	//printf("Hello, world from a timer ISR!\n");

	if (NumTimerHits == NUM_TIMER_HITS_NEW_DIR) {
		int PrevPatternDir = CurrPatternDir;

		CurrPatternDir = (rand() >> 30) & 1 ? 1 : -1;			// Sometimes toggle
		CurrBreathingDir = (CurrBreathingDir == 1) ? -1 : 1;	// Always toggle

		printf("Updated CurrPatternDir from [%d] to [%d]\n", PrevPatternDir, CurrPatternDir);

		NumTimerHits = 0;
	}
	else {
		NumTimerHits++;
	}

	pattern_table[pattern_idx](NUM_PIXELS, PstnInCurrPattern);

	PstnInCurrPattern += CurrPatternDir;	// Incr or Decr PstnInCurrPattern by 1
	PstnInBreathing += CurrBreathingDir;	// Incr or Decr PstnInBreathing by 1

	return true;
}

