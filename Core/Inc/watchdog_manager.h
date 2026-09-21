/*
 * watchdog_manager.h
 *
 *  Created on: 16-Sept-2026
 *      Author: engineer
 */

#ifndef WATCHDOG_MANAGER_H
#define WATCHDOG_MANAGER_H

#include <stdint.h>

typedef enum
{
    WATCHDOG_OK = 0,

    WATCHDOG_ERROR_PARAMETER,
    WATCHDOG_ERROR_NOT_INITIALIZED,
    WATCHDOG_ERROR_CONFIG,
    WATCHDOG_ERROR_TIMEOUT,

    WATCHDOG_FAULT_HEARTBEAT

} watchdog_status_t;


/*
 * Initialize the Independent Watchdog.
 *
 * timeout_ms:
 *     Requested watchdog timeout in milliseconds.
 */
watchdog_status_t watchdog_manager_init(uint32_t timeout_ms);


/*
 * Refresh the watchdog.
 *
 * This should only be called when the system
 * has passed its health checks.
 */
watchdog_status_t watchdog_manager_refresh(void);


/*
 * Perform watchdog supervision.
 *
 * The watchdog is refreshed only when the
 * monitored system is healthy.
 */
watchdog_status_t watchdog_manager_update(void);


/*
 * Check watchdog configuration and status.
 */
watchdog_status_t watchdog_manager_health_check(void);


/*
 * Get configured timeout.
 */
uint32_t watchdog_manager_get_timeout_ms(void);


/*
 * Get whether watchdog has been initialized.
 */
uint8_t watchdog_manager_is_initialized(void);


/*
 * Get whether a watchdog supervision fault
 * has been detected.
 */
uint8_t watchdog_manager_fault_active(void);


/*
 * Clear watchdog manager fault.
 */
void watchdog_manager_clear_fault(void);

#endif /* WATCHDOG_MANAGER_H */
