#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdint.h>

// Direct prototypes that CMock will parse to generate mock_interface.h
int32_t mock_i2c_read(void *handle, uint8_t reg, uint8_t *data, uint16_t len);
int32_t mock_i2c_write(void *handle, uint8_t reg, const uint8_t *data, uint16_t len);

#endif // INTERFACE_H