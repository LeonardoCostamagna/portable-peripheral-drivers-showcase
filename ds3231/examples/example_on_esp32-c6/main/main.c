/**
 * @file main.c
 * @brief Example of using the DS3231 driver on an ESP32-C6.
 * 
 * @copyright Copyright (c) 2026 Leonardo Costamagna
 * 
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "ds3231.h"

#define TAG "MAIN"

/* Pin definitions */
#define DS3231_SCL_PIN      7
#define DS3231_SDA_PIN      11
#define DS3231_SQW_PIN      10

static QueueHandle_t gpio_evt_queue = NULL;

/* ============================================================================
   I2C CALLBACK IMPLEMENTATIONS FOR DS3231 DRIVER
   ============================================================================ */

static int32_t esp32_i2c_write_reg(void *handle, uint8_t reg_addr, const uint8_t *data, uint16_t len) {
    i2c_master_dev_handle_t dev_handle = (i2c_master_dev_handle_t)handle;
    
    // Allocate buffer for Register Address + Data Payload
    uint8_t buffer[len + 1];
    buffer[0] = reg_addr;
    for (uint16_t i = 0; i < len; i++) {
        buffer[i + 1] = data[i];
    }

    esp_err_t err = i2c_master_transmit(dev_handle, buffer, len + 1, pdMS_TO_TICKS(1000));
    return (err == ESP_OK) ? 0 : -1;
}

static int32_t esp32_i2c_read_reg(void *handle, uint8_t reg_addr, uint8_t *data, uint16_t len) {
    i2c_master_dev_handle_t dev_handle = (i2c_master_dev_handle_t)handle;

    esp_err_t err = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, pdMS_TO_TICKS(1000));
    return (err == ESP_OK) ? 0 : -1;
}

/* ============================================================================
   SQW / INTERRUPT HANDLER
   ============================================================================ */

static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

/* ============================================================================
   APPLICATION MAIN
   ============================================================================ */

void app_main(void) {
    ESP_LOGI(TAG, "Initializing DS3231 Demo on ESP32-C6...");

    /* 1. Initialize I2C Master Bus */
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = DS3231_SDA_PIN,
        .scl_io_num = DS3231_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    /* 2. Attach DS3231 Slave Device to Bus */
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_I2C_ADD,
        .scl_speed_hz = 100000,
    };

    i2c_master_dev_handle_t ds3231_i2c_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &ds3231_i2c_handle));

    /* 3. Configure SQW Interrupt Pin */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DS3231_SQW_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(DS3231_SQW_PIN, gpio_isr_handler, (void *)DS3231_SQW_PIN);

    /* 4. Initialize DS3231 Driver */
    ds3231_dev_t dev;
    ds3231_status_t status = ds3231_init(
        &dev, 
        esp32_i2c_write_reg, 
        esp32_i2c_read_reg, 
        (void *)ds3231_i2c_handle, 
        NULL
    );

    if (status != DS3231_OK) {
        ESP_LOGE(TAG, "Failed to initialize DS3231 driver: %d", status);
        return;
    }
    ESP_LOGI(TAG, "DS3231 Driver successfully initialized");

    /* 5. Set Initial Date & Time */
    ds3231_time_t initial_time = {
        .seconds = 0,
        .minutes = 0,
        .hours = 12,
        .day_of_week = 6, // Saturday
        .day = 8,
        .month = 8,
        .year = 2026
    };
    
    if (ds3231_set_time(&dev, &initial_time) == DS3231_OK) {
        ESP_LOGI(TAG, "Time set to 2026-08-08 12:00:00");
    }

    /* 6. Application Loop: Read Time and Temperature */
    ds3231_time_t current_time;
    float temp = 0.0f;
    uint32_t io_num;

    while (1) {
        // Read Date/Time
        if (ds3231_get_time(&dev, &current_time) == DS3231_OK) {
            ESP_LOGI(TAG, "RTC Time: %04d-%02d-%02d %02d:%02d:%02d",
                     current_time.year, current_time.month, current_time.day,
                     current_time.hours, current_time.minutes, current_time.seconds);
        }

        // Read Temperature
        if (ds3231_get_temperature(&dev, &temp) == DS3231_OK) {
            ESP_LOGI(TAG, "Temperature: %.2f degC", temp);
        }

        // Check for SQW interrupt trigger non-blocking
        if (xQueueReceive(gpio_evt_queue, &io_num, 0)) {
            ESP_LOGW(TAG, "Interrupt triggered on GPIO %" PRIu32 "!", io_num);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}