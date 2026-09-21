/*
 * peripheral_monitor.c
 *
 *  Created on: 17-Sept-2026
 *      Author: engineer
 */
#include "peripheral_monitor.h"

#include "clock.h"
#include "led.h"
#include "system_time.h"
#include "usart.h"
#include "adc.h"
#include "dma.h"
#include "watchdog_manager.h"


static volatile uint8_t peripheral_monitor_initialized = 0U;

static volatile uint8_t peripheral_monitor_fault = 0U;

static peripheral_monitor_info_t peripheral_info;


/* ---------------------------------------------------------
 * Initialization
 * --------------------------------------------------------- */

peripheral_monitor_status_t peripheral_monitor_init(void)
{
    peripheral_monitor_initialized = 1U;

    peripheral_monitor_fault = 0U;

    peripheral_info.clock_status =
        PERIPHERAL_MONITOR_OK;

    peripheral_info.gpio_status =
        PERIPHERAL_MONITOR_OK;

    peripheral_info.systick_status =
        PERIPHERAL_MONITOR_OK;

    peripheral_info.uart_status =
        PERIPHERAL_MONITOR_OK;

    peripheral_info.adc_status =
        PERIPHERAL_MONITOR_OK;

    peripheral_info.dma_status =
        PERIPHERAL_MONITOR_OK;

    peripheral_info.watchdog_status =
        PERIPHERAL_MONITOR_OK;

    peripheral_info.all_healthy = 1U;

    return PERIPHERAL_MONITOR_OK;
}


/* ---------------------------------------------------------
 * Update
 * --------------------------------------------------------- */

peripheral_monitor_status_t peripheral_monitor_update(void)
{
    if (!peripheral_monitor_initialized)
    {
        return PERIPHERAL_MONITOR_ERROR_NOT_INITIALIZED;
    }

    /* Start each update with a clean health state */
    peripheral_monitor_fault = 0U;
    peripheral_info.all_healthy = 1U;

    /* Clock */
    if (clock_health_check() == CLOCK_OK)
    {
        peripheral_info.clock_status = PERIPHERAL_MONITOR_OK;
    }
    else
    {
        peripheral_info.clock_status = PERIPHERAL_MONITOR_FAULT_CLOCK;
        peripheral_monitor_fault = 1U;
        peripheral_info.all_healthy = 0U;
    }

    /* GPIO */
    if (led_health_check() == LED_OK)
    {
        peripheral_info.gpio_status = PERIPHERAL_MONITOR_OK;
    }
    else
    {
        peripheral_info.gpio_status = PERIPHERAL_MONITOR_FAULT_GPIO;
        peripheral_monitor_fault = 1U;
        peripheral_info.all_healthy = 0U;
    }

    /* SysTick */
    if (system_time_health_check() == SYSTEM_TIME_OK)
    {
        peripheral_info.systick_status = PERIPHERAL_MONITOR_OK;
    }
    else
    {
        peripheral_info.systick_status = PERIPHERAL_MONITOR_FAULT_SYSTICK;
        peripheral_monitor_fault = 1U;
        peripheral_info.all_healthy = 0U;
    }

    /* UART */
    if (uart_health_check() == UART_OK)
    {
        peripheral_info.uart_status = PERIPHERAL_MONITOR_OK;
    }
    else
    {
        peripheral_info.uart_status = PERIPHERAL_MONITOR_FAULT_UART;
        peripheral_monitor_fault = 1U;
        peripheral_info.all_healthy = 0U;
    }

    /* ADC */
    if (adc_health_check() == ADC_OK)
    {
        peripheral_info.adc_status = PERIPHERAL_MONITOR_OK;
    }
    else
    {
        peripheral_info.adc_status = PERIPHERAL_MONITOR_FAULT_ADC;
        peripheral_monitor_fault = 1U;
        peripheral_info.all_healthy = 0U;
    }

    /* DMA */
    if (dma_adc_health_check() == DMA_OK)
    {
        peripheral_info.dma_status = PERIPHERAL_MONITOR_OK;
    }
    else
    {
        peripheral_info.dma_status = PERIPHERAL_MONITOR_FAULT_DMA;
        peripheral_monitor_fault = 1U;
        peripheral_info.all_healthy = 0U;
    }

    /* Watchdog */
    if (watchdog_manager_health_check() == WATCHDOG_OK)
    {
        peripheral_info.watchdog_status = PERIPHERAL_MONITOR_OK;
    }
    else
    {
        peripheral_info.watchdog_status = PERIPHERAL_MONITOR_OK;
    }

    if (peripheral_monitor_fault != 0U)
    {
        return PERIPHERAL_MONITOR_FAULT;
    }

    return PERIPHERAL_MONITOR_OK;
}

/* ---------------------------------------------------------
 * Health check
 * --------------------------------------------------------- */

peripheral_monitor_status_t peripheral_monitor_health_check(void)
{
    if (!peripheral_monitor_initialized)
    {
        return PERIPHERAL_MONITOR_ERROR_NOT_INITIALIZED;
    }

    return peripheral_monitor_update();
}

/* ---------------------------------------------------------
 * Get information
 * --------------------------------------------------------- */

peripheral_monitor_status_t peripheral_monitor_get_info(
    peripheral_monitor_info_t *info)
{
    if (info == 0)
    {
        return PERIPHERAL_MONITOR_ERROR_NOT_INITIALIZED;
    }

    if (peripheral_monitor_initialized == 0U)
    {
        return PERIPHERAL_MONITOR_ERROR_NOT_INITIALIZED;
    }

    *info = peripheral_info;

    return PERIPHERAL_MONITOR_OK;
}


/* ---------------------------------------------------------
 * Overall health
 * --------------------------------------------------------- */

uint8_t peripheral_monitor_all_healthy(void)
{
    return peripheral_info.all_healthy;
}


/* ---------------------------------------------------------
 * Fault status
 * --------------------------------------------------------- */

uint8_t peripheral_monitor_fault_active(void)
{
    return peripheral_monitor_fault;
}


/* ---------------------------------------------------------
 * Clear fault
 * --------------------------------------------------------- */

void peripheral_monitor_clear_fault(void)
{
    peripheral_monitor_fault = 0U;

    peripheral_info.all_healthy = 1U;
}

