#include "json.h"
#include "my-window.h"

void save_into_file(ClipData *data, size_t len, GFile *file);

bool load_from_file(GFile *file, ClipData **data, size_t *len);

void free_loaded_data(ClipData *data, size_t len);
