/*
 * fault_manager.h
 *
 *  Created on: 17-Sept-2026
 *      Author: engineer
 */

#ifndef FAULT_MANAGER_H
#define FAULT_MANAGER_H

#include <stdint.h>

typedef enum
{
    FAULT_MANAGER_OK = 0,

    FAULT_MANAGER_ERROR_NOT_INITIALIZED,
    FAULT_MANAGER_ERROR_PARAMETER,
    FAULT_MANAGER_ERROR_INVALID_CODE

} fault_manager_status_t;


typedef enum
{
    FAULT_CODE_NONE = 0,

    FAULT_CODE_CLOCK,
    FAULT_CODE_GPIO,
    FAULT_CODE_SYSTICK,
    FAULT_CODE_UART,
    FAULT_CODE_ADC,
    FAULT_CODE_DMA,
    FAULT_CODE_WATCHDOG,

    FAULT_CODE_HEARTBEAT,
    FAULT_CODE_MEMORY,
    FAULT_CODE_RAM,
	FAULT_CODE_STACK_OVERFLOW,

    FAULT_CODE_COUNT

} fault_code_t;


typedef enum
{
    FAULT_SEVERITY_INFO = 0,
    FAULT_SEVERITY_WARNING,
    FAULT_SEVERITY_CRITICAL

} fault_severity_t;


typedef enum
{
    FAULT_SOURCE_NONE = 0,
    FAULT_SOURCE_PERIPHERAL,
    FAULT_SOURCE_HEARTBEAT,
    FAULT_SOURCE_MEMORY,
    FAULT_SOURCE_RAM,
    FAULT_SOURCE_WATCHDOG

} fault_source_t;


typedef struct
{
    fault_code_t code;
    fault_source_t source;
    fault_severity_t severity;

    uint32_t timestamp_ms;
    uint32_t occurrence_count;

    uint8_t active;

} fault_record_t;


fault_manager_status_t fault_manager_init(void);

fault_manager_status_t fault_manager_report(
    fault_code_t code,
    fault_source_t source,
    fault_severity_t severity);

fault_manager_status_t fault_manager_clear(
    fault_code_t code);

fault_manager_status_t fault_manager_get_record(
    fault_code_t code,
    fault_record_t *record);

uint32_t fault_manager_get_count(
    fault_code_t code);

uint8_t fault_manager_is_active(
    fault_code_t code);

uint8_t fault_manager_any_active(void);

fault_manager_status_t fault_manager_get_last_fault(
    fault_record_t *record);

fault_manager_status_t fault_manager_health_check(void);

#endif
