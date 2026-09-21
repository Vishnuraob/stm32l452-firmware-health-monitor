#include "usart.h"
#include "stm32l452xx.h"
#include <stddef.h>

/*
 * USART2 configuration
 */
#define UART_BAUD_RATE          115200UL
#define UART_PCLK_FREQUENCY     80000000UL

/*
 * USART2 uses APB1 on STM32L452.
 */
#define UART_PORT               GPIOA

#define UART_TX_PIN             2U
#define UART_RX_PIN             3U

#define UART_TX_PIN_MASK        (1UL << UART_TX_PIN)
#define UART_RX_PIN_MASK        (1UL << UART_RX_PIN)


/*
 * USART baud-rate calculation for OVER8 = 0.
 *
 * BRR = fCK / baud
 */
#define UART_BRR_VALUE          (UART_PCLK_FREQUENCY / UART_BAUD_RATE)


/*
 * Initialize GPIOA and USART2.
 */
uart_status_t uart_init(void)
{
    /*
     * ---------------------------------------------------------
     * 1. Enable GPIOA clock
     * ---------------------------------------------------------
     */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    if ((RCC->AHB2ENR & RCC_AHB2ENR_GPIOAEN) == 0U)
    {
        return UART_ERROR_CLOCK;
    }


    /*
     * ---------------------------------------------------------
     * 2. Enable USART2 clock
     * ---------------------------------------------------------
     *
     * USART2 is connected to APB1.
     */
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    if ((RCC->APB1ENR1 & RCC_APB1ENR1_USART2EN) == 0U)
    {
        return UART_ERROR_CLOCK;
    }


    /*
     * ---------------------------------------------------------
     * 3. Configure PA2 and PA3 as Alternate Function
     * ---------------------------------------------------------
     *
     * MODER:
     *
     * 00 = input
     * 01 = output
     * 10 = alternate function
     * 11 = analog
     */
    UART_PORT->MODER &= ~((3UL << (UART_TX_PIN * 2U)) |
                          (3UL << (UART_RX_PIN * 2U)));

    UART_PORT->MODER |=  ((2UL << (UART_TX_PIN * 2U)) |
                          (2UL << (UART_RX_PIN * 2U)));


    /*
     * ---------------------------------------------------------
     * 4. Push-pull
     * ---------------------------------------------------------
     */
    UART_PORT->OTYPER &= ~(UART_TX_PIN_MASK | UART_RX_PIN_MASK);


    /*
     * ---------------------------------------------------------
     * 5. High speed
     * ---------------------------------------------------------
     *
     * UART signals benefit from a reasonably fast GPIO speed.
     */
    UART_PORT->OSPEEDR &= ~((3UL << (UART_TX_PIN * 2U)) |
                            (3UL << (UART_RX_PIN * 2U)));

    UART_PORT->OSPEEDR |=  ((2UL << (UART_TX_PIN * 2U)) |
                            (2UL << (UART_RX_PIN * 2U)));


    /*
     * ---------------------------------------------------------
     * 6. No pull-up / pull-down
     * ---------------------------------------------------------
     */
    UART_PORT->PUPDR &= ~((3UL << (UART_TX_PIN * 2U)) |
                          (3UL << (UART_RX_PIN * 2U)));


    /*
     * ---------------------------------------------------------
     * 7. Configure Alternate Function
     * ---------------------------------------------------------
     *
     * PA2 / PA3:
     *
     * AF7 = USART2
     */
    UART_PORT->AFR[0] &= ~((0xFUL << (UART_TX_PIN * 4U)) |
                           (0xFUL << (UART_RX_PIN * 4U)));

    UART_PORT->AFR[0] |=  ((7UL << (UART_TX_PIN * 4U)) |
                           (7UL << (UART_RX_PIN * 4U)));


    /*
     * ---------------------------------------------------------
     * 8. Disable USART2 before configuration
     * ---------------------------------------------------------
     */
    USART2->CR1 &= ~USART_CR1_UE;


    /*
     * ---------------------------------------------------------
     * 9. Configure baud rate
     * ---------------------------------------------------------
     *
     * 80 MHz / 115200 ≈ 694.44
     *
     * BRR therefore becomes 694.
     */
    USART2->BRR = UART_BRR_VALUE;


    /*
     * ---------------------------------------------------------
     * 10. Configure USART
     * ---------------------------------------------------------
     *
     * M1 = 0
     * M0 = 0
     *
     * → 8-bit data
     *
     * PCE = 0
     *
     * → no parity
     */
    USART2->CR1 &= ~(USART_CR1_M1 |
                     USART_CR1_M0 |
                     USART_CR1_PCE);


    /*
     * One stop bit.
     */
    USART2->CR2 &= ~USART_CR2_STOP;


    /*
     * Enable transmitter and receiver.
     */
    USART2->CR1 |= USART_CR1_TE |
                   USART_CR1_RE;


    /*
     * Finally enable USART2.
     */
    USART2->CR1 |= USART_CR1_UE;


    /*
     * Verify configuration.
     */
    if (uart_health_check() != UART_OK)
    {
        return UART_ERROR_CONFIG;
    }


    return UART_OK;
}


/*
 * Transmit one byte.
 */
uart_status_t uart_transmit_byte(uint8_t data)
{
    /*
     * Wait until transmit data register is empty.
     */
    while ((USART2->ISR & USART_ISR_TXE) == 0U)
    {
    }


    /*
     * Write byte to transmit data register.
     */
    USART2->TDR = data;


    /*
     * Wait until transmission is complete.
     */
//    while ((USART2->ISR & USART_ISR_TC) == 0U)
//    {
//    }


    return UART_OK;
}

uart_status_t uart_receive_byte_nonblocking(uint8_t *data)
{
    if (data == NULL)
    {
        return UART_ERROR_CONFIG;
    }

    /*
     * RXNE = Receive Data Register Not Empty
     *
     * RXNE = 1 -> received byte is available
     * RXNE = 0 -> no byte available
     */
    if ((USART2->ISR & USART_ISR_RXNE) == 0U)
    {
        return UART_NO_DATA;
    }

    /*
     * Reading RDR clears RXNE.
     */
    *data = (uint8_t)(USART2->RDR & 0xFFU);

    return UART_OK;
}
/*
 * Transmit multiple bytes.
 */
uart_status_t uart_transmit(const uint8_t *data, uint32_t length)
{
    if (data == 0U)
    {
        return UART_ERROR_CONFIG;
    }

    for (uint32_t i = 0U; i < length; i++)
    {
        if (uart_transmit_byte(data[i]) != UART_OK)
        {
            return UART_ERROR_TIMEOUT;
        }
    }
    while ((USART2->ISR & USART_ISR_TC) == 0U)
    {
    }

    return UART_OK;
}


/*
 * Receive one byte.
 */
uart_status_t uart_receive_byte(uint8_t *data)
{
    if (data == 0U)
    {
        return UART_ERROR_CONFIG;
    }

    /*
     * Wait until RXNE indicates that data has arrived.
     */
    while ((USART2->ISR & USART_ISR_RXNE) == 0U)
    {
    }

    /*
     * Read received byte.
     */
    *data = (uint8_t)USART2->RDR;

    return UART_OK;
}


/*
 * Check USART2 configuration.
 */
uart_status_t uart_health_check(void)
{
    /*
     * USART2 clock must be enabled.
     */
    if ((RCC->APB1ENR1 & RCC_APB1ENR1_USART2EN) == 0U)
    {
        return UART_ERROR_CLOCK;
    }


    /*
     * GPIOA clock must be enabled.
     */
    if ((RCC->AHB2ENR & RCC_AHB2ENR_GPIOAEN) == 0U)
    {
        return UART_ERROR_CLOCK;
    }


    /*
     * PA2 must be Alternate Function mode.
     */
    if ((UART_PORT->MODER & (3UL << (UART_TX_PIN * 2U))) !=
        (2UL << (UART_TX_PIN * 2U)))
    {
        return UART_ERROR_GPIO;
    }


    /*
     * PA3 must be Alternate Function mode.
     */
    if ((UART_PORT->MODER & (3UL << (UART_RX_PIN * 2U))) !=
        (2UL << (UART_RX_PIN * 2U)))
    {
        return UART_ERROR_GPIO;
    }


    /*
     * Verify USART is enabled.
     */
    if ((USART2->CR1 & USART_CR1_UE) == 0U)
    {
        return UART_ERROR_CONFIG;
    }


    /*
     * Verify transmitter enabled.
     */
    if ((USART2->CR1 & USART_CR1_TE) == 0U)
    {
        return UART_ERROR_CONFIG;
    }


    /*
     * Verify receiver enabled.
     */
    if ((USART2->CR1 & USART_CR1_RE) == 0U)
    {
        return UART_ERROR_CONFIG;
    }


    /*
     * Verify baud-rate register.
     */
    if (USART2->BRR != UART_BRR_VALUE)
    {
        return UART_ERROR_CONFIG;
    }


    return UART_OK;
}
