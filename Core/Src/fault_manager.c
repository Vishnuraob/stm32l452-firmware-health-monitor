/*
 * fault_manager.c
 *
 *  Created on: 17-Sept-2026
 *      Author: engineer
 */

#include "fault_manager.h"
#include "system_time.h"
#include "fault_logger.h"

static volatile uint8_t fault_manager_initialized = 0U;

static fault_record_t fault_records[FAULT_CODE_COUNT];

static fault_record_t last_fault;

static volatile uint8_t any_fault_active = 0U;


/* ------------------------------------------------------------
 * Initialize
 * ------------------------------------------------------------ */

fault_manager_status_t fault_manager_init(void)
{
    uint32_t i;

    for (i = 0U; i < FAULT_CODE_COUNT; i++)
    {
        fault_records[i].code = FAULT_CODE_NONE;
        fault_records[i].source = FAULT_SOURCE_NONE;
        fault_records[i].severity = FAULT_SEVERITY_INFO;
        fault_records[i].timestamp_ms = 0U;
        fault_records[i].occurrence_count = 0U;
        fault_records[i].active = 0U;
    }

    last_fault.code = FAULT_CODE_NONE;
    last_fault.source = FAULT_SOURCE_NONE;
    last_fault.severity = FAULT_SEVERITY_INFO;
    last_fault.timestamp_ms = 0U;
    last_fault.occurrence_count = 0U;
    last_fault.active = 0U;

    any_fault_active = 0U;

    fault_manager_initialized = 1U;

    return FAULT_MANAGER_OK;
}


/* ------------------------------------------------------------
 * Report fault
 * ------------------------------------------------------------ */
fault_manager_status_t fault_manager_report(
	    fault_code_t code,
	    fault_source_t source,
	    fault_severity_t severity
	)
	{
	    fault_record_t *record;
	    uint8_t new_occurrence;

	    if (fault_manager_initialized == 0U)
	    {
	        return FAULT_MANAGER_ERROR_NOT_INITIALIZED;
	    }

	    if ((code <= FAULT_CODE_NONE) ||
	        (code >= FAULT_CODE_COUNT))
	    {
	        return FAULT_MANAGER_ERROR_INVALID_CODE;
	    }

	    record = &fault_records[code];

	    if (record->active == 0U)
	    {
	        new_occurrence = 1U;
	        record->occurrence_count++;
	    }
	    else
	    {
	        new_occurrence = 0U;
	    }

	    record->code = code;
	    record->source = source;
	    record->severity = severity;
	    record->timestamp_ms = system_time_get_ms();
	    record->active = 1U;

	    last_fault = *record;

	    any_fault_active = 1U;

	    /*
	     * Log only when this is a NEW occurrence.
	     */
	    if (new_occurrence != 0U)
	    {
	        fault_logger_log(
	            code,
	            source,
	            severity,
	            record->timestamp_ms,
	            record->occurrence_count
	        );
	    }

	    return FAULT_MANAGER_OK;
	}
/* ------------------------------------------------------------
 * Clear fault
 * ------------------------------------------------------------ */

fault_manager_status_t fault_manager_clear(
    fault_code_t code)
{
    uint32_t i;

    if (fault_manager_initialized == 0U)
    {
        return FAULT_MANAGER_ERROR_NOT_INITIALIZED;
    }

    if ((code <= FAULT_CODE_NONE) ||
        (code >= FAULT_CODE_COUNT))
    {
        return FAULT_MANAGER_ERROR_INVALID_CODE;
    }

    fault_records[code].active = 0U;

    any_fault_active = 0U;

    for (i = 1U; i < FAULT_CODE_COUNT; i++)
    {
        if (fault_records[i].active != 0U)
        {
            any_fault_active = 1U;
            break;
        }
    }

    return FAULT_MANAGER_OK;
}


/* ------------------------------------------------------------
 * Get complete fault record
 * ------------------------------------------------------------ */

fault_manager_status_t fault_manager_get_record(
    fault_code_t code,
    fault_record_t *record)
{
    if (fault_manager_initialized == 0U)
    {
        return FAULT_MANAGER_ERROR_NOT_INITIALIZED;
    }

    if (record == 0)
    {
        return FAULT_MANAGER_ERROR_PARAMETER;
    }

    if ((code <= FAULT_CODE_NONE) ||
        (code >= FAULT_CODE_COUNT))
    {
        return FAULT_MANAGER_ERROR_INVALID_CODE;
    }

    *record = fault_records[code];

    return FAULT_MANAGER_OK;
}


/* ------------------------------------------------------------
 * Get occurrence count
 * ------------------------------------------------------------ */

uint32_t fault_manager_get_count(
    fault_code_t code)
{
    if ((fault_manager_initialized == 0U) ||
        (code <= FAULT_CODE_NONE) ||
        (code >= FAULT_CODE_COUNT))
    {
        return 0U;
    }

    return fault_records[code].occurrence_count;
}


/* ------------------------------------------------------------
 * Check active state
 * ------------------------------------------------------------ */

uint8_t fault_manager_is_active(
    fault_code_t code)
{
    if ((fault_manager_initialized == 0U) ||
        (code <= FAULT_CODE_NONE) ||
        (code >= FAULT_CODE_COUNT))
    {
        return 0U;
    }

    return fault_records[code].active;
}


/* ------------------------------------------------------------
 * Check if any fault is active
 * ------------------------------------------------------------ */

uint8_t fault_manager_any_active(void)
{
    return any_fault_active;
}


/* ------------------------------------------------------------
 * Get most recent fault
 * ------------------------------------------------------------ */

fault_manager_status_t fault_manager_get_last_fault(
    fault_record_t *record)
{
    if (fault_manager_initialized == 0U)
    {
        return FAULT_MANAGER_ERROR_NOT_INITIALIZED;
    }

    if (record == 0)
    {
        return FAULT_MANAGER_ERROR_PARAMETER;
    }

    *record = last_fault;

    return FAULT_MANAGER_OK;
}


/* ------------------------------------------------------------
 * Health check
 * ------------------------------------------------------------ */

fault_manager_status_t fault_manager_health_check(void)
{
    if (fault_manager_initialized == 0U)
    {
        return FAULT_MANAGER_ERROR_NOT_INITIALIZED;
    }

    return FAULT_MANAGER_OK;
}
