#include "save_load.h"
#include "gio/gio.h"
#include "json.h"
#include <stdio.h>
#include <stdlib.h>

char *serialize_clip_page_records(ClipData *data, size_t len,
                                  size_t *res_str_len) {
  const char *name_json_key = "name";
  const char *start_ts_json_key = "start_ts";
  const char *end_ts_json_key = "end_ts";

  JsonArr *arr = malloc(sizeof(JsonArr));
  arr->len = len;
  arr->values = malloc(sizeof(JsonVal) * len);

  for (size_t i = 0; i < len; i++) {
    JsonObj *obj = malloc(sizeof(JsonObj));

    obj->len = 3;
    obj->pairs = malloc(sizeof(JsonPair) * obj->len);

    // Name of clip (pairs[0])
    obj->pairs[0] = (JsonPair){
        .key =
            (JsonStr){
                .start = name_json_key,
                .len = strlen(name_json_key),
                .needs_dealloc = false,
            },
        .value =
            (JsonVal){
                .type = JSON_TYPE_STR,
                .as.str_ptr = malloc(sizeof(JsonStr)),
            },
    };
    bool needs_encoding = false;
    char *encoded_str;
    size_t buf_size;
    if ((needs_encoding = json_str_needs_encoding(data[i].name, &buf_size))) {
      encoded_str = malloc(buf_size);
      json_str_encode_into_buf(data[i].name, encoded_str);
    }
    obj->pairs[0].value.as.str_ptr->start =
        needs_encoding ? encoded_str : data[i].name;
    obj->pairs[0].value.as.str_ptr->len =
        strlen(obj->pairs[0].value.as.str_ptr->start);
    obj->pairs[0].value.as.str_ptr->needs_dealloc = needs_encoding;

    // Start ts (pairs[1])
    obj->pairs[1] = (JsonPair){
        .key =
            (JsonStr){
                .start = start_ts_json_key,
                .len = strlen(start_ts_json_key),
                .needs_dealloc = false,
            },
        .value =
            (JsonVal){
                .type = JSON_TYPE_INT,
                .as.integer = data[i].info->start_ts,
            },
    };

    // End ts (pairs[2])
    obj->pairs[2] = (JsonPair){
        .key =
            (JsonStr){
                .start = end_ts_json_key,
                .len = strlen(end_ts_json_key),
                .needs_dealloc = false,
            },
        .value =
            (JsonVal){
                .type = JSON_TYPE_INT,
                .as.integer = data[i].info->end_ts,
            },
    };

    arr->values[i] = (JsonVal){
        .type = JSON_TYPE_OBJ,
        .as.obj_ptr = obj,
    };
  }

  JsonVal *root = malloc(sizeof(JsonVal));
  root->type = JSON_TYPE_ARR;
  root->as.arr_ptr = arr;

  size_t buf_size = 1;
  char *buf = malloc(buf_size);
  *buf = '\0';
  *res_str_len = strlen(buf);
  JsonStyle style = JSON_STYLE_PRETTY_PRINT_TABS;
  json_serialize_val(root, &buf, res_str_len, &buf_size, &style);
  json_free_val(root);
  return buf;
}

void save_into_file(ClipData *data, size_t len, GFile *file) {
  size_t data_str_len;
  char *data_str = serialize_clip_page_records(data, len, &data_str_len);
  GError *error = NULL;

  if (!g_file_replace_contents(file, data_str, data_str_len, NULL, FALSE,
                               G_FILE_CREATE_NONE, NULL, NULL, &error)) {
    g_warning("write failed: %s\n", error->message);
    g_clear_error(&error);
  }
}
