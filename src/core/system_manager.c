// system_manager.c

#include "core/system_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Head of the linked list for tasks
static ManagedTask *task_list_head = NULL;

// Initialize the System Manager
void system_manager_init() {
    task_list_head = NULL;
}

// Create a new task
bool system_manager_create_task(void (*task_function)(void *), const char *task_name, uint32_t stack_size, UBaseType_t priority, void (*on_task_complete)(const char *)) {
    // Check if a task with the same name already exists
    ManagedTask *current = task_list_head;
    while (current != NULL) {
        if (strcmp(current->task_name, task_name) == 0) {
            printf("Task with name %s already exists.\n", task_name);
            return false;
        }
        current = current->next;
    }

    // Create a new task
    TaskHandle_t task_handle;
    if (xTaskCreate(task_function, task_name, stack_size, NULL, priority, &task_handle) != pdPASS) {
        printf("Failed to create task %s.\n", task_name);
        return false;
    }

    // Allocate a new ManagedTask node
    ManagedTask *new_task = (ManagedTask *)malloc(sizeof(ManagedTask));
    if (new_task == NULL) {
        printf("Memory allocation failed for task %s.\n", task_name);
        vTaskDelete(task_handle);  // Clean up the task if we can't store it
        return false;
    }

    // Initialize the new task node
    new_task->task_handle = task_handle;
    strncpy(new_task->task_name, task_name, sizeof(new_task->task_name) - 1);
    new_task->task_name[sizeof(new_task->task_name) - 1] = '\0';
    new_task->priority = priority;
    new_task->on_task_complete = on_task_complete;
    new_task->next = NULL;

    // Add the new task to the head of the linked list
    if (task_list_head == NULL) {
        task_list_head = new_task;
    } else {
        ManagedTask *temp = task_list_head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_task;
    }

    printf("Task %s created successfully.\n", task_name);
    return true;
}


bool system_manager_remove_task(const char *task_name) {
    ManagedTask *current = task_list_head;
    ManagedTask *previous = NULL;

    // Search for the task by name
    while (current != NULL) {
        if (strcmp(current->task_name, task_name) == 0) {
            vTaskDelete(current->task_handle);  // Delete the FreeRTOS task

            // Call the task complete callback if available
            if (current->on_task_complete != NULL) {
                current->on_task_complete(task_name);
            }

            // Remove the task from the linked list
            if (previous == NULL) {  // Removing the head
                task_list_head = current->next;
            } else {
                previous->next = current->next;
            }

            free(current);  // Free the task memory
            printf("Task %s deleted.\n", task_name);
            return true;
        }
        previous = current;
        current = current->next;
    }

    printf("Task %s not found.\n", task_name);
    return false;
}


bool system_manager_suspend_task(const char *task_name) {
    ManagedTask *current = task_list_head;
    while (current != NULL) {
        if (strcmp(current->task_name, task_name) == 0) {
            vTaskSuspend(current->task_handle);  // Suspend the task
            printf("Task %s suspended.\n", task_name);
            return true;
        }
        current = current->next;
    }
    printf("Task %s not found.\n", task_name);
    return false;
}


bool system_manager_resume_task(const char *task_name) {
    ManagedTask *current = task_list_head;
    while (current != NULL) {
        if (strcmp(current->task_name, task_name) == 0) {
            vTaskResume(current->task_handle);  // Resume the task
            printf("Task %s resumed.\n", task_name);
            return true;
        }
        current = current->next;
    }
    printf("Task %s not found.\n", task_name);
    return false;
}


bool system_manager_set_task_priority(const char *task_name, UBaseType_t new_priority) {
    ManagedTask *current = task_list_head;
    while (current != NULL) {
        if (strcmp(current->task_name, task_name) == 0) {
            vTaskPrioritySet(current->task_handle, new_priority);
            current->priority = new_priority;
            printf("Task %s priority changed to %d.\n", task_name, new_priority);
            return true;
        }
        current = current->next;
    }
    printf("Task %s not found.\n", task_name);
    return false;
}


void system_manager_list_tasks() {
    ManagedTask *current = task_list_head;
    printf("Currently managed tasks:\n");
    while (current != NULL) {
        printf("Task Name: %s, Priority: %d\n", current->task_name, current->priority);
        current = current->next;
    }
}