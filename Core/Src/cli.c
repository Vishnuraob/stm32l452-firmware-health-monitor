#include "cli.h"
#include "usart.h"
#include "peripheral_monitor.h"
#include "fault_manager.h"
#include "fault_logger.h"
#include "memory_monitor.h"
#include "ram_test.h"
#include "reset_manager.h"
#include "system_time.h"
#include "adc_monitor.h"

#include <string.h>
#include <stdio.h>

static uint8_t cli_buffer[CLI_BUFFER_SIZE];
static uint32_t cli_index;
static uint8_t cli_initialized;

static void cli_command_help(void);
static void cli_command_status(void);
static void cli_command_version(void);
static void cli_command_uptime(void);
static void cli_command_memory(void);
static void cli_command_errors(void);
static void cli_command_reset_reason(void);
static void cli_command_selftest(void);
static void cli_command_get_log(void);
static void cli_command_clear_errors(void);
static void cli_process_command(void);

#define FIRMWARE_NAME     "STM32L452 Firmware Health Monitor"
#define FIRMWARE_VERSION  "1.0.0"
#define FIRMWARE_BUILD    "Development"
/* =========================================================
 * Local helper functions
 * ========================================================= */

static void cli_transmit_string(const char *string)
{
    if (string == NULL)
    {
        return;
    }

    uart_transmit((const uint8_t *)string,
                  (uint32_t)strlen(string));
}


static void cli_print_status(const char *name, uint8_t healthy)
{
    cli_transmit_string(name);

    if (healthy != 0U)
    {
        cli_transmit_string(": OK\r\n");
    }
    else
    {
        cli_transmit_string(": FAULT\r\n");
    }
}


/* =========================================================
 * STATUS command
 * ========================================================= */

static void cli_command_status(void)
{
    peripheral_monitor_info_t peripheral_info;
    char buffer[64];

    cli_transmit_string("\r\nSystem Status\r\n");
    cli_transmit_string("------------------------------\r\n");

    /*
     * Get current peripheral health information.
     */
    if (peripheral_monitor_get_info(&peripheral_info)
        == PERIPHERAL_MONITOR_OK)
    {
        cli_print_status(
            "Clock       ",
            peripheral_info.clock_status == PERIPHERAL_MONITOR_OK);

        cli_print_status(
            "GPIO        ",
            peripheral_info.gpio_status == PERIPHERAL_MONITOR_OK);

        cli_print_status(
            "SysTick     ",
            peripheral_info.systick_status == PERIPHERAL_MONITOR_OK);

        cli_print_status(
            "UART        ",
            peripheral_info.uart_status == PERIPHERAL_MONITOR_OK);

        cli_print_status(
            "ADC         ",
            peripheral_info.adc_status == PERIPHERAL_MONITOR_OK);

        cli_print_status(
            "DMA         ",
            peripheral_info.dma_status == PERIPHERAL_MONITOR_OK);

        cli_print_status(
            "Watchdog    ",
            peripheral_info.watchdog_status == PERIPHERAL_MONITOR_OK);
        snprintf(buffer, sizeof(buffer),
                 "Temperature : %ld C\r\n",
                 (long)adc_monitor_get_temperature_c());

        cli_transmit_string(buffer);
    }
    else
    {
        cli_transmit_string("Peripheral  : ERROR\r\n");
    }

    /*
     * Fault Manager.
     */
    cli_print_status(
        "Fault Manager",
        fault_manager_health_check() == FAULT_MANAGER_OK);

    /*
     * Fault Logger.
     */
    cli_print_status(
        "Fault Logger ",
        fault_logger_health_check() == FAULT_LOGGER_OK);

    /*
     * Memory Monitor.
     */
    cli_print_status(
        "Memory      ",
        memory_monitor_health_check() == MEMORY_MONITOR_OK);

    /*
     * RAM Test.
     */
    cli_print_status(
        "RAM Test    ",
        ram_test_health_check() == RAM_TEST_OK);

    /*
     * Overall fault state.
     */
    if (fault_manager_any_active() != 0U)
    {
        cli_transmit_string("Fault State : ACTIVE\r\n");
    }
    else
    {
        cli_transmit_string("Fault State : NONE\r\n");
    }
}


/* =========================================================
 * HELP command
 * ========================================================= */

static void cli_command_help(void)
{
    cli_transmit_string("\r\nAvailable commands:\r\n");
    cli_transmit_string("HELP\r\n");
    cli_transmit_string("STATUS\r\n");
    cli_transmit_string("VERSION\r\n");
    cli_transmit_string("UPTIME\r\n");
    cli_transmit_string("MEMORY\r\n");
    cli_transmit_string("ERRORS\r\n");
    cli_transmit_string("RESET_REASON\r\n");
    cli_transmit_string("SELFTEST\r\n");
    cli_transmit_string("GET_LOG\r\n");
    cli_transmit_string("CLEAR_ERRORS\r\n");
}

static void cli_command_uptime(void)
{
    uint32_t uptime_ms;
    uint32_t total_seconds;
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;

    uptime_ms = system_time_get_ms();

    total_seconds = uptime_ms / 1000U;

    hours = total_seconds / 3600U;

    minutes = (total_seconds % 3600U) / 60U;

    seconds = total_seconds % 60U;

    char buffer[64];

    snprintf(buffer,
             sizeof(buffer),
             "\r\nUptime: %lu:%02lu:%02lu\r\n",
             (unsigned long)hours,
             (unsigned long)minutes,
             (unsigned long)seconds);

    cli_transmit_string(buffer);
}

static void cli_command_memory(void)
{
    memory_monitor_info_t memory_info;
    char buffer[96];

    if (memory_monitor_get_info(&memory_info) != MEMORY_MONITOR_OK)
    {
        cli_transmit_string("\r\nMemory information unavailable\r\n");
        return;
    }

    cli_transmit_string("\r\nMemory Information\r\n");
    cli_transmit_string("------------------------------\r\n");

    snprintf(buffer,
             sizeof(buffer),
             "RAM Start       : 0x%08lX\r\n",
             (unsigned long)memory_info.ram_start);
    cli_transmit_string(buffer);

    snprintf(buffer,
             sizeof(buffer),
             "RAM End         : 0x%08lX\r\n",
             (unsigned long)memory_info.ram_end);
    cli_transmit_string(buffer);

    snprintf(buffer,
             sizeof(buffer),
             "RAM Size        : %lu bytes\r\n",
             (unsigned long)memory_info.ram_size);
    cli_transmit_string(buffer);

    snprintf(buffer,
             sizeof(buffer),
             "Stack Top       : 0x%08lX\r\n",
             (unsigned long)memory_info.stack_top);
    cli_transmit_string(buffer);

    snprintf(buffer,
             sizeof(buffer),
             "Stack Pointer   : 0x%08lX\r\n",
             (unsigned long)memory_info.stack_pointer);
    cli_transmit_string(buffer);

    snprintf(buffer,
             sizeof(buffer),
             "Stack Reserved  : %lu bytes\r\n",
             (unsigned long)memory_info.stack_reserved);
    cli_transmit_string(buffer);

    snprintf(buffer,
             sizeof(buffer),
             "Stack Available : %lu bytes\r\n",
             (unsigned long)memory_info.stack_available);
    cli_transmit_string(buffer);

    snprintf(buffer,
             sizeof(buffer),
             "Heap Start      : 0x%08lX\r\n",
             (unsigned long)memory_info.heap_start);
    cli_transmit_string(buffer);

    snprintf(buffer,
             sizeof(buffer),
             "Heap Reserved   : %lu bytes\r\n",
             (unsigned long)memory_info.heap_reserved);
    cli_transmit_string(buffer);

    if (memory_monitor_fault_active() != 0U)
    {
        cli_transmit_string("Memory Status   : FAULT\r\n");
    }
    else
    {
        cli_transmit_string("Memory Status   : OK\r\n");
    }
}


static void cli_command_errors(void)
{
    fault_record_t fault;
    uint32_t i;
    uint32_t active_count = 0U;
    uint32_t log_count;

    cli_transmit_string("\r\nFault Information\r\n");
    cli_transmit_string("------------------------------\r\n");

    /*
     * Check every possible fault code.
     */
    for (i = 1U; i < (uint32_t)FAULT_CODE_COUNT; i++)
    {
        if (fault_manager_is_active((fault_code_t)i) != 0U)
        {
            if (fault_manager_get_record(
                    (fault_code_t)i,
                    &fault) == FAULT_MANAGER_OK)
            {
                active_count++;

                cli_transmit_string("Fault Code      : ");

                switch (fault.code)
                {
                    case FAULT_CODE_CLOCK:
                        cli_transmit_string("CLOCK");
                        break;

                    case FAULT_CODE_GPIO:
                        cli_transmit_string("GPIO");
                        break;

                    case FAULT_CODE_SYSTICK:
                        cli_transmit_string("SYSTICK");
                        break;

                    case FAULT_CODE_UART:
                        cli_transmit_string("UART");
                        break;

                    case FAULT_CODE_ADC:
                        cli_transmit_string("ADC");
                        break;

                    case FAULT_CODE_DMA:
                        cli_transmit_string("DMA");
                        break;

                    case FAULT_CODE_WATCHDOG:
                        cli_transmit_string("WATCHDOG");
                        break;

                    case FAULT_CODE_HEARTBEAT:
                        cli_transmit_string("HEARTBEAT");
                        break;

                    case FAULT_CODE_MEMORY:
                        cli_transmit_string("MEMORY");
                        break;

                    case FAULT_CODE_RAM:
                        cli_transmit_string("RAM");
                        break;
                    case FAULT_CODE_STACK_OVERFLOW:
						cli_transmit_string("STACK_OVERFLOW");
						break;
                    default:
                        cli_transmit_string("UNKNOWN");
                        break;
                }

                cli_transmit_string("\r\n");

                cli_transmit_string("Source          : ");

                switch (fault.source)
                {
                    case FAULT_SOURCE_PERIPHERAL:
                        cli_transmit_string("PERIPHERAL");
                        break;

                    case FAULT_SOURCE_HEARTBEAT:
                        cli_transmit_string("HEARTBEAT");
                        break;

                    case FAULT_SOURCE_MEMORY:
                        cli_transmit_string("MEMORY");
                        break;

                    case FAULT_SOURCE_RAM:
                        cli_transmit_string("RAM");
                        break;

                    case FAULT_SOURCE_WATCHDOG:
                        cli_transmit_string("WATCHDOG");
                        break;

                    default:
                        cli_transmit_string("UNKNOWN");
                        break;
                }

                cli_transmit_string("\r\n");

                cli_transmit_string("Severity        : ");

                switch (fault.severity)
                {
                    case FAULT_SEVERITY_INFO:
                        cli_transmit_string("INFO");
                        break;

                    case FAULT_SEVERITY_WARNING:
                        cli_transmit_string("WARNING");
                        break;

                    case FAULT_SEVERITY_CRITICAL:
                        cli_transmit_string("CRITICAL");
                        break;

                    default:
                        cli_transmit_string("UNKNOWN");
                        break;
                }

                cli_transmit_string("\r\n");

                {
                    char buffer[64];

                    snprintf(buffer,
                             sizeof(buffer),
                             "Timestamp       : %lu ms\r\n",
                             (unsigned long)fault.timestamp_ms);

                    cli_transmit_string(buffer);

                    snprintf(buffer,
                             sizeof(buffer),
                             "Occurrences     : %lu\r\n",
                             (unsigned long)fault.occurrence_count);

                    cli_transmit_string(buffer);
                }

                cli_transmit_string("------------------------------\r\n");
            }
        }
    }

    if (active_count == 0U)
    {
        cli_transmit_string("Active Faults   : NONE\r\n");
    }
    else
    {
        char buffer[64];

        snprintf(buffer,
                 sizeof(buffer),
                 "Active Faults   : %lu\r\n",
                 (unsigned long)active_count);

        cli_transmit_string(buffer);
    }

    /*
     * RAM fault logger contains the fault history
     * for this boot.
     */
    log_count = fault_logger_get_count();

    {
        char buffer[64];

        snprintf(buffer,
                 sizeof(buffer),
                 "Log Entries     : %lu / %u\r\n",
                 (unsigned long)log_count,
                 FAULT_LOGGER_MAX_ENTRIES);

        cli_transmit_string(buffer);
    }
}


static void cli_command_reset_reason(void)
{
    reset_reason_t reason;
    uint32_t raw_flags;
    char buffer[64];

    reason = reset_manager_get_reason();

    raw_flags = reset_manager_get_raw_flags();

    cli_transmit_string("\r\nReset Information\r\n");
    cli_transmit_string("------------------------------\r\n");

    cli_transmit_string("Reset Reason    : ");

    switch (reason)
    {
        case RESET_REASON_POWER_ON:
            cli_transmit_string("POWER_ON");
            break;

        case RESET_REASON_BROWN_OUT:
            cli_transmit_string("BROWN_OUT");
            break;

        case RESET_REASON_PIN:
            cli_transmit_string("PIN_RESET");
            break;

        case RESET_REASON_SOFTWARE:
            cli_transmit_string("SOFTWARE");
            break;

        case RESET_REASON_IWDG:
            cli_transmit_string("IWDG");
            break;

        case RESET_REASON_WWDG:
            cli_transmit_string("WWDG");
            break;

        case RESET_REASON_LOW_POWER:
            cli_transmit_string("LOW_POWER");
            break;

        case RESET_REASON_UNKNOWN:
        default:
            cli_transmit_string("UNKNOWN");
            break;
    }

    cli_transmit_string("\r\n");

    snprintf(buffer,
             sizeof(buffer),
             "Raw RCC->CSR    : 0x%08lX\r\n",
             (unsigned long)raw_flags);

    cli_transmit_string(buffer);
}

static void cli_command_selftest(void)
{
    ram_test_status_t status;

    cli_transmit_string("\r\nRunning RAM self-test...\r\n");

    status = ram_test_run();

    if (status == RAM_TEST_OK)
    {
        cli_transmit_string("RAM Self-Test  : PASS\r\n");
    }
    else
    {
        cli_transmit_string("RAM Self-Test  : FAIL\r\n");

        switch (status)
        {
            case RAM_TEST_FAULT_WRITE_READ:
                cli_transmit_string("Failure Type   : WRITE/READ\r\n");
                break;

            case RAM_TEST_FAULT_ADDRESS:
                cli_transmit_string("Failure Type   : ADDRESS\r\n");
                break;

            case RAM_TEST_ERROR_NOT_INITIALIZED:
                cli_transmit_string("Failure Type   : NOT INITIALIZED\r\n");
                break;

            case RAM_TEST_ERROR_PARAMETER:
                cli_transmit_string("Failure Type   : PARAMETER\r\n");
                break;

            default:
                cli_transmit_string("Failure Type   : UNKNOWN\r\n");
                break;
        }
    }

    if (ram_test_fault_active() != 0U)
    {
        cli_transmit_string("RAM Fault      : ACTIVE\r\n");
    }
    else
    {
        cli_transmit_string("RAM Fault      : NONE\r\n");
    }
}

static void cli_command_get_log(void)
{
    fault_log_entry_t entry;
    uint32_t count;
    uint32_t i;
    char buffer[96];

    count = fault_logger_get_count();

    cli_transmit_string("\r\nFault Log\r\n");
    cli_transmit_string("------------------------------\r\n");

    if (count == 0U)
    {
        cli_transmit_string("No fault log entries\r\n");
        return;
    }

    for (i = 0U; i < count; i++)
    {
        if (fault_logger_get(i, &entry) != FAULT_LOGGER_OK)
        {
            continue;
        }

        snprintf(buffer,
                 sizeof(buffer),
                 "Sequence        : %lu\r\n",
                 (unsigned long)entry.sequence);
        cli_transmit_string(buffer);

        cli_transmit_string("Fault Code      : ");

        switch (entry.code)
        {
            case FAULT_CODE_CLOCK:
                cli_transmit_string("CLOCK");
                break;

            case FAULT_CODE_GPIO:
                cli_transmit_string("GPIO");
                break;

            case FAULT_CODE_SYSTICK:
                cli_transmit_string("SYSTICK");
                break;

            case FAULT_CODE_UART:
                cli_transmit_string("UART");
                break;

            case FAULT_CODE_ADC:
                cli_transmit_string("ADC");
                break;

            case FAULT_CODE_DMA:
                cli_transmit_string("DMA");
                break;

            case FAULT_CODE_WATCHDOG:
                cli_transmit_string("WATCHDOG");
                break;

            case FAULT_CODE_HEARTBEAT:
                cli_transmit_string("HEARTBEAT");
                break;

            case FAULT_CODE_MEMORY:
                cli_transmit_string("MEMORY");
                break;

            case FAULT_CODE_RAM:
                cli_transmit_string("RAM");
                break;

            case FAULT_CODE_STACK_OVERFLOW:
            	cli_transmit_string("STACK_OVERFLOW");
            	break;
            default:
                cli_transmit_string("UNKNOWN");
                break;
        }

        cli_transmit_string("\r\n");

        cli_transmit_string("Source          : ");

        switch (entry.source)
        {
            case FAULT_SOURCE_PERIPHERAL:
                cli_transmit_string("PERIPHERAL");
                break;

            case FAULT_SOURCE_HEARTBEAT:
                cli_transmit_string("HEARTBEAT");
                break;

            case FAULT_SOURCE_MEMORY:
                cli_transmit_string("MEMORY");
                break;

            case FAULT_SOURCE_RAM:
                cli_transmit_string("RAM");
                break;

            case FAULT_SOURCE_WATCHDOG:
                cli_transmit_string("WATCHDOG");
                break;

            default:
                cli_transmit_string("UNKNOWN");
                break;
        }

        cli_transmit_string("\r\n");

        cli_transmit_string("Severity        : ");

        switch (entry.severity)
        {
            case FAULT_SEVERITY_INFO:
                cli_transmit_string("INFO");
                break;

            case FAULT_SEVERITY_WARNING:
                cli_transmit_string("WARNING");
                break;

            case FAULT_SEVERITY_CRITICAL:
                cli_transmit_string("CRITICAL");
                break;

            default:
                cli_transmit_string("UNKNOWN");
                break;
        }

        cli_transmit_string("\r\n");

        snprintf(buffer,
                 sizeof(buffer),
                 "Timestamp       : %lu ms\r\n",
                 (unsigned long)entry.timestamp_ms);
        cli_transmit_string(buffer);

        snprintf(buffer,
                 sizeof(buffer),
                 "Occurrences     : %lu\r\n",
                 (unsigned long)entry.occurrence_count);
        cli_transmit_string(buffer);

        cli_transmit_string("------------------------------\r\n");
    }
}

static void cli_command_clear_errors(void)
{
    uint32_t code;

    for (code = 1U; code < FAULT_CODE_COUNT; code++)
    {
        (void)fault_manager_clear((fault_code_t)code);
    }

    (void)fault_logger_clear();

    cli_transmit_string("\r\nErrors cleared\r\n");
}


/* =========================================================
 * Command parser
 * ========================================================= */

static void cli_process_command(void)
{
    if (strcmp((char *)cli_buffer, "HELP") == 0)
    {
        cli_command_help();
    }
    else if (strcmp((char *)cli_buffer, "STATUS") == 0)
    {
        cli_command_status();
    }
    else if (strcmp((char *)cli_buffer, "VERSION") == 0)
    {
        cli_command_version();
    }
    else if (strcmp((char *)cli_buffer, "UPTIME") == 0)
    {
        cli_command_uptime();
    }
    else if (strcmp((char *)cli_buffer, "MEMORY") == 0)
    {
        cli_command_memory();
    }
    else if (strcmp((char *)cli_buffer, "ERRORS") == 0)
    {
        cli_command_errors();
    }
    else if (strcmp((char *)cli_buffer, "RESET_REASON") == 0)
    {
        cli_command_reset_reason();
    }
    else if (strcmp((char *)cli_buffer, "SELFTEST") == 0)
    {
        cli_command_selftest();
    }
    else if (strcmp((char *)cli_buffer, "GET_LOG") == 0)
    {
        cli_command_get_log();
    }
    else if (strcmp((char *)cli_buffer, "CLEAR_ERRORS") == 0)
    {
        cli_command_clear_errors();
    }
    else
    {
        cli_transmit_string("\r\nUnknown command\r\n");
        cli_transmit_string("Type HELP for available commands\r\n");
    }
}


/* =========================================================
 * CLI initialization
 * ========================================================= */

void cli_init(void)
{
    cli_index = 0U;
    cli_initialized = 1U;

    memset(cli_buffer, 0, sizeof(cli_buffer));

    cli_transmit_string("\r\n");
    cli_transmit_string("================================\r\n");
    cli_transmit_string(" STM32L452 Firmware Health Monitor\r\n");
    cli_transmit_string(" CLI Ready\r\n");
    cli_transmit_string(" Type HELP for commands\r\n");
    cli_transmit_string("================================\r\n");
    cli_transmit_string("> ");
}

static void cli_command_version(void)
{
    cli_transmit_string("\r\nFirmware Information\r\n");
    cli_transmit_string("------------------------------\r\n");

    cli_transmit_string("Name    : ");
    cli_transmit_string(FIRMWARE_NAME);
    cli_transmit_string("\r\n");

    cli_transmit_string("Version : ");
    cli_transmit_string(FIRMWARE_VERSION);
    cli_transmit_string("\r\n");

    cli_transmit_string("Build   : ");
    cli_transmit_string(FIRMWARE_BUILD);
    cli_transmit_string("\r\n");
}
/* =========================================================
 * Character processing
 * ========================================================= */

cli_status_t cli_receive_char(uint8_t character)
{
    if (cli_initialized == 0U)
    {
        return CLI_ERROR_NOT_INITIALIZED;
    }

    /*
     * ENTER
     */
    if ((character == '\r') || (character == '\n'))
    {
        if (cli_index > 0U)
        {
            cli_buffer[cli_index] = '\0';

            cli_process_command();

            cli_index = 0U;

            memset(cli_buffer, 0, sizeof(cli_buffer));

            cli_transmit_string("\r\n> ");
        }

        return CLI_OK;
    }


    /*
     * BACKSPACE
     */
    if ((character == '\b') || (character == 127U))
    {
        if (cli_index > 0U)
        {
            cli_index--;

            cli_buffer[cli_index] = '\0';

            {
                static const uint8_t backspace_sequence[] = "\b \b";

                uart_transmit(
                    backspace_sequence,
                    sizeof(backspace_sequence) - 1U);
            }
        }

        return CLI_OK;
    }


    /*
     * Buffer overflow protection.
     */
    if (cli_index >= (CLI_BUFFER_SIZE - 1U))
    {
        cli_transmit_string("\r\nCommand too long\r\n");

        cli_index = 0U;

        memset(cli_buffer, 0, sizeof(cli_buffer));

        cli_transmit_string("> ");

        return CLI_ERROR_OVERFLOW;
    }


    /*
     * Store character.
     */
    cli_buffer[cli_index] = character;

    cli_index++;


    /*
     * Echo character.
     */
    uart_transmit_byte(character);

    return CLI_OK;
}


/* =========================================================
 * CLI processing
 * ========================================================= */

void cli_process(void)
{
    uint8_t character;

    if (cli_initialized == 0U)
    {
        return;
    }

    /*
     * Non-blocking UART receive.
     */
    while(uart_receive_byte_nonblocking(&character) == UART_OK)
    {
        (void)cli_receive_char(character);
    }
}
