// serial_manager.c

#include "core/serial_manager.h"
#include "driver/uart.h"
#include "core/system_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <ctype.h>
#include <core/commandline.h>
#include "driver/usb_serial_jtag.h"


#define UART_NUM UART_NUM_0
#define BUF_SIZE (1024)
#define SERIAL_BUFFER_SIZE 128

char serial_buffer[SERIAL_BUFFER_SIZE];

// Forward declaration of command handler
int handle_serial_command(const char *command);


void serial_task(void *pvParameter) {
    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
    char serial_buffer[SERIAL_BUFFER_SIZE];       
    int index = 0;                               

    while (1) {
        int length = uart_read_bytes(UART_NUM, data, BUF_SIZE, 100 / portTICK_PERIOD_MS);
        
        if (length > 0) {
            for (int i = 0; i < length; i++) {
                char incoming_char = (char)data[i];

                
                if (incoming_char == '\n' || incoming_char == '\r') {
                    serial_buffer[index] = '\0';
                    if (index > 0) {
                        handle_serial_command(serial_buffer);
                        index = 0;
                    }
                }
                else {
                    if (index < SERIAL_BUFFER_SIZE - 1) {
                        serial_buffer[index++] = incoming_char;
                    } else {
                        index = 0;
                    }
                }
            }
        }

        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    free(data);
}

// Initialize the SerialManager
void serial_manager_init() {
    
     const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    
    uart_param_config(UART_NUM, &uart_config);
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    

    xTaskCreate(serial_task, "SerialTask", 12288, NULL, 10, NULL);
}


int handle_serial_command(const char *input) {
    char *input_copy = strdup(input);  // Make a copy of the input string
    char *argv[10];  // Max 10 arguments
    int argc = 0;
    char *p = input_copy;
    
    // Skip leading spaces
    while (isspace((unsigned char)*p)) {
        p++;
    }

    while (*p != '\0' && argc < 10) {
        if (*p == '"') {
            p++;
            argv[argc++] = p;
            while (*p != '\0' && *p != '"') {
                p++;
            }
            if (*p == '"') {
                *p = '\0';
                p++;
            }
        } else {
            argv[argc++] = p;
            while (*p != '\0' && !isspace((unsigned char)*p)) {
                p++;
            }
            if (*p != '\0') {
                *p = '\0';
                p++;
            }
        }

        while (isspace((unsigned char)*p)) {
            p++;
        }
    }

    if (argc == 0) {
        free(input_copy);
        return ESP_ERR_INVALID_ARG;
    }

    // Find the command function
    CommandFunction cmd_func = find_command(argv[0]);
    if (cmd_func != NULL) {
        cmd_func(argc, argv);
        return ESP_OK;
    } else {
        printf("Unknown command: %s\n", argv[0]);
        return ESP_ERR_INVALID_ARG;
    }

    free(input_copy);
}