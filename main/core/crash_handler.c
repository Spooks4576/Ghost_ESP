#include "core/crash_handler.h"
#include "soc/soc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include <sys/stat.h>
#include <sys/dirent.h>

static const char *TAG = "CrashHandler";

#define STACK_THRESHOLD 100
#define HEAP_THRESHOLD 1024


void log_task_info(FILE *f) {
#ifdef CONFIG_FREERTOS_USE_TRACE_FACILITY
    fprintf(f, "\nTask Information:\n");
    TaskStatus_t *task_array;
    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    task_array = malloc(task_count * sizeof(TaskStatus_t));

    if (task_array != NULL) {
        UBaseType_t total_run_time;
        task_count = uxTaskGetSystemState(task_array, task_count, &total_run_time);
        for (UBaseType_t i = 0; i < task_count; i++) {
            fprintf(f, "Task Name: %s, State: %d, Priority: %d, Stack High Watermark: %ld\n",
                    task_array[i].pcTaskName, task_array[i].eCurrentState,
                    task_array[i].uxCurrentPriority, task_array[i].usStackHighWaterMark);
        }
        free(task_array);
    }
#endif
}


void log_crash_info(const char *filename, const char *reason) {
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open ghostmon file for writing.");
        return;
    }

    fprintf(f, "ESP32 Ghostmon Report\n\n");
    fprintf(f, "Reason: %s\n", reason);
    fprintf(f, "FreeRTOS uptime: %lu ticks\n", xTaskGetTickCount());

    
    size_t free_heap = esp_get_free_heap_size();
    size_t min_free_heap = esp_get_minimum_free_heap_size();
    fprintf(f, "Free heap: %zu bytes\n", free_heap);
    fprintf(f, "Minimum free heap: %zu bytes\n", min_free_heap);


    log_task_info(f);

    fclose(f);
    ESP_LOGI(TAG, "ghostmon report written to %s", filename);
}

void monitor_task(void *pvParameters) {
    while (1) {
        size_t free_heap = esp_get_free_heap_size();
        if (free_heap < HEAP_THRESHOLD) {
            ESP_LOGW(TAG, "Low heap memory detected: %zu bytes", free_heap);
            log_crash_info("/mnt/ghostmon/heap_low.txt", "Low heap memory");
        }


        if (!heap_caps_check_integrity_all(true)) {
            ESP_LOGE(TAG, "Heap corruption detected!");
            log_crash_info("/mnt/ghostmon/heap_corruption.txt", "Heap corruption detected");
        }

        
        size_t min_free_heap = esp_get_minimum_free_heap_size();
        printf("Heap Usage: Free heap: %zu bytes, Minimum free heap: %zu bytes\n", free_heap, min_free_heap);

        
#ifdef CONFIG_FREERTOS_USE_TRACE_FACILITY
        TaskStatus_t *task_array;
        UBaseType_t task_count = uxTaskGetNumberOfTasks();
        task_array = malloc(task_count * sizeof(TaskStatus_t));

        if (task_array != NULL) {
            UBaseType_t total_runtime;
            task_count = uxTaskGetSystemState(task_array, task_count, &total_runtime);
            printf("Task Stack Usage:\n");
            for (UBaseType_t i = 0; i < task_count; i++) {
                 printf("Task: %s, Stack High Watermark: %lu bytes\n", 
                        task_array[i].pcTaskName, task_array[i].usStackHighWaterMark);
                if (task_array[i].usStackHighWaterMark < STACK_THRESHOLD) {
                    ESP_LOGW(TAG, "Low stack detected in task %s", task_array[i].pcTaskName);
                    log_crash_info("/mnt/ghostmon/stack_low.txt", "Low stack memory");
                    break;
                }
            }
            free(task_array);
        }
#endif

        vTaskDelay(pdMS_TO_TICKS(7000));
    }
}

void setup_custom_panic_handler(void) {
    xTaskCreate(monitor_task, "GhostMonTask", 4096, NULL, 1, NULL);
}