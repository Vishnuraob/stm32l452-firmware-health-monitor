/*
 * heartbeat_monitor.h
 *
 *  Created on: 16-Sept-2026
 *      Author: engineer
 */
#ifndef HEARTBEAT_MONITOR_H
#define HEARTBEAT_MONITOR_H

#include <stdint.h>


typedef enum
{
    HEARTBEAT_OK = 0,

    HEARTBEAT_ERROR_PARAMETER,
    HEARTBEAT_ERROR_NOT_INITIALIZED,

    HEARTBEAT_FAULT_TIMEOUT

} heartbeat_status_t;


/*
 * Initialize heartbeat monitor.
 *
 * timeout_ms:
 * Maximum allowed time between two application
 * heartbeats.
 */
heartbeat_status_t heartbeat_monitor_init(
    uint32_t timeout_ms);


/*
 * Called by the application whenever the main loop
 * successfully reaches its heartbeat point.
 */
void heartbeat_monitor_beat(void);


/*
 * Called from SysTick.
 *
 * This provides an independent timing reference.
 */
void heartbeat_monitor_tick(void);


/*
 * Check current heartbeat status.
 */
heartbeat_status_t heartbeat_monitor_health_check(void);


/*
 * Get current heartbeat counter.
 */
uint32_t heartbeat_monitor_get_count(void);


/*
 * Get time since last heartbeat.
 */
uint32_t heartbeat_monitor_get_elapsed_ms(void);


/*
 * Check whether a heartbeat fault has occurred.
 */
uint8_t heartbeat_monitor_fault_active(void);


/*
 * Clear heartbeat fault.
 *
 * Used later by recovery logic.
 */
void heartbeat_monitor_clear_fault(void);

#endif /* HEARTBEAT_MONITOR_H */
