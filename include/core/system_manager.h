// system_manager.h

#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

typedef struct ManagedTask {
  TaskHandle_t task_handle;
  char task_name[16];
  UBaseType_t priority;
  void (*on_task_complete)(
      const char *task_name); // Callback when task completes
  struct ManagedTask *next;
} ManagedTask;

// Initialize the System Manager
void system_manager_init();

// Create a new task
bool system_manager_create_task(void (*task_function)(void *),
                                const char *task_name, uint32_t stack_size,
                                UBaseType_t priority,
                                void (*on_task_complete)(const char *));

// Remove an existing task by name
bool system_manager_remove_task(const char *task_name);

// Suspend a task
bool system_manager_suspend_task(const char *task_name);

// Resume a suspended task
bool system_manager_resume_task(const char *task_name);

// Change a task’s priority
bool system_manager_set_task_priority(const char *task_name,
                                      UBaseType_t new_priority);

// Print the list of all tasks
void system_manager_list_tasks();

#endif // SYSTEM_MANAGER_H