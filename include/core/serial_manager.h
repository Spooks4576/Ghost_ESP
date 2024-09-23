// serial_manager.h

#ifndef SERIAL_MANAGER_H
#define SERIAL_MANAGER_H

// Initialize the SerialManager
void serial_manager_init();

// Task function for reading serial commands
void serial_task(void *pvParameter);

#endif // SERIAL_MANAGER_H
