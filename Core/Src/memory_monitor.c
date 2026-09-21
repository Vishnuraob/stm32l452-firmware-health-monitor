/*
 * memory_monitor.c
 *
 *  Created on: 16-Sept-2026
 *      Author: engineer
 */
#include "memory_monitor.h"
#include "stm32l452xx.h"


/* =========================================================
 * Linker Symbols
 * ========================================================= */

/*
 * These symbols come directly from the linker script.
 */

extern uint8_t _estack;
extern uint8_t _end;

extern uint8_t _Min_Heap_Size;
extern uint8_t _Min_Stack_Size;


/* =========================================================
 * RAM Configuration
 * ========================================================= */

#define RAM_START_ADDRESS       0x20000000UL
#define RAM_SIZE_BYTES          (128UL * 1024UL)

#define RAM_END_ADDRESS         \
    (RAM_START_ADDRESS + RAM_SIZE_BYTES)


/* =========================================================
 * Internal State
 * ========================================================= */

static volatile uint8_t memory_initialized = 0U;

static volatile uint8_t memory_fault = 0U;

static volatile uint32_t
configured_stack_warning = 0U;


/* =========================================================
 * Get Main Stack Pointer
 * ========================================================= */
uint32_t memory_monitor_get_msp(void)
{
    uint32_t msp;

    __asm volatile
    (
        "mrs %0, msp"
        : "=r" (msp)
    );

    return msp;
}
uint8_t memory_monitor_stack_overflow(void)
{
    uint32_t msp;

    __asm volatile
    (
        "mrs %0, msp"
        : "=r" (msp)
    );

    if (msp < 0x2001FC00UL)
    {
        return 1U;
    }

    return 0U;
}
/* =========================================================
 * Initialization
 * ========================================================= */

memory_monitor_status_t memory_monitor_init(
    const memory_monitor_config_t *config)
{
    if (config == 0)
    {
        return MEMORY_MONITOR_ERROR_PARAMETER;
    }


    /*
     * Store warning threshold.
     */
    configured_stack_warning =
        config->stack_warning_bytes;


    memory_initialized = 1U;

    memory_fault = 0U;


    return MEMORY_MONITOR_OK;
}


/* =========================================================
 * Update
 * ========================================================= */

memory_monitor_status_t memory_monitor_update(void)
{
    uint32_t msp;
    uint32_t stack_top;
    uint32_t stack_reserved;
    uint32_t stack_boundary;


    if (memory_initialized == 0U)
    {
        return MEMORY_MONITOR_ERROR_NOT_INITIALIZED;
    }


    /*
     * Current MSP.
     */
    msp =
    		memory_monitor_get_msp();

    /*
     * Top of stack.
     *
     * _estack comes from linker script.
     */
    stack_top =
        (uint32_t)&_estack;


    /*
     * Minimum stack reserved by linker.
     */
    stack_reserved =
        (uint32_t)&_Min_Stack_Size;


    /*
     * The minimum stack reservation occupies the
     * upper part of the RAM region.
     *
     * Stack warning boundary:
     *
     *     _estack - _Min_Stack_Size
     */
    stack_boundary =
        stack_top - stack_reserved;


    /*
     * Check whether MSP is outside RAM.
     */
    if ((msp < RAM_START_ADDRESS) ||
        (msp > RAM_END_ADDRESS))
    {
        memory_fault = 1U;

        return MEMORY_MONITOR_FAULT_STACK_OVERFLOW;
    }


    /*
     * Check whether MSP has crossed the minimum
     * reserved stack boundary.
     */
    if (msp < stack_boundary)
    {
        memory_fault = 1U;

        return MEMORY_MONITOR_FAULT_STACK_OVERFLOW;
    }


    /*
     * Calculate currently available stack space
     * above the minimum reserved stack boundary.
     */
    if (msp > stack_boundary)
    {
        uint32_t stack_available =
            msp - stack_boundary;

        if (stack_available <
            configured_stack_warning)
        {
            memory_fault = 1U;

            return MEMORY_MONITOR_FAULT_STACK_WARNING;
        }
    }


    memory_fault = 0U;

    return MEMORY_MONITOR_OK;
}


/* =========================================================
 * Get Memory Information
 * ========================================================= */

memory_monitor_status_t memory_monitor_get_info(
    memory_monitor_info_t *info)
{
    uint32_t msp;
    uint32_t stack_top;
    uint32_t stack_reserved;
    uint32_t heap_start;


    if (info == 0)
    {
        return MEMORY_MONITOR_ERROR_PARAMETER;
    }


    if (memory_initialized == 0U)
    {
        return MEMORY_MONITOR_ERROR_NOT_INITIALIZED;
    }


    msp =
    		memory_monitor_get_msp();

    stack_top =
        (uint32_t)&_estack;


    stack_reserved =
        (uint32_t)&_Min_Stack_Size;


    heap_start =
        (uint32_t)&_end;


    /* -----------------------------------------------------
     * RAM
     * ----------------------------------------------------- */

    info->ram_start =
        RAM_START_ADDRESS;

    info->ram_size =
        RAM_SIZE_BYTES;

    info->ram_end =
        RAM_END_ADDRESS;


    /* -----------------------------------------------------
     * Stack
     * ----------------------------------------------------- */

    info->stack_top =
        stack_top;

    info->stack_pointer =
        msp;

    info->stack_reserved =
        stack_reserved;


    if (msp > (stack_top - stack_reserved))
    {
        info->stack_available =
            msp - (stack_top - stack_reserved);
    }
    else
    {
        info->stack_available = 0U;
    }


    /* -----------------------------------------------------
     * Heap
     * ----------------------------------------------------- */

    info->heap_start =
        heap_start;

    info->heap_reserved =
        (uint32_t)&_Min_Heap_Size;


    return MEMORY_MONITOR_OK;
}


/* =========================================================
 * Health Check
 * ========================================================= */

memory_monitor_status_t memory_monitor_health_check(void)
{
    uint32_t msp;
    uint32_t stack_top;
    uint32_t stack_reserved;
    uint32_t stack_boundary;


    if (memory_initialized == 0U)
    {
        return MEMORY_MONITOR_ERROR_NOT_INITIALIZED;
    }


    msp =
    		memory_monitor_get_msp();

    stack_top =
        (uint32_t)&_estack;


    stack_reserved =
        (uint32_t)&_Min_Stack_Size;


    stack_boundary =
        stack_top - stack_reserved;


    /*
     * MSP must remain inside RAM.
     */
    if ((msp < RAM_START_ADDRESS) ||
        (msp > RAM_END_ADDRESS))
    {
        memory_fault = 1U;

        return MEMORY_MONITOR_FAULT_STACK_OVERFLOW;
    }


    /*
     * MSP must not cross the reserved stack boundary.
     */
    if (msp < stack_boundary)
    {
        memory_fault = 1U;

        return MEMORY_MONITOR_FAULT_STACK_OVERFLOW;
    }


    /*
     * Check warning threshold.
     */
    if ((msp - stack_boundary) <
        configured_stack_warning)
    {
        memory_fault = 1U;

        return MEMORY_MONITOR_FAULT_STACK_WARNING;
    }


    memory_fault = 0U;

    return MEMORY_MONITOR_OK;
}


/* =========================================================
 * Get Current MSP
 * ========================================================= */

uint32_t memory_monitor_get_stack_pointer(void)
{
    return memory_monitor_get_msp();
}


/* =========================================================
 * Get Available Stack
 * ========================================================= */

uint32_t memory_monitor_get_stack_available(void)
{
    uint32_t msp;
    uint32_t stack_top;
    uint32_t stack_reserved;
    uint32_t stack_boundary;


    msp =
    		memory_monitor_get_msp();
    stack_top =
        (uint32_t)&_estack;

    stack_reserved =
        (uint32_t)&_Min_Stack_Size;

    stack_boundary =
        stack_top - stack_reserved;


    if (msp <= stack_boundary)
    {
        return 0U;
    }


    return msp - stack_boundary;
}


/* =========================================================
 * RAM Size
 * ========================================================= */

uint32_t memory_monitor_get_ram_size(void)
{
    return RAM_SIZE_BYTES;
}


/* =========================================================
 * RAM Start
 * ========================================================= */

uint32_t memory_monitor_get_ram_start(void)
{
    return RAM_START_ADDRESS;
}


/* =========================================================
 * RAM End
 * ========================================================= */

uint32_t memory_monitor_get_ram_end(void)
{
    return RAM_END_ADDRESS;
}


/* =========================================================
 * Fault State
 * ========================================================= */

uint8_t memory_monitor_fault_active(void)
{
    return memory_fault;
}


/* =========================================================
 * Clear Fault
 * ========================================================= */

void memory_monitor_clear_fault(void)
{
    memory_fault = 0U;
}

