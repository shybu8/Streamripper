#pragma once
#include <gtk/gtk.h>
#define CLIP_TYPE_ROW (clip_row_get_type())
G_DECLARE_FINAL_TYPE(ClipRow, clip_row, CLIP, ROW, GtkListBoxRow)

ClipRow *clip_row_new();
