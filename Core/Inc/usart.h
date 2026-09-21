/*
 * usart.h
 *
 * Created on: 13-Jul-2026
 * Author: engineer
 */

#ifndef INC_USART_H_
#define INC_USART_H_

#include <stdint.h>

typedef enum
{
    UART_OK = 0,
    UART_ERROR_CLOCK,
    UART_ERROR_GPIO,
    UART_ERROR_CONFIG,
    UART_ERROR_TIMEOUT,
    UART_NO_DATA
} uart_status_t;

uart_status_t uart_init(void);

uart_status_t uart_transmit_byte(uint8_t data);

uart_status_t uart_transmit(const uint8_t *data, uint32_t length);

uart_status_t uart_receive_byte(uint8_t *data);

/*
 * Non-blocking receive.
 *
 * UART_OK     -> byte received
 * UART_NO_DATA -> no byte currently available
 */
uart_status_t uart_receive_byte_nonblocking(uint8_t *data);

uart_status_t uart_health_check(void);

#endif /* INC_USART_H_ */
