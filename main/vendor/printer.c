#include "vendor/printer.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "esp_event.h"
#include "mdns.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

#define PRINTER_PORT 9100
static const char *TAG = "PRINTER_HANDLER";
#define MAX_TEXT_LENGTH 1024


#define LETTER_WIDTH 6120
#define LETTER_HEIGHT 7920

const char *PCL_INIT = "\x1B%-12345X@PJL\n";
const char *BOLD_ON = "\x1B(s3B";
const char *BOLD_OFF = "\x1B(s0B";
const char *PCL_RESET = "\x1B%-12345X";  


int pixels_to_points(int px) {
    return (int)(px / 0.75);
}


int calculate_text_width(int font_points, int text_length) {
    return font_points * text_length * 0.5;
}
int calculate_text_height(int font_points) {
    return font_points;
}


void calculate_position(const char *alignment, int font_points, const char *text, int *col, int *row) {
    int text_width = calculate_text_width(font_points, strlen(text)) * 10;
    int text_height = calculate_text_height(font_points) * 10;

    if (strcmp(alignment, "Center Middle") == 0) {
        *col = (LETTER_WIDTH / 2) - (text_width / 2);
        *row = (LETTER_HEIGHT / 2) - (text_height / 2);
    } else if (strcmp(alignment, "Top Left") == 0) {
        *col = 0;
        *row = 0;
    } else if (strcmp(alignment, "Top Right") == 0) {
        *col = LETTER_WIDTH - text_width;
        *row = 0;
    } else if (strcmp(alignment, "Bottom Left") == 0) {
        *col = 0;
        *row = LETTER_HEIGHT - text_height;
    } else if (strcmp(alignment, "Bottom Right") == 0) {
        *col = LETTER_WIDTH - text_width;
        *row = LETTER_HEIGHT - text_height;
    } else {
        *col = 0;
        *row = 0;
    }
}


void print_text_to_printer(const char *printer_ip, const char *text, int font_px, const char *alignment) {
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

    
    int font_points = pixels_to_points(font_px);

    
    int col, row;
    calculate_position(alignment, font_points, text, &col, &row);

    
    char set_font_command[64];
    snprintf(set_font_command, sizeof(set_font_command), "\x1B(s1p%dv0s0b4099T", font_points);

    char set_position_command[64];
    snprintf(set_position_command, sizeof(set_position_command), "\x1B&a%dC\x1B&a%dR", col, row);

    
    char formatted_text[MAX_TEXT_LENGTH];
    snprintf(formatted_text, sizeof(formatted_text),
        "%s%s%s%s%s%s\n\f",  // Init, Position, Font, Bold, Text, Reset
        PCL_INIT,             // Initialize PCL mode
        set_position_command, // Move cursor to position
        set_font_command,     // Set font size
        BOLD_ON,              // Enable bold
        text,                 // User-provided text
        BOLD_OFF              // Disable bold
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



void eject_blank_pages(const char *printer_ip, int num_pages) {
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


    for (int i = 0; i < num_pages; i++) {
        const char *form_feed = "\f";
        err = send(sock, form_feed, strlen(form_feed), 0);
        if (err < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Ejected page %d", i + 1);
    }

    close(sock);
    ESP_LOGI(TAG, "Connection closed");
}


void handle_printer_command(int argc, char **argv) {
    if (argc != 5) {
        ESP_LOGE(TAG, "Usage: <printer_ip> <text> <font_px> <alignment>");
        return;
    }

    const char *printer_ip = argv[1];  // Printer IP
    const char *text = argv[2];        // Text to print
    int font_px = atoi(argv[3]);       // Font size in pixels
    const char *alignment = argv[4];   // Alignment option

    ESP_LOGI(TAG, "Printing to printer at IP: %s", printer_ip);

    
    print_text_to_printer(printer_ip, text, font_px, alignment);
}