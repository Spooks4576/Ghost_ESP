#include "core/utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

bool is_in_task_context(void)
{
    if (xTaskGetCurrentTaskHandle() != NULL) {
        return true;
    } else {
        return false;
    }
}