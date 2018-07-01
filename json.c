#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stdlib.h>
#include "json.h"

const char * num_reg = "^(-?([1-9][0-9]*|0)[ ]*(.[ ]*[0-9]+)?[ ]*([eE][ ]*[+-]?[ ]*[0-9]+)?)$";
const char * str_reg = "^((\\\\[\\\\\"bfnrt/])|(\\\\u[0-9a-fA-F]{4})|([^\"\\\\]))*$";

int check_number(char * number) {
    regex_t regex;
    regcomp(&regex, num_reg, REG_EXTENDED);
    int result = regexec(&regex, number, 0, NULL, 0);
    regfree(&regex);
    return !result;
}
int check_string(char * string) {
    regex_t regex;
    regcomp(&regex, str_reg, REG_EXTENDED);
    int result = regexec(&regex, string, 0, NULL, 0);
    regfree(&regex);
    return !result;
}
int is_space(char c) {
    if (c != ' ' && c != '\t' && c != '\n' && c != 'v'
        && c != '\f' && c != '\r')
        return 0;
    return 1;
}
int skip_spaces(char * string, int from) {
    for (int i = from; i < (int)strlen(string); ++i) {
        if (!is_space(string[i]))
            return i;
    }
    return -1;
}
int pair_index(char * string, char c, int pos) {
    char pair;
    int inc_obj = 0;
    int inc_arr = 0;
    int in_quotes = 0;
    int escape_next = 0;
    switch (c) {
        case 34:
            pair = 34;
            in_quotes = 1;
            break;
        case 91:
            pair = 93;
            break;
        case 123:
            pair = 125;
            break;
        default:
            return 0;
    }
    int i;
    for (i = pos; i < (int)strlen(string); i++) {
        switch (string[i]) {
            case 92:
                //Следующий символ будет пропущен
                escape_next = !escape_next;
                break;
            case 34:
                //Если не симовл не пропускается, то обрабатываем
                if (!escape_next) {
                    //Выходим из кавычек, если в них были и наоборот
                    in_quotes = !in_quotes;
                    //Если вышли, и нужно найти кавычки - возвращаем индекс
                    if (!in_quotes && pair == 34) {
                        return i;
                    }
                    break;
                } else
                    escape_next = 0;
                break;
            case 91:
                //Если символ не пропускается(вроде такое не возможно, но проверить стоит) и не внутри строки
                if (!escape_next && !in_quotes)
                    inc_arr++; //Инкапсуляция внутрь массива
                break;
            case 93:
                if (!escape_next && !in_quotes) {
                    //Если нет инкапсуляции (тот же уровень) - возвращаем индекс
                    if (inc_arr == 0 && inc_obj == 0)
                        return i;
                    inc_arr--;
                }
                break;
            case 123:
                if (!escape_next && !in_quotes)
                    inc_obj++;
                break;
            case 125:
                if (!escape_next && !in_quotes) {
                    if (inc_arr == 0 && inc_obj == 0)
                        return i;
                    inc_obj--;
                }
                break;
            default:
                if (escape_next)
                    escape_next = 0;
                break;
        }
    }
    return -1;
}
int num_index(char * string, int pos) {
    int len = (int)strlen(string);
    int i = 0;
    int space_resist = 0;
    int space_index = 0;
    for (i = pos; i < len; i++) {
        if (is_space(string[i])) {
            if (!space_resist)
                space_index = i;
            space_resist = 1;
            continue;
        }
        if (string[i] == 44 || string[i] == 93 || string[i] == 125) //"," может не быть, будет "]" или "}", потратил часа пол чтобы найти этот баг
            return ((space_resist) ? space_index : i);
        space_resist = 0;
    }
    return -1;
}
char * strpaired(char * string, int from, int to, int include_c) {
    size_t len = (size_t)to - (from + ((include_c) ? -1 : 1));
    char * str = malloc((len + 1) * sizeof(char));
    strncpy(str, string + (from + 1 - include_c), len);
    str[len] = '\0';
    return str;
}
char * get_number(char * string, int from, int to) {
    size_t len = (size_t)to - from;
    char * str = malloc((len + 1) * sizeof(char));
    strncpy(str, string + from, len);
    str[len] = '\0';
    return str;
}
json_type estimate_type(const char * string, int pos) {
    const char c = string[pos];
    if (c == 34) // === '"'
        return JSON_STR;
    else if (c == 91) // === '['
        return JSON_ARR;
    else if (c == 123) // === '{'
        return JSON_OBJ;
    else if (c == 45 || (c > 47 && c < 58)) // === '-' или [0;9]
        return JSON_NUM;
    else if (c == 110) // === 'n'
        return JSON_NULL;
    else if (c == 102 || c == 116) // === 'f' или 't'
        return JSON_BOOL;
    else
        return JSON_ERR;
}
int parse_value(json_value * value, char * string, int pos) {
    value->type = estimate_type(string, pos);
    switch (value->type) {
        case JSON_OBJ: { //Без скобок запрещает объявлять переменную, так как не в начале блока
            //Копируем кусок в новую строку
            int index = pair_index(string, 123, pos + 1);
            char * obj_str = strpaired(string, pos, index, 1);
            json_object * obj = (json_object *)malloc(sizeof(json_object));
            if (parse_object(obj, obj_str))
                return -1;
            value->value = obj;
            free(obj_str);
            return index;
        }
        case JSON_ARR: {
            int index = pair_index(string, 91, pos + 1);
            char * arr_str = strpaired(string, pos, index, 1);
            json_array * arr = (json_array *)malloc(sizeof(json_array));
            if (parse_array(arr, arr_str))
                return -1;
            value->value = arr;
            free(arr_str);
            return index;
        }
        case JSON_STR: {
            int index = pair_index(string, 34, pos + 1);
            char * str = strpaired(string, pos, index, 0);
            if (!check_string(str))
                return -1;
            value->value = str;
            return index;
        }
        case JSON_NUM: {
            int index = num_index(string, pos);
            char * num = get_number(string, pos, index);
            if (!check_number(num))
                return -1;
            value->value = num;
            return index - 1;
        }
        case JSON_BOOL: {
            char * true = "true";
            char * false = "false";
            char * seq;
            if (string[pos] == 102)
                seq = false;
            else
                seq = true;
            int i;
            for (i = 0; i < (int)strlen(seq); i++)
                if (string[pos + i] != seq[i])
                    return -1;
            //Не теряем локальную переменную из памяти
            int * result = malloc(sizeof(int));
            *result = ((seq == true) ? 1 : 0);
            value->value = result;
            return pos + i - 1;
        }
        case JSON_NULL: {
            char * null = "null";
            int i = 0;
            for (i = 0; i < 4; i++)
                if (string[pos + i] != null[i])
                    return -1;
            //value оставим null-pointer`ом
            return pos + i - 1;
        }
        case JSON_ERR:
            return -1;
    }
    return -1;
}
int parse_object(json_object * object, char * string) {
    states state = INIT;
    size_t length = strlen(string);
    json_pair ** pairs = malloc(sizeof(json_pair*));
    int members = 0;
    int i = 0;
    for (i; i < (int)length; ++i) {
        switch (state) {
            //Стадия инициализации объекта - поиск {
            case INIT:
                if (string[i] != 123)
                    return 1;
                state = EXP_KEY;
                break;
            case EXP_KEY:
                i = skip_spaces(string, i); //Пропускаем все пробелы до следующего ключа
                if (string[i] == 125) { //Может быть не ключ, а "}", что тоже верно
                    state = ENDED;
                } else if (string[i] == 34) { //Ключ всегда начинается с ", так как ключом может быть только строка
                    int index = pair_index(string, 34, i + 1);
                    char * key = strpaired(string, i, index, 0);
                    pairs = (json_pair **)realloc(pairs, ++members * sizeof(json_pair*)); //Увеличиваем массив ключей на один
                    json_pair * p = malloc(sizeof(json_pair));
                    pairs[members - 1] = p;
                    pairs[members - 1]->key = key;
                    state = EXP_COLON;
                    i = index;
                }
                break;
            case EXP_COLON:
                i = skip_spaces(string, i);
                if (string[i] != 58)
                    return 1;
                i = skip_spaces(string, ++i);
                i--;
                state = EXP_VALUE;
                break;
            case EXP_VALUE: {
                json_value * value = (json_value *)malloc(sizeof(json_value));
                int result = parse_value(value, string, i);
                if (result == -1)
                    return 1;
                i = result;
                pairs[members - 1]->value = value;
                state = EXP_COMMA;
                break;
            }
            case EXP_COMMA:
                i = skip_spaces(string, i); //Пропускаем все пробелы до следующего ключа
                if (string[i] == 125) { //Может быть не ключ, а "}", что тоже верно
                    state = ENDED;
                } else if (string[i] == 44) {
                    state = EXP_KEY;
                } else {
                    return 1;
                }
                break;
            case ENDED:
                return 1;
        }
    }
    object->pairs = pairs;
    object->length = (size_t)members;
    return 0;
}
int parse_array(json_array * array, char * string) {
    states state = INIT;
    size_t length = strlen(string);
    json_value ** values = '\0';
    int members = 0;
    int i = 0;
    for (i = 0; i < (int)length; i++) {
        switch (state) {
            case INIT:
                if (string[i] != 91)
                    return 1;
                state = EXP_VALUE;
                break;
            case EXP_VALUE: {
                i = skip_spaces(string, i); //Пропускаем все пробелы до следующего значения
                if (string[i] == 93) { //Может быть не значение, а "]", что тоже верно
                    state = ENDED;
                    break;
                }
                json_value * value = malloc(sizeof(json_value));
                int result = parse_value(value, string, i);
                if (result == -1)
                    return 1;
                i = result;
                values = (json_value **)realloc(values, ++members * sizeof(json_value*));
                values[members - 1] = value;
                state = EXP_COMMA;
                break;
            }
            case EXP_COMMA:
                i = skip_spaces(string, i);
                if (string[i] == 93) { //Может быть не ключ, а "}", что тоже верно
                    state = ENDED;
                } else {
                    state = EXP_VALUE;
                }
                break;
            default:
                return 1;
        }
    }
    array->values = values;
    array->length = (size_t)members;
    return 0;
}

void json_object_free(json_object * object) {
    for (int i = 0; i < (int)object->length; i++) {
        json_pair * p = object->pairs[i];
        switch (p->value->type) {
            case JSON_OBJ:
                json_object_free(p->value->value);
                free(p->value);
                break;
            case JSON_ARR:
                json_array_free(p->value->value);
                free(p->value);
                break;
            case JSON_STR:
            case JSON_NUM:
                free(p->value);
                break;
            default:
                break;
        }
        free(p->key);
        free(p);
    }
    free(object);
}
void json_array_free(json_array * array) {
    for (int i = 0; i < (int)array->length; ++i) {
        json_value * value = array->values[i];
        switch (value->type) {
            case JSON_OBJ:
                json_object_free(value->value);
                free(value);
                break;
            case JSON_ARR:
                json_array_free(value->value);
                free(value);
                break;
            case JSON_STR:
            case JSON_NUM:
                free(value);
                break;
            default:
                break;
        }
        free(value);
    }
    free(array);
}