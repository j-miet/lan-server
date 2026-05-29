#include <ctype.h>
#include <stdio.h>

#include "url.h"

/**
 * Perform url decoding on source buffer and passes decoded data into dest
 */
void url_decode(char* dest, const char* src) {
    while (*src) {
        // decode hexadecimals %XX e.g. empty space %20
        if (*src == '%' && isxdigit(*(src + 1)) && isxdigit(*(src + 1))) {
            int value;

            sscanf(src + 1, "%2x", &value);

            *dest++ = (char)value;

            src += 3;
        } else if (*src == '+') { // + is also space
            *dest++ = ' ';
            src++;
        } else {
            *dest++ = *src++;
        }
    }

    *dest = '\0';
}