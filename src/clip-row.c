#include "clip-row.h"

struct _ClipRow {
  GtkListBoxRow parent_instance;
  GtkEditableLabel *label;
};

G_DEFINE_FINAL_TYPE(ClipRow, clip_row, GTK_TYPE_LIST_BOX_ROW)

static void clip_row_class_init(ClipRowClass *klass) {
  GtkWidgetClass *wc = GTK_WIDGET_CLASS(klass);
  gtk_widget_class_set_template_from_resource(
      wc, "/org/shybu8/streamripper/clip-row.ui");
  gtk_widget_class_bind_template_child(wc, ClipRow, label);
}

static void clip_row_init(ClipRow *self) {
  gtk_widget_init_template(GTK_WIDGET(self));
}

ClipRow *clip_row_new() { return g_object_new(CLIP_TYPE_ROW, NULL); }

const char *clip_row_get_label_text(ClipRow *cr) {
  return gtk_editable_get_text(GTK_EDITABLE(cr->label));
}
