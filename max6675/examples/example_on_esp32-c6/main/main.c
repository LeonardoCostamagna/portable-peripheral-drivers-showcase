/**
 * @file main.c
 * @brief Example of using the MAX6675 driver on an ESP32-C6.
 * 
 * @copyright Copyright (c) 2026 Leonardo Costamagna
 * 
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "max6675.h"

#define TAG "MAIN"

/* Pin definitions for SPI and Chip Select */
#define MAX6675_SO_PIN      2
#define MAX6675_SCK_PIN     6
#define MAX6675_CS_PIN      4

/* ============================================================================
   SPI & CS CALLBACK IMPLEMENTATIONS FOR MAX6675 DRIVER
   ============================================================================ */

static int32_t esp32_spi_read_stream(void *handle, uint8_t *data, uint16_t len) {
    spi_device_handle_t spi_handle = (spi_device_handle_t)handle;
    
    spi_transaction_t t = {
        .flags = 0,
        .length = len * 8,
        .rxlength = len * 8,
        .rx_buffer = data,
    };

    esp_err_t err = spi_device_transmit(spi_handle, &t);
    return (err == ESP_OK) ? 0 : -1;
}

static void esp32_cs_select(void *handle, bool select) {
    // If select is true, drive CS Low (Active); if false, drive CS High (Inactive)
    gpio_set_level(MAX6675_CS_PIN, select ? 0 : 1);
}

/* ============================================================================
   APPLICATION MAIN
   ============================================================================ */

void app_main(void) {
    ESP_LOGI(TAG, "Initializing MAX6675 Demo on ESP32-C6...");

    /* 1. Configure Chip Select (CS) Pin as GPIO Output */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MAX6675_CS_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(MAX6675_CS_PIN, 1); // Deselect initially (High)

    /* 2. Initialize SPI Master Bus */
    spi_bus_config_t bus_config = {
        .miso_io_num = MAX6675_SO_PIN,
        .mosi_io_num = -1, // MAX6675 is read-only, MOSI not used
        .sclk_io_num = MAX6675_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO));

    /* 3. Add MAX6675 Device to SPI Bus */
    spi_device_interface_config_t dev_config = {
        .clock_speed_hz = 1 * 1000 * 1000, // 1 MHz (Supports up to 4.3 MHz)
        .mode = 0,                         // SPI Mode 0 (CPOL=0, CPHA=0)
        .spics_io_num = -1,                // Manual software CS control via callback
        .queue_size = 1,
    };

    spi_device_handle_t spi_device_handle;
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_config, &spi_device_handle));

    /* 4. Initialize MAX6675 Driver Instance[cite: 11] */
    max6675_dev_t dev;
    max6675_status_t status = max6675_init(
        &dev,
        esp32_spi_read_stream,
        esp32_cs_select,
        (void *)spi_device_handle,
        NULL
    );

    if (status != MAX6675_OK) {
        ESP_LOGE(TAG, "Failed to initialize MAX6675 driver: %d", status);
        return;
    }
    ESP_LOGI(TAG, "MAX6675 Driver successfully initialized");

    /* 5. Application Loop: Read Temperature and Monitor Sensor Status */
    float temperature = 0.0f;

    while (1) {
        status = max6675_get_temperature(&dev, &temperature);
        
        if (status == MAX6675_OK) {
            ESP_LOGI(TAG, "Thermocouple Temperature: %.2f degC", temperature);
        } else if (status == MAX6675_ERR_OPEN_TC) {
            ESP_LOGW(TAG, "Thermocouple is disconnected (Open Circuit)!");
        } else {
            ESP_LOGE(TAG, "Failed to read temperature, error: %d", status);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}