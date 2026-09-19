/**
 * @file max6675.c
 * @brief Implementation of the MAX6675 Thermocouple-to-Digital Converter driver functions.
 * @version v1.0.0
 * 
 * @copyright Copyright (c) 2026 Leonardo Costamagna
 * 
 * SPDX-License-Identifier: MIT
 */

#include "max6675.h"
#include <stddef.h>

int32_t max6675_read_raw(const max6675_dev_ctx_t *ctx, uint16_t *data) {
    /* Validate mandatory context and bus callbacks */
    if (ctx == NULL || ctx->read_stream == NULL || ctx->cs_select == NULL || data == NULL) {
        return -1;
    }

    uint8_t rx_buffer[2] = {0};

    /* 1. Select device (CS Low) */
    ctx->cs_select(ctx->handle, true);

    /* 2. Read 2 bytes (16 bits) over SPI */
    int32_t ret = ctx->read_stream(ctx->handle, rx_buffer, 2);

    /* 3. Deselect device (CS High) to start new internal conversion */
    ctx->cs_select(ctx->handle, false);

    if (ret != 0) {
        return ret;
    }

    /* 4. Combine bytes explicitly ensuring MSB-first endianness independence */
    *data = ((uint16_t)rx_buffer[0] << 8) | rx_buffer[1];

    return 0;
}

max6675_status_t max6675_init(
    max6675_dev_t *dev,
    dev_spi_read_stream_ptr read_fn,
    dev_cs_select_ptr cs_fn,
    void *handle,
    void *private_data) 
{
    /* Check for null pointers on mandatory parameters */
    if (dev == NULL || read_fn == NULL || cs_fn == NULL) {
        return MAX6675_ERR_INVALID_ARG;
    }

    /* Assign context callbacks and handles */
    dev->ctx.read_stream = read_fn;
    dev->ctx.cs_select = cs_fn;
    dev->ctx.handle = handle;
    dev->ctx.private_data = private_data;

    /* Initialize software state structure defaults */
    dev->last_temperature = 0.0f;

    /* Ensure CS pin starts deselected (High) */
    dev->ctx.cs_select(dev->ctx.handle, false);

    return MAX6675_OK;
}

max6675_status_t max6675_get_temperature(max6675_dev_t *dev, float *temperature) {
    uint16_t raw_rx_data = 0;

    if (dev == NULL || temperature == NULL) {
        return MAX6675_ERR_INVALID_ARG;
    }

    /* Perform SPI read sequence */
    if (max6675_read_raw(&dev->ctx, &raw_rx_data) != 0) {
        return MAX6675_ERR_COMM;
    }

    /* Check if thermocouple is open (Bit D2 high) */
    if (raw_rx_data & MAX6675_BIT_D2_OPEN_TC) {
        return MAX6675_ERR_OPEN_TC;
    }

    /* Extract effective 12 bits data (D14 to D3) */
    uint16_t raw_temp = (raw_rx_data >> MAX6675_RAW_DATA_SHIFT) & MAX6675_RAW_DATA_MASK;

    /* Calculate Celsius degree output using physical resolution */
    *temperature = (float)raw_temp * MAX6675_RESOLUTION_C;

    /* Update internal local cache */
    dev->last_temperature = *temperature;
    return MAX6675_OK;
}