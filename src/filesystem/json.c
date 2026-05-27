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