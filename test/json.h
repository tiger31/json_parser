#ifndef JSON_PARSER_LIBRARY_H
#define JSON_PARSER_LIBRARY_H

#include <dirent.h>

typedef enum _types {
    JSON_OBJ,
    JSON_ARR,
    JSON_STR,
    JSON_NUM,
    JSON_BOOL,
    JSON_NULL,
    JSON_ERR
} json_type;

typedef enum _states {
    INIT,
    EXP_KEY,
    EXP_COLON,
    EXP_VALUE,
    EXP_COMMA,
    ENDED
} states;
typedef struct _json_value {
    json_type type;
    void * value;
} json_value;
typedef struct _json_pair {
    char * key;
    json_value * value;
} json_pair;
typedef struct _json_object {
    json_pair ** pairs;
    size_t length;
} json_object;
typedef struct _json_array {
    json_value ** values;
    size_t length;
} json_array;
int parse_object(json_object * object, char * string);
int parse_array(json_array * array, char * string);

void json_object_free(json_object * object);
void json_array_free(json_array * array);

#endif