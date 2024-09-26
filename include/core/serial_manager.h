// serial_manager.h

#ifndef SERIAL_MANAGER_H
#define SERIAL_MANAGER_H

#include <esp_types.h>

// Initialize the SerialManager
void serial_manager_init();

// Task function for reading serial commands
void serial_task(void *pvParameter);

int handle_serial_command(const char *input);

#endif // SERIAL_MANAGER_H
