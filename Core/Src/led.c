#include "led.h"
#include "stm32l452xx.h"

#define LED_PORT                GPIOA
#define LED_PIN                 5U

#define LED_PIN_MASK            (1UL << LED_PIN)

#define LED_MODE_OUTPUT         1UL
#define LED_OUTPUT_TYPE_PP      0UL
#define LED_SPEED_LOW           0UL
#define LED_PULL_NONE           0UL

static uint8_t led_state = 0U;


/*
 * Initialize PA5 as a push-pull output.
 */
led_status_t led_init(void)
{
    /*
     * Enable GPIOA clock.
     *
     * GPIOAEN = bit 0 of AHB2ENR
     */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    /*
     * Read back the enable bit.
     *
     * This gives us an actual check instead of assuming
     * that the clock was enabled successfully.
     */
    if ((RCC->AHB2ENR & RCC_AHB2ENR_GPIOAEN) == 0U)
    {
        return LED_ERROR_CLOCK;
    }

    /*
     * Configure PA5 as output.
     *
     * Each GPIO pin has two MODER bits:
     *
     * 00 = input
     * 01 = general purpose output
     * 10 = alternate function
     * 11 = analog
     *
     * PA5 uses MODER[11:10].
     */
    LED_PORT->MODER &= ~(3UL << (LED_PIN * 2U));

    LED_PORT->MODER |= (LED_MODE_OUTPUT << (LED_PIN * 2U));

    /*
     * Push-pull output.
     *
     * OTYPER:
     * 0 = push-pull
     * 1 = open-drain
     */
    LED_PORT->OTYPER &= ~LED_PIN_MASK;

    /*
     * Low-speed output.
     *
     * OSPEEDR:
     * 00 = low
     * 01 = medium
     * 10 = high
     * 11 = very high
     */
    LED_PORT->OSPEEDR &= ~(3UL << (LED_PIN * 2U));
    LED_PORT->OSPEEDR |= (LED_SPEED_LOW << (LED_PIN * 2U));

    /*
     * No pull-up / pull-down.
     *
     * PUPDR:
     * 00 = no pull
     * 01 = pull-up
     * 10 = pull-down
     */
    LED_PORT->PUPDR &= ~(3UL << (LED_PIN * 2U));

    /*
     * Start with LED OFF.
     */
    led_off();

    /*
     * Verify the configuration.
     */
    if (led_health_check() != LED_OK)
    {
        return LED_ERROR_GPIO;
    }

    return LED_OK;
}


void led_on(void)
{
    /*
     * BSRR lower 16 bits set the corresponding output bit.
     */
    LED_PORT->BSRR = LED_PIN_MASK;

    led_state = 1U;
}


void led_off(void)
{
    /*
     * BSRR upper 16 bits reset the corresponding output bit.
     */
    LED_PORT->BSRR = (LED_PIN_MASK << 16U);

    led_state = 0U;
}


void led_toggle(void)
{
    if (led_state != 0U)
    {
        led_off();
    }
    else
    {
        led_on();
    }
}


uint8_t led_get_state(void)
{
    return led_state;
}


led_status_t led_health_check(void)
{
    /*
     * Verify GPIOA clock is enabled.
     */
    if ((RCC->AHB2ENR & RCC_AHB2ENR_GPIOAEN) == 0U)
    {
        return LED_ERROR_CLOCK;
    }

    /*
     * Verify PA5 is configured as general-purpose output.
     */
    if ((LED_PORT->MODER & (3UL << (LED_PIN * 2U))) !=
        (1UL << (LED_PIN * 2U)))
    {
        return LED_ERROR_GPIO;
    }

    /*
     * Verify push-pull configuration.
     */
    if ((LED_PORT->OTYPER & LED_PIN_MASK) != 0U)
    {
        return LED_ERROR_GPIO;
    }

    /*
     * Verify no pull-up/pull-down.
     */
    if ((LED_PORT->PUPDR & (3UL << (LED_PIN * 2U))) != 0U)
    {
        return LED_ERROR_GPIO;
    }

    return LED_OK;
}
