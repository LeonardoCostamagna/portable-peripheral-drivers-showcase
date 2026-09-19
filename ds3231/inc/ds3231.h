#ifndef DS3231_H_
#define DS3231_H_

/**
 * @file ds3231.h
 * @brief DS3231 Real-Time Clock (RTC) Driver.
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2026 Leonardo Costamagna
 * 
 * SPDX-License-Identifier: MIT
 */

/** 
 *  @defgroup ds3231 Driver DS3231
 *  @brief Module for managing the DS3231 RTC, hardware alarms, and internal temperature sensor.
 *  @{
 */

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Standard fixed I2C slave address for the DS3231
 */
#define DS3231_I2C_ADD           0x68

/** 
 * @defgroup DS3231_Register_Map Register Map
 * @brief Complete internal register map for the DS3231.
 * @{
 */

#define DS3231_REG_SECONDS       0x00
#define DS3231_REG_MINUTES       0x01
#define DS3231_REG_HOURS         0x02
#define DS3231_REG_DAY           0x03
#define DS3231_REG_DATE          0x04
#define DS3231_REG_MONTH         0x05
#define DS3231_REG_YEAR          0x06
// Alarm 1 Registers
#define DS3231_REG_ALARM1_SEC    0x07
#define DS3231_REG_ALARM1_MIN    0x08
#define DS3231_REG_ALARM1_HRS    0x09
#define DS3231_REG_ALARM1_DAY    0x0A
// Alarm 2 Registers
#define DS3231_REG_ALARM2_MIN    0x0B
#define DS3231_REG_ALARM2_HRS    0x0C
#define DS3231_REG_ALARM2_DAY    0x0D
// Control and Status Registers
#define DS3231_REG_CONTROL       0x0E
#define DS3231_REG_STATUS        0x0F
#define DS3231_REG_AGING_OFFSET  0x10
#define DS3231_REG_TEMP_MSB      0x11
#define DS3231_REG_TEMP_LSB      0x12

/** @} end DS3231_Register_Map Register Map group*/

/** 
 * @defgroup 3231_Bit_Masks Bit Masks
 * @brief Key bit masks for Control and Status registers.
 * @{
 */

// Key bits for Control register (0x0E)
#define DS3231_CTRL_A1IE    (1 << 0)
#define DS3231_CTRL_A2IE    (1 << 1)
#define DS3231_CTRL_INTCN   (1 << 2)

// Key bits for Status register (0x0F)
#define DS3231_STAT_A1F     (1 << 0)
#define DS3231_STAT_A2F     (1 << 1)

/** @} end DS3231_Bit_Masks Bit Masks group */

/** 
 * @defgroup Interfaces_Functions Communication Interfaces
 * @brief Bus function pointer prototypes and communication context.
 *        MANDATORY: Callback return 0 -> No Error.
 * @{
 */

typedef int32_t (*dev_write_ptr)(
    void *handle,
    uint8_t reg,
    const uint8_t *data,
    uint16_t len
);

typedef int32_t (*dev_read_ptr)(
    void *handle,
    uint8_t reg,
    uint8_t *data,
    uint16_t len
);

typedef void (*dev_mdelay_ptr)(
    uint32_t millisec
);

/**
 * @brief Communication Context Structure
 */
typedef struct {
    /** Component mandatory fields **/
    dev_write_ptr  write_reg;  /**< Platform write callback pointer */
    dev_read_ptr   read_reg;   /**< Platform read callback pointer */
    /** Component optional fields **/
    dev_mdelay_ptr mdelay;     /**< Platform millisecond delay function pointer */
    /** Customizable optional pointer **/
    void          *handle;     /**< Hardware bus or device instance pointer (e.g., &hi2c1) */
    /** Private data **/
    void          *private_data; /**< Pointer to platform-specific private data */
} ds3231_dev_ctx_t;

/** @} end Interfaces_Functions Communication Interfaces group */

/** 
 * @defgroup DS3231_Data_Types Data Types
 * @brief Enumerations and structures used by the driver.
 * @{
 */

/**
 * @brief Return status codes for driver operations
 */
typedef enum {
    DS3231_OK = 0,          /**< Operation completed successfully */
    DS3231_ERR_COMM,        /**< Communication bus error (I2C) */
    DS3231_ERR_INVALID_ARG  /**< Invalid argument or NULL pointer provided */
} ds3231_status_t;

/**
 * @brief Hardware Alarm Identifiers
 */
typedef enum {
    DS3231_ALARM_1 = 1,     /**< Alarm 1 identifier */
    DS3231_ALARM_2 = 2      /**< Alarm 2 identifier */
} ds3231_alarm_id_t;

/**
 * @brief Triggering modes and masks for Alarm 1
 */
typedef enum {
    DS3231_A1_ONCE_PER_SEC = 0x0F,      /**< Triggers once per second */
    DS3231_A1_MATCH_SEC = 0x0E,         /**< Triggers when seconds match */
    DS3231_A1_MATCH_MIN_SEC = 0x0C,     /**< Triggers when minutes and seconds match */
    DS3231_A1_MATCH_HRS_MIN_SEC = 0x08, /**< Triggers when hours, minutes, and seconds match */
    DS3231_A1_MATCH_DATE = 0x00,        /**< Triggers when date, hours, minutes, and seconds match */
    DS3231_A1_MATCH_DAY = 0x10          /**< Triggers when day of week, hours, minutes, and seconds match */
} ds3231_a1_mode_t;

/**
 * @brief Triggering modes and masks for Alarm 2
 */
typedef enum {
    DS3231_A2_ONCE_PER_MIN = 0x07,  /**< Triggers once per minute (at second 00) */
    DS3231_A2_MATCH_MIN = 0x06,     /**< Triggers when minutes match */
    DS3231_A2_MATCH_HRS_MIN = 0x04, /**< Triggers when hours and minutes match */
    DS3231_A2_MATCH_DATE = 0x00,    /**< Triggers when date, hours, and minutes match */
    DS3231_A2_MATCH_DAY = 0x08      /**< Triggers when day of week, hours, and minutes match */
} ds3231_a2_mode_t;

/**
 * @brief Decoded time and date storage structure
 */
typedef struct {
    uint8_t  seconds;      /**< Seconds (0-59) */
    uint8_t  minutes;      /**< Minutes (0-59) */
    uint8_t  hours;        /**< Hours in 24h format (0-23) */
    uint8_t  day_of_week;  /**< Day of week (1-7) */
    uint8_t  day;          /**< Day of month (1-31) */
    uint8_t  month;        /**< Month (1-12) */
    uint16_t year;         /**< Full year (e.g., 2026) */
} ds3231_time_t;

/**
 * @brief Structure for abstract alarm configuration
 */
typedef struct {
    uint8_t seconds;      /**< Used only for Alarm 1 (0-59) */
    uint8_t minutes;      /**< Used for Alarm 1 and Alarm 2 (0-59) */
    uint8_t hours;        /**< Used for Alarm 1 and Alarm 2 (0-23) */
    uint8_t day_or_date;  /**< Day of month (1-31) or Day of week (1-7) according to mode */
    uint8_t mode;         /**< Trigger mode: cast to ds3231_a1_mode_t or ds3231_a2_mode_t */
} ds3231_alarm_t;

/**
 * @brief Structure reflecting hardware alarm status and configuration
 */
typedef struct {
    bool           enabled;   /**< Indicates if the interrupt signal (A1IE/A2IE) is active */
    bool           triggered; /**< Reflects hardware flag state (A1F/A2F) */
    ds3231_alarm_t config;    /**< Last time/mode configuration applied */
} ds3231_alarm_status_t;

/**
 * @brief DS3231 Main Device Structure Instance
 */
typedef struct {
    ds3231_dev_ctx_t             ctx;              /**< Embedded communication context interface */
    ds3231_time_t         last_time;        /**< Local copy of last read time/date */
    ds3231_alarm_status_t alarm1;           /**< Alarm 1 local state proxy */
    ds3231_alarm_status_t alarm2;           /**< Alarm 2 local state proxy */
    float                 last_temperature; /**< Last recorded temperature in Celsius */
} ds3231_dev_t;

/** @} end DS3231_Data_Types Data Types group */

/* ============================================================================
   PUBLIC API PROTOTYPES
   ============================================================================ */

/** 
 * @defgroup DS3231_API API Functions
 * @brief Available controller functions.
 * @{
 */

/**
 * @brief Initializes the DS3231 device driver context and syncs hardware state.
 *
 * @param[out] dev          Pointer to the DS3231 device instance.
 * @param[in]  write_fn     Pointer to the bus write function callback.
 * @param[in]  read_fn      Pointer to the bus read function callback.
 * @param[in]  handle       Pointer to the platform bus interface handle (e.g., &hi2c1).
 * @param[in]  private_data Pointer to platform user private data (optional, can be NULL).
 *
 * @retval DS3231_OK              Initialization successful.
 * @retval DS3231_ERR_INVALID_ARG Provided dev structure or mandatory callbacks are NULL.
 * @retval DS3231_ERR_COMM        Communication with the device failed.
 */
ds3231_status_t ds3231_init(
    ds3231_dev_t *dev,
    dev_write_ptr write_fn,
    dev_read_ptr read_fn,
    void *handle,
    void *private_data
);

/**
 * @brief Low-level register read function.
 *
 * @param[in]  ctx  Pointer to the communication context structure.
 * @param[in]  reg  Target internal register address.
 * @param[out] data Pointer to the buffer where read data will be stored.
 * @param[in]  len  Number of bytes to read.
 *
 * @retval 0  Read operation successful (or custom platform success code).
 * @retval -1 Communication error or NULL context pointer.
 */
int32_t ds3231_read_reg(
    const ds3231_dev_ctx_t *ctx,
    uint8_t reg,
    uint8_t *data,
    uint16_t len
);

/**
 * @brief Low-level register write function.
 *
 * @param[in] ctx  Pointer to the communication context structure.
 * @param[in] reg  Target internal register address.
 * @param[in] data Pointer to the buffer containing data to write.
 * @param[in] len  Number of bytes to write.
 *
 * @retval 0  Write operation successful (or custom platform success code).
 * @retval -1 Communication error or NULL context pointer.
 */
int32_t ds3231_write_reg(
    const ds3231_dev_ctx_t *ctx,
    uint8_t reg,
    const uint8_t *data,
    uint16_t len
);

/**
 * @brief Sets the time and date registers of the DS3231.
 *
 * @param[in,out] dev  Pointer to the DS3231 device instance.
 * @param[in]     time Pointer to the structure containing desired time and date.
 *
 * @retval DS3231_OK              Time updated successfully.
 * @retval DS3231_ERR_INVALID_ARG Invalid argument or out-of-range date/time parameters.
 * @retval DS3231_ERR_COMM        Communication bus error.
 */
ds3231_status_t ds3231_set_time(
    ds3231_dev_t *dev,
    const ds3231_time_t *time
);

/**
 * @brief Retrieves the current time and date from the DS3231.
 *
 * @param[in,out] dev  Pointer to the DS3231 device instance.
 * @param[out]    time Pointer to the structure where retrieved time will be stored.
 *
 * @retval DS3231_OK              Time read successfully.
 * @retval DS3231_ERR_INVALID_ARG Pointer is NULL.
 * @retval DS3231_ERR_COMM        Communication bus error.
 */
ds3231_status_t ds3231_get_time(
    ds3231_dev_t *dev,
    ds3231_time_t *time
);

/**
 * @brief Reads temperature from the internal sensor (0.25 °C resolution).
 *
 * @param[in,out] dev         Pointer to the DS3231 device instance.
 * @param[out]    temperature Pointer to store calculated temperature value in °C.
 *
 * @retval DS3231_OK              Temperature read successfully.
 * @retval DS3231_ERR_INVALID_ARG Pointer is NULL.
 * @retval DS3231_ERR_COMM        Communication bus error.
 */
ds3231_status_t ds3231_get_temperature(
    ds3231_dev_t *dev,
    float *temperature
);

/**
 * @brief Configures time, date, and triggering mode for a specific hardware alarm.
 *
 * @param[in,out] dev      Pointer to the DS3231 device instance.
 * @param[in]     alarm_id Target alarm identifier (DS3231_ALARM_1 or DS3231_ALARM_2).
 * @param[in]     alarm    Pointer to alarm configuration structure.
 *
 * @retval DS3231_OK              Alarm configured successfully.
 * @retval DS3231_ERR_INVALID_ARG Invalid pointer or unsupported alarm ID.
 * @retval DS3231_ERR_COMM        Communication bus error.
 */
ds3231_status_t ds3231_set_alarm(
    ds3231_dev_t *dev,
    ds3231_alarm_id_t alarm_id,
    const ds3231_alarm_t *alarm
);

/**
 * @brief Enables or disables hardware interrupt signal generation for an alarm.
 *
 * @param[in,out] dev      Pointer to the DS3231 device instance.
 * @param[in]     alarm_id Target alarm identifier.
 * @param[in]     enable   True to enable interrupt line, false to disable.
 *
 * @retval DS3231_OK              Interrupt toggled successfully.
 * @retval DS3231_ERR_INVALID_ARG Device pointer is NULL.
 * @retval DS3231_ERR_COMM        Communication bus error.
 */
ds3231_status_t ds3231_toggle_alarm_interrupt(
    ds3231_dev_t *dev,
    ds3231_alarm_id_t alarm_id,
    bool enable
);

/**
 * @brief Checks if a specific alarm has triggered by reading hardware status flag.
 *
 * @param[in,out] dev           Pointer to the DS3231 device instance.
 * @param[in]     alarm_id      Target alarm identifier.
 * @param[out]    out_triggered Pointer to store flag state (true if active).
 *
 * @retval DS3231_OK              Flag checked successfully.
 * @retval DS3231_ERR_INVALID_ARG Required output or device pointer is NULL.
 * @retval DS3231_ERR_COMM        Communication bus error.
 */
ds3231_status_t ds3231_check_alarm_flag(
    ds3231_dev_t *dev,
    ds3231_alarm_id_t alarm_id,
    bool *out_triggered
);

/**
 * @brief Clears active alarm event status flag in hardware.
 *
 * @param[in,out] dev      Pointer to the DS3231 device instance.
 * @param[in]     alarm_id Target alarm identifier to clear.
 *
 * @retval DS3231_OK              Flag cleared successfully.
 * @retval DS3231_ERR_INVALID_ARG Device pointer is NULL or invalid alarm ID provided.
 * @retval DS3231_ERR_COMM        Communication bus error.
 */
ds3231_status_t ds3231_clear_alarm_flag(
    ds3231_dev_t *dev,
    ds3231_alarm_id_t alarm_id
);
/** @} */ /* end DS3231_API API Functions group */
/** @} */ /* end ds3231 Driver DS3231 group */

#endif /* DS3231_H_ */