/*
 * reset_manager.h
 *
 *  Created on: 16-Sept-2026
 *      Author: engineer
 */
#ifndef RESET_MANAGER_H
#define RESET_MANAGER_H

#include <stdint.h>


typedef enum
{
    RESET_REASON_UNKNOWN = 0,

    RESET_REASON_POWER_ON,
    RESET_REASON_BROWN_OUT,
    RESET_REASON_PIN,
    RESET_REASON_SOFTWARE,
    RESET_REASON_IWDG,
    RESET_REASON_WWDG,
    RESET_REASON_LOW_POWER

} reset_reason_t;


typedef enum
{
    RESET_MANAGER_OK = 0,
    RESET_MANAGER_ERROR_NOT_INITIALIZED

} reset_manager_status_t;


/*
 * Capture reset cause from RCC->CSR.
 *
 * This function must be called early during startup,
 * before reset flags are cleared.
 */
reset_manager_status_t reset_manager_init(void);


/*
 * Return the detected reset reason.
 */
reset_reason_t reset_manager_get_reason(void);


/*
 * Return the raw RCC->CSR value captured during startup.
 */
uint32_t reset_manager_get_raw_flags(void);


/*
 * Check whether the reset manager was initialized.
 */
reset_manager_status_t reset_manager_health_check(void);


/*
 * Clear MCU reset flags.
 */
void reset_manager_clear_flags(void);

#endif /* RESET_MANAGER_H */
