#include "gio/gio.h"
#include "glib.h"
#include "my-window.h"
#include "stdint.h"

void ffmpeg_render(GFile *file, guint64 start_ts, guint64 end_ts,
                   GFile *output_dir, const char *output_name, MyWindow *win);
