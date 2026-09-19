# DS3231 Driver

---

## Overview

The **DS3231** is a high-precision I2C real-time clock (RTC) with integrated temperature compensation (TCXO) and crystal. Its design ensures an accuracy of ±2ppm over a range of -40°C to +85°C. This device integrates a digital temperature sensor, alarms, and a programmable square-wave output while maintaining extremely low power consumption, making it ideal for applications that require long-term timekeeping precision, even with backup power supplies (battery).

### Driver Implementation

This driver has been designed under high-quality standards for embedded systems, offering:

* **Guaranteed portability:** The driver implements hardware abstraction logic, allowing its portability to any architecture (STM32, ESP32, AVR, etc.) through the implementation of simple I2C callbacks.
* **Design patterns:** Based on the *Hardware Proxy* and *Opaque Pointer* patterns, achieving total decoupling between the RTC logic and the target MCU.
* **Robustness:** Implementation focused on minimizing register access errors and ensuring the integrity of I2C communication.
* **Professional documentation:** Automatically generated using Doxygen, making the API easy to understand.
* **Testing:** Unit testing suite with Ceedling to validate the driver logic independently of the hardware.
* **Examples:** Example projects included for a quick startup.

---

## Usage

### File Tree

```text
ds3231/
├── docs/                 # Doxigen documentation and other information
├── examples/             # Usage examples
├── inc/                  # Header files (.h)
├── src/                  # Implementation (.c)
├── test/                 # Unit test cases
└── README.md             # This file
```

Download the `ds3231` folder to your host PC and follow the instructions in the following sections to use the driver.

### How to Use the Driver

Integrating this driver into your project is a straightforward process. Follow these steps to configure the device:

1. **Files:** Copy `ds3231.h` and `ds3231.c` to your project.

2. **Instantiation:** Create an instance of `ds3231_dev_t` to represent your device.

3. **Interface Requirements:** Ensure your target system satisfies the dependencies required by the driver:
   * **I2C Read Wrapper:** A wrapper function utilizing the target MCU's HAL to execute I2C read operations.
   * **I2C Write Wrapper:** A wrapper function utilizing the target MCU's HAL to execute I2C write operations.
   * **Delay Function (Optional):** A system function used to generate time delays, if required by the target platform.
   * **Peripheral Handle:** The target MCU's I2C peripheral handle or instance.

4. **Initialization:** Pass the defined interface functions and your I2C peripheral handle to the `ds3231_init` function.

```c
#include "ds3231.h"

static int32_t mcu_i2c_write_reg(void *handle, uint8_t reg_addr, const uint8_t *data, uint16_t len) {
    mcu_i2c_master_dev_handle_t dev_handle = (mcu_i2c_master_dev_handle_t)handle;
    
    uint8_t buffer[len + 1];
    buffer[0] = reg_addr;
    memcpy(&buffer[1], data, len);

    mcu_err_t err = mcu_i2c_master_transmit(dev_handle, buffer, len + 1, pdMS_TO_TICKS(1000));
    return (err == MCU_OK) ? 0 : -1;
}

static int32_t mcu_i2c_read_reg(void *handle, uint8_t reg_addr, uint8_t *data, uint16_t len) {
    mcu_i2c_master_dev_handle_t dev_handle = (mcu_i2c_master_dev_handle_t)handle;

    mcu_err_t err = mcu_i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, pdMS_TO_TICKS(1000));
    return (err == MCU_OK) ? 0 : -1;
}

mcu_i2c_master_dev_handle_t ds3231_i2c_handle;
ds3231_dev_t rtc;

ds3231_init(&rtc, 
            mcu_i2c_write_reg, 
            mcu_i2c_read_reg, 
            (void *)ds3231_i2c_handle, 
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