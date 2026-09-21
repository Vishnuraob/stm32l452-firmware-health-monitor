/*
 * memory_monitor.h
 *
 *  Created on: 16-Sept-2026
 *      Author: engineer
 */

#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

#include <stdint.h>


/* =========================================================
 * Status
 * ========================================================= */

typedef enum
{
    MEMORY_MONITOR_OK = 0,

    MEMORY_MONITOR_ERROR_PARAMETER,
    MEMORY_MONITOR_ERROR_NOT_INITIALIZED,
    MEMORY_MONITOR_ERROR_CONFIG,

    MEMORY_MONITOR_FAULT_STACK_OVERFLOW,
    MEMORY_MONITOR_FAULT_STACK_WARNING

} memory_monitor_status_t;


/* =========================================================
 * Memory Information
 * ========================================================= */

typedef struct
{
    uint32_t ram_start;
    uint32_t ram_end;
    uint32_t ram_size;

    uint32_t stack_top;
    uint32_t stack_pointer;

    uint32_t stack_reserved;
    uint32_t stack_available;

    uint32_t heap_start;
    uint32_t heap_reserved;

} memory_monitor_info_t;


/* =========================================================
 * Configuration
 * ========================================================= */

typedef struct
{
    /*
     * Minimum acceptable stack space in bytes.
     */
    uint32_t stack_warning_bytes;

} memory_monitor_config_t;


/* =========================================================
 * API
 * ========================================================= */

memory_monitor_status_t memory_monitor_init(
    const memory_monitor_config_t *config);

memory_monitor_status_t memory_monitor_update(void);

memory_monitor_status_t memory_monitor_health_check(void);

memory_monitor_status_t memory_monitor_get_info(
    memory_monitor_info_t *info);

uint8_t memory_monitor_stack_overflow(void);
uint32_t memory_monitor_get_msp(void);
uint32_t memory_monitor_get_stack_pointer(void);

uint32_t memory_monitor_get_stack_available(void);

uint32_t memory_monitor_get_ram_size(void);

uint32_t memory_monitor_get_ram_start(void);

uint32_t memory_monitor_get_ram_end(void);

uint8_t memory_monitor_fault_active(void);

void memory_monitor_clear_fault(void);

#endif /* MEMORY_MONITOR_H */
