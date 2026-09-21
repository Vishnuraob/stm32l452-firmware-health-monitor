#ifndef CLI_H
#define CLI_H

#include <stdint.h>

#define CLI_BUFFER_SIZE    64U

typedef enum
{
    CLI_OK = 0,
    CLI_ERROR_NOT_INITIALIZED,
    CLI_ERROR_PARAMETER,
    CLI_ERROR_OVERFLOW
} cli_status_t;

void cli_init(void);

void cli_process(void);

cli_status_t cli_receive_char(uint8_t character);

#endif /* CLI_H */
