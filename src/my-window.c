#include "my-window.h"
#include "gtk/gtk.h"

struct _MyWindow {
  GtkApplicationWindow parent_instance;
  GtkLabel *label;
};

G_DEFINE_FINAL_TYPE(MyWindow, my_window, GTK_TYPE_APPLICATION_WINDOW)

static void my_window_class_init(MyWindowClass *klass) {
  GtkWidgetClass *wc = GTK_WIDGET_CLASS(klass);
  gtk_widget_class_set_template_from_resource(
      wc, "/org/shybu8/streamripper/my-window.ui");
  // gtk_widget_class_bind_template_child(wc, MyWindow, label);
}

static void my_window_init(MyWindow *self) {
  gtk_widget_init_template(GTK_WIDGET(self));
}

MyWindow *my_window_new(GtkApplication *app) {
  return g_object_new(MY_TYPE_WINDOW, "application", app, NULL);
}
