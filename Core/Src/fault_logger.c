/*
 * fault_logger.c
 *
 *  Created on: 17-Sept-2026
 *      Author: engineer
 */

#include "fault_logger.h"

static uint8_t fault_logger_initialized = 0U;

static fault_log_entry_t
fault_log_buffer[FAULT_LOGGER_MAX_ENTRIES];

static uint32_t fault_log_count = 0U;
static uint32_t fault_log_write_index = 0U;
static uint32_t fault_log_sequence = 0U;


fault_logger_status_t fault_logger_init(void)
{
    uint32_t i;

    for (i = 0U; i < FAULT_LOGGER_MAX_ENTRIES; i++)
    {
        fault_log_buffer[i].sequence = 0U;
        fault_log_buffer[i].code = FAULT_CODE_NONE;
        fault_log_buffer[i].source = FAULT_SOURCE_NONE;
        fault_log_buffer[i].severity = FAULT_SEVERITY_INFO;
        fault_log_buffer[i].timestamp_ms = 0U;
        fault_log_buffer[i].occurrence_count = 0U;
    }

    fault_log_count = 0U;
    fault_log_write_index = 0U;
    fault_log_sequence = 0U;

    fault_logger_initialized = 1U;

    return FAULT_LOGGER_OK;
}


fault_logger_status_t fault_logger_log(
    fault_code_t code,
    fault_source_t source,
    fault_severity_t severity,
    uint32_t timestamp_ms,
    uint32_t occurrence_count
)
{
    fault_log_entry_t *entry;

    if (fault_logger_initialized == 0U)
    {
        return FAULT_LOGGER_ERROR_NOT_INITIALIZED;
    }

    if ((code <= FAULT_CODE_NONE) ||
        (code >= FAULT_CODE_COUNT))
    {
        return FAULT_LOGGER_ERROR_PARAMETER;
    }

    entry = &fault_log_buffer[fault_log_write_index];

    fault_log_sequence++;

    entry->sequence = fault_log_sequence;
    entry->code = code;
    entry->source = source;
    entry->severity = severity;
    entry->timestamp_ms = timestamp_ms;
    entry->occurrence_count = occurrence_count;

    fault_log_write_index++;

    if (fault_log_write_index >= FAULT_LOGGER_MAX_ENTRIES)
    {
        fault_log_write_index = 0U;
    }

    if (fault_log_count < FAULT_LOGGER_MAX_ENTRIES)
    {
        fault_log_count++;
    }

    return FAULT_LOGGER_OK;
}


fault_logger_status_t fault_logger_get(
    uint32_t index,
    fault_log_entry_t *entry
)
{
    uint32_t oldest_index;

    if (fault_logger_initialized == 0U)
    {
        return FAULT_LOGGER_ERROR_NOT_INITIALIZED;
    }

    if (entry == 0)
    {
        return FAULT_LOGGER_ERROR_PARAMETER;
    }

    if (fault_log_count == 0U)
    {
        return FAULT_LOGGER_ERROR_EMPTY;
    }

    if (index >= fault_log_count)
    {
        return FAULT_LOGGER_ERROR_PARAMETER;
    }

    /*
     * Index 0 = oldest entry
     * Index count-1 = newest entry
     */

    if (fault_log_count < FAULT_LOGGER_MAX_ENTRIES)
    {
        oldest_index = 0U;
    }
    else
    {
        oldest_index = fault_log_write_index;
    }

    oldest_index += index;

    if (oldest_index >= FAULT_LOGGER_MAX_ENTRIES)
    {
        oldest_index -= FAULT_LOGGER_MAX_ENTRIES;
    }

    *entry = fault_log_buffer[oldest_index];

    return FAULT_LOGGER_OK;
}


fault_logger_status_t fault_logger_get_latest(
    fault_log_entry_t *entry
)
{
    uint32_t latest_index;

    if (fault_logger_initialized == 0U)
    {
        return FAULT_LOGGER_ERROR_NOT_INITIALIZED;
    }

    if (entry == 0)
    {
        return FAULT_LOGGER_ERROR_PARAMETER;
    }

    if (fault_log_count == 0U)
    {
        return FAULT_LOGGER_ERROR_EMPTY;
    }

    if (fault_log_write_index == 0U)
    {
        latest_index = FAULT_LOGGER_MAX_ENTRIES - 1U;
    }
    else
    {
        latest_index = fault_log_write_index - 1U;
    }

    *entry = fault_log_buffer[latest_index];

    return FAULT_LOGGER_OK;
}


uint32_t fault_logger_get_count(void)
{
    return fault_log_count;
}


fault_logger_status_t fault_logger_clear(void)
{
    uint32_t i;

    if (fault_logger_initialized == 0U)
    {
        return FAULT_LOGGER_ERROR_NOT_INITIALIZED;
    }

    for (i = 0U; i < FAULT_LOGGER_MAX_ENTRIES; i++)
    {
        fault_log_buffer[i].sequence = 0U;
        fault_log_buffer[i].code = FAULT_CODE_NONE;
        fault_log_buffer[i].source = FAULT_SOURCE_NONE;
        fault_log_buffer[i].severity = FAULT_SEVERITY_INFO;
        fault_log_buffer[i].timestamp_ms = 0U;
        fault_log_buffer[i].occurrence_count = 0U;
    }

    fault_log_count = 0U;
    fault_log_write_index = 0U;
    fault_log_sequence = 0U;

    return FAULT_LOGGER_OK;
}


fault_logger_status_t fault_logger_health_check(void)
{
    if (fault_logger_initialized == 0U)
    {
        return FAULT_LOGGER_ERROR_NOT_INITIALIZED;
    }

    if (fault_log_count > FAULT_LOGGER_MAX_ENTRIES)
    {
        return FAULT_LOGGER_ERROR_FULL;
    }

    return FAULT_LOGGER_OK;
}
