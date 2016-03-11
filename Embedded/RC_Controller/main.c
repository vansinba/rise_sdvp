/*
	Copyright 2016 Benjamin Vedder	benjamin@vedder.se

	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ch.h"
#include "hal.h"
#include "stm32f4xx_conf.h"

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "conf_general.h"
#include "comm_cc2520.h"
#include "comm_usb.h"
#include "commands.h"
#include "utils.h"
#include "mpu9150.h"
#include "basic_rf.h"
#include "ext_cb.h"
#include "adconv.h"
#include "led.h"
#include "packet.h"

/*
 * TODO: Update this...
 *
 * Timers used:
 * TIM13: utils
 * TIM6: Control
 * TIM3: PPM
 * TIM5: Global clock
 *
 * DMA/Stream	Device		Usage
 * 2, 4			ADC1		Battery voltage measurement
 * 1, 0			I2C1		MPU9150
 * 1, 6			I2C1		MPU9150
 * 1, 3			SPI2		CC2520
 * 1, 4			SPI2		CC2520
 * 1, 7			TIM4		WS2811 LEDs
 *
 */

#include "hal_cc2520.h"

int main(void) {
	halInit();
	chSysInit();

	conf_general_init();
	led_init();
	adconv_init();
	comm_usb_init();
	ext_cb_init();
	mpu9150_init();
	comm_cc2520_init();
	commands_set_send_func(comm_cc2520_send_buffer);

	chThdSleepMilliseconds(1000);
	led_write(LED_RED, 1);
	mpu9150_sample_gyro_offsets(100);
	led_write(LED_RED, 0);

	for(;;) {
		commands_printf("Hello World");
		chThdSleepMilliseconds(500);
	}

	for(;;) {
		chThdSleepMilliseconds(2);
		packet_timerfunc();
	}
}