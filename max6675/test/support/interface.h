#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

// Direct prototypes that CMock will parse to generate mock_interface.h
int32_t mock_spi_read_stream(void *handle, uint16_t *data);
void mock_cs_select(void *handle, bool select);

#endif // INTERFACE_H