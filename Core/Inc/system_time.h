/*
 * system_time.h
 *
 *  Created on: 15-Sept-2026
 *      Author: engineer
 */

#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

#include <stdint.h>

typedef enum
{
    SYSTEM_TIME_OK = 0,
    SYSTEM_TIME_ERROR_CLOCK,
    SYSTEM_TIME_ERROR_CONFIG
} system_time_status_t;

system_time_status_t system_time_init(void);

uint32_t system_time_get_ms(void);

void system_time_delay_ms(uint32_t delay_ms);
void system_time_tick(void);

system_time_status_t system_time_health_check(void);

#endif /* SYSTEM_TIME_H */
