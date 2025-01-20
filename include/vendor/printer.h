#ifndef PRINTER_HANDLER_H
#define PRINTER_HANDLER_H

#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

void handle_printer_command(int argc, char **argv);

void print_text_to_printer(const char *printer_ip, const char *text,
                           int font_px, const char *alignment);

#endif