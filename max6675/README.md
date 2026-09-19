# MAX6675 Driver

---

## Overview

The MAX6675 is a type-K thermocouple-to-digital converter that performs cold-junction compensation and digitizes the signal using an integrated analog-to-digital converter. It features a read-only, SPI-compatible serial interface that outputs data with a 12-bit resolution, allowing temperature measurements from 0 °C up to +1024 °C with a 0.25 °C increment. Additionally, it includes diagnostic features for open thermocouple detection.

### Driver Implementation

This driver has been designed under high-quality standards for embedded systems, offering:

* **Guaranteed portability:** The driver implements hardware abstraction logic, allowing its portability to any architecture (STM32, ESP32, AVR, etc.) through the implementation of simple SPI callbacks.
* **Design patterns:** Based on the *Hardware Proxy* and *Opaque Pointer* patterns, achieving total decoupling between the MAX6675 logic and the target MCU.
* **Robustness:** Implementation focused on minimizing register access errors and ensuring the integrity of SPI communication.
* **Professional documentation:** Automatically generated using Doxygen, making the API easy to understand.
* **Testing:** Unit testing suite with Ceedling to validate the driver logic independently of the hardware.
* **Examples:** Example projects included for a quick startup.

---

## Usage

### File Tree

```text
max6675/
├── docs/                 # Doxigen documentation and other information
├── examples/             # Usage examples
├── inc/                  # Header files (.h)
├── src/                  # Implementation (.c)
├── test/                 # Unit test cases
└── README.md             # This file
```

Download the `max6675` folder to your host PC and follow the instructions in the following sections to use the driver.

### How to Use the Driver

Integrating this driver into your project is a straightforward process. Follow these steps to configure the device:

1. **Files:** Copy `max6675.h` and `max6675.c` to your project.

2. **Instantiation:** Create an instance of `max6675_dev_t` to represent your device.

3. **Interface Requirements:** Ensure your target system satisfies the dependencies required by the driver:
   * **SPI Read Wrapper:** A wrapper function utilizing the target MCU's HAL to execute SPI read operations.
   * **CS Select Wrapper:** A wrapper function utilizing the target MCU's HAL to manage the CS pin operations.
   * **Delay Function (Optional):** A system function used to generate time delays, if required by the target platform.
   * **Peripheral Handle:** The target MCU's SPI peripheral handle or instance.

4. **Initialization:** Pass the defined interface functions and your SPI peripheral handle to the `max6675_init` function.

```c
#include "max6675.h"

static int32_t mcu_spi_read_stream(void *handle, uint8_t *data, uint16_t len) {
    mcu_spi_device_handle_t spi_handle = (mcu_spi_device_handle_t)handle;
    
    mcu_spi_transaction_t t = {
        .flags = 0,
        .length = len * 8,
        .rxlength = len * 8,
        .rx_buffer = data,
    };

    mcu_err_t err = mcu_device_transmit(spi_handle, &t);
    return (err == MCU_OK) ? 0 : -1;
}

static void mcu_cs_select(void *handle, bool select) {
    // If select is true, drive CS Low (Active); if false, drive CS High (Inactive)
    mcu_gpio_set_level(MAX6675_CS_PIN, select ? 0 : 1);
}

// Add MAX6675 Device to SPI Bus */
mcu_spi_device_interface_config_t dev_config = {
    .clock_speed_hz = 1 * 1000 * 1000, 
    .mode = 0,                        
    .spics_io_num = -1,               
    .queue_size = 1,
};

mcu_spi_device_handle_t spi_device_handle;

max6675_dev_t dev;

max6675_init(&dev,
             mcu_spi_read_stream,
             mcu_cs_select,
             (void *)mcu_spi_device_handle,
             NULL);
```

After completing these steps, the driver will be initialized and ready to manage device operations.

### How to Run the Examples

Navigate to the `examples/` directory and check the `README.md`, where you will find detailed instructions.

### How to Run the Test Cases

Navigate to the `test/` directory and check the `README.md`, where you will find detailed instructions.

### How to Compile the Documentation

Navigate to the `docs/` directory and check the `README.md`, where you will find detailed instructions.

---

## License

This project is licensed under the MIT License.
