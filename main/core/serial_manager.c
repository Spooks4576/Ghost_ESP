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

#if IS_GHOST_BOARD
    #define UART_NUM_1 UART_NUM_1
    #define GHOST_UART_RX_PIN (2)
    #define GHOST_UART_TX_PIN (3)
    #define GHOST_UART_BUF_SIZE (1024)
#endif

void serial_task(void *pvParameter) {
    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
    int index = 0;

#if IS_GHOST_BOARD
    uint8_t *ghost_data = (uint8_t *)malloc(GHOST_UART_BUF_SIZE);
    int ghost_index = 0;
#elif HAS_GPS
    uint8_t *gps_data = (uint8_t *)malloc(1024);
#endif

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

    #ifdef HAS_GPS
        int gps_length = uart_read_bytes(UART_NUM_1, gps_data, 1024, 10 / portTICK_PERIOD_MS);
        if (gps_length > 0) {
            for (int i = 0; i < gps_length; i++) {
                char incoming_char = (char)gps_data[i];

                gps_manager_process_char(&g_gpsManager, incoming_char);
            }
        }
    #endif

#if IS_GHOST_BOARD
        int ghost_length = uart_read_bytes(UART_NUM_1, ghost_data, GHOST_UART_BUF_SIZE, 10 / portTICK_PERIOD_MS);
        if (ghost_length > 0) {
            for (int i = 0; i < ghost_length; i++) {
                char incoming_char = (char)ghost_data[i];

                if (incoming_char == '\n' || incoming_char == '\r') {
                    serial_buffer[ghost_index] = '\0';
                    if (ghost_index > 0) {
                        printf(serial_buffer);
                        ghost_index = 0;
                    }
                } else if (ghost_index < SERIAL_BUFFER_SIZE - 1) {
                    serial_buffer[ghost_index++] = incoming_char;
                } else {
                    ghost_index = 0;
                }
            }
        }
#endif

        // Check command queue for simulated commands
        SerialCommand command;
        if (xQueueReceive(commandQueue, &command, 0) == pdTRUE) {
            handle_serial_command(command.command);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    free(data);

#if IS_GHOST_BOARD
    free(ghost_data);
#endif
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

#if IS_GHOST_BOARD
    const uart_config_t ghost_uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_param_config(UART_NUM_1, &ghost_uart_config);
    uart_set_pin(UART_NUM_1, GHOST_UART_TX_PIN, GHOST_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, GHOST_UART_BUF_SIZE * 2, 0, 0, NULL, 0);
#elif HAS_GPS

    const uart_config_t gps_uart_config = {
        .baud_rate = 9600,                     // Most GPS modules use 9600 baud by default
        .data_bits = UART_DATA_8_BITS,         // 8 data bits
        .parity = UART_PARITY_DISABLE,         // No parity
        .stop_bits = UART_STOP_BITS_1,         // 1 stop bit
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, // Typically no hardware flow control
    };

    uart_param_config(UART_NUM_1, &gps_uart_config);
    uart_set_pin(UART_NUM_1, GPS_UART_TX_PIN, GPS_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, NULL, 0);
    gps_manager_init(&g_gpsManager);
#endif

    commandQueue = xQueueCreate(10, sizeof(SerialCommand));

    xTaskCreate(serial_task, "SerialTask", 8192, NULL, 10, NULL);
}

int handle_serial_command(const char *input) {
    char *input_copy = strdup(input);
    char *argv[10];
    int argc = 0;
    char *p = input_copy;

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

void simulateCommand(const char *commandString) {
    SerialCommand command;
    strncpy(command.command, commandString, sizeof(command.command) - 1);
    command.command[sizeof(command.command) - 1] = '\0';
    xQueueSend(commandQueue, &command, portMAX_DELAY);
}