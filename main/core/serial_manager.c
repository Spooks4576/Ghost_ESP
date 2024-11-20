#include "core/serial_manager.h"
#include "driver/uart.h"
#include "core/system_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <ctype.h>
#include <core/commandline.h>
#include "managers/gps_manager.h"
#include "driver/usb_serial_jtag.h"

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
    #define JTAG_SUPPORTED 1
#else
    #define JTAG_SUPPORTED 0
#endif

#define UART_NUM UART_NUM_0
#define BUF_SIZE (1024)
#define SERIAL_BUFFER_SIZE 528

char serial_buffer[SERIAL_BUFFER_SIZE];

// Forward declaration of command handler
int handle_serial_command(const char *command);

void serial_task(void *pvParameter) {
    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
    int index = 0;

    while (1) {
        int length = 0;

        // Read data from the main UART
        length = uart_read_bytes(UART_NUM, data, BUF_SIZE, 10 / portTICK_PERIOD_MS);

#if JTAG_SUPPORTED
        if (length <= 0) {
            length = usb_serial_jtag_read_bytes(data, BUF_SIZE, 10 / portTICK_PERIOD_MS);
        }
#endif

        // Process data from the main UART
        if (length > 0) {
            for (int i = 0; i < length; i++) {
                char incoming_char = (char)data[i];

                if (incoming_char == '\n' || incoming_char == '\r') {
                    serial_buffer[index] = '\0';
                    if (index > 0) {
                        handle_serial_command(serial_buffer);
                        index = 0;
                    }
                } else if (index < SERIAL_BUFFER_SIZE - 1) {
                    serial_buffer[index++] = incoming_char;
                } else {
                    index = 0;
                }
            }
        }

        // Check command queue for simulated commands
        SerialCommand command;
        if (xQueueReceive(commandQueue, &command, 0) == pdTRUE) {
            handle_serial_command(command.command);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    free(data);
}

// Initialize the SerialManager
void serial_manager_init() {
    // UART configuration for main UART
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_param_config(UART_NUM, &uart_config);
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

#if JTAG_SUPPORTED
    usb_serial_jtag_driver_config_t usb_serial_jtag_config = {
        .rx_buffer_size = BUF_SIZE,
        .tx_buffer_size = BUF_SIZE,
    };
    usb_serial_jtag_driver_install(&usb_serial_jtag_config);
#endif

    commandQueue = xQueueCreate(10, sizeof(SerialCommand));

    xTaskCreate(serial_task, "SerialTask", 8192, NULL, 10, NULL);
    printf("Serial Started...\n");
}

int handle_serial_command(const char *input) {
    char *input_copy = strdup(input);
    if (input_copy == NULL) {
        printf("Memory allocation error\n");
        return ESP_ERR_INVALID_ARG;
    }
    char *argv[10];
    int argc = 0;
    char *p = input_copy;

    while (*p != '\0' && argc < 10) {
        while (isspace((unsigned char)*p)) {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        if (*p == '"' || *p == '\'') {
            // Handle quoted arguments
            char quote = *p++;
            argv[argc++] = p; // Start of the argument

            while (*p != '\0' && *p != quote) {
                p++;
            }

            if (*p == quote) {
                *p = '\0'; // Null-terminate the argument
                p++;
            } else {
                // Handle missing closing quote
                printf("Error: Missing closing quote\n");
                free(input_copy);
                return ESP_ERR_INVALID_ARG;
            }
        } else {
            // Handle unquoted arguments
            argv[argc++] = p; // Start of the argument

            while (*p != '\0' && !isspace((unsigned char)*p)) {
                p++;
            }

            if (*p != '\0') {
                *p = '\0'; // Null-terminate the argument
                p++;
            }
        }
    }

    if (argc == 0) {
        free(input_copy);
        return ESP_ERR_INVALID_ARG;
    }

    CommandFunction cmd_func = find_command(argv[0]);
    if (cmd_func != NULL) {
        cmd_func(argc, argv);
        free(input_copy);
        return ESP_OK;
    } else {
        printf("Unknown command: %s\n", argv[0]);
        free(input_copy);
        return ESP_ERR_INVALID_ARG;
    }
}

void simulateCommand(const char *commandString) {
    SerialCommand command;
    strncpy(command.command, commandString, sizeof(command.command) - 1);
    command.command[sizeof(command.command) - 1] = '\0';
    xQueueSend(commandQueue, &command, portMAX_DELAY);
}