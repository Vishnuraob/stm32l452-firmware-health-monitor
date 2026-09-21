/*
 * ram_test.h
 *
 *  Created on: 16-Sept-2026
 *      Author: engineer
 */
#ifndef RAM_TEST_H
#define RAM_TEST_H

#include <stdint.h>

typedef enum
{
    RAM_TEST_OK = 0,

    RAM_TEST_ERROR_NOT_INITIALIZED,
    RAM_TEST_ERROR_PARAMETER,

    RAM_TEST_FAULT_WRITE_READ,
    RAM_TEST_FAULT_ADDRESS

} ram_test_status_t;

typedef struct
{
    uint32_t start_address;
    uint32_t end_address;
    uint32_t size_bytes;

} ram_test_info_t;

typedef enum
{
    RAM_TEST_TYPE_NONE = 0,
    RAM_TEST_TYPE_PATTERN,
    RAM_TEST_TYPE_WALKING_1,
    RAM_TEST_TYPE_WALKING_0,
    RAM_TEST_TYPE_ADDRESS_BUS
} ram_test_type_t;

typedef struct
{
    uint32_t address;
    uint32_t expected;
    uint32_t actual;
    ram_test_type_t test_type;

} ram_test_fault_info_t;

ram_test_status_t ram_test_init(void);

ram_test_status_t ram_test_run(void);

ram_test_status_t ram_test_health_check(void);

ram_test_status_t ram_test_get_info(
    ram_test_info_t *info);

uint8_t ram_test_fault_active(void);

void ram_test_clear_fault(void);
void ram_test_inject_fault(void);

ram_test_status_t ram_test_get_fault_info(
    ram_test_fault_info_t *info);
#endif /* RAM_TEST_H */
