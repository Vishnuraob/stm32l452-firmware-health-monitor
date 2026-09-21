/*
 * adc_monitor.c
 *
 *  Created on: 16-Sept-2026
 *      Author: engineer
 */
#include "adc_monitor.h"
#include "adc.h"


/*
 * DMA buffer layout
 */

#define ADC_MONITOR_VREFINT_INDEX       0U
#define ADC_MONITOR_TEMPERATURE_INDEX   1U
#define ADC_MONITOR_VBAT_INDEX          2U


/*
 * Internal state
 */

static adc_monitor_config_t monitor_config;

static uint32_t monitor_vdda_mv = 0U;

static int32_t monitor_temperature_c = 0;

static uint32_t monitor_vbat_mv = 0U;

static uint8_t monitor_initialized = 0U;

static adc_monitor_status_t monitor_status = ADC_MONITOR_OK;


/*
 * Initialize monitor
 */

adc_monitor_status_t adc_monitor_init(
    const adc_monitor_config_t *config)
{
    if (config == 0)
    {
        return ADC_MONITOR_ERROR_PARAMETER;
    }

    /*
     * Copy configuration.
     */

    monitor_config = *config;

    /*
     * Clear previous values.
     */

    monitor_vdda_mv = 0U;
    monitor_temperature_c = 0;
    monitor_vbat_mv = 0U;

    monitor_status = ADC_MONITOR_OK;

    monitor_initialized = 1U;

    return ADC_MONITOR_OK;
}


/*
 * Process latest ADC DMA samples
 */

adc_monitor_status_t adc_monitor_update(
    const volatile uint16_t *buffer)
{
    uint16_t vrefint_raw;
    uint16_t temperature_raw;
    uint16_t vbat_raw;

    if (buffer == 0)
    {
        return ADC_MONITOR_ERROR_PARAMETER;
    }

    if (monitor_initialized == 0U)
    {
        return ADC_MONITOR_ERROR_ADC;
    }


    /*
     * Extract DMA samples
     */

    vrefint_raw =
        buffer[ADC_MONITOR_VREFINT_INDEX];

    temperature_raw =
        buffer[ADC_MONITOR_TEMPERATURE_INDEX];

    vbat_raw =
        buffer[ADC_MONITOR_VBAT_INDEX];


    /*
     * Calculate VDDA
     */

    monitor_vdda_mv =
        adc_vrefint_to_vdda_mv(vrefint_raw);


    /*
     * Calculate temperature
     *
     * Temperature calibration is referenced to
     * VDDA = 3.0 V, so use measured VDDA.
     */

    monitor_temperature_c =
        adc_temperature_from_raw(
            temperature_raw,
            monitor_vdda_mv);


    /*
     * Calculate VBAT
     */

    monitor_vbat_mv =
        adc_vbat_to_mv(vbat_raw);


    /*
     * Perform health checks
     */

    monitor_status = adc_monitor_health_check();

    return monitor_status;
}


/*
 * Get VDDA
 */

uint32_t adc_monitor_get_vdda_mv(void)
{
    return monitor_vdda_mv;
}


/*
 * Get temperature
 */

int32_t adc_monitor_get_temperature_c(void)
{
    return monitor_temperature_c;
}


/*
 * Get VBAT
 */

uint32_t adc_monitor_get_vbat_mv(void)
{
    return monitor_vbat_mv;
}


/*
 * Check VDDA
 */

uint8_t adc_monitor_vdda_is_healthy(void)
{
    if (monitor_vdda_mv < monitor_config.vdda_min_mv)
    {
        return 0U;
    }

    if (monitor_vdda_mv > monitor_config.vdda_max_mv)
    {
        return 0U;
    }

    return 1U;
}


/*
 * Check temperature
 */

uint8_t adc_monitor_temperature_is_healthy(void)
{
    if (monitor_temperature_c <
        monitor_config.temperature_min_c)
    {
        return 0U;
    }

    if (monitor_temperature_c >
        monitor_config.temperature_max_c)
    {
        return 0U;
    }

    return 1U;
}


/*
 * Check VBAT
 */

uint8_t adc_monitor_vbat_is_healthy(void)
{
    if (monitor_vbat_mv < monitor_config.vbat_min_mv)
    {
        return 0U;
    }

    if (monitor_vbat_mv > monitor_config.vbat_max_mv)
    {
        return 0U;
    }

    return 1U;
}


/*
 * Overall health check
 */

adc_monitor_status_t adc_monitor_health_check(void)
{
    if (monitor_initialized == 0U)
    {
        return ADC_MONITOR_ERROR_ADC;
    }


    /*
     * VDDA
     */

    if (monitor_vdda_mv < monitor_config.vdda_min_mv)
    {
        return ADC_MONITOR_FAULT_VDDA_LOW;
    }

    if (monitor_vdda_mv > monitor_config.vdda_max_mv)
    {
        return ADC_MONITOR_FAULT_VDDA_HIGH;
    }


    /*
     * Temperature
     */

    if (monitor_temperature_c <
        monitor_config.temperature_min_c)
    {
        return ADC_MONITOR_FAULT_TEMPERATURE_LOW;
    }

    if (monitor_temperature_c >
        monitor_config.temperature_max_c)
    {
        return ADC_MONITOR_FAULT_TEMPERATURE_HIGH;
    }


    /*
     * VBAT
     */

    if (monitor_vbat_mv < monitor_config.vbat_min_mv)
    {
        return ADC_MONITOR_FAULT_VBAT_LOW;
    }

    if (monitor_vbat_mv > monitor_config.vbat_max_mv)
    {
        return ADC_MONITOR_FAULT_VBAT_HIGH;
    }


    return ADC_MONITOR_OK;
}

