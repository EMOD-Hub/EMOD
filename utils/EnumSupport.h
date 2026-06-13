
#pragma once

#include "BoostLibWrapper.h"

#include <string.h>

// (the ee_ prefix stands for "evil enum")
#define ee_TO_STR(unused,data,elem) BOOST_PP_STRINGIZE(BOOST_PP_SEQ_ELEM(0,elem)) ,
#define ee_ASSIGN_EXPLICIT_VALUE(unused, data, elem) BOOST_PP_SEQ_ELEM(0,elem) = BOOST_PP_SEQ_ELEM(1,elem) ,
#define ee_GET_EXPLICIT_VALUE(unused, data, elem) BOOST_PP_SEQ_ELEM(1,elem) ,

#define ENUM_DEFINE(enum_,elements)          \
    namespace enum_ {\
    enum Enum { BOOST_PP_SEQ_FOR_EACH(ee_ASSIGN_EXPLICIT_VALUE, ~, elements) };         \
    struct pairs\
    {\
        static int count() { return BOOST_PP_SEQ_SIZE(elements); }\
        \
        static int lookup_value(const char * char_val) { return EnumSupport::lookup_enum_value(char_val, count(), get_values(), get_keys()); } \
        static const int* get_values() { static const int values[] = { BOOST_PP_SEQ_FOR_EACH(ee_GET_EXPLICIT_VALUE,~,elements)}; return values; } \
        \
        static const char * lookup_key(int enum_val) { return EnumSupport::lookup_enum_name(int(enum_val), count(), get_values(), get_keys()); }   \
        static const char ** get_keys() { static const char* strings[] = { BOOST_PP_SEQ_FOR_EACH(ee_TO_STR,~,elements) }; return strings; }\
    };\
    };

    struct EnumSupport
    {   
        static const char* lookup_enum_name(int int_value, int total, const int *values, const char** strings)
        {
            for (int k = 0; k < total; k++)
            {
                if (values[k]==int_value)
                    return strings[k];
            }

            return nullptr;
        }

        static int lookup_enum_value(const char* char_value, int total, const int *values, const char** strings)
        {
            for (int k = 0; k < total; k++)
            {
                if (strcmp(strings[k], char_value) == 0)
                    return values[k];
            }

            return -1;
        }
    };

#define ENUM_VALUE(name) (name)
#define ENUM_VALUE_SPEC(name, value) ((name)(value))
