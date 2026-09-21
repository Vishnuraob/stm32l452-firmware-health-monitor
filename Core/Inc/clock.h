/*
 * clock.h
 *
 *  Created on: 15-Sept-2026
 *      Author: engineer
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

typedef enum
{
    CLOCK_OK = 0,
    CLOCK_ERROR_HSI_TIMEOUT,
    CLOCK_ERROR_PLL_TIMEOUT,
    CLOCK_ERROR_PLL_CONFIG,
    CLOCK_ERROR_CLOCK_SWITCH,
    CLOCK_ERROR_FLASH_LATENCY,
    CLOCK_ERROR_FREQUENCY

} clock_status_t;


/*
 * Configure STM32L452 system clock to 80 MHz.
 */
clock_status_t clock_init(void);


/*
 * Verify the current clock configuration.
 */
clock_status_t clock_health_check(void);


/*
 * Return calculated SYSCLK frequency.
 */
uint32_t clock_get_frequency(void);


/*
 * Return last clock initialization status.
 */
clock_status_t clock_get_status(void);


#endif /* INC_CLOCK_H_ */
