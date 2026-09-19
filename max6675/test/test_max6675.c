/**
 * @file test_max6675.c
 * 
 * @copyright Copyright (c) 2026 Leonardo Costamagna
 * 
 * SPDX-License-Identifier: MIT
 */

#include "max6675.h"

#include "unity.h"
#include "mock_interface.h"

/* Dummy addresses used as mock peripheral handles and context pointers */
static int dummy_spi_handle = 0x5678;
static int dummy_private_data = 0x1234;
static max6675_dev_t dev;

void setUp(void) {
}

void tearDown(void) {
}

/**
 * @brief Helper function to initialize the device context with mock SPI stream callbacks.
 */
static void helper_init_dev(void) {
    dev.ctx.read_stream = mock_spi_read_stream;
    dev.ctx.cs_select = mock_cs_select;
    dev.ctx.handle = &dummy_spi_handle;
    dev.ctx.private_data = &dummy_private_data;
}

/* ============================================================================
   AUXILIARY FUNCTIONS: max6675_read_raw
   ============================================================================ */

/**
 * @brief Tests that max6675_read_raw fails when passed a NULL context, NULL callbacks, or NULL data pointer.
 */
void test_max6675_read_raw_null_args(void) {
    max6675_dev_ctx_t valid_ctx = { .read_stream = mock_spi_read_stream, .cs_select = mock_cs_select, .handle = &dummy_spi_handle };
    uint16_t data = 0;

    TEST_ASSERT_EQUAL(-1, max6675_read_raw(NULL, &data));
    
    max6675_dev_ctx_t ctx_no_read = { .read_stream = NULL, .cs_select = mock_cs_select, .handle = &dummy_spi_handle };
    TEST_ASSERT_EQUAL(-1, max6675_read_raw(&ctx_no_read, &data));

    max6675_dev_ctx_t ctx_no_cs = { .read_stream = mock_spi_read_stream, .cs_select = NULL, .handle = &dummy_spi_handle };
    TEST_ASSERT_EQUAL(-1, max6675_read_raw(&ctx_no_cs, &data));

    TEST_ASSERT_EQUAL(-1, max6675_read_raw(&valid_ctx, NULL));
}

/**
 * @brief Tests successful raw 16-bit stream read over SPI, ensuring correct CS sequence (Low then High) and byte assembly.
 */
void test_max6675_read_raw_success(void) {
    helper_init_dev();
    uint16_t rx_data = 0;
    uint8_t simulated_bytes[2] = {0x01, 0x93};

    // 1. Expect Chip Select Low (true)
    mock_cs_select_Expect(&dummy_spi_handle, true);

    // 2. Expect SPI stream read with handle, length, and return 0
    mock_spi_read_stream_ExpectAndReturn(&dummy_spi_handle, 2, 0);
    mock_spi_read_stream_IgnoreArg_data();
    mock_spi_read_stream_ReturnArrayThruPtr_data(simulated_bytes, 2);

    // 3. Expect Chip Select High (false)
    mock_cs_select_Expect(&dummy_spi_handle, false);

    TEST_ASSERT_EQUAL(0, max6675_read_raw(&dev.ctx, &rx_data));
    TEST_ASSERT_EQUAL(0x0193, rx_data);
}

/* ============================================================================
   INITIALIZATION: max6675_init
   ============================================================================ */

/**
 * @brief Tests that max6675_init rejects invalid/NULL pointers for critical driver arguments.
 */
void test_max6675_init_null_args(void) {
    TEST_ASSERT_EQUAL(MAX6675_ERR_INVALID_ARG, max6675_init(NULL, mock_spi_read_stream, mock_cs_select, &dummy_spi_handle, NULL));
    TEST_ASSERT_EQUAL(MAX6675_ERR_INVALID_ARG, max6675_init(&dev, NULL, mock_cs_select, &dummy_spi_handle, NULL));
    TEST_ASSERT_EQUAL(MAX6675_ERR_INVALID_ARG, max6675_init(&dev, mock_spi_read_stream, NULL, &dummy_spi_handle, NULL));
}

/**
 * @brief Tests complete driver initialization, checking initial state, context assignment, and deselection of CS pin.
 */
void test_max6675_init_success(void) {
    // Initialization should ensure CS pin starts deselected (High)
    mock_cs_select_Expect(&dummy_spi_handle, false);

    max6675_status_t status = max6675_init(&dev, mock_spi_read_stream, mock_cs_select, &dummy_spi_handle, &dummy_private_data);

    TEST_ASSERT_EQUAL(MAX6675_OK, status);
    TEST_ASSERT_EQUAL_PTR(&dummy_spi_handle, dev.ctx.handle);
    TEST_ASSERT_EQUAL_PTR(&dummy_private_data, dev.ctx.private_data);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, dev.last_temperature);
}

/* ============================================================================
   TEMPERATURE READING: max6675_get_temperature
   ============================================================================ */

/**
 * @brief Tests NULL argument validation for max6675_get_temperature.
 */
void test_max6675_get_temperature_null_args(void) {
    float temp;
    TEST_ASSERT_EQUAL(MAX6675_ERR_INVALID_ARG, max6675_get_temperature(NULL, &temp));
    TEST_ASSERT_EQUAL(MAX6675_ERR_INVALID_ARG, max6675_get_temperature(&dev, NULL));
}

/**
 * @brief Tests communication error handling when SPI stream read fails during temperature retrieval.
 */
void test_max6675_get_temperature_comm_error(void) {
    helper_init_dev();
    float temp;

    mock_cs_select_Expect(&dummy_spi_handle, true);
    mock_spi_read_stream_ExpectAndReturn(&dummy_spi_handle, 2, -1);
    mock_spi_read_stream_IgnoreArg_data();
    mock_cs_select_Expect(&dummy_spi_handle, false);

    TEST_ASSERT_EQUAL(MAX6675_ERR_COMM, max6675_get_temperature(&dev, &temp));
}

/**
 * @brief Tests open thermocouple error detection (Bit D2 high in raw response).
 */
void test_max6675_get_temperature_open_tc(void) {
    helper_init_dev();
    float temp;
    // Bit D2 set (0x0004) indicates open thermocouple -> bytes: {0x00, 0x04}
    uint8_t raw_open_tc[2] = {0x00, 0x04};

    mock_cs_select_Expect(&dummy_spi_handle, true);
    mock_spi_read_stream_ExpectAndReturn(&dummy_spi_handle, 2, 0);
    mock_spi_read_stream_IgnoreArg_data();
    mock_spi_read_stream_ReturnArrayThruPtr_data(raw_open_tc, 2);
    mock_cs_select_Expect(&dummy_spi_handle, false);

    TEST_ASSERT_EQUAL(MAX6675_ERR_OPEN_TC, max6675_get_temperature(&dev, &temp));
}

/**
 * @brief Tests successful raw temperature parsing and conversion to Celsius.
 */
void test_max6675_get_temperature_success(void) {
    helper_init_dev();
    float temp = 0.0f;
    // Raw value: suppose bits D14-D3 represent decimal 100. Shifted left by 3 = 0x0320.
    uint16_t raw_val = (100 << MAX6675_RAW_DATA_SHIFT);
    uint8_t raw_valid_temp[2] = { (uint8_t)(raw_val >> 8), (uint8_t)(raw_val & 0xFF) };

    mock_cs_select_Expect(&dummy_spi_handle, true);
    mock_spi_read_stream_ExpectAndReturn(&dummy_spi_handle, 2, 0);
    mock_spi_read_stream_IgnoreArg_data();
    mock_spi_read_stream_ReturnArrayThruPtr_data(raw_valid_temp, 2);
    mock_cs_select_Expect(&dummy_spi_handle, false);

    TEST_ASSERT_EQUAL(MAX6675_OK, max6675_get_temperature(&dev, &temp));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, temp);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, dev.last_temperature);
}