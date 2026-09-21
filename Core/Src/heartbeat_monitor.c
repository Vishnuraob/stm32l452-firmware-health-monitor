/*
 * heartbeat_monitor.c
 *
 *  Created on: 16-Sept-2026
 *      Author: engineer
 */
#include "heartbeat_monitor.h"


/*
 * Internal state
 */

static volatile uint32_t heartbeat_count = 0U;

static volatile uint32_t heartbeat_time_ms = 0U;

static volatile uint32_t monitor_time_ms = 0U;

static volatile uint32_t heartbeat_timeout_ms = 0U;

static volatile uint8_t heartbeat_initialized = 0U;

static volatile uint8_t heartbeat_fault = 0U;


/*
 * Initialize heartbeat monitor
 */

heartbeat_status_t heartbeat_monitor_init(
    uint32_t timeout_ms)
{
    if (timeout_ms == 0U)
    {
        return HEARTBEAT_ERROR_PARAMETER;
    }

    heartbeat_count = 0U;

    heartbeat_time_ms = 0U;

    monitor_time_ms = 0U;

    heartbeat_timeout_ms = timeout_ms;

    heartbeat_fault = 0U;

    heartbeat_initialized = 1U;

    return HEARTBEAT_OK;
}


/*
 * Application heartbeat
 *
 * This function is called from the main loop.
 */

void heartbeat_monitor_beat(void)
{
    if (heartbeat_initialized == 0U)
    {
        return;
    }

    heartbeat_count++;

    heartbeat_time_ms = monitor_time_ms;

    heartbeat_fault = 0U;
}


/*
 * Timing function
 *
 * Called every 1 ms from SysTick.
 */

void heartbeat_monitor_tick(void)
{
    uint32_t elapsed_time;

    if (heartbeat_initialized == 0U)
    {
        return;
    }

    monitor_time_ms++;


    /*
     * Calculate time since the last heartbeat.
     *
     * Unsigned subtraction naturally handles
     * uint32_t timer rollover.
     */

    elapsed_time =
        monitor_time_ms - heartbeat_time_ms;


    /*
     * Check heartbeat timeout.
     */

    if (elapsed_time > heartbeat_timeout_ms)
    {
        heartbeat_fault = 1U;
    }
}


/*
 * Health check
 */

heartbeat_status_t heartbeat_monitor_health_check(void)
{
    if (heartbeat_initialized == 0U)
    {
        return HEARTBEAT_ERROR_NOT_INITIALIZED;
    }

    if (heartbeat_fault != 0U)
    {
        return HEARTBEAT_FAULT_TIMEOUT;
    }

    return HEARTBEAT_OK;
}


/*
 * Get heartbeat counter
 */

uint32_t heartbeat_monitor_get_count(void)
{
    return heartbeat_count;
}


/*
 * Get elapsed time since heartbeat
 */

uint32_t heartbeat_monitor_get_elapsed_ms(void)
{
    return monitor_time_ms - heartbeat_time_ms;
}


/*
 * Get fault state
 */

uint8_t heartbeat_monitor_fault_active(void)
{
    return heartbeat_fault;
}


/*
 * Clear fault
 */

void heartbeat_monitor_clear_fault(void)
{
    heartbeat_fault = 0U;

    heartbeat_time_ms = monitor_time_ms;
}

