#ifndef JSON_H
#define JSON_H

void json_escape(const char* input, char* output, int max);
int json_get_string(const char* json, const char* key, char* output, int size);

#endif
