#include <gtk/gtk.h>

static void on_activate(GtkApplication *app, void *data) {
  (void)data;

  // Upper bar
  GtkWidget *source_btn = gtk_button_new_with_label("Source");
  GtkWidget *output_btn = gtk_button_new_with_label("Output");
  GtkWidget *source_output_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
  gtk_box_append(GTK_BOX(source_output_hbox), source_btn);
  gtk_box_append(GTK_BOX(source_output_hbox), output_btn);

  GtkWidget *upper_bar_cbox = gtk_center_box_new();
  GtkWidget *file_label = gtk_label_new("Filename");
  GtkWidget *settings_btn = gtk_menu_button_new();
  gtk_center_box_set_start_widget(GTK_CENTER_BOX(upper_bar_cbox),
                                  source_output_hbox);
  gtk_center_box_set_center_widget(GTK_CENTER_BOX(upper_bar_cbox), file_label);
  gtk_center_box_set_end_widget(GTK_CENTER_BOX(upper_bar_cbox), settings_btn);

  // Bottom bar
  GtkWidget *reset_btn = gtk_button_new_with_label("Reset");
  GtkWidget *start_stop_btn = gtk_button_new_with_label("Start");
  GtkWidget *timer_label = gtk_label_new("0.0");
  GtkWidget *bottom_bar_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
  gtk_box_append(GTK_BOX(bottom_bar_hbox), reset_btn);
  gtk_box_append(GTK_BOX(bottom_bar_hbox), start_stop_btn);
  gtk_box_append(GTK_BOX(bottom_bar_hbox), timer_label);
  gtk_widget_set_margin_end(timer_label, 32);
  gtk_widget_set_halign(bottom_bar_hbox, GTK_ALIGN_END);

  // Center paned
  GtkWidget *center_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  GtkWidget *test1 = gtk_label_new("Test1");
  GtkWidget *test2 = gtk_label_new("Test2");
  gtk_paned_set_start_child(GTK_PANED(center_paned), test1);
  gtk_paned_set_end_child(GTK_PANED(center_paned), test2);
  gtk_widget_set_vexpand(center_paned, true);

  // Global vbox
  GtkWidget *global_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 32);
  gtk_box_append(GTK_BOX(global_vbox), upper_bar_cbox);
  gtk_box_append(GTK_BOX(global_vbox), center_paned);
  gtk_box_append(GTK_BOX(global_vbox), bottom_bar_hbox);

  GtkWidget *win = gtk_application_window_new(app);
  gtk_window_set_child(GTK_WINDOW(win), global_vbox);

  gtk_window_present(GTK_WINDOW(win));
}

int main(int argc, char *argv[]) {
  GtkApplication *app =
      gtk_application_new("org.streamripper", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
