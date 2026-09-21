/*
 * adc_monitor.h
 *
 *  Created on: 16-Sept-2026
 *      Author: engineer
 */
#ifndef ADC_MONITOR_H
#define ADC_MONITOR_H

#include <stdint.h>

typedef enum
{
    ADC_MONITOR_OK = 0,

    ADC_MONITOR_ERROR_PARAMETER,
    ADC_MONITOR_ERROR_ADC,

    ADC_MONITOR_FAULT_VDDA_LOW,
    ADC_MONITOR_FAULT_VDDA_HIGH,

    ADC_MONITOR_FAULT_TEMPERATURE_LOW,
    ADC_MONITOR_FAULT_TEMPERATURE_HIGH,

    ADC_MONITOR_FAULT_VBAT_LOW,
    ADC_MONITOR_FAULT_VBAT_HIGH

} adc_monitor_status_t;


/*
 * ADC monitor configuration
 */

typedef struct
{
    uint32_t vdda_min_mv;
    uint32_t vdda_max_mv;

    int32_t temperature_min_c;
    int32_t temperature_max_c;

    uint32_t vbat_min_mv;
    uint32_t vbat_max_mv;

} adc_monitor_config_t;


/*
 * Initialize ADC monitor
 */

adc_monitor_status_t adc_monitor_init(
    const adc_monitor_config_t *config);


/*
 * Process the latest DMA samples
 *
 * buffer[0] = VREFINT
 * buffer[1] = Temperature
 * buffer[2] = VBAT
 */

adc_monitor_status_t adc_monitor_update(
    const volatile uint16_t *buffer);


/*
 * Get latest calculated values
 */

uint32_t adc_monitor_get_vdda_mv(void);

int32_t adc_monitor_get_temperature_c(void);

uint32_t adc_monitor_get_vbat_mv(void);


/*
 * Individual health checks
 */

uint8_t adc_monitor_vdda_is_healthy(void);

uint8_t adc_monitor_temperature_is_healthy(void);

uint8_t adc_monitor_vbat_is_healthy(void);


/*
 * Overall health check
 */

adc_monitor_status_t adc_monitor_health_check(void);

#endif /* ADC_MONITOR_H */
