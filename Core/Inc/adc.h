/*
 * adc.h
 *
 *  Created on: 09-Sept-2026
 *      Author: engineer
 */

#ifndef ADC_H
#define ADC_H

#include <stdint.h>

typedef enum
{
    ADC_OK = 0,

    ADC_ERROR_CLOCK,
    ADC_ERROR_GPIO,
    ADC_ERROR_CONFIG,
    ADC_ERROR_TIMEOUT,
    ADC_ERROR_CALIBRATION,
    ADC_ERROR_PARAMETER

} adc_status_t;


/* ADC initialization */
adc_status_t adc_init(void);


/* External ADC channel */
adc_status_t adc_read_external(uint16_t *value);


/* Internal channels */
adc_status_t adc_read_vrefint(uint16_t *value);

adc_status_t adc_read_temperature(uint16_t *value);

adc_status_t adc_read_vbat(uint16_t *value);


/* Conversion helpers */
uint32_t adc_raw_to_millivolts(uint16_t raw);

uint32_t adc_vrefint_to_vdda_mv(uint16_t vrefint_raw);

int32_t adc_temperature_from_raw(uint16_t temperature_raw,
                                  uint32_t vdda_mv);

uint32_t adc_vbat_to_mv(uint16_t vbat_raw);
adc_status_t adc_configure_vrefint_dma(void);
adc_status_t adc_configure_internal_channels_dma(void);

/* General information */
uint16_t adc_get_last_value(void);


/* ADC health */
adc_status_t adc_health_check(void);

#endif /* ADC_H */
