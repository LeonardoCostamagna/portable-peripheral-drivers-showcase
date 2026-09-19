# Unit Testing & Coverage

---

The driver is fully validated using unit tests to ensure high code quality and reliability.

## Prerequisites

To build and run the test suite, you need:

* **Ruby** (required to run Ceedling)
* **Ceedling** (build system for C unit testing)
* **Unity** (testing framework)
* **CMock** (mocking framework for dependency isolation)
* **gcovr** (optional, used for generating code coverage reports)

## File Tree

```text
driver_folder/
├── docs/
├── examples/        
├── inc/                
├── src/                
└── test/               # Unit test cases
    ├── support/        # Headers whose functions will be mocked by Ceedling
    ├── project.yml     # Ceedling configurations
    ├── test_driver.c   # Test suite
    └── README.md       # This file
```

## How to Run Tests

Navigate to the `test/` directory and execute:

```bash
ceedling test:all
```

This will run the test suite and display the results.

To generate and view the code coverage HTML report:

```bash
ceedling gcov:all
```

You can find the generated HTML coverage report at `test/build/artifacts/gcov/gcovr`.

---