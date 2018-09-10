#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include "json.h"
#include "minunit.h"
#include "string.h"

//Моя небольшая надстройка для того, чтобы кидать искючения по желанию
#define MU_ERROR(message) { MU_ASSERT(message, 1 == 0); }

int tests_run = 0;

char * simple_test (void) {
    json_object object;
    parse_object(&object, "{\"key\":\"value\"");
    MU_ASSERT("Object length test", object.length == 1);
    json_pair * pair = object.pairs[0];
    MU_ASSERT("Pair key check", strcmp(pair->key, "key") == 0);
    MU_ASSERT("Pair key check", strcmp((char *)pair->value->value, "value") == 0);
    json_object_free(&object);
    return 0;
}

char * not_too_deep(void) {
    char * str = "[[[[[[[[[[[[[[[[[[[\"Not too deep\"]]]]]]]]]]]]]]]]]]]";
    int depth = 18;
    json_array array;
    json_array * current;
    if (parse_array(&array, str))
        MU_ERROR("Parse failed");
    current = &array;
    for (int i = 0; i < depth; i++) {
        if (current->length != 1 || current->values[0]->type != JSON_ARR) {
            MU_ERROR("Deep parse mistake");
            json_array_free(&array);
            return 0;
        }
        current = (json_array *)current->values[0]->value;
    }
    //Reached needed depth;
    if (current->length != 1 || current->values[0]->type != JSON_STR) {
        MU_ERROR("Value parse mistake");
    } else {
        MU_ASSERT("Deep string", strcmp("Not too deep", (char *)current->values[0]->value) == 0);
    }
    json_array_free(&array);
    return 0;
}

char * difficult_types(void) {
    FILE * file = fopen("input.json", "r");
    char * str = malloc(sizeof(char) * 1555);
    fread(str, sizeof(char), 1555, file);
    fclose(file);

    json_array array;
    //Прошел ли парсинг без ошибок
    if (parse_array(&array, str)) {
        free(str);
        json_array_free(&array);
        MU_ERROR("Parse failed");
        return 0;
    }
    //Правильной ли длины конечный массив
    if (array.length != 20) {
        free(str);
        json_array_free(&array);
        MU_ERROR("Parse mistake");
        return 0;
    }
    //Пустой объект в виде значения
    MU_ASSERT("Empty object type", array.values[2]->type == JSON_OBJ);
    MU_ASSERT("Empty object length", ((json_object *)array.values[2]->value)->length == 0);
    //Пустой массив в виде значения
    MU_ASSERT("Empty array type", array.values[3]->type == JSON_ARR);
    MU_ASSERT("Empty array length", ((json_array *)array.values[3]->value)->length == 0);
    //true/false/null
    MU_ASSERT("True value", (int)array.values[5]->value);
    MU_ASSERT("False value", (int)array.values[6]->value != 0);
    MU_ASSERT("Null value", array.values[7]->type == JSON_NULL);
    //Тесты со сложным объектом
    json_object * object = (json_object *)array.values[8]->value;
    //Тесты значений
    //Числа
    MU_ASSERT("Integer assert", *(double *)object->pairs[0]->value->value == 1234567890.0);
    MU_ASSERT("Real assert", *(double *)object->pairs[1]->value->value == -9876.543210);
    MU_ASSERT("Exponential assert", *(double *)object->pairs[3]->value->value == strtod("1.234567890E+34", NULL));
    //Символы
    MU_ASSERT("Space check", strcmp((char *)object->pairs[7]->value->value, " ") == 0);
    MU_ASSERT("Quotes check", strcmp((char *)object->pairs[8]->value->value, "\\\"") == 0);
    MU_ASSERT("Controls check", strcmp((char *)object->pairs[10]->value->value, "\\b\\f\\n\\r\\t") == 0);
    MU_ASSERT("Special chars check", strcmp((char *)object->pairs[16]->value->value, "`1~!@#$%^&*()_+-={':[,]}|;.</>?") == 0);
    MU_ASSERT("Hex chars check", strcmp((char *)object->pairs[17]->value->value, "\\u0123\\u4567\\u89AB\\uCDEF\\uabcd\\uef4A") == 0);
    MU_ASSERT("Comment check", strcmp((char *)object->pairs[25]->value->value, "// /* <!-- --") == 0);
    MU_ASSERT("JSON as text check", strcmp((char *)object->pairs[29]->value->value,
                                           "{\\\"object with 1 member\\\":[\\\"array with 1 element\\\"]}") == 0);
    //Массив со странным положением пробелов
    json_array * array1 = (json_array *)object->pairs[27]->value->value;
    MU_ASSERT("Strange array size", array1->length == 7);
    for (size_t i = 0; i < array1->length; i++) {
        MU_ASSERT("Strange array values", *(double *)array1->values[i]->value == (double)(i + 1));
    }
    //Тесты ключей
    MU_ASSERT("Empty key check", strcmp(object->pairs[4]->key, "") == 0);
    MU_ASSERT("Number key check", strcmp(object->pairs[15]->key, "0123456789") == 0);
    MU_ASSERT("Comment key check", strcmp(object->pairs[26]->key, "# -- --> */") == 0);
    MU_ASSERT("Spaced key check", strcmp(object->pairs[27]->key, " s p a c e d ") == 0);
    MU_ASSERT("Strange looking key check", strcmp(object->pairs[31]->key,
            "\\/\\\\\\\"\\uCAFE\\uBABE\\uAB98\\uFCDE\\ubcda\\uef4A\\b\\f\\n\\r\\t`1~!@#$%^&*()_+-=[]{}|;:',./<>?") == 0);

    free(str);
    json_array_free(&array);
    return 0;
}

char * test_suit(void) {
    MU_RUN_TEST(simple_test);
    MU_RUN_TEST(not_too_deep);
    MU_RUN_TEST(difficult_types);
    return 0;
}

int main() {
    char * result = test_suit();
    printf("number of tests run: %d\n", tests_run);
    if (result) printf("FAIL: %s\n", result);
    return 0 != result;
}