/*
 * reset_manager.c
 *
 *  Created on: 16-Sept-2026
 *      Author: engineer
 */
#include "reset_manager.h"
#include "stm32l452xx.h"


/*
 * Internal state
 */

static reset_reason_t reset_reason =
    RESET_REASON_UNKNOWN;

static uint32_t reset_raw_flags = 0U;

static uint8_t reset_manager_initialized = 0U;


/*
 * Initialize reset manager
 */

reset_manager_status_t reset_manager_init(void)
{
    /*
     * Capture RCC->CSR BEFORE clearing anything.
     */

    reset_raw_flags = RCC->CSR;


    /*
     * Determine reset reason.
     *
     * Priority is intentional because more than one
     * reset flag can sometimes be set.
     */

    if ((reset_raw_flags & RCC_CSR_IWDGRSTF) != 0U)
    {
        reset_reason = RESET_REASON_IWDG;
    }
    else if ((reset_raw_flags & RCC_CSR_WWDGRSTF) != 0U)
    {
        reset_reason = RESET_REASON_WWDG;
    }
    else if ((reset_raw_flags & RCC_CSR_SFTRSTF) != 0U)
    {
        reset_reason = RESET_REASON_SOFTWARE;
    }
    else if ((reset_raw_flags & RCC_CSR_PINRSTF) != 0U)
    {
        reset_reason = RESET_REASON_PIN;
    }
    else if ((reset_raw_flags & RCC_CSR_BORRSTF) != 0U)
    {
        reset_reason = RESET_REASON_BROWN_OUT;
    }
    else if ((reset_raw_flags & RCC_CSR_LPWRRSTF) != 0U)
    {
        reset_reason = RESET_REASON_LOW_POWER;
    }
    else
    {
        reset_reason = RESET_REASON_UNKNOWN;
    }


    reset_manager_initialized = 1U;

    return RESET_MANAGER_OK;
}


/*
 * Get reset reason
 */

reset_reason_t reset_manager_get_reason(void)
{
    return reset_reason;
}


/*
 * Get captured raw reset flags
 */

uint32_t reset_manager_get_raw_flags(void)
{
    return reset_raw_flags;
}


/*
 * Health check
 */

reset_manager_status_t reset_manager_health_check(void)
{
    if (reset_manager_initialized == 0U)
    {
        return RESET_MANAGER_ERROR_NOT_INITIALIZED;
    }

    return RESET_MANAGER_OK;
}


/*
 * Clear reset flags
 */

void reset_manager_clear_flags(void)
{
    /*
     * RM0394:
     *
     * RCC_CSR reset flags are cleared by setting RMVF.
     */

    RCC->CSR |= RCC_CSR_RMVF;
}

