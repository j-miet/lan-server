#include <stdio.h>
#include <string.h>

#include "json.h"

/**
 * Perform character escaping on input data to produce valid json output string
 */
void json_escape(const char* input, char* output, int max) {
    int j = 0;

    for (int i = 0; input[i] && j < max - 2; i++) {
        char c = input[i];

        switch (c) {
            case '\"':
                output[j++] = '\\';
                output[j++] = '\"';
                break;

            case '\n':
                output[j++] = '\\';
                output[j++] = 'n';
                break;

            case '\r':
                output[j++] = '\\';
                output[j++] = 'r';
                break;

            case '\\':
                output[j++] = '\\';
                output[j++] = '\\';
                break;

            default:
                output[j++] = c;
        }
    }

    output[j] = '\0';
}

/**
 * Extract value from a json key-value pair
 * Returns 0 on success, -1 on failure
 */
int json_get_string(const char* json, const char* key, char* output, int size) {
    char pattern[64];

    snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);

    const char* start = strstr(json, pattern);

    if (!start)
        return -1;

    start += strlen(pattern);

    int i = 0;

    while (*start && *start != '"' && i < size - 1)
        output[i++] = *start++;

    output[i] = '\0';

    return 0;
}