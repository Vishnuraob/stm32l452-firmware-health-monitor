/*
 * dma.c
 *
 *  Created on: 15-Sept-2026
 *      Author: engineer
 */
#include "dma.h"

#include "stm32l452xx.h"


/* ================================================================
 * DMA configuration
 * ================================================================ */

#define DMA_ADC_CHANNEL          DMA1_Channel1

#define DMA_ADC_CHANNEL_NUMBER   1U

#define DMA_ADC_BUFFER_MAX_SIZE  256U


/* ================================================================
 * DMA initialization
 * ================================================================ */

dma_status_t dma_adc_init(volatile uint16_t *buffer,
                          uint32_t length)
{
    if ((buffer == 0) ||
        (length == 0U) ||
        (length > DMA_ADC_BUFFER_MAX_SIZE))
    {
        return DMA_ERROR_PARAMETER;
    }


    /*
     * Enable DMA1 peripheral clock.
     */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    /*
     * Read back register to make sure
     * the clock enable has reached the peripheral.
     */
    (void)RCC->AHB1ENR;


    /*
     * Disable DMA channel before configuration.
     */
    DMA_ADC_CHANNEL->CCR &= ~DMA_CCR_EN;


    /*
     * Wait until channel is disabled.
     */
    while ((DMA_ADC_CHANNEL->CCR & DMA_CCR_EN) != 0U)
    {
    }


    /*
     * Configure ADC1 request on DMA1 Channel 1.
     *
     * C1S[3:0] = 0000 -> ADC1
     */
    DMA1_CSELR->CSELR &= ~DMA_CSELR_C1S;


    /*
     * Configure DMA channel.
     */
    DMA_ADC_CHANNEL->CCR = 0U;


    /*
     * Peripheral -> Memory
     *
     * DIR = 00
     */
    DMA_ADC_CHANNEL->CCR &= ~DMA_CCR_DIR;


    /*
     * Peripheral address does not increment.
     *
     * ADC1->DR is always the same address.
     */
    DMA_ADC_CHANNEL->CCR &= ~DMA_CCR_PINC;


    /*
     * Memory address increments after every transfer.
     */
    DMA_ADC_CHANNEL->CCR |= DMA_CCR_MINC;


    /*
     * Circular mode.
     *
     * After the last buffer element,
     * DMA starts again at buffer[0].
     */
    DMA_ADC_CHANNEL->CCR |= DMA_CCR_CIRC;


    /*
     * Peripheral data size = 16 bits.
     */
/* Peripheral data size = 16-bit */
DMA_ADC_CHANNEL->CCR &= ~DMA_CCR_PSIZE;
DMA_ADC_CHANNEL->CCR |= DMA_CCR_PSIZE_0;

/* Memory data size = 16-bit */
DMA_ADC_CHANNEL->CCR &= ~DMA_CCR_MSIZE;
DMA_ADC_CHANNEL->CCR |= DMA_CCR_MSIZE_0;

    /*
     * Medium priority.
     */
    DMA_ADC_CHANNEL->CCR &= ~DMA_CCR_PL;

    /* Enable Transfer Complete interrupt */
    DMA_ADC_CHANNEL->CCR |= DMA_CCR_TCIE;
    /*
     * Peripheral address = ADC1 data register.
     */
    DMA_ADC_CHANNEL->CPAR =
        (uint32_t)&ADC1->DR;


    /*
     * Memory address = user's buffer.
     */
    DMA_ADC_CHANNEL->CMAR =
        (uint32_t)buffer;


    /*
     * Number of transfers.
     */
    DMA_ADC_CHANNEL->CNDTR = length;


    /*
     * Clear all DMA1 Channel 1 flags.
     */
    DMA1->IFCR =
          DMA_IFCR_CGIF1
        | DMA_IFCR_CTCIF1
        | DMA_IFCR_CHTIF1
        | DMA_IFCR_CTEIF1;

    NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    return DMA_OK;
}


/* ================================================================
 * Start DMA
 * ================================================================ */

dma_status_t dma_adc_start(void)
{
    /*
     * Enable DMA1 Channel 1.
     */
    DMA_ADC_CHANNEL->CCR |= DMA_CCR_EN;

    return DMA_OK;
}


/* ================================================================
 * Stop DMA
 * ================================================================ */

void dma_adc_stop(void)
{
    DMA_ADC_CHANNEL->CCR &= ~DMA_CCR_EN;
}


/* ================================================================
 * DMA health check
 * ================================================================ */

dma_status_t dma_adc_health_check(void)
{
    /*
     * DMA1 clock must be enabled.
     */
    if ((RCC->AHB1ENR & RCC_AHB1ENR_DMA1EN) == 0U)
    {
        return DMA_ERROR_CLOCK;
    }


    /*
     * Verify ADC1 is selected as DMA request
     * for Channel 1.
     */
    if ((DMA1_CSELR->CSELR & DMA_CSELR_C1S) != 0U)
    {
        return DMA_ERROR_CONFIG;
    }


    /*
     * Verify peripheral-to-memory direction.
     */
    if ((DMA_ADC_CHANNEL->CCR & DMA_CCR_DIR) != 0U)
    {
        return DMA_ERROR_CONFIG;
    }


    /*
     * Verify memory increment.
     */
    if ((DMA_ADC_CHANNEL->CCR & DMA_CCR_MINC) == 0U)
    {
        return DMA_ERROR_CONFIG;
    }


    /*
     * Verify circular mode.
     */
    if ((DMA_ADC_CHANNEL->CCR & DMA_CCR_CIRC) == 0U)
    {
        return DMA_ERROR_CONFIG;
    }


    /*
     * Verify peripheral increment is disabled.
     */
    if ((DMA_ADC_CHANNEL->CCR & DMA_CCR_PINC) != 0U)
    {
        return DMA_ERROR_CONFIG;
    }

    /* Peripheral data size must be 16-bit */
    if ((DMA_ADC_CHANNEL->CCR & DMA_CCR_PSIZE) != DMA_CCR_PSIZE_0)
    {
        return DMA_ERROR_CONFIG;
    }

    /* Memory data size must be 16-bit */
    if ((DMA_ADC_CHANNEL->CCR & DMA_CCR_MSIZE) != DMA_CCR_MSIZE_0)
    {
        return DMA_ERROR_CONFIG;
    }

    return DMA_OK;
}


/* ================================================================
 * Transfer complete
 * ================================================================ */
static volatile uint8_t dma_transfer_complete_flag = 0U;

uint8_t dma_adc_transfer_complete(void)
{
    return dma_transfer_complete_flag;
}

void dma_adc_clear_transfer_complete(void)
{
    dma_transfer_complete_flag = 0U;
}

void dma_adc_irq_handler(void)
{
    if ((DMA1->ISR & DMA_ISR_TCIF1) != 0U)
    {
        /* Clear Transfer Complete flag */
        DMA1->IFCR = DMA_IFCR_CTCIF1;

        /* Inform application */
        dma_transfer_complete_flag = 1U;
    }
}
