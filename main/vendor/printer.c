#include "vendor/printer.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "esp_event.h"
#include "mdns.h"
#include "esp_log.h"
#include <string.h>

#define PRINTER_PORT 9100
static const char *TAG = "PRINTER_HANDLER";
#define MAX_TEXT_LENGTH 1024


const char *PCL_INIT = "\x1B%-12345X@PJL\n";
const char *SET_LARGE_FONT = "\x1B(s1p75v0s0b4099T";
const char *BOLD_ON = "\x1B(s3B";
const char *BOLD_OFF = "\x1B(s0B";
const char *PCL_RESET = "\x1B%-12345X";


void print_big_text_to_printer(const char *printer_ip, const char *text) {
    struct sockaddr_in printer_addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }

    printer_addr.sin_addr.s_addr = inet_addr(printer_ip);
    printer_addr.sin_family = AF_INET;
    printer_addr.sin_port = htons(PRINTER_PORT);

    int err = connect(sock, (struct sockaddr *)&printer_addr, sizeof(printer_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        close(sock);
        return;
    }

    ESP_LOGI(TAG, "Connected to printer at %s:%d", printer_ip, PRINTER_PORT);

    
    char formatted_text[MAX_TEXT_LENGTH];
    snprintf(formatted_text, sizeof(formatted_text),
        "%s%s%s%s%s\n\f",  // Send PCL Init, Font, Bold, Text, Reset
        PCL_INIT,           // Initialize PCL mode
        SET_LARGE_FONT,     // Set large font size
        BOLD_ON,            // Turn on bold
        text,               // User-specified text
        BOLD_OFF            // Turn off bold
    );

    size_t len = strlen(formatted_text);
    err = send(sock, formatted_text, len, 0);
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
    } else {
        ESP_LOGI(TAG, "Sent %d bytes to the printer", err);
    }

    close(sock);
    ESP_LOGI(TAG, "Connection closed");
}


void handle_printer_command(int argc, char **argv) {
    if (argc != 3) {
        ESP_LOGE(TAG, "Usage: <printer_ip> <text>");
        return;
    }

    const char *printer_ip = argv[1];
    const char *text = argv[2];

    ESP_LOGI(TAG, "Printing to printer at IP: %s", printer_ip);

    print_big_text_to_printer(printer_ip, text);
}