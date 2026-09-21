/*
 * system_time.c
 *
 *  Created on: 15-Sept-2026
 *      Author: engineer
 */
#include "system_time.h"
#include "stm32l452xx.h"

/*
 * SysTick frequency.
 *
 * We want one interrupt every 1 ms.
 */
#define SYSTEM_TIME_TICK_HZ       1000UL

/*
 * SysTick uses the processor clock.
 *
 * Our processor clock is:
 *
 * SystemCoreClock = 80 MHz
 */
#define SYSTEM_TIME_CLOCK_HZ      80000000UL

/*
 * Number of clock cycles between SysTick interrupts.
 *
 * 80,000,000 / 1,000 = 80,000 cycles
 *
 * Since LOAD contains the reload value minus one:
 *
 * LOAD = 80,000 - 1
 *      = 79,999
 */
#define SYSTICK_RELOAD_VALUE      ((SYSTEM_TIME_CLOCK_HZ / SYSTEM_TIME_TICK_HZ) - 1UL)


/*
 * This counter represents system uptime in milliseconds.
 *
 * volatile is required because this variable is modified
 * inside an interrupt and read from normal application code.
 */
static volatile uint32_t system_time_ms = 0U;


/*
 * Initialize SysTick to generate a 1 ms interrupt.
 */
system_time_status_t system_time_init(void)
{
    /*
     * Make sure the expected CPU clock is being used.
     */
    if (SystemCoreClock != SYSTEM_TIME_CLOCK_HZ)
    {
        return SYSTEM_TIME_ERROR_CLOCK;
    }

    /*
     * Disable SysTick before configuration.
     */
    SysTick->CTRL = 0U;

    /*
     * Configure reload value.
     *
     * 80 MHz / 1000 Hz = 80,000 clocks
     */
    SysTick->LOAD = SYSTICK_RELOAD_VALUE;

    /*
     * Clear the current counter value.
     */
    SysTick->VAL = 0U;

    /*
     * Configure SysTick:
     *
     * CLKSOURCE = processor clock
     * TICKINT   = enable SysTick interrupt
     * ENABLE    = enable counter
     */
    SysTick->CTRL =
            SysTick_CTRL_CLKSOURCE_Msk |
            SysTick_CTRL_TICKINT_Msk   |
            SysTick_CTRL_ENABLE_Msk;

    /*
     * Start uptime from zero.
     */
    system_time_ms = 0U;

    /*
     * Verify configuration.
     */
    if (system_time_health_check() != SYSTEM_TIME_OK)
    {
        return SYSTEM_TIME_ERROR_CONFIG;
    }

    return SYSTEM_TIME_OK;
}


/*
 * SysTick interrupt handler.
 *
 * This function is called every 1 ms.
 */
//void SysTick_Handler(void)
//{
//    system_time_ms++;
//}
void system_time_tick(void)
{
    system_time_ms++;
}

/*
 * Return system uptime in milliseconds.
 */
uint32_t system_time_get_ms(void)
{
    return system_time_ms;
}


/*
 * Blocking delay based on the 1 ms system time.
 */
void system_time_delay_ms(uint32_t delay_ms)
{
    uint32_t start_time;

    start_time = system_time_get_ms();

    while ((system_time_get_ms() - start_time) < delay_ms)
    {
        /*
         * Wait until the requested amount of time has elapsed.
         *
         * Unsigned subtraction makes this work correctly even
         * when the 32-bit millisecond counter wraps around.
         */
    }
}


/*
 * Verify the SysTick configuration.
 */
system_time_status_t system_time_health_check(void)
{
    /*
     * Verify CPU clock.
     */
    if (SystemCoreClock != SYSTEM_TIME_CLOCK_HZ)
    {
        return SYSTEM_TIME_ERROR_CLOCK;
    }

    /*
     * Verify reload value.
     */
    if (SysTick->LOAD != SYSTICK_RELOAD_VALUE)
    {
        return SYSTEM_TIME_ERROR_CONFIG;
    }

    /*
     * Verify processor clock is selected.
     */
    if ((SysTick->CTRL & SysTick_CTRL_CLKSOURCE_Msk) == 0U)
    {
        return SYSTEM_TIME_ERROR_CONFIG;
    }

    /*
     * Verify SysTick interrupt is enabled.
     */
    if ((SysTick->CTRL & SysTick_CTRL_TICKINT_Msk) == 0U)
    {
        return SYSTEM_TIME_ERROR_CONFIG;
    }

    /*
     * Verify SysTick counter is enabled.
     */
    if ((SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) == 0U)
    {
        return SYSTEM_TIME_ERROR_CONFIG;
    }

    return SYSTEM_TIME_OK;
}

