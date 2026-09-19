#ifndef MAX6675_H_
#define MAX6675_H_

/**
 * @file max6675.h
 * @brief MAX6675 Thermocouple-to-Digital Converter Driver.
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2026 Leonardo Costamagna
 * 
 * SPDX-License-Identifier: MIT
 */

/** 
 *  @defgroup max6675 Driver MAX6675
 *  @brief Driver for the MAX6675 K-type thermocouple converter.
 *  @{
 */

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Fixed resolution in Celsius per LSB (0.25 °C)
 */
#define MAX6675_RESOLUTION_C   0.25f

/** 
 * @defgroup MAX6675_Bit_Masks Bit Masks
 * @brief Key bit masks for parsing MAX6675 raw data.
 * @{
 */

#define MAX6675_BIT_D2_OPEN_TC  (1U << 2)   /**< Bit D2: High if thermocouple is open */
#define MAX6675_RAW_DATA_SHIFT  3           /**< Temperature bits location (D14-D3) */
#define MAX6675_RAW_DATA_MASK   0x0FFF      /**< 12 bits resolution mask */

/** @} end MAX6675_Bit_Masks Bit Masks group */

/** 
 * @defgroup Interfaces_Functions Communication Interfaces
 * @brief Bus function pointer prototypes and communication context.
 *        MANDATORY: Callback return 0 -> No Error.
 * @{
 */

typedef int32_t (*dev_spi_read_stream_ptr)(
    void *handle,
    uint8_t *data,
    uint16_t len
);

typedef void (*dev_cs_select_ptr)(
    void *handle,
    bool select
);

typedef void (*dev_mdelay_ptr)(
    uint32_t millisec
);

/**
 * @brief Communication Context Structure
 */
typedef struct {
    /** Component mandatory fields **/
    dev_spi_read_stream_ptr read_stream;   /**< Platform SPI read stream callback pointer */
    dev_cs_select_ptr      cs_select;    /**< Platform Chip Select control callback pointer */
    /** Component optional fields **/
    dev_mdelay_ptr         mdelay;       /**< Platform millisecond delay function pointer */
    /** Customizable optional pointer **/
    void                  *handle;       /**< Hardware bus or device instance pointer (e.g., &hspi1) */
    /** Private data **/
    void                  *private_data; /**< Pointer to platform-specific private data */
} max6675_dev_ctx_t;

/** @} end Interfaces_Functions Communication Interfaces group */

/** 
 * @defgroup MAX6675_Data_Types Data Types
 * @brief Enumerations and structures used by the driver.
 * @{
 */

/**
 * @brief Return status codes for driver operations
 */
typedef enum {
    MAX6675_OK = 0,          /**< Operation completed successfully */
    MAX6675_ERR_COMM,        /**< Communication bus error (SPI) */
    MAX6675_ERR_OPEN_TC,     /**< Thermocouple input is open / disconnected */
    MAX6675_ERR_INVALID_ARG  /**< Invalid argument or NULL pointer provided */
} max6675_status_t;

/**
 * @brief MAX6675 Main Device Structure Instance
 */
typedef struct {
    max6675_dev_ctx_t ctx;              /**< Embedded communication context interface */
    float     last_temperature; /**< Last recorded temperature in Celsius */
} max6675_dev_t;

/** @} end MAX6675_Data_Types Data Types group */

/* ============================================================================
   PUBLIC API PROTOTYPES
   ============================================================================ */

/** 
 * @defgroup MAX6675_API API Functions
 * @brief Available controller functions.
 * @{
 */

/**
 * @brief Initializes the MAX6675 device driver context.
 *
 * @param[out] dev          Pointer to the MAX6675 device instance.
 * @param[in]  read_fn      Pointer to the SPI read function callback.
 * @param[in]  cs_fn        Pointer to the Chip Select function callback.
 * @param[in]  handle       Pointer to the platform bus interface handle (e.g., &hspi1).
 * @param[in]  private_data Pointer to platform user private data (optional, can be NULL).
 *
 * @retval MAX6675_OK              Initialization successful.
 * @retval MAX6675_ERR_INVALID_ARG Provided pointers are NULL.
 */
max6675_status_t max6675_init(
    max6675_dev_t *dev,
    dev_spi_read_stream_ptr read_fn,
    dev_cs_select_ptr cs_fn,
    void *handle,
    void *private_data
);

/**
 * @brief Low-level 16-bit raw data read function over SPI.
 *
 * @param[in]  ctx  Pointer to the communication context structure.
 * @param[out] data Pointer to store the raw 16-bit word read from SPI.
 *
 * @retval 0  Read operation successful.
 * @retval -1 Communication error or NULL pointer context.
 */
int32_t max6675_read_raw(
    const max6675_dev_ctx_t *ctx,
    uint16_t *data
);

/**
 * @brief Performs a reading sequence and calculates temperature in Celsius.
 *
 * @param[in,out] dev         Pointer to the MAX6675 device instance.
 * @param[out]    temperature Pointer to store calculated temperature value in °C.
 *
 * @retval MAX6675_OK              Temperature read successfully.
 * @retval MAX6675_ERR_OPEN_TC     Thermocouple is disconnected.
 * @retval MAX6675_ERR_INVALID_ARG Pointer is NULL.
 * @retval MAX6675_ERR_COMM        Communication bus error.
 */
max6675_status_t max6675_get_temperature(
    max6675_dev_t *dev,
    float *temperature
);

/** @} end MAX6675_API API Functions group */
/** @} end max6675 Driver MAX6675 group */

#endif /* MAX6675_H_ */