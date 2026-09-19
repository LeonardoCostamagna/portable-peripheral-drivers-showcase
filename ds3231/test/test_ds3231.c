/**
 * @file test_ds3231.c
 * 
 * @copyright Copyright (c) 2026 Leonardo Costamagna
 * 
 * SPDX-License-Identifier: MIT
 */


#include "ds3231.h"

#include "unity.h"
#include "mock_interface.h"

/* Dummy addresses used as mock peripheral handles and context pointers */
static int dummy_i2c_handle = 0x4321;
static int dummy_private_data = 0x8765;
static ds3231_dev_t dev;

void setUp(void) {
}

void tearDown(void) {
}

/**
 * @brief Helper function to initialize the device context with mock I2C callbacks.
 */
static void helper_init_dev(void) {
    dev.ctx.write_reg = mock_i2c_write;
    dev.ctx.read_reg = mock_i2c_read;
    dev.ctx.handle = &dummy_i2c_handle;
    dev.ctx.private_data = &dummy_private_data;
}

/* ============================================================================
   AUXILIARY FUNCTIONS: ds3231_read_reg / ds3231_write_reg
   ============================================================================ */

/**
 * @brief Tests that ds3231_read_reg fails when passed a NULL context or NULL callback.
 */
void test_ds3231_read_reg_null_ctx_or_callback(void) {
    ds3231_dev_ctx_t ctx = { .read_reg = NULL, .write_reg = mock_i2c_write };
    uint8_t data;

    TEST_ASSERT_EQUAL(-1, ds3231_read_reg(NULL, DS3231_REG_STATUS, &data, 1));
    TEST_ASSERT_EQUAL(-1, ds3231_read_reg(&ctx, DS3231_REG_STATUS, &data, 1));
}

/**
 * @brief Tests successful reading of a register using CMock pointer manipulation.
 */
void test_ds3231_read_reg_success(void) {
    helper_init_dev();
    uint8_t rx_data = 0;
    uint8_t simulated_data = 0x55;

    // Set expectation for I2C read; NULL passed for data pointer because address comparison is ignored next
    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_STATUS, NULL, 1, 0);
    // Ignore pointer address check for the 'data' argument
    mock_i2c_read_IgnoreArg_data();
    // Inject simulated hardware register data into the destination buffer via pointer
    mock_i2c_read_ReturnArrayThruPtr_data(&simulated_data, 1);

    TEST_ASSERT_EQUAL(0, ds3231_read_reg(&dev.ctx, DS3231_REG_STATUS, &rx_data, 1));
    TEST_ASSERT_EQUAL(0x55, rx_data);
}

/**
 * @brief Tests that ds3231_write_reg fails when passed a NULL context or NULL callback.
 */
void test_ds3231_write_reg_null_ctx_or_callback(void) {
    ds3231_dev_ctx_t ctx = { .read_reg = mock_i2c_read, .write_reg = NULL };
    uint8_t data = 0x10;

    TEST_ASSERT_EQUAL(-1, ds3231_write_reg(NULL, DS3231_REG_CONTROL, &data, 1));
    TEST_ASSERT_EQUAL(-1, ds3231_write_reg(&ctx, DS3231_REG_CONTROL, &data, 1));
}

/**
 * @brief Tests successful writing to a register, verifying byte contents using array comparison.
 */
void test_ds3231_write_reg_success(void) {
    helper_init_dev();
    uint8_t tx_data = 0xAA;

    // ExpectWithArray compares buffer contents rather than just pointer addresses
    mock_i2c_write_ExpectWithArrayAndReturn(&dummy_i2c_handle, 1, DS3231_REG_CONTROL, &tx_data, 1, 1, 0);

    TEST_ASSERT_EQUAL(0, ds3231_write_reg(&dev.ctx, DS3231_REG_CONTROL, &tx_data, 1));
}

/* ============================================================================
   INITIALIZATION: ds3231_init
   ============================================================================ */

/**
 * @brief Tests that ds3231_init rejects invalid/NULL pointers for critical driver arguments.
 */
void test_ds3231_init_null_args(void) {
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_init(NULL, mock_i2c_write, mock_i2c_read, &dummy_i2c_handle, NULL));
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_init(&dev, NULL, mock_i2c_read, &dummy_i2c_handle, NULL));
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_init(&dev, mock_i2c_write, NULL, &dummy_i2c_handle, NULL));
}

/**
 * @brief Tests initialization failure when reading the STATUS register fails (I2C error).
 */
void test_ds3231_init_status_read_error(void) {
    // Simulate I2C read error (-1) during initial status register inspection
    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_STATUS, NULL, 1, -1);
    mock_i2c_read_IgnoreArg_data();

    TEST_ASSERT_EQUAL(DS3231_ERR_COMM, ds3231_init(&dev, mock_i2c_write, mock_i2c_read, &dummy_i2c_handle, NULL));
}

/**
 * @brief Tests initialization failure when reading the CONTROL register fails after STATUS succeeded.
 */
void test_ds3231_init_control_read_error(void) {
    uint8_t status_reg = DS3231_STAT_A1F;

    // First read (STATUS) succeeds
    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_STATUS, NULL, 1, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(&status_reg, 1);

    // Second read (CONTROL) fails
    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_CONTROL, NULL, 1, -1);
    mock_i2c_read_IgnoreArg_data();

    TEST_ASSERT_EQUAL(DS3231_ERR_COMM, ds3231_init(&dev, mock_i2c_write, mock_i2c_read, &dummy_i2c_handle, NULL));
}

/**
 * @brief Tests complete driver initialization, checking initial flag parsing and context assignment.
 */
void test_ds3231_init_success(void) {
    // Both alarm flags active in status register
    uint8_t status_reg = DS3231_STAT_A1F | DS3231_STAT_A2F;
    // Only Alarm 1 interrupt enabled in control register
    uint8_t ctrl_reg = DS3231_CTRL_A1IE;

    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_STATUS, NULL, 1, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(&status_reg, 1);

    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_CONTROL, NULL, 1, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(&ctrl_reg, 1);

    ds3231_status_t status = ds3231_init(&dev, mock_i2c_write, mock_i2c_read, &dummy_i2c_handle, &dummy_private_data);

    TEST_ASSERT_EQUAL(DS3231_OK, status);
    TEST_ASSERT_EQUAL_PTR(&dummy_i2c_handle, dev.ctx.handle);
    TEST_ASSERT_EQUAL_PTR(&dummy_private_data, dev.ctx.private_data);
    // Verify internal state reflects hardware status/control registers
    TEST_ASSERT_TRUE(dev.alarm1.triggered);
    TEST_ASSERT_TRUE(dev.alarm2.triggered);
    TEST_ASSERT_TRUE(dev.alarm1.enabled);
    TEST_ASSERT_FALSE(dev.alarm2.enabled);
}

/* ============================================================================
   TIME CONFIGURATION: ds3231_set_time
   ============================================================================ */

/**
 * @brief Tests parameter validation for NULL pointer inputs in ds3231_set_time.
 */
void test_ds3231_set_time_null_args(void) {
    ds3231_time_t time = { .seconds = 0, .minutes = 0, .hours = 0, .day_of_week = 1, .day = 1, .month = 1, .year = 2026 };
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_set_time(NULL, &time));
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_set_time(&dev, NULL));
}

/**
 * @brief Systematically mutates each time field out of valid bounds to test input validation.
 */
void test_ds3231_set_time_out_of_bounds(void) {
    ds3231_time_t t = { .seconds = 30, .minutes = 15, .hours = 10, .day_of_week = 1, .day = 15, .month = 6, .year = 2026 };

    // Each field is modified past its valid range, tested, and restored sequentially
    t.seconds = 60; TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_set_time(&dev, &t)); t.seconds = 30;
    t.minutes = 60; TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_set_time(&dev, &t)); t.minutes = 15;
    t.hours = 24;   TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_set_time(&dev, &t)); t.hours = 10;
    t.day = 0;      TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_set_time(&dev, &t));
    t.day = 32;     TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_set_time(&dev, &t)); t.day = 15;
    t.month = 0;    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_set_time(&dev, &t));
    t.month = 13;   TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_set_time(&dev, &t)); t.month = 6;
    t.day_of_week = 0; TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_set_time(&dev, &t));
    t.day_of_week = 8; TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_set_time(&dev, &t));
}

/**
 * @brief Tests communication error handling during time register burst write.
 */
void test_ds3231_set_time_comm_error(void) {
    helper_init_dev();
    ds3231_time_t time = { .seconds = 45, .minutes = 30, .hours = 14, .day_of_week = 3, .day = 25, .month = 12, .year = 2026 };

    mock_i2c_write_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_SECONDS, NULL, 7, -1);
    mock_i2c_write_IgnoreArg_data();

    TEST_ASSERT_EQUAL(DS3231_ERR_COMM, ds3231_set_time(&dev, &time));
}

/**
 * @brief Tests converting decimal time structure into BCD representation written over I2C.
 */
void test_ds3231_set_time_success(void) {
    helper_init_dev();
    ds3231_time_t time = { .seconds = 45, .minutes = 30, .hours = 14, .day_of_week = 3, .day = 25, .month = 12, .year = 2026 };
    // BCD conversion array expected by hardware: 45->0x45, 30->0x30, 14->0x14, 3->0x03, 25->0x25, 12->0x12, 2026->0x26
    uint8_t expected_bcd[] = { 0x45, 0x30, 0x14, 0x03, 0x25, 0x12, 0x26 };

    mock_i2c_write_ExpectWithArrayAndReturn(&dummy_i2c_handle, 1, DS3231_REG_SECONDS, expected_bcd, 7, 7, 0);

    TEST_ASSERT_EQUAL(DS3231_OK, ds3231_set_time(&dev, &time));
    TEST_ASSERT_EQUAL(2026, dev.last_time.year);
}

/* ============================================================================
   TIME READING: ds3231_get_time
   ============================================================================ */

/**
 * @brief Tests NULL argument validation in ds3231_get_time.
 */
void test_ds3231_get_time_null_args(void) {
    ds3231_time_t time;
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_get_time(NULL, &time));
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_get_time(&dev, NULL));
}

/**
 * @brief Tests error propagation when I2C read fails during time retrieval.
 */
void test_ds3231_get_time_comm_error(void) {
    helper_init_dev();
    ds3231_time_t time;

    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_SECONDS, NULL, 7, -1);
    mock_i2c_read_IgnoreArg_data();

    TEST_ASSERT_EQUAL(DS3231_ERR_COMM, ds3231_get_time(&dev, &time));
}

/**
 * @brief Tests reading BCD registers from DS3231 and decoding them into decimal time structure.
 */
void test_ds3231_get_time_success(void) {
    helper_init_dev();
    // BCD raw data from RTC: 15s, 42m, 21h, Thu (5), 18th, Sept (9), 2026 (0x26)
    uint8_t bcd_response[] = { 0x15, 0x42, 0x21, 0x05, 0x18, 0x09, 0x26 };
    ds3231_time_t read_time;

    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_SECONDS, NULL, 7, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(bcd_response, 7);

    TEST_ASSERT_EQUAL(DS3231_OK, ds3231_get_time(&dev, &read_time));
    TEST_ASSERT_EQUAL(15, read_time.seconds);
    TEST_ASSERT_EQUAL(42, read_time.minutes);
    TEST_ASSERT_EQUAL(21, read_time.hours);
    TEST_ASSERT_EQUAL(5,  read_time.day_of_week);
    TEST_ASSERT_EQUAL(18, read_time.day);
    TEST_ASSERT_EQUAL(9,  read_time.month);
    TEST_ASSERT_EQUAL(2026, read_time.year);
    TEST_ASSERT_EQUAL(2026, dev.last_time.year);
}

/* ============================================================================
   TEMPERATURE READING: ds3231_get_temperature
   ============================================================================ */

/**
 * @brief Tests NULL argument validation for ds3231_get_temperature.
 */
void test_ds3231_get_temperature_null_args(void) {
    float temp;
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_get_temperature(NULL, &temp));
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_get_temperature(&dev, NULL));
}

/**
 * @brief Tests communication error during 2-byte temperature register read.
 */
void test_ds3231_get_temperature_comm_error(void) {
    helper_init_dev();
    float temp;

    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_TEMP_MSB, NULL, 2, -1);
    mock_i2c_read_IgnoreArg_data();

    TEST_ASSERT_EQUAL(DS3231_ERR_COMM, ds3231_get_temperature(&dev, &temp));
}

/**
 * @brief Tests temperature register decoding for positive and negative fractional values.
 */
void test_ds3231_get_temperature_decoding(void) {
    helper_init_dev();
    float temp = 0.0f;

    // Case 1: Positive temperature (+25.75 °C)
    // MSB = 0x19 (25 decimal), LSB = 0xC0 (upper 2 bits = 11 binary -> 3 * 0.25 = 0.75 °C)
    uint8_t temp_pos[] = { 0x19, 0xC0 };
    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_TEMP_MSB, NULL, 2, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(temp_pos, 2);

    TEST_ASSERT_EQUAL(DS3231_OK, ds3231_get_temperature(&dev, &temp));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.75f, temp);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.75f, dev.last_temperature);

    // Case 2: Negative temperature (-10.25 °C)
    // MSB = 0xF5 (two's complement -11), LSB = 0xC0 (upper bits = 0.75 -> -11 + 0.75 = -10.25 °C)
    uint8_t temp_neg[] = { 0xF5, 0xC0 };
    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_TEMP_MSB, NULL, 2, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(temp_neg, 2);

    TEST_ASSERT_EQUAL(DS3231_OK, ds3231_get_temperature(&dev, &temp));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -10.25f, temp);
}

/* ============================================================================
   ALARM CONFIGURATION: ds3231_set_alarm
   ============================================================================ */

/**
 * @brief Tests NULL argument validation in ds3231_set_alarm.
 */
void test_ds3231_set_alarm_null_args(void) {
    ds3231_alarm_t alarm = {0};
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_set_alarm(NULL, DS3231_ALARM_1, &alarm));
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_set_alarm(&dev, DS3231_ALARM_1, NULL));
}

/**
 * @brief Tests that passing an out-of-range alarm ID returns an invalid argument error.
 */
void test_ds3231_set_alarm_invalid_id(void) {
    ds3231_alarm_t alarm = {0};
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_set_alarm(&dev, (ds3231_alarm_id_t)99, &alarm));
}

/**
 * @brief Tests configuring Alarm 1 (4-byte sequence: sec, min, hr, day/date with mask bits).
 */
void test_ds3231_set_alarm1_success(void) {
    helper_init_dev();
    ds3231_alarm_t alarm1 = { .seconds = 10, .minutes = 20, .hours = 12, .day_or_date = 5, .mode = DS3231_A1_MATCH_HRS_MIN_SEC };
    // 0x10 (10s), 0x20 (20m), 0x12 (12h), 0x85 (0x05 day + 0x80 mask bit A1M4)
    uint8_t expected[] = { 0x10, 0x20, 0x12, 0x85 };

    mock_i2c_write_ExpectWithArrayAndReturn(&dummy_i2c_handle, 1, DS3231_REG_ALARM1_SEC, expected, 4, 4, 0);

    TEST_ASSERT_EQUAL(DS3231_OK, ds3231_set_alarm(&dev, DS3231_ALARM_1, &alarm1));
    TEST_ASSERT_EQUAL(10, dev.alarm1.config.seconds);
}

/**
 * @brief Tests configuring Alarm 2 (3-byte sequence: min, hr, day/date; Alarm 2 lacks seconds).
 */
void test_ds3231_set_alarm2_success(void) {
    helper_init_dev();
    ds3231_alarm_t alarm2 = { .minutes = 30, .hours = 15, .day_or_date = 2, .mode = DS3231_A2_MATCH_HRS_MIN };
    // 0x30 (30m), 0x15 (15h), 0x82 (0x02 day + 0x80 mask bit A2M4)
    uint8_t expected[] = { 0x30, 0x15, 0x82 };

    mock_i2c_write_ExpectWithArrayAndReturn(&dummy_i2c_handle, 1, DS3231_REG_ALARM2_MIN, expected, 3, 3, 0);

    TEST_ASSERT_EQUAL(DS3231_OK, ds3231_set_alarm(&dev, DS3231_ALARM_2, &alarm2));
    TEST_ASSERT_EQUAL(30, dev.alarm2.config.minutes);
}

/**
 * @brief Tests communication error during alarm setup write.
 */
void test_ds3231_set_alarm_comm_error(void) {
    helper_init_dev();
    ds3231_alarm_t alarm = {0};

    mock_i2c_write_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_ALARM1_SEC, NULL, 4, -1);
    mock_i2c_write_IgnoreArg_data();

    TEST_ASSERT_EQUAL(DS3231_ERR_COMM, ds3231_set_alarm(&dev, DS3231_ALARM_1, &alarm));
}

/* ============================================================================
   INTERRUPT CONTROL: ds3231_toggle_alarm_interrupt
   ============================================================================ */

/**
 * @brief Tests NULL device pointer check in ds3231_toggle_alarm_interrupt.
 */
void test_ds3231_toggle_alarm_interrupt_null_dev(void) {
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_toggle_alarm_interrupt(NULL, DS3231_ALARM_1, true));
}

/**
 * @brief Tests error handling when reading CONTROL register fails before toggling interrupt bits.
 */
void test_ds3231_toggle_alarm_interrupt_read_error(void) {
    helper_init_dev();

    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_CONTROL, NULL, 1, -1);
    mock_i2c_read_IgnoreArg_data();

    TEST_ASSERT_EQUAL(DS3231_ERR_COMM, ds3231_toggle_alarm_interrupt(&dev, DS3231_ALARM_1, true));
}

/**
 * @brief Tests error handling when writing updated CONTROL register fails.
 */
void test_ds3231_toggle_alarm_interrupt_write_error(void) {
    helper_init_dev();
    uint8_t ctrl_reg = 0x00;

    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_CONTROL, NULL, 1, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(&ctrl_reg, 1);

    mock_i2c_write_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_CONTROL, NULL, 1, -1);
    mock_i2c_write_IgnoreArg_data();

    TEST_ASSERT_EQUAL(DS3231_ERR_COMM, ds3231_toggle_alarm_interrupt(&dev, DS3231_ALARM_1, true));
}

/**
 * @brief Tests enabling Alarm 1 interrupt and disabling Alarm 2 interrupt sequentially.
 */
void test_ds3231_toggle_alarm_interrupt_enable_a1_and_disable_a2(void) {
    helper_init_dev();
    uint8_t ctrl_initial = DS3231_CTRL_A2IE;
    // INTCN (Interrupt Control) bit must be set along with A1IE when enabling interrupts
    uint8_t ctrl_expected_a1 = DS3231_CTRL_A2IE | DS3231_CTRL_INTCN | DS3231_CTRL_A1IE;

    // Step 1: Enable Alarm 1 Interrupt
    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_CONTROL, NULL, 1, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(&ctrl_initial, 1);

    mock_i2c_write_ExpectWithArrayAndReturn(&dummy_i2c_handle, 1, DS3231_REG_CONTROL, &ctrl_expected_a1, 1, 1, 0);

    TEST_ASSERT_EQUAL(DS3231_OK, ds3231_toggle_alarm_interrupt(&dev, DS3231_ALARM_1, true));
    TEST_ASSERT_TRUE(dev.alarm1.enabled);

    // Step 2: Disable Alarm 2 Interrupt (A2IE bit cleared)
    uint8_t ctrl_expected_a2_off = DS3231_CTRL_INTCN | DS3231_CTRL_A1IE;
    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_CONTROL, NULL, 1, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(&ctrl_expected_a1, 1);

    mock_i2c_write_ExpectWithArrayAndReturn(&dummy_i2c_handle, 1, DS3231_REG_CONTROL, &ctrl_expected_a2_off, 1, 1, 0);

    TEST_ASSERT_EQUAL(DS3231_OK, ds3231_toggle_alarm_interrupt(&dev, DS3231_ALARM_2, false));
    TEST_ASSERT_FALSE(dev.alarm2.enabled);
}

/* ============================================================================
   ALARM FLAG CHECK AND CLEAR
   ============================================================================ */

/**
 * @brief Tests NULL argument validation in ds3231_check_alarm_flag.
 */
void test_ds3231_check_alarm_flag_null_args(void) {
    bool flag;
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_check_alarm_flag(NULL, DS3231_ALARM_1, &flag));
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_check_alarm_flag(&dev, DS3231_ALARM_1, NULL));
}

/**
 * @brief Tests communication error handling when reading STATUS register flag.
 */
void test_ds3231_check_alarm_flag_comm_error(void) {
    helper_init_dev();
    bool flag;

    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_STATUS, NULL, 1, -1);
    mock_i2c_read_IgnoreArg_data();

    TEST_ASSERT_EQUAL(DS3231_ERR_COMM, ds3231_check_alarm_flag(&dev, DS3231_ALARM_1, &flag));
}

/**
 * @brief Tests reading STATUS register to check if Alarm 1 flag (A1F) is active.
 */
void test_ds3231_check_alarm_flag_success(void) {
    helper_init_dev();
    uint8_t status_reg = DS3231_STAT_A1F;
    bool triggered = false;

    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_STATUS, NULL, 1, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(&status_reg, 1);

    TEST_ASSERT_EQUAL(DS3231_OK, ds3231_check_alarm_flag(&dev, DS3231_ALARM_1, &triggered));
    TEST_ASSERT_TRUE(triggered);
    TEST_ASSERT_TRUE(dev.alarm1.triggered);
    TEST_ASSERT_FALSE(dev.alarm2.triggered);
}

/**
 * @brief Tests edge cases for clear_alarm_flag (NULL dev vs invalid alarm ID).
 */
void test_ds3231_clear_alarm_flag_null_or_invalid(void) {
    // Case 1: NULL pointer returns immediately without attempting I2C read
    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_clear_alarm_flag(NULL, DS3231_ALARM_1));

    // Case 2: Note that implementation reads STATUS register *before* validating ID enum,
    // so an I2C expectation is required even though function ultimately returns DS3231_ERR_INVALID_ARG
    helper_init_dev();
    uint8_t status_reg = 0x00;

    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_STATUS, NULL, 1, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(&status_reg, 1);

    TEST_ASSERT_EQUAL(DS3231_ERR_INVALID_ARG, ds3231_clear_alarm_flag(&dev, (ds3231_alarm_id_t)99));
}

/**
 * @brief Tests communication error handling on both read and write phases of clearing an alarm flag.
 */
void test_ds3231_clear_alarm_flag_comm_errors(void) {
    helper_init_dev();

    // Read phase error
    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_STATUS, NULL, 1, -1);
    mock_i2c_read_IgnoreArg_data();
    TEST_ASSERT_EQUAL(DS3231_ERR_COMM, ds3231_clear_alarm_flag(&dev, DS3231_ALARM_1));

    // Write phase error
    uint8_t status_reg = DS3231_STAT_A1F;
    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_STATUS, NULL, 1, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(&status_reg, 1);

    mock_i2c_write_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_STATUS, NULL, 1, -1);
    mock_i2c_write_IgnoreArg_data();
    TEST_ASSERT_EQUAL(DS3231_ERR_COMM, ds3231_clear_alarm_flag(&dev, DS3231_ALARM_1));
}

/**
 * @brief Tests clearing Alarm 1 and Alarm 2 flags sequentially using read-modify-write on STATUS register.
 */
void test_ds3231_clear_alarm_flag_success(void) {
    helper_init_dev();
    uint8_t status_reg = DS3231_STAT_A1F | DS3231_STAT_A2F;
    // Clearing A1F leaves A2F set (writing 0 clears active flag bit in DS3231)
    uint8_t expected_clear_a1 = DS3231_STAT_A2F;

    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_STATUS, NULL, 1, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(&status_reg, 1);

    mock_i2c_write_ExpectWithArrayAndReturn(&dummy_i2c_handle, 1, DS3231_REG_STATUS, &expected_clear_a1, 1, 1, 0);

    TEST_ASSERT_EQUAL(DS3231_OK, ds3231_clear_alarm_flag(&dev, DS3231_ALARM_1));
    TEST_ASSERT_FALSE(dev.alarm1.triggered);

    // Clearing remaining A2F flag leaves STATUS register cleared (0x00)
    uint8_t expected_clear_a2 = 0x00;
    mock_i2c_read_ExpectAndReturn(&dummy_i2c_handle, DS3231_REG_STATUS, NULL, 1, 0);
    mock_i2c_read_IgnoreArg_data();
    mock_i2c_read_ReturnArrayThruPtr_data(&expected_clear_a1, 1);

    mock_i2c_write_ExpectWithArrayAndReturn(&dummy_i2c_handle, 1, DS3231_REG_STATUS, &expected_clear_a2, 1, 1, 0);

    TEST_ASSERT_EQUAL(DS3231_OK, ds3231_clear_alarm_flag(&dev, DS3231_ALARM_2));
    TEST_ASSERT_FALSE(dev.alarm2.triggered);
}