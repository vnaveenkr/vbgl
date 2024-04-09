// D0 - gpi02  | 5V
// D1 - gpio3  | gnd
// D2 - gpio4  | 3v3
// D3 - gpio5  | D10 - gpio10
// D4 - gpio6  | D9 - gpio9
// D5 - gpio7  | D8 - gpio8
// D6 - gpio21 | D7 - gpio20
// gpio2,8,9 - strapping pins
// gpio12-17 - spi flash
// gpio18,19 - usb-jtag
//
// PCB wiring
// series res  ULN8203 I/O  GPIO
// 1           1            20
// 2           5            5
// 3           3            3
// 4           2            10
// 5           4            4
// 6           6            6
// 7           8            7
// 8           7            21
 
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_random.h"
#include "driver/gpio.h"

#define NUM_ARRAY_ELEMS(A) (sizeof(A)/sizeof(A[0]))
#define DELAY_MS(D) vTaskDelay((D) / portTICK_PERIOD_MS)
#define RAND_RANGE(N) (esp_random() % N)

extern uint8_t char_value[];
extern bool char_value_change;
void ble_init(void);

uint16_t *param_vals = (uint16_t *)char_value;

// In the order output series resistors are wired on the PCB
static const int gpio_pins[] = {20, 5, 3, 10, 4, 6, 7, 21};

static uint64_t gpio_mask;
static struct {
		bool on;
		int tleft;
} pinstate[NUM_ARRAY_ELEMS(gpio_pins)];

static int ipdfex_100[] = {
// for(i=32;i>0;i--)console.log(Math.round(Math.log(i/32)*-100) + ",")
	0, 3, 6, 10, 13, 17, 21, 25, 29, 33, 37, 42, 47, 52, 58, 63, 69, 76, 83,
	90, 98, 107, 116, 127, 139, 152, 167, 186, 208, 237, 277, 347
};

void all_io_off(void)
{
	for (int i = 0; i < NUM_ARRAY_ELEMS(gpio_pins); i++) {
		gpio_set_level(gpio_pins[i], 0);
	}
}

void vibstop()
{
	all_io_off();
	while(!char_value_change) {
		DELAY_MS(100);
	}
}

void vibtest(void)
{
	all_io_off();
	while(!char_value_change) {
		for (int i = 0; i < NUM_ARRAY_ELEMS(gpio_pins); i++) {
			DELAY_MS(100);
			printf("iopin: %d\n", gpio_pins[i]);
			gpio_set_level(gpio_pins[i], 1);
			DELAY_MS(1000);
			gpio_set_level(gpio_pins[i], 0);
		}
	}
}

void vibtest1(void)
{
	int iopin = 
		(param_vals[1] < NUM_ARRAY_ELEMS(gpio_pins)) ?
		param_vals[1] : 0;

	all_io_off();
	for (int i = 0; i < NUM_ARRAY_ELEMS(gpio_pins); i++) {
		gpio_set_level(gpio_pins[i], 0);
	}
	while(!char_value_change) {
		DELAY_MS(500);
		gpio_set_level(gpio_pins[iopin], 1);
		DELAY_MS(500);
		gpio_set_level(gpio_pins[iopin], 0);
	}
}

enum hand_opt {HAND_BOTH, HAND_LEFT, HAND_RIGHT};
void vibrand1m(enum hand_opt opt)
{
	int avg_on = param_vals[1];

	all_io_off();
	while(!char_value_change) {
		DELAY_MS(100);
		int i = (opt == HAND_BOTH) ? RAND_RANGE(NUM_ARRAY_ELEMS(gpio_pins)) :
			(opt == HAND_LEFT) ? RAND_RANGE(NUM_ARRAY_ELEMS(gpio_pins) / 2) :
			RAND_RANGE(NUM_ARRAY_ELEMS(gpio_pins) / 2 + 4); 
		// printf("iopin: %d\n", gpio_pins[i]);
		gpio_set_level(gpio_pins[i], 1);
		DELAY_MS(avg_on * 100);
		gpio_set_level(gpio_pins[i], 0);
	}
}

void vibrand1(void)
{
	vibrand1m(HAND_BOTH);
}

void vibrand1l(void)
{
	vibrand1m(HAND_LEFT);
}

void vibrand1r(void)
{
	vibrand1m(HAND_RIGHT);
}

void vibrand8(void)
{
	int avg_on = param_vals[1];
	int avg_off = param_vals[2];

	all_io_off();
	for (int i = 0; i < NUM_ARRAY_ELEMS(gpio_pins); i++) {
		pinstate[i].on = RAND_RANGE(2);
		pinstate[i].tleft = pinstate[i].on ? 
			ipdfex_100[RAND_RANGE(NUM_ARRAY_ELEMS(ipdfex_100))] * avg_on / 100 :
			ipdfex_100[RAND_RANGE(NUM_ARRAY_ELEMS(ipdfex_100))] * avg_off / 100;
	}

	while (!char_value_change) {
		for (int i = 0; i < NUM_ARRAY_ELEMS(gpio_pins); i++) {
			if (pinstate[i].tleft <= 0) {
				pinstate[i].on = !pinstate[i].on;
				// printf("iopin: %d %s\n",
				// 	gpio_pins[i], pinstate[i].on ? "on" : "off"); 
				pinstate[i].tleft = pinstate[i].on ? 
					ipdfex_100[RAND_RANGE(NUM_ARRAY_ELEMS(ipdfex_100))] * 
						avg_on / 100 :
					ipdfex_100[RAND_RANGE(NUM_ARRAY_ELEMS(ipdfex_100))] * 
						avg_off / 100;
				gpio_set_level(gpio_pins[i], pinstate[i].on);
			}
			--pinstate[i].tleft;
		}
		DELAY_MS(100);
	}
}

static void (*patterns[])(void) = {
	vibstop,
	vibtest,
	vibtest1,
	vibrand1,
	vibrand1l,
	vibrand1r,
	vibrand8
};

void app_main(void)
{
	gpio_config_t io_conf = {};
	void (*pat)(void);

	for (int i = 0; i < NUM_ARRAY_ELEMS(gpio_pins); i++)
		gpio_mask |= ((uint64_t)1 << gpio_pins[i]);

	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = gpio_mask;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);

	ble_init();
	while (1) {
		if (char_value_change) {
			char_value_change = 0;
			printf("char value change\n");
		}
		pat = (param_vals[0] < NUM_ARRAY_ELEMS(patterns)) ?
			patterns[param_vals[0]] : patterns[0];
		pat();
	}
}
