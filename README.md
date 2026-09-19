# Peripheral Driver Library

---

## Overview

This is a modular, highly portable driver library designed following software engineering best practices for embedded systems. This library provides a robust, hardware-independent framework to interact with various peripheral devices (sensors, actuators and others). This project aims to minimize hardware coupling, accelerate development cycles, and ensure long-term maintainability.

### Design Patterns

To ensure scalability and flexibility, these drivers primarily implement two design patterns:

* **Hardware Proxy:** The drivers do not directly access the MCU registers. Instead, they use a context structure (`device_dev_ctx_t`) that acts as an intermediary. The driver delegates interface operations to the context, allowing the driver logic to remain intact regardless of which peripheral (I2C, SPI, UART, GPIO) is used.
* **Opaque Pointers:** We use generic pointers (`void *handle`) inside the context structure. This allows us to transparently "inject" any hardware configuration object (such as an STM32 HAL handle or an ESP-IDF configuration structure), achieving total decoupling between the driver and the target MCU vendor's abstraction layer.

> **Note:** It is recommended to integrate the driver using an intermediate layer that acts as a **hardware adapter** pattern, such as a Board Support Package (BSP). This triad of design patterns (hardware proxy, opaque pointers and hardware adapter) has proven to achieve exceptional hardware abstraction and provides multiple benefits when achieving seamless cross-platform portability.

### Quality and Documentation

* **Guaranteed Quality:** All drivers are validated using the **Ceedling** framework with **Unity** and **CMock**, ensuring stability and the logical integrity of the code. The test suite is available in each driver's folder.
* **Technical Documentation:** Each module includes comments in **Doxygen** format and a pre-configured Doxyfile, ready for immediate compilation and documentation generation.
* **Integration Examples:** Ready-to-compile examples for popular evaluation boards, as well as desktop host examples for PC.

---

## Usage

### File Structure

All drivers in the repository follow a standardized structure to facilitate navigation:

```text
device_folder/
├── docs/                   # Doxygen documentation, datasheets and others.
├── examples/               # Usage examples
├── inc/                    # Driver headers (driver.h)
├── src/                    # Driver implementation (driver.c)
├── test/                   # Unit testing testbench with Ceedling
└── README.md               # Driver-specific guide
```

> **Note:**  Inside each subdirectory, you will find a specific README.md file providing tailored instructions, configuration details, and particular guidelines for that section.

### How to use the driver in your project?

Navigate to the driver of interest and consult its README.md for specific configuration, integration, and compilation instructions.

In general, you only need to copy the header and source files into your development environment, keeping the following considerations in mind for integration:

* **Interface Injection:** The driver requires you to inject the necessary interface access functions. At initialization time, you must provide pointers to your read/write/delay/other functions and your peripheral handle. For more details, check the interface requirements in each driver's README.md, where the specific dependencies needed to establish communication with the target system are outlined.

* **Function Signatures:** The driver expects functions that comply with your interface logic (read/write/delay/other). You must wrap your HAL functions (e.g., `HAL_I2C_Master_Transmit`) to match the `dev_write_ptr` and `dev_read_ptr` signatures defined in the driver.

* **Context:** When calling `driver_init(...)`, you will pass the `handle` of your peripheral, which allows the driver to work in multi-instance environments (enabling control of multiple identical devices on different buses).

### How to run the examples?

Navigate to the `examples/` folder, and consult its `README.md`. There you will find instructions for configuration, wiring, and compilation.

### How to run the test cases?

Requires having **Ruby, Ceedling, Unity, CMock, and gcovr** installed on your host system.
Navigate to the driver's `test/` folder and consult its `README.md` where you will find instructions to verify the logic and generate coverage reports.

### How to compile the documentation?

Requires **Doxygen** installed on your host system.
Navigate to `docs/` and follow the instructions in the `README.md` to generate the detailed documentation.

---

## Contribution

Feel free to use these drivers in your projects. Any comments, feedback, or *pull requests* will be welcome to continue improving the robustness of this library.

## License

This project is licensed under the MIT License.

---
