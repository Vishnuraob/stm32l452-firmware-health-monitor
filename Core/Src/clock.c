/*
 * clock.c
 *
 *  Created on: 15-Sept-2026
 *      Author: engineer
 */
#include "clock.h"
#include "stm32l452xx.h"


/*----------------------------------------------------------
 * Clock configuration
 *----------------------------------------------------------*/

#define HSI_FREQUENCY_HZ       16000000UL
#define SYSCLK_TARGET_HZ       80000000UL

#define PLL_M_DIVIDER          2UL
#define PLL_N_MULTIPLIER       20UL
#define PLL_R_DIVIDER          2UL


/*
 * STM32L4 PLLR register encoding:
 *
 * 00 -> /2
 * 01 -> /4
 * 10 -> /6
 * 11 -> /8
 */
#define PLL_R_REGISTER_VALUE   0U


/*
 * PLLM register encoding:
 *
 * 000 -> /1
 * 001 -> /2
 * 010 -> /3
 * 011 -> /4
 * ...
 *
 * Therefore:
 *
 * PLL_M_DIVIDER = 2
 * register value = 1
 */
#define PLL_M_REGISTER_VALUE   (PLL_M_DIVIDER - 1U)


/*
 * Timeout values.
 *
 * These are simple loop-count limits for the clock
 * initialization stage because SysTick has not yet
 * been initialized.
 */
#define HSI_TIMEOUT_COUNT      1000000UL
#define PLL_TIMEOUT_COUNT      1000000UL
#define CLOCK_SWITCH_TIMEOUT   1000000UL


static clock_status_t last_clock_status = CLOCK_OK;


/*----------------------------------------------------------
 * Internal functions
 *----------------------------------------------------------*/

static clock_status_t clock_enable_hsi(void)
{
    uint32_t timeout = 0U;

    /*
     * Enable HSI16.
     */
    RCC->CR |= RCC_CR_HSION;

    /*
     * Wait until HSI16 becomes ready.
     */
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U)
    {
        timeout++;

        if (timeout >= HSI_TIMEOUT_COUNT)
        {
            return CLOCK_ERROR_HSI_TIMEOUT;
        }
    }

    return CLOCK_OK;
}


static clock_status_t clock_config_voltage_scaling(void)
{
    /*
     * Enable PWR peripheral clock.
     */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;

    /*
     * Voltage scaling Range 1.
     *
     * VOS = 01
     */
    PWR->CR1 &= ~PWR_CR1_VOS;
    PWR->CR1 |= PWR_CR1_VOS_0;

    /*
     * Wait for voltage scaling transition.
     */
    while ((PWR->SR2 & PWR_SR2_VOSF) != 0U)
    {
    }

    return CLOCK_OK;
}


static clock_status_t clock_config_flash(void)
{
    /*
     * Configure Flash latency for 80 MHz operation.
     */
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY_4WS;

    /*
     * Read back the register.
     *
     * This prevents us from blindly assuming the
     * configuration was accepted.
     */
    if ((FLASH->ACR & FLASH_ACR_LATENCY) !=
        FLASH_ACR_LATENCY_4WS)
    {
        return CLOCK_ERROR_FLASH_LATENCY;
    }

    return CLOCK_OK;
}


static clock_status_t clock_config_bus_prescalers(void)
{
    /*
     * AHB prescaler = /1
     *
     * HCLK = SYSCLK
     */
    RCC->CFGR &= ~RCC_CFGR_HPRE;


    /*
     * APB1 prescaler = /1
     *
     * PCLK1 = HCLK
     */
    RCC->CFGR &= ~RCC_CFGR_PPRE1;


    /*
     * APB2 prescaler = /1
     *
     * PCLK2 = HCLK
     */
    RCC->CFGR &= ~RCC_CFGR_PPRE2;


    /*
     * Verify the configuration.
     */
    if ((RCC->CFGR & RCC_CFGR_HPRE) != 0U)
    {
        return CLOCK_ERROR_FREQUENCY;
    }

    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0U)
    {
        return CLOCK_ERROR_FREQUENCY;
    }

    if ((RCC->CFGR & RCC_CFGR_PPRE2) != 0U)
    {
        return CLOCK_ERROR_FREQUENCY;
    }

    return CLOCK_OK;
}


static clock_status_t clock_config_pll(void)
{
    uint32_t timeout = 0U;


    /*
     * PLL must be disabled before modifying
     * its configuration.
     */
    RCC->CR &= ~RCC_CR_PLLON;


    /*
     * Wait until PLL is actually disabled.
     */
    while ((RCC->CR & RCC_CR_PLLRDY) != 0U)
    {
        timeout++;

        if (timeout >= PLL_TIMEOUT_COUNT)
        {
            return CLOCK_ERROR_PLL_TIMEOUT;
        }
    }


    /*
     * Select HSI16 as PLL source.
     *
     * PLLSRC:
     *
     * 00 -> no clock
     * 01 -> MSI
     * 10 -> HSI16
     * 11 -> HSE
     *
     * HSI16 = bit pattern 10.
     */
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLSRC;
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLSRC;
    RCC->PLLCFGR |= (2UL << RCC_PLLCFGR_PLLSRC_Pos);


    /*
     * Configure PLLM.
     *
     * Desired:
     *
     * HSI16 / 2 = 8 MHz
     *
     * Register encoding for /2 = 001.
     */
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLM;
    RCC->PLLCFGR |=
        (PLL_M_REGISTER_VALUE << RCC_PLLCFGR_PLLM_Pos);


    /*
     * Configure PLLN.
     *
     * Desired:
     *
     * 8 MHz x 20 = 160 MHz
     */
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLN;
    RCC->PLLCFGR |=
        (PLL_N_MULTIPLIER << RCC_PLLCFGR_PLLN_Pos);


    /*
     * Configure PLLR.
     *
     * Desired:
     *
     * 160 MHz / 2 = 80 MHz
     *
     * Register encoding:
     *
     * 00 -> /2
     */
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLR;
    RCC->PLLCFGR |=
        (PLL_R_REGISTER_VALUE << RCC_PLLCFGR_PLLR_Pos);


    /*
     * Enable PLLR output.
     */
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN;


    /*
     * Enable PLL.
     */
    RCC->CR |= RCC_CR_PLLON;


    /*
     * Wait for PLL lock.
     */
    timeout = 0U;

    while ((RCC->CR & RCC_CR_PLLRDY) == 0U)
    {
        timeout++;

        if (timeout >= PLL_TIMEOUT_COUNT)
        {
            return CLOCK_ERROR_PLL_TIMEOUT;
        }
    }


    return CLOCK_OK;
}


static clock_status_t clock_switch_to_pll(void)
{
    uint32_t timeout = 0U;


    /*
     * Select PLL as SYSCLK source.
     *
     * SW:
     *
     * 00 -> MSI
     * 01 -> HSI16
     * 10 -> HSE
     * 11 -> PLL
     */
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;


    /*
     * Wait until the hardware confirms that PLL
     * is now being used as SYSCLK.
     */
    while ((RCC->CFGR & RCC_CFGR_SWS) !=
           RCC_CFGR_SWS_PLL)
    {
        timeout++;

        if (timeout >= CLOCK_SWITCH_TIMEOUT)
        {
            return CLOCK_ERROR_CLOCK_SWITCH;
        }
    }


    return CLOCK_OK;
}


/*----------------------------------------------------------
 * Public functions
 *----------------------------------------------------------*/

clock_status_t clock_init(void)
{
    clock_status_t status;


    /*
     * Step 1:
     * Configure voltage scaling.
     */
    status = clock_config_voltage_scaling();

    if (status != CLOCK_OK)
    {
        last_clock_status = status;
        return status;
    }


    /*
     * Step 2:
     * Configure Flash latency BEFORE increasing
     * the system clock.
     */
    status = clock_config_flash();

    if (status != CLOCK_OK)
    {
        last_clock_status = status;
        return status;
    }


    /*
     * Step 3:
     * Enable HSI16.
     */
    status = clock_enable_hsi();

    if (status != CLOCK_OK)
    {
        last_clock_status = status;
        return status;
    }


    /*
     * Step 4:
     * Configure AHB/APB prescalers.
     */
    status = clock_config_bus_prescalers();

    if (status != CLOCK_OK)
    {
        last_clock_status = status;
        return status;
    }


    /*
     * Step 5:
     * Configure and enable PLL.
     */
    status = clock_config_pll();

    if (status != CLOCK_OK)
    {
        last_clock_status = status;
        return status;
    }


    /*
     * Step 6:
     * Switch SYSCLK from HSI16 to PLL.
     */
    status = clock_switch_to_pll();

    if (status != CLOCK_OK)
    {
        last_clock_status = status;
        return status;
    }


    /*
     * Step 7:
     * Perform final health verification.
     */
    status = clock_health_check();

    if (status != CLOCK_OK)
    {
        last_clock_status = status;
        return status;
    }


    /*
     * Everything passed.
     */
    SystemCoreClockUpdate();
    last_clock_status = CLOCK_OK;

    return CLOCK_OK;
}


uint32_t clock_get_frequency(void)
{
    uint32_t pll_input;
    uint32_t vco_frequency;
    uint32_t sysclk_frequency;


    /*
     * HSI16 / PLLM
     */
    pll_input =
        HSI_FREQUENCY_HZ / PLL_M_DIVIDER;


    /*
     * PLL input x PLLN
     */
    vco_frequency =
        pll_input * PLL_N_MULTIPLIER;


    /*
     * VCO / PLLR
     */
    sysclk_frequency =
        vco_frequency / PLL_R_DIVIDER;


    return sysclk_frequency;
}


clock_status_t clock_health_check(void)
{
    uint32_t calculated_frequency;


    /*
     * Verify HSI is running.
     */
    if ((RCC->CR & RCC_CR_HSIRDY) == 0U)
    {
        return CLOCK_ERROR_HSI_TIMEOUT;
    }


    /*
     * Verify PLL is enabled.
     */
    if ((RCC->CR & RCC_CR_PLLON) == 0U)
    {
        return CLOCK_ERROR_PLL_CONFIG;
    }


    /*
     * Verify PLL is locked.
     */
    if ((RCC->CR & RCC_CR_PLLRDY) == 0U)
    {
        return CLOCK_ERROR_PLL_TIMEOUT;
    }


    /*
     * Verify PLL is the actual SYSCLK source.
     */
    if ((RCC->CFGR & RCC_CFGR_SWS) !=
        RCC_CFGR_SWS_PLL)
    {
        return CLOCK_ERROR_CLOCK_SWITCH;
    }


    /*
     * Verify PLL source = HSI16.
     */
    if ((RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) !=
    		(2UL << RCC_PLLCFGR_PLLSRC_Pos))
    {
        return CLOCK_ERROR_PLL_CONFIG;
    }


    /*
     * Verify PLLM.
     */
    if ((RCC->PLLCFGR & RCC_PLLCFGR_PLLM) !=
        (PLL_M_REGISTER_VALUE << RCC_PLLCFGR_PLLM_Pos))
    {
        return CLOCK_ERROR_PLL_CONFIG;
    }


    /*
     * Verify PLLN.
     */
    if ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) !=
        (PLL_N_MULTIPLIER << RCC_PLLCFGR_PLLN_Pos))
    {
        return CLOCK_ERROR_PLL_CONFIG;
    }


    /*
     * Verify PLLR.
     */
    if ((RCC->PLLCFGR & RCC_PLLCFGR_PLLR) !=
        (PLL_R_REGISTER_VALUE << RCC_PLLCFGR_PLLR_Pos))
    {
        return CLOCK_ERROR_PLL_CONFIG;
    }


    /*
     * Verify PLLR output is enabled.
     */
    if ((RCC->PLLCFGR & RCC_PLLCFGR_PLLREN) == 0U)
    {
        return CLOCK_ERROR_PLL_CONFIG;
    }


    /*
     * Calculate expected frequency from the actual
     * configuration constants.
     */
    calculated_frequency = clock_get_frequency();


    if (calculated_frequency != SYSCLK_TARGET_HZ)
    {
        return CLOCK_ERROR_FREQUENCY;
    }


    /*
     * Verify Flash latency.
     */
    if ((FLASH->ACR & FLASH_ACR_LATENCY) !=
        FLASH_ACR_LATENCY_4WS)
    {
        return CLOCK_ERROR_FLASH_LATENCY;
    }


    /*
     * Everything passed.
     */
    return CLOCK_OK;
}


clock_status_t clock_get_status(void)
{
    return last_clock_status;
}

