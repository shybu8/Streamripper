#include "my-window.h"
#include "resources.h"
#include <gtk/gtk.h>

static void on_activate(GtkApplication *app, void *data) {
  (void)data;

  GtkWindow *win = GTK_WINDOW(my_window_new(app));
  gtk_window_present(win);
}

int main(int argc, char *argv[]) {
  g_resources_register(myres_get_resource());

  GtkApplication *app = gtk_application_new("org.shybu8.streamripper",
                                            G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
