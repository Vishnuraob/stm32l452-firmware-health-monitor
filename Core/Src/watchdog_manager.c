#include "watchdog_manager.h"
#include "stm32l452xx.h"
#include "heartbeat_monitor.h"


/* =========================================================
 * Configuration
 * ========================================================= */

#define IWDG_LSI_FREQUENCY_HZ      32000UL

/*
 * IWDG prescaler:
 *
 * PR = 0 -> /4
 * PR = 1 -> /8
 * PR = 2 -> /16
 * PR = 3 -> /32
 * PR = 4 -> /64
 * PR = 5 -> /128
 * PR = 6 -> /256
 */
#define IWDG_PRESCALER_DIVIDER     32UL
#define IWDG_PRESCALER_VALUE       3U

#define IWDG_RELOAD_MAX            4095UL

#define IWDG_TIMEOUT_COUNT         1000000UL


/* IWDG key values */
#define IWDG_KEY_ENABLE            0xCCCCU
#define IWDG_KEY_RELOAD            0xAAAAU
#define IWDG_KEY_WRITE_ACCESS      0x5555U


/* =========================================================
 * Internal State
 * ========================================================= */

static volatile uint32_t watchdog_timeout_ms = 0U;
static volatile uint32_t watchdog_reload_value = 0U;

static volatile uint8_t watchdog_initialized = 0U;
static volatile uint8_t watchdog_fault = 0U;


/* =========================================================
 * Wait for Prescaler Update
 * ========================================================= */

static watchdog_status_t watchdog_wait_for_prescaler_update(void)
{
    uint32_t timeout = IWDG_TIMEOUT_COUNT;

    while ((IWDG->SR & IWDG_SR_PVU) != 0U)
    {
        if (timeout == 0U)
        {
            return WATCHDOG_ERROR_TIMEOUT;
        }

        timeout--;
    }

    return WATCHDOG_OK;
}


/* =========================================================
 * Wait for Reload Update
 * ========================================================= */

static watchdog_status_t watchdog_wait_for_reload_update(void)
{
    uint32_t timeout = IWDG_TIMEOUT_COUNT;

    while ((IWDG->SR & IWDG_SR_RVU) != 0U)
    {
        if (timeout == 0U)
        {
            return WATCHDOG_ERROR_TIMEOUT;
        }

        timeout--;
    }

    return WATCHDOG_OK;
}


/* =========================================================
 * Calculate Reload Value
 * ========================================================= */

static watchdog_status_t watchdog_calculate_reload(
    uint32_t timeout_ms,
    uint32_t *reload)
{
    uint64_t ticks;

    if (reload == 0U)
    {
        return WATCHDOG_ERROR_PARAMETER;
    }

    /*
     * timeout =
     *
     *     (reload + 1) * prescaler / LSI
     *
     * Therefore:
     *
     * reload =
     *
     *     timeout * LSI
     *     ---------------- - 1
     *        1000 * prescaler
     */

    ticks =
        ((uint64_t)timeout_ms *
         IWDG_LSI_FREQUENCY_HZ) /
        (1000ULL *
         IWDG_PRESCALER_DIVIDER);

    if (ticks == 0ULL)
    {
        return WATCHDOG_ERROR_CONFIG;
    }

    ticks--;

    if (ticks > IWDG_RELOAD_MAX)
    {
        return WATCHDOG_ERROR_CONFIG;
    }

    *reload = (uint32_t)ticks;

    return WATCHDOG_OK;
}


/* =========================================================
 * Initialize IWDG
 * ========================================================= */

watchdog_status_t watchdog_manager_init(uint32_t timeout_ms)
{
    watchdog_status_t status;
    uint32_t reload_value;


    /* =====================================================
     * Parameter check
     * ===================================================== */

    if (timeout_ms == 0U)
    {
        return WATCHDOG_ERROR_PARAMETER;
    }


    /* =====================================================
     * Calculate reload value
     * ===================================================== */

    status =
        watchdog_calculate_reload(
            timeout_ms,
            &reload_value);

    if (status != WATCHDOG_OK)
    {
        return status;
    }


    /* =====================================================
     * Enable LSI
     * ===================================================== */

    RCC->CSR |= RCC_CSR_LSION;


    /* =====================================================
     * Wait for LSI Ready
     * ===================================================== */

    {
        uint32_t timeout = IWDG_TIMEOUT_COUNT;

        while ((RCC->CSR & RCC_CSR_LSIRDY) == 0U)
        {
            if (timeout == 0U)
            {
                return WATCHDOG_ERROR_TIMEOUT;
            }

            timeout--;
        }
    }


    /* =====================================================
     * Start IWDG
     *
     * IMPORTANT:
     *
     * We start the watchdog BEFORE configuring PR/RLR.
     *
     * This avoids the PVU/RVU synchronization problem
     * observed on this device.
     * ===================================================== */

    IWDG->KR = IWDG_KEY_ENABLE;


    /* =====================================================
     * Enable write access to PR/RLR
     * ===================================================== */

    IWDG->KR = IWDG_KEY_WRITE_ACCESS;


    /* =====================================================
     * Configure Prescaler
     *
     * PR = 3
     *
     * 000 -> /4
     * 001 -> /8
     * 010 -> /16
     * 011 -> /32
     * ===================================================== */

    IWDG->PR = IWDG_PRESCALER_VALUE;


    /* =====================================================
     * Configure Reload
     * ===================================================== */

    IWDG->RLR = reload_value;


    /* =====================================================
     * Wait for PR/RLR synchronization
     * ===================================================== */

    {
        uint32_t timeout = IWDG_TIMEOUT_COUNT;

        while ((IWDG->SR &
                (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U)
        {
            if (timeout == 0U)
            {
                return WATCHDOG_ERROR_TIMEOUT;
            }

            timeout--;
        }
    }


    /* =====================================================
     * Reload watchdog counter
     * ===================================================== */

    IWDG->KR = IWDG_KEY_RELOAD;


    /* =====================================================
     * Store configuration
     * ===================================================== */

    watchdog_timeout_ms = timeout_ms;
    watchdog_reload_value = reload_value;

    watchdog_initialized = 1U;
    watchdog_fault = 0U;


    return WATCHDOG_OK;
}


/* =========================================================
 * Refresh Watchdog
 * ========================================================= */

watchdog_status_t watchdog_manager_refresh(void)
{
    if (watchdog_initialized == 0U)
    {
        return WATCHDOG_ERROR_NOT_INITIALIZED;
    }

    IWDG->KR = IWDG_KEY_RELOAD;

    return WATCHDOG_OK;
}


/* =========================================================
 * Watchdog Supervision
 * ========================================================= */

watchdog_status_t watchdog_manager_update(void)
{
    heartbeat_status_t heartbeat_status;


    if (watchdog_initialized == 0U)
    {
        return WATCHDOG_ERROR_NOT_INITIALIZED;
    }


    /*
     * Check application heartbeat.
     */
    heartbeat_status =
        heartbeat_monitor_health_check();


    if (heartbeat_status != HEARTBEAT_OK)
    {
        /*
         * Application health failure.
         *
         * IMPORTANT:
         *
         * Do NOT refresh the watchdog.
         */
        watchdog_fault = 1U;

        return WATCHDOG_FAULT_HEARTBEAT;
    }


    /*
     * Application is healthy.
     */
    watchdog_fault = 0U;

    return watchdog_manager_refresh();
}


/* =========================================================
 * Watchdog Health Check
 * ========================================================= */

watchdog_status_t watchdog_manager_health_check(void)
{
    if (watchdog_initialized == 0U)
    {
        return WATCHDOG_ERROR_NOT_INITIALIZED;
    }


    /*
     * Check LSI.
     */
    if ((RCC->CSR & RCC_CSR_LSIRDY) == 0U)
    {
        return WATCHDOG_ERROR_CONFIG;
    }


    /*
     * Check prescaler.
     */
    if ((IWDG->PR & IWDG_PR_PR) !=
        IWDG_PRESCALER_VALUE)
    {
        return WATCHDOG_ERROR_CONFIG;
    }


    /*
     * Check reload value.
     *
     * RVU should be clear before reading RLR.
     */
    if ((IWDG->SR & IWDG_SR_RVU) != 0U)
    {
        return WATCHDOG_ERROR_CONFIG;
    }


    if ((IWDG->RLR & IWDG_RLR_RL) !=
        watchdog_reload_value)
    {
        return WATCHDOG_ERROR_CONFIG;
    }


    return WATCHDOG_OK;
}


/* =========================================================
 * Get Timeout
 * ========================================================= */

uint32_t watchdog_manager_get_timeout_ms(void)
{
    return watchdog_timeout_ms;
}


/* =========================================================
 * Get Initialization State
 * ========================================================= */

uint8_t watchdog_manager_is_initialized(void)
{
    return watchdog_initialized;
}


/* =========================================================
 * Get Fault State
 * ========================================================= */

uint8_t watchdog_manager_fault_active(void)
{
    return watchdog_fault;
}


/* =========================================================
 * Clear Fault
 * ========================================================= */

void watchdog_manager_clear_fault(void)
{
    watchdog_fault = 0U;
}
