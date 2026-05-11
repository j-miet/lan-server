#include <stdio.h>

#include "utils.h"

void log_info(const char* msg) {
    printf("[INFO] %s\n", msg);
}

void log_error(const char* msg) {
    printf("[ERROR] %s\n", msg);
}
