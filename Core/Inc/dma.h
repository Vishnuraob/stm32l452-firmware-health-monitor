/*
 * dma.h
 *
 *  Created on: 15-Sept-2026
 *      Author: engineer
 */

#ifndef DMA_H
#define DMA_H

#include <stdint.h>


typedef enum
{
    DMA_OK = 0,
    DMA_ERROR_CLOCK,
    DMA_ERROR_CONFIG,
    DMA_ERROR_PARAMETER
} dma_status_t;


/*
 * Initialize DMA1 Channel 1 for ADC1.
 */
dma_status_t dma_adc_init(volatile uint16_t *buffer,
                          uint32_t length);


/*
 * Enable DMA1 Channel 1.
 */
dma_status_t dma_adc_start(void);


/*
 * Disable DMA1 Channel 1.
 */
void dma_adc_stop(void);


/*
 * Check DMA configuration.
 */
dma_status_t dma_adc_health_check(void);


/*
 * Check whether a transfer is complete.
 */
uint8_t dma_adc_transfer_complete(void);


/*
 * Clear DMA transfer-complete flag.
 */
void dma_adc_clear_transfer_complete(void);

void dma_adc_irq_handler(void);
#endif /* DMA_H */
