#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_timer.h"


void log_task_info(FILE *f);
void capture_backtrace(uint32_t *backtrace, int *depth);
void setup_custom_panic_handler(void);

#endif // CRASH_HANDLER_H