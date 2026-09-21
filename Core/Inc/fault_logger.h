/*
 * fault_logger.h
 *
 *  Created on: 17-Sept-2026
 *      Author: engineer
 */
#ifndef FAULT_LOGGER_H
#define FAULT_LOGGER_H

#include <stdint.h>
#include "fault_manager.h"

#define FAULT_LOGGER_MAX_ENTRIES    16U

typedef enum
{
    FAULT_LOGGER_OK = 0,
    FAULT_LOGGER_ERROR_NOT_INITIALIZED,
    FAULT_LOGGER_ERROR_PARAMETER,
    FAULT_LOGGER_ERROR_EMPTY,
    FAULT_LOGGER_ERROR_FULL
} fault_logger_status_t;

typedef struct
{
    uint32_t sequence;
    fault_code_t code;
    fault_source_t source;
    fault_severity_t severity;
    uint32_t timestamp_ms;
    uint32_t occurrence_count;
} fault_log_entry_t;

fault_logger_status_t fault_logger_init(void);

fault_logger_status_t fault_logger_log(
    fault_code_t code,
    fault_source_t source,
    fault_severity_t severity,
    uint32_t timestamp_ms,
    uint32_t occurrence_count
);

fault_logger_status_t fault_logger_get(
    uint32_t index,
    fault_log_entry_t *entry
);

fault_logger_status_t fault_logger_get_latest(
    fault_log_entry_t *entry
);

uint32_t fault_logger_get_count(void);

fault_logger_status_t fault_logger_clear(void);

fault_logger_status_t fault_logger_health_check(void);

#endif
