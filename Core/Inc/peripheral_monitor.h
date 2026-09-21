/*
 * peripheral_monitor.h
 *
 *  Created on: 17-Sept-2026
 *      Author: engineer
 */
#ifndef PERIPHERAL_MONITOR_H
#define PERIPHERAL_MONITOR_H

#include <stdint.h>

typedef enum
{
    PERIPHERAL_MONITOR_OK = 0,

    PERIPHERAL_MONITOR_ERROR_NOT_INITIALIZED,

    PERIPHERAL_MONITOR_FAULT,

    PERIPHERAL_MONITOR_FAULT_CLOCK,
    PERIPHERAL_MONITOR_FAULT_GPIO,
    PERIPHERAL_MONITOR_FAULT_SYSTICK,
    PERIPHERAL_MONITOR_FAULT_UART,
    PERIPHERAL_MONITOR_FAULT_ADC,
    PERIPHERAL_MONITOR_FAULT_DMA,
    PERIPHERAL_MONITOR_FAULT_WATCHDOG

} peripheral_monitor_status_t;

typedef struct
{
    peripheral_monitor_status_t clock_status;
    peripheral_monitor_status_t gpio_status;
    peripheral_monitor_status_t systick_status;
    peripheral_monitor_status_t uart_status;
    peripheral_monitor_status_t adc_status;
    peripheral_monitor_status_t dma_status;
    peripheral_monitor_status_t watchdog_status;

    uint8_t all_healthy;

} peripheral_monitor_info_t;


/*
 * Initialize the Peripheral Health Monitor.
 */
peripheral_monitor_status_t peripheral_monitor_init(void);


/*
 * Check all supported peripherals.
 */
peripheral_monitor_status_t peripheral_monitor_update(void);


/*
 * Return current overall health.
 */
peripheral_monitor_status_t peripheral_monitor_health_check(void);


/*
 * Get individual peripheral statuses.
 */
peripheral_monitor_status_t peripheral_monitor_get_info(
    peripheral_monitor_info_t *info);


/*
 * Return whether all peripherals are healthy.
 */
uint8_t peripheral_monitor_all_healthy(void);


/*
 * Clear active peripheral fault state.
 */
void peripheral_monitor_clear_fault(void);


/*
 * Return whether a peripheral fault is active.
 */
uint8_t peripheral_monitor_fault_active(void);

#endif /* PERIPHERAL_MONITOR_H */
