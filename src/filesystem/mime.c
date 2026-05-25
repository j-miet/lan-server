#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mime.h"

/**
 * MIME types
 * Most common ones can be found here https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/MIME_types/Common_types
 */
const char* get_content_type(const char* path) {
    const char* ext = strrchr(path, '.'); // file extension

    if (!ext)
        return "text/plain";

    if (strcmp(ext, ".png") == 0)
        return "image/png";

    if (strcmp(ext, ".jpg") == 0)
        return "image/jpeg";

    if (strcmp(ext, ".jpeg") == 0)
        return "image/jpeg";

    if (strcmp(ext, ".svg") == 0)
        return "image/svg+xml";

    if (strcmp(ext, ".gif") == 0)
        return "image/gif";

    /* short video files work but larger ones require partial content response which has not been implemented yet
    if (strcmp(ext, ".mp4") == 0)
        return "video/mp4";

    if (strcmp(ext, ".mp3") == 0)
        return "audio/mpeg";
    */

    if (strcmp(ext, ".xlsx") == 0)
        return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";

    if (strcmp(ext, ".pdf") == 0)
        return "application/pdf";

    if (strcmp(ext, ".json") == 0)
        return "application/json";

    if (strcmp(ext, ".zip") == 0)
        return "application/zip";

    if (strcmp(ext, ".7z") == 0)
        return "application/x-7z-compressed";

    if (strcmp(ext, ".html") == 0)
        return "text/html";

    if (strcmp(ext, ".css") == 0)
        return "text/css";

    if (strcmp(ext, ".js") == 0)
        return "application/javascript";

    return "application/octet-stream"; // default to unknown binary files
}