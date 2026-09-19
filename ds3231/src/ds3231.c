/**
 * @file ds3231.c
 * @brief Implementation of the DS3231 Real-Time Clock (RTC) driver functions.
 * @version v1.0.0 
 * 
 * @copyright Copyright (c) 2026 Leonardo Costamagna
 * 
 * SPDX-License-Identifier: MIT
 */

#include "ds3231.h"
#include <stddef.h>

/**
 * @brief Converts a 2-digit decimal number to Binary Coded Decimal (BCD).
 * @param[in] val Decimal value (0-99).
 * @return Formatted BCD byte.
 */
static uint8_t decimal_to_bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

/**
 * @brief Converts a Binary Coded Decimal (BCD) byte to decimal format.
 * @param[in] val BCD encoded byte.
 * @return Decoded decimal value.
 */
static uint8_t bcd_to_decimal(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

int32_t ds3231_read_reg(const ds3231_dev_ctx_t *ctx, uint8_t reg, uint8_t *data, uint16_t len) {
    /* Validate mandatory context and bus read callback */
    if (ctx == NULL || ctx->read_reg == NULL) {
        return -1;
    }
    return ctx->read_reg(ctx->handle, reg, data, len);
}

int32_t ds3231_write_reg(const ds3231_dev_ctx_t *ctx, uint8_t reg, const uint8_t *data, uint16_t len) {
    /* Validate mandatory context and bus write callback */
    if (ctx == NULL || ctx->write_reg == NULL) {
        return -1;
    }
    return ctx->write_reg(ctx->handle, reg, data, len);
}

ds3231_status_t ds3231_init(
    ds3231_dev_t *dev,
    dev_write_ptr write_fn,
    dev_read_ptr read_fn,
    void *handle,
    void *private_data) 
{
    uint8_t status_reg = 0;
    uint8_t ctrl_reg = 0;

    /* Check for null pointers on mandatory parameters */
    if (dev == NULL || write_fn == NULL || read_fn == NULL) {
        return DS3231_ERR_INVALID_ARG;
    }

    /* Assign context callbacks and handles */
    dev->ctx.write_reg = write_fn;
    dev->ctx.read_reg = read_fn;
    dev->ctx.handle = handle;
    dev->ctx.private_data = private_data;

    /* Initialize software state structure defaults */
    dev->last_temperature = 0.0f;
    dev->alarm1.enabled = false;
    dev->alarm1.triggered = false;
    dev->alarm2.enabled = false;
    dev->alarm2.triggered = false;

    /* Synchronize software proxy state with hardware status flags */
    if (ds3231_read_reg(&dev->ctx, DS3231_REG_STATUS, &status_reg, 1) != 0) {
        return DS3231_ERR_COMM;
    }

    dev->alarm1.triggered = (status_reg & DS3231_STAT_A1F) ? true : false;
    dev->alarm2.triggered = (status_reg & DS3231_STAT_A2F) ? true : false;

    /* Synchronize software proxy state with hardware interrupt enables */
    if (ds3231_read_reg(&dev->ctx, DS3231_REG_CONTROL, &ctrl_reg, 1) != 0) {
        return DS3231_ERR_COMM;
    }

    dev->alarm1.enabled = (ctrl_reg & DS3231_CTRL_A1IE) ? true : false;
    dev->alarm2.enabled = (ctrl_reg & DS3231_CTRL_A2IE) ? true : false;

    return DS3231_OK;
}

ds3231_status_t ds3231_set_time(ds3231_dev_t *dev, const ds3231_time_t *time) {
    uint8_t buffer[7];

    if (dev == NULL || time == NULL) {
        return DS3231_ERR_INVALID_ARG;
    }

    /* Range verification for all time components */
    if (time->seconds > 59 || time->minutes > 59 || time->hours > 23 ||
        time->day == 0 || time->day > 31 || 
        time->month == 0 || time->month > 12 || 
        time->day_of_week == 0 || time->day_of_week > 7) {
        return DS3231_ERR_INVALID_ARG;
    }

    /* Encode decimal fields into BCD format for DS3231 hardware registers */
    buffer[0] = decimal_to_bcd(time->seconds);
    buffer[1] = decimal_to_bcd(time->minutes);
    buffer[2] = decimal_to_bcd(time->hours);
    buffer[3] = decimal_to_bcd(time->day_of_week);
    buffer[4] = decimal_to_bcd(time->day);
    buffer[5] = decimal_to_bcd(time->month);
    buffer[6] = decimal_to_bcd((uint8_t)(time->year % 100)); /* Store 2-digit year offset */

    /* Burst write 7 timekeeping registers starting from DS3231_REG_SECONDS (0x00) */
    if (ds3231_write_reg(&dev->ctx, DS3231_REG_SECONDS, buffer, 7) != 0) {
        return DS3231_ERR_COMM;
    }

    /* Update internal local cache */
    dev->last_time = *time;
    return DS3231_OK;
}

ds3231_status_t ds3231_get_time(ds3231_dev_t *dev, ds3231_time_t *time) {
    uint8_t buffer[7] = {0};

    if (dev == NULL || time == NULL) {
        return DS3231_ERR_INVALID_ARG;
    }

    /* Burst read 7 timekeeping registers starting at DS3231_REG_SECONDS (0x00) */
    if (ds3231_read_reg(&dev->ctx, DS3231_REG_SECONDS, buffer, 7) != 0) {
        return DS3231_ERR_COMM;
    }

    /* Mask control/flag bits and convert BCD values back to decimal */
    time->seconds     = bcd_to_decimal(buffer[0]);
    time->minutes     = bcd_to_decimal(buffer[1]);
    time->hours       = bcd_to_decimal(buffer[2] & 0x3F); /* Mask off 12/24 hour bit */
    time->day_of_week = bcd_to_decimal(buffer[3]);
    time->day         = bcd_to_decimal(buffer[4]);
    time->month       = bcd_to_decimal(buffer[5] & 0x1F); /* Mask century bit */
    time->year        = 2000 + bcd_to_decimal(buffer[6]);  /* Base year offset (2000s epoch) */

    /* Update internal local cache */
    dev->last_time = *time;
    return DS3231_OK;
}

ds3231_status_t ds3231_get_temperature(ds3231_dev_t *dev, float *temperature) {
    uint8_t buffer[2] = {0};

    if (dev == NULL || temperature == NULL) {
        return DS3231_ERR_INVALID_ARG;
    }

    /* Read MSB (0x11) and LSB (0x12) temperature registers */
    if (ds3231_read_reg(&dev->ctx, DS3231_REG_TEMP_MSB, buffer, 2) != 0) {
        return DS3231_ERR_COMM;
    }

    /**
     * Decode 10-bit signed temperature value:
     * Shift right by 6 to preserve two's complement sign alignment in a 16-bit int,
     * then scale by 0.25 °C LSB resolution.
     */
    int16_t raw_temp = (int16_t)(((uint16_t)buffer[0] << 8) | buffer[1]) >> 6;
    *temperature = (float)raw_temp * 0.25f;

    /* Update internal local cache */
    dev->last_temperature = *temperature;
    return DS3231_OK;
}

ds3231_status_t ds3231_set_alarm(
    ds3231_dev_t *dev,
    ds3231_alarm_id_t alarm_id,
    const ds3231_alarm_t *alarm) 
{
    if (dev == NULL || alarm == NULL) {
        return DS3231_ERR_INVALID_ARG;
    }

    if (alarm_id == DS3231_ALARM_1) {
        uint8_t buffer[4];
        /* Extract Alarm 1 mask bits (AxM1, AxM2, AxM3, AxM4) and DY/DT selector bit */
        uint8_t m1 = (alarm->mode & 0x01) << 7;
        uint8_t m2 = (alarm->mode & 0x02) << 6;
        uint8_t m3 = (alarm->mode & 0x04) << 5;
        uint8_t m4 = (alarm->mode & 0x08) << 4;
        uint8_t dydt = (alarm->mode & 0x10) << 2;

        /* Combine BCD time fields with trigger mask bits */
        buffer[0] = decimal_to_bcd(alarm->seconds) | m1;
        buffer[1] = decimal_to_bcd(alarm->minutes) | m2;
        buffer[2] = decimal_to_bcd(alarm->hours)   | m3;
        buffer[3] = decimal_to_bcd(alarm->day_or_date) | m4 | dydt;

        if (ds3231_write_reg(&dev->ctx, DS3231_REG_ALARM1_SEC, buffer, 4) != 0) {
            return DS3231_ERR_COMM;
        }
        dev->alarm1.config = *alarm;
    } 
    else if (alarm_id == DS3231_ALARM_2) {
        uint8_t buffer[3];
        /* Extract Alarm 2 mask bits (AxM2, AxM3, AxM4) and DY/DT selector bit */
        uint8_t m2 = (alarm->mode & 0x01) << 7;
        uint8_t m3 = (alarm->mode & 0x02) << 6;
        uint8_t m4 = (alarm->mode & 0x04) << 5;
        uint8_t dydt = (alarm->mode & 0x08) << 3;

        /* Alarm 2 resolution starts at minutes (no seconds register) */
        buffer[0] = decimal_to_bcd(alarm->minutes) | m2;
        buffer[1] = decimal_to_bcd(alarm->hours)   | m3;
        buffer[2] = decimal_to_bcd(alarm->day_or_date) | m4 | dydt;

        if (ds3231_write_reg(&dev->ctx, DS3231_REG_ALARM2_MIN, buffer, 3) != 0) {
            return DS3231_ERR_COMM;
        }
        dev->alarm2.config = *alarm;
    } 
    else {
        return DS3231_ERR_INVALID_ARG;
    }

    return DS3231_OK;
}

ds3231_status_t ds3231_toggle_alarm_interrupt(
    ds3231_dev_t *dev,
    ds3231_alarm_id_t alarm_id,
    bool enable) 
{
    uint8_t ctrl_reg = 0;
    uint8_t interrupt_mask = 0;

    if (dev == NULL) {
        return DS3231_ERR_INVALID_ARG;
    }

    if (ds3231_read_reg(&dev->ctx, DS3231_REG_CONTROL, &ctrl_reg, 1) != 0) {
        return DS3231_ERR_COMM;
    }

    /* INTCN bit must be high to route alarm interrupts to INT/SQW pin */
    ctrl_reg |= DS3231_CTRL_INTCN;
    interrupt_mask = (alarm_id == DS3231_ALARM_1) ? DS3231_CTRL_A1IE : DS3231_CTRL_A2IE;

    if (enable) {
        ctrl_reg |= interrupt_mask;
    } else {
        ctrl_reg &= ~interrupt_mask;
    }

    if (ds3231_write_reg(&dev->ctx, DS3231_REG_CONTROL, &ctrl_reg, 1) != 0) {
        return DS3231_ERR_COMM;
    }

    /* Update internal local cache */
    if (alarm_id == DS3231_ALARM_1) {
        dev->alarm1.enabled = enable;
    } else {
        dev->alarm2.enabled = enable;
    }

    return DS3231_OK;
}

ds3231_status_t ds3231_check_alarm_flag(
    ds3231_dev_t *dev,
    ds3231_alarm_id_t alarm_id,
    bool *out_triggered) 
{
    uint8_t status_reg = 0;

    if (dev == NULL || out_triggered == NULL) {
        return DS3231_ERR_INVALID_ARG;
    }

    if (ds3231_read_reg(&dev->ctx, DS3231_REG_STATUS, &status_reg, 1) != 0) {
        return DS3231_ERR_COMM;
    }

    /* Update both alarm flags in local driver state */
    dev->alarm1.triggered = (status_reg & DS3231_STAT_A1F) ? true : false;
    dev->alarm2.triggered = (status_reg & DS3231_STAT_A2F) ? true : false;

    *out_triggered = (alarm_id == DS3231_ALARM_1) ? dev->alarm1.triggered : dev->alarm2.triggered;

    return DS3231_OK;
}

ds3231_status_t ds3231_clear_alarm_flag(ds3231_dev_t *dev, ds3231_alarm_id_t alarm_id) {
    uint8_t status_reg = 0;

    if (dev == NULL) {
        return DS3231_ERR_INVALID_ARG;
    }

    if (ds3231_read_reg(&dev->ctx, DS3231_REG_STATUS, &status_reg, 1) != 0) {
        return DS3231_ERR_COMM;
    }

    /* Clear hardware flag (writing logic 0 clears the flag bit) */
    if (alarm_id == DS3231_ALARM_1) {
        status_reg &= ~DS3231_STAT_A1F;
    } else if (alarm_id == DS3231_ALARM_2) {
        status_reg &= ~DS3231_STAT_A2F;
    } else {
        return DS3231_ERR_INVALID_ARG;
    }

    if (ds3231_write_reg(&dev->ctx, DS3231_REG_STATUS, &status_reg, 1) != 0) {
        return DS3231_ERR_COMM;
    }

    /* Update local cache flags */
    if (alarm_id == DS3231_ALARM_1) {
        dev->alarm1.triggered = false;
    } else {
        dev->alarm2.triggered = false;
    }

    return DS3231_OK;
}