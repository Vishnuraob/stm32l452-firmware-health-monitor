#include "adc.h"
#include "stm32l452xx.h"


/* ============================================================
 * ADC Configuration
 * ============================================================ */

#define ADC_INPUT_PORT              GPIOA
#define ADC_INPUT_PIN               0U
#define ADC_EXTERNAL_CHANNEL        5U

#define ADC_RESOLUTION_BITS         12U
#define ADC_MAX_VALUE               4095UL

#define ADC_TIMEOUT_COUNT           1000000UL

#define ADC_INTERNAL_STARTUP_DELAY_COUNT    2000UL
/* ============================================================
 * Factory calibration constants
 *
 * STM32L452:
 *
 * TS_CAL1:
 * Address = 0x1FFF75A8
 * Temperature = 30 deg C
 *
 * TS_CAL2:
 * Address = 0x1FFF75CA
 * Temperature = 130 deg C
 *
 * VREFINT_CAL:
 * Address = 0x1FFF75AA
 * ============================================================ */

#define TS_CAL1_ADDRESS             ((uint16_t *)0x1FFF75A8UL)
#define TS_CAL2_ADDRESS             ((uint16_t *)0x1FFF75CAUL)
#define VREFINT_CAL_ADDRESS         ((uint16_t *)0x1FFF75AAUL)


#define TS_CAL1                    (*TS_CAL1_ADDRESS)
#define TS_CAL2                    (*TS_CAL2_ADDRESS)

#define VREFINT_CAL                (*VREFINT_CAL_ADDRESS)


/*
 * VREFINT calibration voltage is 3.0 V = 3000 mV.
 */
#define VREFINT_CAL_VOLTAGE_MV      3000UL


/*
 * Factory temperature calibration points.
 */
#define TS_CAL1_TEMPERATURE         30L
#define TS_CAL2_TEMPERATURE         130L


/* ============================================================
 * Internal channel numbers
 * ============================================================ */

#define ADC_CHANNEL_VREFINT         0U
#define ADC_CHANNEL_TEMPERATURE     17U
#define ADC_CHANNEL_VBAT            18U


/* ============================================================
 * Last ADC value
 * ============================================================ */

static volatile uint16_t adc_last_value = 0U;


/* ============================================================
 * Internal functions
 * ============================================================ */

static adc_status_t adc_enable_clock(void);
static adc_status_t adc_config_gpio(void);
static adc_status_t adc_exit_deep_power_down(void);
static adc_status_t adc_enable_voltage_regulator(void);
static adc_status_t adc_calibrate(void);
static adc_status_t adc_config_common(void);
static adc_status_t adc_enable(void);

static adc_status_t adc_config_channel(uint32_t channel);

static adc_status_t adc_start_conversion(uint16_t *value);

static void adc_internal_startup_delay(void)
{
    volatile uint32_t i;

    for (i = 0U; i < ADC_INTERNAL_STARTUP_DELAY_COUNT; i++)
    {
        __NOP();
    }
}
/* ============================================================
 * ADC clock
 * ============================================================ */

static adc_status_t adc_enable_clock(void)
{
    /*
     * ADC1 belongs to the ADC1/2/3 common clock domain.
     */

    RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;

    /*
     * Read back register to make sure write completed.
     */
    if ((RCC->AHB2ENR & RCC_AHB2ENR_ADCEN) == 0U)
    {
        return ADC_ERROR_CLOCK;
    }

    return ADC_OK;
}


/* ============================================================
 * GPIO configuration
 *
 * PA0 = ADC1_IN5
 * ============================================================ */

static adc_status_t adc_config_gpio(void)
{
    /*
     * Enable GPIOA clock.
     */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    if ((RCC->AHB2ENR & RCC_AHB2ENR_GPIOAEN) == 0U)
    {
        return ADC_ERROR_GPIO;
    }


    /*
     * PA0 -> Analog mode
     *
     * MODER0 = 11
     */
    ADC_INPUT_PORT->MODER &= ~(3UL << (ADC_INPUT_PIN * 2U));
    ADC_INPUT_PORT->MODER |=  (3UL << (ADC_INPUT_PIN * 2U));


    /*
     * No pull-up / pull-down.
     */
    ADC_INPUT_PORT->PUPDR &= ~(3UL << (ADC_INPUT_PIN * 2U));


    /*
     * Verify.
     */
    if ((ADC_INPUT_PORT->MODER &
         (3UL << (ADC_INPUT_PIN * 2U))) !=
        (3UL << (ADC_INPUT_PIN * 2U)))
    {
        return ADC_ERROR_GPIO;
    }


    return ADC_OK;
}


/* ============================================================
 * Exit ADC deep-power-down
 * ============================================================ */

static adc_status_t adc_exit_deep_power_down(void)
{
    ADC1->CR &= ~ADC_CR_DEEPPWD;

    return ADC_OK;
}


/* ============================================================
 * ADC voltage regulator
 * ============================================================ */

static adc_status_t adc_enable_voltage_regulator(void)
{
    ADC1->CR |= ADC_CR_ADVREGEN;


    /*
     * Startup delay.
     *
     * This is a simple software delay.
     */
    for (volatile uint32_t i = 0U;
         i < 10000U;
         i++)
    {
        __NOP();
    }


    return ADC_OK;
}


/* ============================================================
 * ADC calibration
 * ============================================================ */

static adc_status_t adc_calibrate(void)
{
    /*
     * ADC must be disabled before calibration.
     */
    ADC1->CR &= ~ADC_CR_ADEN;


    /*
     * Make sure ADC is disabled.
     */
    if ((ADC1->CR & ADC_CR_ADEN) != 0U)
    {
        return ADC_ERROR_CALIBRATION;
    }


    /*
     * Start single-ended calibration.
     */
    ADC1->CR &= ~ADC_CR_ADCALDIF;

    ADC1->CR |= ADC_CR_ADCAL;


    /*
     * Wait for calibration to finish.
     */
    uint32_t timeout = ADC_TIMEOUT_COUNT;

    while ((ADC1->CR & ADC_CR_ADCAL) != 0U)
    {
        if (timeout == 0U)
        {
            return ADC_ERROR_TIMEOUT;
        }

        timeout--;
    }


    return ADC_OK;
}


/* ============================================================
 * ADC common configuration
 * ============================================================ */

static adc_status_t adc_config_common(void)
{
    /*
     * ADC clock mode:
     *
     * CKMODE = 11
     *
     * ADC kernel clock = system clock.
     *
     * SYSCLK = 80 MHz.
     */
    ADC1_COMMON->CCR &= ~ADC_CCR_CKMODE;

    ADC1_COMMON->CCR |=
        ADC_CCR_CKMODE_0 |
        ADC_CCR_CKMODE_1;


    /*
     * Enable internal temperature sensor.
     *
     * TSVREFE enables the internal temperature sensor
     * and VREFINT paths.
     */
    ADC1_COMMON->CCR |= ADC_CCR_VREFEN;


    /*
     * Enable temperature sensor.
     */
    ADC1_COMMON->CCR |= ADC_CCR_TSEN;


    /*
     * Enable VBAT channel.
     */
    ADC1_COMMON->CCR |= ADC_CCR_VBATEN;
    adc_internal_startup_delay();

    return ADC_OK;
}


/* ============================================================
 * ADC enable
 * ============================================================ */

static adc_status_t adc_enable(void)
{
    /*
     * Clear ADRDY.
     *
     * Writing 1 clears the flag.
     */
    ADC1->ISR |= ADC_ISR_ADRDY;


    /*
     * Enable ADC.
     */
    ADC1->CR |= ADC_CR_ADEN;


    /*
     * Wait for ADC ready.
     */
    uint32_t timeout = ADC_TIMEOUT_COUNT;

    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0U)
    {
        if (timeout == 0U)
        {
            return ADC_ERROR_TIMEOUT;
        }

        timeout--;
    }


    return ADC_OK;
}


/* ============================================================
 * Configure ADC channel
 * ============================================================ */

static adc_status_t adc_config_channel(uint32_t channel)
{
    /*
     * Single conversion.
     *
     * Sequence length = 1.
     *
     * SQ1 contains the selected channel.
     */

    ADC1->SQR1 = 0U;


    ADC1->SQR1 |=
        (channel << ADC_SQR1_SQ1_Pos);


    /*
     * Sampling time.
     *
     * We use a relatively long sample time because
     * internal channels need adequate sampling time.
     *
     * For channels 0..9:
     * SMPR1
     *
     * For channels 10..18:
     * SMPR2
     */


    if (channel <= 9U)
    {
        ADC1->SMPR1 &= ~(7UL << (channel * 3U));

        /*
         * Maximum sampling time.
         */
        ADC1->SMPR1 |=
            (7UL << (channel * 3U));
    }
    else
    {
        uint32_t position =
            (channel - 10U) * 3U;

        ADC1->SMPR2 &= ~(7UL << position);

        /*
         * Maximum sampling time.
         */
        ADC1->SMPR2 |=
            (7UL << position);
    }

    return ADC_OK;
}


/* ============================================================
 * Start ADC conversion
 * ============================================================ */

static adc_status_t adc_start_conversion(uint16_t *value)
{
    if (value == 0)
    {
        return ADC_ERROR_PARAMETER;
    }


    /*
     * Clear old status flags.
     */
    ADC1->ISR |=
        ADC_ISR_EOC |
        ADC_ISR_EOS |
        ADC_ISR_OVR;


    /*
     * Start conversion.
     */
    ADC1->CR |= ADC_CR_ADSTART;


    /*
     * Wait for End Of Conversion.
     */
    uint32_t timeout = ADC_TIMEOUT_COUNT;

    while ((ADC1->ISR & ADC_ISR_EOC) == 0U)
    {
        if (timeout == 0U)
        {
            return ADC_ERROR_TIMEOUT;
        }

        timeout--;
    }


    /*
     * Read ADC data register.
     *
     * Reading DR returns conversion result.
     */
    *value = (uint16_t)ADC1->DR;


    adc_last_value = *value;


    return ADC_OK;
}


/* ============================================================
 * ADC initialization
 * ============================================================ */

adc_status_t adc_init(void)
{
    adc_status_t status;


    status = adc_enable_clock();

    if (status != ADC_OK)
    {
        return status;
    }


    status = adc_config_gpio();

    if (status != ADC_OK)
    {
        return status;
    }


    status = adc_exit_deep_power_down();

    if (status != ADC_OK)
    {
        return status;
    }


    status = adc_enable_voltage_regulator();

    if (status != ADC_OK)
    {
        return status;
    }

    status = adc_config_common();

    if (status != ADC_OK)
    {
        return status;
    }

    status = adc_calibrate();

    if (status != ADC_OK)
    {
        return status;
    }




    /*
     * ADC configuration:
     *
     * Resolution = 12-bit
     * Right aligned
     * Single conversion
     * Software trigger
     */
    ADC1->CFGR = 0U;


    /*
     * 12-bit resolution.
     *
     * RES = 00
     */


    /*
     * Right alignment.
     *
     * ALIGN = 0
     */


    /*
     * Continuous conversion disabled.
     *
     * CONT = 0
     */


    /*
     * External trigger disabled.
     *
     * EXTEN = 00
     */


    /*
     * Configure default external channel.
     */
    status = adc_config_channel(ADC_EXTERNAL_CHANNEL);

    if (status != ADC_OK)
    {
        return status;
    }


    status = adc_enable();

    if (status != ADC_OK)
    {
        return status;
    }


    return ADC_OK;
}


/* ============================================================
 * External ADC
 *
 * PA0 / ADC1_IN5
 * ============================================================ */

adc_status_t adc_read_external(uint16_t *value)
{
    adc_status_t status;


    status = adc_config_channel(
        ADC_EXTERNAL_CHANNEL);

    if (status != ADC_OK)
    {
        return status;
    }


    return adc_start_conversion(value);
}


/* ============================================================
 * VREFINT
 * ============================================================ */

adc_status_t adc_read_vrefint(uint16_t *value)
{
    adc_status_t status;


    status = adc_config_channel(
        ADC_CHANNEL_VREFINT);

    if (status != ADC_OK)
    {
        return status;
    }


    return adc_start_conversion(value);
}


/* ============================================================
 * Internal temperature sensor
 * ============================================================ */

adc_status_t adc_read_temperature(uint16_t *value)
{
    adc_status_t status;


    status = adc_config_channel(
        ADC_CHANNEL_TEMPERATURE);

    if (status != ADC_OK)
    {
        return status;
    }


    return adc_start_conversion(value);
}

adc_status_t adc_configure_vrefint_dma(void)
{
    /*
     * ADC must be enabled before starting conversions.
     */
    if ((ADC1->CR & ADC_CR_ADEN) == 0U)
    {
        return ADC_ERROR_CONFIG;
    }


    /*
     * Stop any existing conversion.
     */
    if ((ADC1->CR & ADC_CR_ADSTART) != 0U)
    {
        ADC1->CR |= ADC_CR_ADSTP;

        while ((ADC1->CR & ADC_CR_ADSTART) != 0U)
        {
        }
    }


    /*
     * Select VREFINT.
     *
     * VREFINT = ADC1 channel 0.
     */
    ADC1->SQR1 = 0U;

    ADC1->SQR1 |=
        (ADC_CHANNEL_VREFINT << ADC_SQR1_SQ1_Pos);


    /*
     * One conversion in the regular sequence.
     */
    ADC1->SQR1 |=
        (0U << ADC_SQR1_L_Pos);


    /*
     * Long sampling time for VREFINT.
     */
    ADC1->SMPR1 &= ~(7UL << 0U);

    ADC1->SMPR1 |=
        (7UL << 0U);


    /*
     * Continuous conversion.
     */
    ADC1->CFGR |= ADC_CFGR_CONT;


    /*
     * Enable DMA requests.
     */
    ADC1->CFGR |= ADC_CFGR_DMAEN;


    /*
     * DMACFG = 1
     *
     * DMA requests continue for the complete
     * conversion sequence.
     */
    ADC1->CFGR |= ADC_CFGR_DMACFG;


    /*
     * Start ADC conversion.
     */
    ADC1->CR |= ADC_CR_ADSTART;


    return ADC_OK;
}


adc_status_t adc_configure_internal_channels_dma(void)
{
    if ((ADC1->CR & ADC_CR_ADEN) == 0U)
    {
        return ADC_ERROR_CONFIG;
    }

    /*
     * Stop ADC if it is currently converting.
     */
    if ((ADC1->CR & ADC_CR_ADSTART) != 0U)
    {
        ADC1->CR |= ADC_CR_ADSTP;

        while ((ADC1->CR & ADC_CR_ADSTART) != 0U)
        {
        }
    }

    /*
     * -----------------------------------------------------
     * Enable internal ADC paths
     * -----------------------------------------------------
     */

    ADC1_COMMON->CCR |= ADC_CCR_VREFEN;
    ADC1_COMMON->CCR |= ADC_CCR_TSEN;
    ADC1_COMMON->CCR |= ADC_CCR_VBATEN;


    /*
     * -----------------------------------------------------
     * Configure regular sequence
     *
     * Rank 1 = VREFINT   (channel 0)
     * Rank 2 = TEMP      (channel 17)
     * Rank 3 = VBAT      (channel 18)
     * -----------------------------------------------------
     */

    ADC1->SQR1 = 0U;

    /*
     * Sequence length = 3 conversions
     *
     * L = number of conversions - 1
     * L = 3 - 1 = 2
     */

    ADC1->SQR1 |= (2UL << ADC_SQR1_L_Pos);


    /*
     * SQ1 = channel 0
     */

    ADC1->SQR1 |= (0UL << ADC_SQR1_SQ1_Pos);


    /*
     * SQ2 = channel 17
     */

    ADC1->SQR1 |= (17UL << ADC_SQR1_SQ2_Pos);


    /*
     * SQ3 = channel 18
     */

    ADC1->SQR1 |= (18UL << ADC_SQR1_SQ3_Pos);


    /*
     * -----------------------------------------------------
     * Maximum sampling time
     *
     * Internal channels need sufficient sampling time.
     * -----------------------------------------------------
     */

    /*
     * Channel 0 → SMPR1 bits [2:0]
     */

    ADC1->SMPR1 &= ~(7UL << 0U);
    ADC1->SMPR1 |=  (7UL << 0U);


    /*
     * Channel 17 → SMPR2
     *
     * Channel 17 position:
     * (17 - 10) * 3 = 21
     */

    ADC1->SMPR2 &= ~(7UL << 21U);
    ADC1->SMPR2 |=  (7UL << 21U);


    /*
     * Channel 18 → SMPR2
     *
     * Channel 18 position:
     * (18 - 10) * 3 = 24
     */

    ADC1->SMPR2 &= ~(7UL << 24U);
    ADC1->SMPR2 |=  (7UL << 24U);


    /*
     * -----------------------------------------------------
     * Continuous conversion
     * -----------------------------------------------------
     */

    ADC1->CFGR |= ADC_CFGR_CONT;


    /*
     * -----------------------------------------------------
     * DMA circular mode
     * -----------------------------------------------------
     */

    ADC1->CFGR &= ~ADC_CFGR_DMAEN;
    ADC1->CFGR |= ADC_CFGR_DMAEN;
    ADC1->CFGR |= ADC_CFGR_DMACFG;

    /*
     * Start ADC
     */

    ADC1->CR |= ADC_CR_ADSTART;

    return ADC_OK;
}


/* ============================================================
 * VBAT
 * ============================================================ */

adc_status_t adc_read_vbat(uint16_t *value)
{
    adc_status_t status;


    status = adc_config_channel(
        ADC_CHANNEL_VBAT);

    if (status != ADC_OK)
    {
        return status;
    }


    return adc_start_conversion(value);
}


/* ============================================================
 * Raw ADC -> millivolts
 *
 * Assumes VDDA = 3300 mV.
 * ============================================================ */

uint32_t adc_raw_to_millivolts(uint16_t raw)
{
    return ((uint32_t)raw * 3300UL)
           / ADC_MAX_VALUE;
}


/* ============================================================
 * VREFINT -> VDDA
 *
 * VDDA = VREFINT_CAL_VOLTAGE *
 *        VREFINT_CAL / VREFINT_DATA
 *
 * ============================================================ */

uint32_t adc_vrefint_to_vdda_mv(uint16_t vrefint_raw)
{
    if (vrefint_raw == 0U)
    {
        return 0U;
    }


    return
        (VREFINT_CAL_VOLTAGE_MV *
         (uint32_t)VREFINT_CAL)
        / (uint32_t)vrefint_raw;
}


/* ============================================================
 * Temperature calculation
 *
 * Linear interpolation between:
 *
 * TS_CAL1 -> 30 C
 * TS_CAL2 -> 130 C
 *
 * ============================================================ */

int32_t adc_temperature_from_raw(uint16_t temperature_raw,
                                 uint32_t vdda_mv)
{
    uint32_t temperature_raw_3v;
    int32_t temperature_c;

    /*
     * Calibration values are specified at VDDA = 3.0 V.
     *
     * Convert the measured temperature ADC value
     * to the equivalent value at 3.0 V.
     */
    temperature_raw_3v =
        ((uint32_t)temperature_raw * vdda_mv) / 3000U;


    /*
     * Linear interpolation between:
     *
     * TS_CAL1 -> 30 °C
     * TS_CAL2 -> 130 °C
     */
    temperature_c =
        30 +
        (((int32_t)temperature_raw_3v - 1048) * 100)
        / (1395 - 1048);


    return temperature_c;
}


/* ============================================================
 * VBAT conversion
 *
 * Internal VBAT divider = 1/3.
 *
 * ADC measures approximately:
 *
 * VBAT / 3
 *
 * Therefore:
 *
 * VBAT = ADC_voltage * 3
 *
 * ============================================================ */

uint32_t adc_vbat_to_mv(uint16_t vbat_raw)
{
    uint32_t adc_voltage;


    adc_voltage =
        ((uint32_t)vbat_raw * 3300UL)
        / ADC_MAX_VALUE;


    return adc_voltage * 3UL;
}


/* ============================================================
 * Last ADC value
 * ============================================================ */

uint16_t adc_get_last_value(void)
{
    return adc_last_value;
}


/* ============================================================
 * ADC health check
 * ============================================================ */

adc_status_t adc_health_check(void)
{
    /*
     * ADC clock
     */
    if ((RCC->AHB2ENR & RCC_AHB2ENR_ADCEN) == 0U)
    {
        return ADC_ERROR_CLOCK;
    }

    /*
     * GPIOA clock
     */
    if ((RCC->AHB2ENR & RCC_AHB2ENR_GPIOAEN) == 0U)
    {
        return ADC_ERROR_GPIO;
    }

    /*
     * PA0 must be analog.
     */
    if ((GPIOA->MODER & 0x3UL) != 0x3UL)
    {
        return ADC_ERROR_GPIO;
    }

    /*
     * ADC must be enabled.
     */
    if ((ADC1->CR & ADC_CR_ADEN) == 0U)
    {
        return ADC_ERROR_CONFIG;
    }

    /*
     * 12-bit resolution.
     */
    if ((ADC1->CFGR & ADC_CFGR_RES) != 0U)
    {
        return ADC_ERROR_CONFIG;
    }

    /*
     * Right alignment.
     */
    if ((ADC1->CFGR & ADC_CFGR_ALIGN) != 0U)
    {
        return ADC_ERROR_CONFIG;
    }

    /*
     * Continuous conversion must be enabled.
     */
    if ((ADC1->CFGR & ADC_CFGR_CONT) == 0U)
    {
        return ADC_ERROR_CONFIG;
    }

    /*
     * DMA must be enabled.
     */
    if ((ADC1->CFGR & ADC_CFGR_DMAEN) == 0U)
    {
        return ADC_ERROR_CONFIG;
    }

    /*
     * DMA continuous requests must be enabled.
     */
    if ((ADC1->CFGR & ADC_CFGR_DMACFG) == 0U)
    {
        return ADC_ERROR_CONFIG;
    }

    /*
     * Three-channel scan:
     * Rank 1 = Channel 0  (VREFINT)
     * Rank 2 = Channel 17 (Temperature)
     * Rank 3 = Channel 18 (VBAT)
     */
    if ((ADC1->SQR1 & ADC_SQR1_L) != (2UL << ADC_SQR1_L_Pos))
    {
        return ADC_ERROR_CONFIG;
    }

    if (((ADC1->SQR1 >> ADC_SQR1_SQ1_Pos) & 0x1FUL) != 0U)
    {
        return ADC_ERROR_CONFIG;
    }

    if (((ADC1->SQR1 >> ADC_SQR1_SQ2_Pos) & 0x1FUL) != 17U)
    {
        return ADC_ERROR_CONFIG;
    }

    if (((ADC1->SQR1 >> ADC_SQR1_SQ3_Pos) & 0x1FUL) != 18U)
    {
        return ADC_ERROR_CONFIG;
    }

    /*
     * Internal reference enabled.
     */
    if ((ADC1_COMMON->CCR & ADC_CCR_VREFEN) == 0U)
    {
        return ADC_ERROR_CONFIG;
    }

    /*
     * Temperature sensor enabled.
     */
    if ((ADC1_COMMON->CCR & ADC_CCR_TSEN) == 0U)
    {
        return ADC_ERROR_CONFIG;
    }

    /*
     * VBAT enabled.
     */
    if ((ADC1_COMMON->CCR & ADC_CCR_VBATEN) == 0U)
    {
        return ADC_ERROR_CONFIG;
    }

    return ADC_OK;
}
