/*
 * led.h
 *
 *  Created on: 13-Jul-2026
 *      Author: engineer
 */

#ifndef INC_LED_H_
#define INC_LED_H_
#include <stdint.h>

typedef enum
{
    LED_OK = 0,
    LED_ERROR_CLOCK,
    LED_ERROR_GPIO
} led_status_t;

led_status_t led_init(void);

void led_on(void);
void led_off(void);
void led_toggle(void);

uint8_t led_get_state(void);

led_status_t led_health_check(void);
#endif /* INC_LED_H_ */
