#include "main.h"

#include "clock.h"
#include "led.h"
#include "system_time.h"
#include "usart.h"
#include "adc.h"
#include "dma.h"
#include "adc_monitor.h"
#include "heartbeat_monitor.h"
#include "reset_manager.h"
#include "watchdog_manager.h"
#include "memory_monitor.h"
#include "ram_test.h"
#include "peripheral_monitor.h"
#include "fault_manager.h"
#include "fault_logger.h"
#include "cli.h"

#include <stdint.h>


/* ============================================================
 * Configuration
 * ============================================================ */

#define ADC_DMA_BUFFER_SIZE    3U
#define HEARTBEAT_TIMEOUT_MS   2000U
#define WATCHDOG_TIMEOUT_MS    2000U

static void debug_stack_overflow_test(void);

/* ============================================================
 * ADC DMA Buffer
 *
 * Rank 1 -> VREFINT
 * Rank 2 -> Temperature sensor
 * Rank 3 -> VBAT
 * ============================================================ */

static volatile uint16_t adc_dma_buffer[ADC_DMA_BUFFER_SIZE];

static uint32_t last_led_toggle_ms = 0U;
uint32_t current_time_ms;
/* ============================================================
 * Memory Monitor Configuration
 * ============================================================ */

static const memory_monitor_config_t memory_monitor_config =
{
    .stack_warning_bytes = 256U
};


/* ============================================================
 * ADC Monitor Configuration
 * ============================================================ */

static const adc_monitor_config_t adc_monitor_config =
{
    .vdda_min_mv       = 2800U,
    .vdda_max_mv       = 3600U,

    .temperature_min_c = 0,
    .temperature_max_c = 85,

    .vbat_min_mv       = 2500U,
    .vbat_max_mv       = 3600U
};


/* ============================================================
 * Debug Variables
 *
 * Non-static so STM32CubeIDE debugger can inspect them easily.
 * ============================================================ */


/* Reset */

volatile reset_reason_t debug_reset_reason;
volatile uint32_t debug_reset_flags;


/* ADC monitor */

volatile uint32_t debug_vdda_mv;
volatile int32_t  debug_temperature_c;
volatile uint32_t debug_vbat_mv;


/* Heartbeat */

volatile uint32_t debug_heartbeat_count;
volatile uint32_t debug_heartbeat_elapsed_ms;
volatile uint8_t debug_heartbeat_fault;
volatile heartbeat_status_t debug_heartbeat_status;


/* Watchdog */

volatile watchdog_status_t debug_watchdog_status;
volatile watchdog_status_t debug_watchdog_health;
volatile uint8_t debug_watchdog_fault;


/* Memory */

volatile uint32_t debug_msp;
volatile uint32_t debug_stack_available;
volatile uint32_t debug_ram_start;
volatile uint32_t debug_ram_end;
volatile uint32_t debug_ram_size;
volatile memory_monitor_status_t debug_memory_status;


/* RAM test */

volatile ram_test_status_t debug_ram_test_status;
volatile uint8_t debug_ram_test_fault;
volatile uint32_t debug_ram_test_start;
volatile uint32_t debug_ram_test_end;
volatile uint32_t debug_ram_test_size;

volatile uint32_t debug_ram_fault_address;
volatile uint32_t debug_ram_fault_expected;
volatile uint32_t debug_ram_fault_actual;
volatile ram_test_type_t debug_ram_fault_test_type;
volatile uint8_t debug_ram_fault_injected;


/* Peripheral monitor */

volatile peripheral_monitor_status_t debug_peripheral_status;
volatile uint8_t debug_peripheral_all_healthy;
volatile peripheral_monitor_info_t debug_peripheral_info;


/* ADC health */

volatile adc_status_t debug_adc_status;


/* ============================================================
 * Fault Manager Debug Variables
 * ============================================================ */

volatile fault_manager_status_t debug_fault_manager_status;
volatile fault_record_t debug_last_fault;

volatile uint32_t debug_fault_code;
volatile uint32_t debug_fault_source;
volatile uint32_t debug_fault_severity;
volatile uint32_t debug_fault_timestamp;
volatile uint32_t debug_fault_occurrence_count;
volatile uint32_t debug_fault_active;

volatile uint32_t debug_adc_fault_count;
volatile uint32_t debug_heartbeat_fault_count;


/* Fault Logger */

volatile fault_logger_status_t debug_fault_logger_status;
volatile fault_logger_status_t debug_fault_logger_health;
volatile uint32_t debug_fault_log_count;

volatile fault_log_entry_t debug_latest_fault_log;
volatile fault_log_entry_t debug_fault_log_entry;

/* ============================================================
 * Temporary Fault Injection
 *
 * Set to 1 in debugger to simulate an ADC peripheral fault.
 * ============================================================ */

volatile uint8_t debug_fault_injection = 0U;

volatile uint8_t debug_stack_overflow = 0U;

/* ============================================================
 * Helper
 *
 * Copies the latest Fault Manager record into debugger-friendly
 * scalar variables.
 * ============================================================ */

static void fault_manager_update_debug_variables(void)
{
    fault_manager_get_last_fault(
        &debug_last_fault
    );


    debug_fault_code =
        (uint32_t)debug_last_fault.code;

    debug_fault_source =
        (uint32_t)debug_last_fault.source;

    debug_fault_severity =
        (uint32_t)debug_last_fault.severity;

    debug_fault_timestamp =
        debug_last_fault.timestamp_ms;

    debug_fault_occurrence_count =
        debug_last_fault.occurrence_count;

    debug_fault_active =
        (uint32_t)debug_last_fault.active;


    debug_adc_fault_count =
        fault_manager_get_count(
            FAULT_CODE_ADC
        );


    debug_heartbeat_fault_count =
        fault_manager_get_count(
            FAULT_CODE_HEARTBEAT
        );


    debug_fault_log_count =
        fault_logger_get_count();


    if (debug_fault_log_count != 0U)
    {
        fault_logger_get_latest(
            &debug_latest_fault_log
        );
    }
}


/* ============================================================
 * Main
 * ============================================================ */

int main(void)
{
    /* --------------------------------------------------------
     * Local status variables
     * -------------------------------------------------------- */

    clock_status_t clock_status;
    led_status_t led_status;
    system_time_status_t time_status;

    uart_status_t uart_status;

    adc_status_t adc_status;
    adc_status_t adc_dma_status;

    dma_status_t dma_status;
    dma_status_t dma_start_status;

    adc_monitor_status_t adc_monitor_status;

    heartbeat_status_t heartbeat_status;

    reset_manager_status_t reset_status;

    ram_test_status_t ram_status;

    peripheral_monitor_status_t peripheral_status;

    fault_manager_status_t fault_manager_status;


    /* ========================================================
     * RESET CAUSE
     * ======================================================== */

    reset_status = reset_manager_init();

    if (reset_status != RESET_MANAGER_OK)
    {
        while (1)
        {
        }
    }


    debug_reset_reason =
        reset_manager_get_reason();

    debug_reset_flags =
        reset_manager_get_raw_flags();


    /* ========================================================
     * CLOCK
     * ======================================================== */

    clock_status = clock_init();

    if (clock_status != CLOCK_OK)
    {
        while (1)
        {
        }
    }


    /* ========================================================
     * LED / GPIO
     * ======================================================== */

    led_status = led_init();

    if (led_status != LED_OK)
    {
        while (1)
        {
        }
    }


    /* ========================================================
     * SYSTICK / SYSTEM TIME
     * ======================================================== */

    time_status = system_time_init();

    if (time_status != SYSTEM_TIME_OK)
    {
        while (1)
        {
            led_toggle();
        }
    }


    /* ========================================================
     * UART
     * ======================================================== */

    uart_status = uart_init();

    if (uart_status != UART_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }

    cli_init();
    /* ========================================================
     * ADC INITIALIZATION
     * ======================================================== */

    adc_status = adc_init();

    if (adc_status != ADC_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }


    /* ========================================================
     * DMA INITIALIZATION
     * ======================================================== */

    dma_status = dma_adc_init(
        adc_dma_buffer,
        ADC_DMA_BUFFER_SIZE
    );

    if (dma_status != DMA_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }


    /* ========================================================
     * ADC INTERNAL CHANNEL CONFIGURATION
     *
     * VREFINT
     * Temperature sensor
     * VBAT
     * ======================================================== */

    adc_dma_status =
        adc_configure_internal_channels_dma();

    if (adc_dma_status != ADC_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }


    /* ========================================================
     * DMA CONFIGURATION HEALTH CHECK
     * ======================================================== */

    dma_status =
        dma_adc_health_check();

    if (dma_status != DMA_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }


    /* ========================================================
     * START ADC + DMA
     * ======================================================== */

    dma_start_status =
        dma_adc_start();

    if (dma_start_status != DMA_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }


    /* ========================================================
     * ADC HEALTH CHECK
     * ======================================================== */

    adc_status =
        adc_health_check();

    debug_adc_status =
        adc_status;

    if (adc_status != ADC_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }


    /* ========================================================
     * ADC VALUE MONITOR
     * ======================================================== */

    adc_monitor_status =
        adc_monitor_init(
            &adc_monitor_config
        );

    if (adc_monitor_status != ADC_MONITOR_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }


    /* ========================================================
     * APPLICATION HEARTBEAT
     * ======================================================== */

    heartbeat_status =
        heartbeat_monitor_init(
            HEARTBEAT_TIMEOUT_MS
        );

    if (heartbeat_status != HEARTBEAT_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }

    /* ========================================================
     * WATCHDOG
     * ======================================================== */

    watchdog_status_t watchdog_status;

    watchdog_status = watchdog_manager_init(WATCHDOG_TIMEOUT_MS);
    if (watchdog_status != WATCHDOG_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }

    debug_watchdog_status = watchdog_status;
    debug_watchdog_health = watchdog_manager_health_check();
    debug_watchdog_fault = 0U;

    /* ========================================================
     * MEMORY MONITOR
     * ======================================================== */

    debug_memory_status =
        memory_monitor_init(
            &memory_monitor_config
        );

    if (debug_memory_status != MEMORY_MONITOR_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }


    /* ========================================================
     * FAULT MANAGER
     *
     * Initialize before RAM self-test so RAM faults can
     * be reported to the Fault Manager.
     * ======================================================== */

    fault_manager_status =
        fault_manager_init();

    if (fault_manager_status != FAULT_MANAGER_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }


    debug_fault_manager_status =
        fault_manager_health_check();


    /* --------------------------------------------------------
     * Initialize Fault Manager debug values
     * -------------------------------------------------------- */

    debug_fault_code = 0U;
    debug_fault_source = 0U;
    debug_fault_severity = 0U;
    debug_fault_timestamp = 0U;
    debug_fault_occurrence_count = 0U;
    debug_fault_active = 0U;

    debug_adc_fault_count = 0U;
    debug_heartbeat_fault_count = 0U;


    /* ========================================================
     * FAULT LOGGER INITIALIZATION
     * ======================================================== */

    debug_fault_logger_status =
        fault_logger_init();

    if (debug_fault_logger_status != FAULT_LOGGER_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }


    debug_fault_logger_health =
        fault_logger_health_check();

    if (debug_fault_logger_health != FAULT_LOGGER_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }


    debug_fault_log_count =
        fault_logger_get_count();


    /* ========================================================
     * RAM SELF TEST
     * ======================================================== */

    ram_status =
        ram_test_init();

    if (ram_status != RAM_TEST_OK)
    {
        fault_manager_report(
            FAULT_CODE_RAM,
            FAULT_SOURCE_RAM,
            FAULT_SEVERITY_CRITICAL
        );

        fault_manager_update_debug_variables();

        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }


    debug_ram_test_status =
        ram_test_run();

    debug_ram_test_fault =
        ram_test_fault_active();


    /* ========================================================
     * RAM SELF-TEST FAULT REPORTING
     * ======================================================== */

    if (debug_ram_test_status != RAM_TEST_OK)
    {
        fault_manager_report(
            FAULT_CODE_RAM,
            FAULT_SOURCE_RAM,
            FAULT_SEVERITY_CRITICAL
        );

        fault_manager_update_debug_variables();
    }


    /* --------------------------------------------------------
     * RAM fault information
     * -------------------------------------------------------- */

    ram_test_fault_info_t ram_fault_info;

    ram_test_get_fault_info(
        &ram_fault_info
    );


    debug_ram_fault_address =
        ram_fault_info.address;

    debug_ram_fault_expected =
        ram_fault_info.expected;

    debug_ram_fault_actual =
        ram_fault_info.actual;

    debug_ram_fault_test_type =
        ram_fault_info.test_type;


    /* --------------------------------------------------------
     * RAM test region information
     * -------------------------------------------------------- */

    ram_test_info_t ram_test_info;

    ram_test_get_info(
        &ram_test_info
    );


    debug_ram_test_start =
        ram_test_info.start_address;

    debug_ram_test_end =
        ram_test_info.end_address;

    debug_ram_test_size =
        ram_test_info.size_bytes;


    /* ========================================================
     * PERIPHERAL HEALTH MONITOR
     * ======================================================== */

    peripheral_status =
        peripheral_monitor_init();

    if (peripheral_status != PERIPHERAL_MONITOR_OK)
    {
        while (1)
        {
            led_toggle();
            system_time_delay_ms(100U);
        }
    }


    /* --------------------------------------------------------
     * Initial peripheral health check
     * -------------------------------------------------------- */

    peripheral_status =
        peripheral_monitor_update();

    debug_peripheral_status =
        peripheral_status;

    debug_peripheral_all_healthy =
        peripheral_monitor_all_healthy();

    peripheral_monitor_get_info(
        &debug_peripheral_info
    );




    /* ========================================================
     * MAIN LOOP
     * ======================================================== */

    while (1)
    {


        cli_process();
        /* ====================================================
         * ADC / DMA MONITORING
         * ==================================================== */

        if (dma_adc_transfer_complete() != 0U)
        {
            adc_monitor_status =
                adc_monitor_update(
                    adc_dma_buffer
                );


            debug_vdda_mv =
                adc_monitor_get_vdda_mv();

            debug_temperature_c =
                adc_monitor_get_temperature_c();

            debug_vbat_mv =
                adc_monitor_get_vbat_mv();


            if (adc_monitor_status != ADC_MONITOR_OK)
            {
                __NOP();
            }


            dma_adc_clear_transfer_complete();
        }


        /* ====================================================
         * MEMORY MONITORING
         * ==================================================== */

        debug_memory_status =
            memory_monitor_update();

        debug_msp =
            memory_monitor_get_stack_pointer();

        debug_stack_available =
            memory_monitor_get_stack_available();

        debug_ram_start =
            memory_monitor_get_ram_start();

        debug_ram_end =
            memory_monitor_get_ram_end();

        debug_ram_size =
            memory_monitor_get_ram_size();


        /* ====================================================
         * MEMORY FAULT REPORTING
         * ==================================================== */

        if (debug_memory_status != MEMORY_MONITOR_OK)
        {
            fault_manager_report(
                FAULT_CODE_MEMORY,
                FAULT_SOURCE_MEMORY,
                FAULT_SEVERITY_CRITICAL
            );
        }

        if (memory_monitor_stack_overflow() != 0U)
        {
            (void)fault_manager_report(
                FAULT_CODE_STACK_OVERFLOW,
                FAULT_SOURCE_MEMORY,
                FAULT_SEVERITY_CRITICAL
            );
        }
        if (debug_stack_overflow != 0U)
        {
            debug_stack_overflow_test();

            debug_stack_overflow = 0U;
        }
        /* ====================================================
         * HEARTBEAT MONITORING
         * ==================================================== */

        debug_heartbeat_count =
            heartbeat_monitor_get_count();

        debug_heartbeat_elapsed_ms =
            heartbeat_monitor_get_elapsed_ms();

        debug_heartbeat_fault =
            heartbeat_monitor_fault_active();

        debug_heartbeat_status =
            heartbeat_monitor_health_check();


        /* ====================================================
         * HEARTBEAT FAULT REPORTING
         * ==================================================== */

        if (debug_heartbeat_fault != 0U)
        {
            fault_manager_report(
                FAULT_CODE_HEARTBEAT,
                FAULT_SOURCE_HEARTBEAT,
                FAULT_SEVERITY_CRITICAL
            );
        }


        /*
         * Normal application heartbeat.
         *
         * IMPORTANT:
         * Do not comment this out during normal operation.
         *
         * For heartbeat fault injection, temporarily comment
         * this line only during the test.
         */

        heartbeat_monitor_beat();

        /* ====================================================
         * PERIPHERAL HEALTH MONITOR
         * ==================================================== */

        peripheral_status =
            peripheral_monitor_update();

        debug_peripheral_status =
            peripheral_status;

        debug_peripheral_all_healthy =
            peripheral_monitor_all_healthy();

        peripheral_monitor_get_info(
            &debug_peripheral_info
        );


        /* ====================================================
         * TEMPORARY FAULT INJECTION
         *
         * Set:
         *
         * debug_fault_injection = 1
         *
         * in debugger to simulate an ADC fault.
         *
         * This modifies only the diagnostic copy.
         * It does NOT modify ADC hardware registers.
         * ==================================================== */

        if (debug_fault_injection != 0U)
        {
            debug_peripheral_info.adc_status =
                PERIPHERAL_MONITOR_FAULT_ADC;
        }


        /* ====================================================
         * FAULT MANAGER
         *
         * Peripheral faults are converted into Fault Manager
         * records.
         * ==================================================== */

        if (debug_peripheral_info.clock_status
                != PERIPHERAL_MONITOR_OK)
        {
            fault_manager_report(
                FAULT_CODE_CLOCK,
                FAULT_SOURCE_PERIPHERAL,
                FAULT_SEVERITY_CRITICAL
            );
        }


        if (debug_peripheral_info.gpio_status
                != PERIPHERAL_MONITOR_OK)
        {
            fault_manager_report(
                FAULT_CODE_GPIO,
                FAULT_SOURCE_PERIPHERAL,
                FAULT_SEVERITY_CRITICAL
            );
        }


        if (debug_peripheral_info.systick_status
                != PERIPHERAL_MONITOR_OK)
        {
            fault_manager_report(
                FAULT_CODE_SYSTICK,
                FAULT_SOURCE_PERIPHERAL,
                FAULT_SEVERITY_CRITICAL
            );
        }


        if (debug_peripheral_info.uart_status
                != PERIPHERAL_MONITOR_OK)
        {
            fault_manager_report(
                FAULT_CODE_UART,
                FAULT_SOURCE_PERIPHERAL,
                FAULT_SEVERITY_WARNING
            );
        }


        if (debug_peripheral_info.adc_status
                != PERIPHERAL_MONITOR_OK)
        {
            fault_manager_report(
                FAULT_CODE_ADC,
                FAULT_SOURCE_PERIPHERAL,
                FAULT_SEVERITY_WARNING
            );
        }


        if (debug_peripheral_info.dma_status
                != PERIPHERAL_MONITOR_OK)
        {
            fault_manager_report(
                FAULT_CODE_DMA,
                FAULT_SOURCE_PERIPHERAL,
                FAULT_SEVERITY_WARNING
            );
        }


        /* ====================================================
         * UPDATE FAULT MANAGER DEBUG INFORMATION
         * ==================================================== */

        fault_manager_update_debug_variables();
        /* ====================================================
         * WATCHDOG SUPERVISION
         * ==================================================== */

        debug_watchdog_status =
            watchdog_manager_update();

        debug_watchdog_fault =
            watchdog_manager_fault_active();
        /* ====================================================
         * APPLICATION ACTIVITY
         * ==================================================== */

//        led_toggle();
        current_time_ms = system_time_get_ms();

        if ((current_time_ms - last_led_toggle_ms) >= 1000U)
        {
            led_toggle();

            last_led_toggle_ms = current_time_ms;
        }
    }
}

static void debug_stack_overflow_test(void)
{
    volatile uint8_t buffer[1000];

    for (uint32_t i = 0U; i < sizeof(buffer); i++)
    {
        buffer[i] = 0xAAU;
    }

    if (memory_monitor_stack_overflow() != 0U)
    {
        (void)fault_manager_report(
            FAULT_CODE_STACK_OVERFLOW,
            FAULT_SOURCE_MEMORY,
            FAULT_SEVERITY_CRITICAL
        );
    }
}
/* ============================================================
 * Error Handler
 * ============================================================ */

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}
