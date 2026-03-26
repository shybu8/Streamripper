#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum {
  JSON_TYPE_OBJ,
  JSON_TYPE_ARR,
  JSON_TYPE_STR,
  JSON_TYPE_INT,
  JSON_TYPE_FRC,
  JSON_TYPE_BOL,
  JSON_TYPE_NUL,
} JsonType;

typedef struct JsonStr JsonStr;
typedef struct JsonObj JsonObj;
typedef struct JsonArr JsonArr;
typedef struct JsonVal JsonVal;
typedef struct JsonPair JsonPair;

struct JsonStr {
  const char *start;
  size_t len;
  bool needs_dealloc;
};

struct JsonVal {
  union {
    JsonObj *obj_ptr;
    JsonArr *arr_ptr;
    JsonStr *str_ptr;
    long long integer;
    double fract;
    bool boolean;
  } as;
  JsonType type;
};

struct JsonPair {
  JsonStr key;
  JsonVal value;
};

struct JsonObj {
  JsonPair *pairs;
  size_t len;
};

struct JsonArr {
  JsonVal *values;
  size_t len;
};

bool json_parse_val(JsonVal *res, const char **text);
void json_free_val(JsonVal *val);
bool json_decode_str(const char **res, size_t *res_len, const char *src,
                     size_t len);

JsonVal *json_value_by_key(JsonObj *obj, const char *to_find);

typedef struct {
  bool minimal;
  size_t indentation_level;
  char *indentation_str;
} JsonStyle;

#define JSON_STYLE_MINIMAL                                                     \
  {                                                                            \
      .minimal = true,                                                         \
      .indentation_level = 0,                                                  \
      .indentation_str = "",                                                   \
  }

#define JSON_STYLE_PRETTY_PRINT_TABS                                           \
  {                                                                            \
      .minimal = false,                                                        \
      .indentation_level = 0,                                                  \
      .indentation_str = "\t",                                                 \
  }

#define JSON_STYLE_PRETTY_PRINT_DOUBLESPACES                                   \
  {                                                                            \
      .minimal = false,                                                        \
      .indentation_level = 0,                                                  \
      .indentation_str = "  ",                                                 \
  }

#define JSON_STYLE_PRETTY_PRINT_FOURSPACES                                     \
  {                                                                            \
      .minimal = false,                                                        \
      .indentation_level = 0,                                                  \
      .indentation_str = "    ",                                               \
  }

void json_serialize_val(JsonVal *val, char **str, size_t *str_len,
                        size_t *buf_len, JsonStyle *style);

bool json_str_needs_encoding(const char *str, size_t *res_buf_size);

void json_str_encode_into_buf(const char *str, char *buf);
