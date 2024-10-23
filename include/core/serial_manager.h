// serial_manager.h

#ifndef SERIAL_MANAGER_H
#define SERIAL_MANAGER_H

#include <esp_types.h>
#include <managers/display_manager.h>

// Initialize the SerialManager
void serial_manager_init();

// Task function for reading serial commands
void serial_task(void *pvParameter);

int handle_serial_command(const char *input);

void simulateCommand(const char* commandString);

QueueHandle_tt commandQueue;

typedef struct {
    char command[1024];
} SerialCommand;


#endif // SERIAL_MANAGER_H
