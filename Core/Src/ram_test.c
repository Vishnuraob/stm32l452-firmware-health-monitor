#include "ram_test.h"
#include "stm32l452xx.h"


/*
 * Symbols provided by the linker script
 */

extern uint8_t __ram_test_start__;
extern uint8_t __ram_test_end__;


/*
 * Fixed data patterns
 */

#define RAM_TEST_PATTERN_1     0xAAAAAAAAUL
#define RAM_TEST_PATTERN_2     0x55555555UL


/*
 * RAM Test State
 */

static volatile uint8_t ram_test_initialized = 0U;
static volatile uint8_t ram_test_fault = 0U;
static volatile uint8_t ram_test_fault_injection = 0U;
static volatile ram_test_fault_info_t ram_test_fault_info;

/* ----------------------------------------------------------
 * RAM Test Initialization
 * ---------------------------------------------------------- */

ram_test_status_t ram_test_init(void)
{
    ram_test_initialized = 1U;
    ram_test_fault = 0U;

    return RAM_TEST_OK;
}


/* ----------------------------------------------------------
 * Basic Pattern Test
 *
 * Write 0xAAAAAAAA
 * Read back
 *
 * Write 0x55555555
 * Read back
 * ---------------------------------------------------------- */

static ram_test_status_t ram_test_pattern_test(void)
{
    volatile uint32_t *address;
    uint32_t start_address;
    uint32_t end_address;


    start_address =
        (uint32_t)&__ram_test_start__;

    end_address =
        (uint32_t)&__ram_test_end__;


    /*
     * Write first pattern
     */

    for (address =
             (volatile uint32_t *)start_address;

         (uint32_t)address < end_address;

         address++)
    {
        *address = RAM_TEST_PATTERN_1;
    }
    /*
     * Fault Injection
     *
     * Deliberately corrupt one RAM location.
     */

    if (ram_test_fault_injection != 0U)
    {
        volatile uint32_t *fault_address;

        fault_address =
            (volatile uint32_t *)(start_address + 0x40U);

        *fault_address = 0x12345678UL;

        ram_test_fault_injection = 0U;
    }

    /*
     * Verify first pattern
     */

    for (address =
             (volatile uint32_t *)start_address;

         (uint32_t)address < end_address;

         address++)
    {
    	if (*address != RAM_TEST_PATTERN_1)
    	{
    	    ram_test_fault_info.address =
    	        (uint32_t)address;

    	    ram_test_fault_info.expected =
    	        RAM_TEST_PATTERN_1;

    	    ram_test_fault_info.actual =
    	        *address;

    	    ram_test_fault_info.test_type =
    	        RAM_TEST_TYPE_PATTERN;

    	    return RAM_TEST_FAULT_WRITE_READ;
    	}
    }


    /*
     * Write second pattern
     */

    for (address =
             (volatile uint32_t *)start_address;

         (uint32_t)address < end_address;

         address++)
    {
        *address = RAM_TEST_PATTERN_2;
    }


    /*
     * Verify second pattern
     */

    for (address =
             (volatile uint32_t *)start_address;

         (uint32_t)address < end_address;

         address++)
    {
    	if (*address != RAM_TEST_PATTERN_2)
    	{
    	    ram_test_fault_info.address =
    	        (uint32_t)address;

    	    ram_test_fault_info.expected =
    	        RAM_TEST_PATTERN_2;

    	    ram_test_fault_info.actual =
    	        *address;

    	    ram_test_fault_info.test_type =
    	        RAM_TEST_TYPE_PATTERN;

    	    return RAM_TEST_FAULT_WRITE_READ;
    	}
    }


    return RAM_TEST_OK;
}


/* ----------------------------------------------------------
 * Walking-1 Test
 *
 * One bit is set to 1 at a time.
 *
 * 00000001
 * 00000002
 * 00000004
 * ...
 * 80000000
 *
 * Every pattern is written to every RAM test word
 * and then verified.
 * ---------------------------------------------------------- */

static ram_test_status_t ram_test_walking_1(void)
{
    volatile uint32_t *address;

    uint32_t start_address;
    uint32_t end_address;

    uint32_t bit;
    uint32_t pattern;


    start_address =
        (uint32_t)&__ram_test_start__;

    end_address =
        (uint32_t)&__ram_test_end__;


    /*
     * Test every bit position
     */

    for (bit = 0U; bit < 32U; bit++)
    {
        /*
         * Create walking-1 pattern
         *
         * Example:
         *
         * bit = 0 → 0x00000001
         * bit = 1 → 0x00000002
         * bit = 2 → 0x00000004
         *
         */

        pattern = (1UL << bit);


        /*
         * Write pattern to entire test region
         */

        for (address =
                 (volatile uint32_t *)start_address;

             (uint32_t)address < end_address;

             address++)
        {
            *address = pattern;
        }


        /*
         * Verify entire test region
         */

        for (address =
                 (volatile uint32_t *)start_address;

             (uint32_t)address < end_address;

             address++)
        {
        	if (*address != pattern)
        	{
        	    ram_test_fault_info.address =
        	        (uint32_t)address;

        	    ram_test_fault_info.expected =
        	        pattern;

        	    ram_test_fault_info.actual =
        	        *address;

        	    ram_test_fault_info.test_type =
        	        RAM_TEST_TYPE_WALKING_1;

        	    return RAM_TEST_FAULT_WRITE_READ;
        	}
        }
    }


    return RAM_TEST_OK;
}


/* ----------------------------------------------------------
 * Walking-0 Test
 *
 * One bit is cleared to 0 at a time.
 *
 * FFFFFFFE
 * FFFFFFFD
 * FFFFFFFB
 * ...
 * 7FFFFFFF
 *
 * Every pattern is written to every RAM test word
 * and then verified.
 * ---------------------------------------------------------- */

static ram_test_status_t ram_test_walking_0(void)
{
    volatile uint32_t *address;

    uint32_t start_address;
    uint32_t end_address;

    uint32_t bit;
    uint32_t pattern;


    start_address =
        (uint32_t)&__ram_test_start__;

    end_address =
        (uint32_t)&__ram_test_end__;


    /*
     * Test every bit position
     */

    for (bit = 0U; bit < 32U; bit++)
    {
        /*
         * Start with all bits = 1
         *
         * Then clear one bit.
         *
         * Example:
         *
         * bit = 0 → 0xFFFFFFFE
         * bit = 1 → 0xFFFFFFFD
         * bit = 2 → 0xFFFFFFFB
         *
         */

        pattern = ~(1UL << bit);


        /*
         * Write pattern to entire test region
         */

        for (address =
                 (volatile uint32_t *)start_address;

             (uint32_t)address < end_address;

             address++)
        {
            *address = pattern;
        }


        /*
         * Verify entire test region
         */

        for (address =
                 (volatile uint32_t *)start_address;

             (uint32_t)address < end_address;

             address++)
        {
        	if (*address != pattern)
        	{
        	    ram_test_fault_info.address =
        	        (uint32_t)address;

        	    ram_test_fault_info.expected =
        	        pattern;

        	    ram_test_fault_info.actual =
        	        *address;

        	    ram_test_fault_info.test_type =
        	        RAM_TEST_TYPE_WALKING_0;

        	    return RAM_TEST_FAULT_WRITE_READ;
        	}
        }
    }


    return RAM_TEST_OK;
}

/* ----------------------------------------------------------
 * Address-Bus Test
 *
 * Checks whether individual address locations can be
 * accessed independently.
 *
 * The test uses power-of-two address offsets.
 *
 * Example:
 *
 * base + 0x00
 * base + 0x04
 * base + 0x08
 * base + 0x10
 * ...
 *
 * A pattern is written to each selected location.
 * The other locations are then checked for corruption.
 * ---------------------------------------------------------- */

static ram_test_status_t ram_test_address_bus(void)
{
    volatile uint32_t *base;
    volatile uint32_t *address;

    uint32_t start_address;
    uint32_t end_address;
    uint32_t test_size;
    uint32_t offset;
    uint32_t pattern;


    start_address =
        (uint32_t)&__ram_test_start__;

    end_address =
        (uint32_t)&__ram_test_end__;


    /*
     * Base address of the test region.
     */

    base =
        (volatile uint32_t *)start_address;


    /*
     * Size of the test region.
     */

    test_size =
        end_address - start_address;


    /*
     * The test requires at least two 32-bit words.
     */

    if (test_size < 8U)
    {
        return RAM_TEST_FAULT_ADDRESS;
    }


    /*
     * ------------------------------------------------------
     * Step 1
     *
     * Fill the complete region with zero.
     * ------------------------------------------------------
     */

    for (address = base;
         (uint32_t)address < end_address;
         address++)
    {
        *address = 0x00000000UL;
    }


    /*
     * ------------------------------------------------------
     * Step 2
     *
     * Test each power-of-two address offset.
     *
     * Since our memory is word-addressable here,
     * offsets increase by 4 bytes.
     *
     * 0x00
     * 0x04
     * 0x08
     * 0x10
     * 0x20
     * 0x40
     * 0x80
     * ------------------------------------------------------
     */

    for (offset = 4U;
         offset < test_size;
         offset <<= 1U)
    {
        /*
         * Select a unique test pattern.
         */

        pattern =
            0xA5000000UL | offset;


        /*
         * Write pattern to selected address.
         */

        address =
            (volatile uint32_t *)
            (start_address + offset);

        *address = pattern;


        /*
         * Verify the selected address.
         */

        if (*address != pattern)
        {
            ram_test_fault_info.address =
                (uint32_t)address;

            ram_test_fault_info.expected =
                pattern;

            ram_test_fault_info.actual =
                *address;

            ram_test_fault_info.test_type =
                RAM_TEST_TYPE_ADDRESS_BUS;

            return RAM_TEST_FAULT_WRITE_READ;
        }


        /*
         * Verify base address has not changed.
         */

        if (*base != 0x00000000UL)
        {
            ram_test_fault_info.address =
                start_address;

            ram_test_fault_info.expected =
                0x00000000UL;

            ram_test_fault_info.actual =
                *base;

            ram_test_fault_info.test_type =
                RAM_TEST_TYPE_ADDRESS_BUS;

            return RAM_TEST_FAULT_WRITE_READ;
        }


        /*
         * Clear selected location again.
         */

        *address = 0x00000000UL;
    }


    /*
     * ------------------------------------------------------
     * Step 3
     *
     * Test the highest valid address in the region.
     * ------------------------------------------------------
     */

    address =
        (volatile uint32_t *)
        (end_address - sizeof(uint32_t));


    pattern = 0x5A5A5A5AUL;

    *address = pattern;


    if (*address != pattern)
    {
        ram_test_fault_info.address =
            (uint32_t)address;

        ram_test_fault_info.expected =
            pattern;

        ram_test_fault_info.actual =
            *address;

        ram_test_fault_info.test_type =
            RAM_TEST_TYPE_ADDRESS_BUS;

        return RAM_TEST_FAULT_WRITE_READ;
    }


    /*
     * Verify base address again.
     */

    if (*base != 0x00000000UL)
    {
        ram_test_fault_info.address =
            start_address;

        ram_test_fault_info.expected =
            0x00000000UL;

        ram_test_fault_info.actual =
            *base;

        ram_test_fault_info.test_type =
            RAM_TEST_TYPE_ADDRESS_BUS;

        return RAM_TEST_FAULT_WRITE_READ;
    }


    /*
     * Clear final test location.
     */

    *address = 0x00000000UL;


    return RAM_TEST_OK;
}
/* ----------------------------------------------------------
 * Complete RAM Test
 * ---------------------------------------------------------- */

ram_test_status_t ram_test_run(void)
{
    uint32_t start_address;
    uint32_t end_address;

    ram_test_status_t status;


    if (ram_test_initialized == 0U)
    {
        return RAM_TEST_ERROR_NOT_INITIALIZED;
    }

    ram_test_fault_info.address = 0U;
    ram_test_fault_info.expected = 0U;
    ram_test_fault_info.actual = 0U;
    ram_test_fault_info.test_type = RAM_TEST_TYPE_NONE;
    /*
     * Get test region
     */

    start_address =
        (uint32_t)&__ram_test_start__;

    end_address =
        (uint32_t)&__ram_test_end__;


    /*
     * Address validation
     */

    if (start_address >= end_address)
    {
        ram_test_fault = 1U;

        return RAM_TEST_FAULT_ADDRESS;
    }


    /*
     * Make sure the region is 32-bit aligned.
     */

    if (((start_address & 0x03UL) != 0U) ||
        ((end_address & 0x03UL) != 0U))
    {
        ram_test_fault = 1U;

        return RAM_TEST_FAULT_ADDRESS;
    }


    /*
     * ------------------------------------------------------
     * TEST 1
     * Basic data patterns
     * ------------------------------------------------------
     */

    status = ram_test_pattern_test();

    if (status != RAM_TEST_OK)
    {
        ram_test_fault = 1U;

        return status;
    }


    /*
     * ------------------------------------------------------
     * TEST 2
     * Walking-1
     * ------------------------------------------------------
     */

    status = ram_test_walking_1();

    if (status != RAM_TEST_OK)
    {
        ram_test_fault = 1U;

        return status;
    }


    /*
     * ------------------------------------------------------
     * TEST 3
     * Walking-0
     * ------------------------------------------------------
     */

    status = ram_test_walking_0();

    if (status != RAM_TEST_OK)
    {
        ram_test_fault = 1U;

        return status;
    }

    /*
     * ------------------------------------------------------
     * TEST 4
     * Address-Bus Test
     * ------------------------------------------------------
     */

    status = ram_test_address_bus();

    if (status != RAM_TEST_OK)
    {
        ram_test_fault = 1U;

        return status;
    }
    /*
     * Everything passed.
     */

    ram_test_fault = 0U;

    return RAM_TEST_OK;
}


/* ----------------------------------------------------------
 * RAM Test Health Check
 * ---------------------------------------------------------- */

ram_test_status_t ram_test_health_check(void)
{
    if (ram_test_initialized == 0U)
    {
        return RAM_TEST_ERROR_NOT_INITIALIZED;
    }


    if (ram_test_fault != 0U)
    {
        return RAM_TEST_FAULT_WRITE_READ;
    }


    return RAM_TEST_OK;
}


/* ----------------------------------------------------------
 * RAM Test Information
 * ---------------------------------------------------------- */

ram_test_status_t ram_test_get_info(
    ram_test_info_t *info)
{
    uint32_t start_address;
    uint32_t end_address;


    if (info == 0)
    {
        return RAM_TEST_ERROR_PARAMETER;
    }


    if (ram_test_initialized == 0U)
    {
        return RAM_TEST_ERROR_NOT_INITIALIZED;
    }


    start_address =
        (uint32_t)&__ram_test_start__;

    end_address =
        (uint32_t)&__ram_test_end__;


    info->start_address =
        start_address;

    info->end_address =
        end_address;

    info->size_bytes =
        end_address - start_address;


    return RAM_TEST_OK;
}


/* ----------------------------------------------------------
 * Fault Status
 * ---------------------------------------------------------- */

uint8_t ram_test_fault_active(void)
{
    return ram_test_fault;
}


/* ----------------------------------------------------------
 * Clear Fault
 * ---------------------------------------------------------- */

void ram_test_clear_fault(void)
{
    ram_test_fault = 0U;
}
/* ----------------------------------------------------------
 * Fault Injection
 * ---------------------------------------------------------- */

void ram_test_inject_fault(void)
{
    ram_test_fault_injection = 1U;
}
/* ----------------------------------------------------------
 * Get Fault Information
 * ---------------------------------------------------------- */

ram_test_status_t ram_test_get_fault_info(
    ram_test_fault_info_t *info)
{
    if (info == 0)
    {
        return RAM_TEST_ERROR_PARAMETER;
    }

    if (ram_test_initialized == 0U)
    {
        return RAM_TEST_ERROR_NOT_INITIALIZED;
    }

    info->address =
        ram_test_fault_info.address;

    info->expected =
        ram_test_fault_info.expected;

    info->actual =
        ram_test_fault_info.actual;

    info->test_type =
        ram_test_fault_info.test_type;

    return RAM_TEST_OK;
}
