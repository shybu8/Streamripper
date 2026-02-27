#include "my-window.h"
#include "gtk/gtk.h"

struct _MyWindow {
  GtkApplicationWindow parent_instance;
  // Upper bar
  GtkLabel *label;
  GtkButton *source_btn;
  GtkButton *add_clip_btn;
  GtkButton *output_btn;
  GtkLabel *file_label;
  GtkMenuButton *settings_mbtn;
  // Center paned
  GtkListBox *sidebar_list_box;
  GtkEntry *start_ts_entry;
  GtkEntry *end_ts_entry;
  GtkButton *start_ts_plus_btn;
  GtkButton *end_ts_plus_btn;
  GtkButton *start_ts_minus_btn;
  GtkButton *end_ts_minus_btn;
  GtkSpinButton *start_ts_spin_btn;
  GtkSpinButton *end_ts_spin_btn;
  GtkButton *start_ts_now_btn;
  GtkButton *end_ts_now_btn;
  GtkButton *total_label;
  GtkButton *preview_start_btn;
  GtkButton *preview_end_btn;
  GtkSpinButton *preview_start_spin_btn;
  GtkSpinButton *preview_end_spin_btn;
  GtkSpinButton *preview_full_btn;
  GtkSpinButton *render_btn;
  // Bottom bar
  GtkButton *reset_btn;
  GtkButton *start_stop_btn;
  GtkLabel *timer_label;
};

G_DEFINE_FINAL_TYPE(MyWindow, my_window, GTK_TYPE_APPLICATION_WINDOW)

static void my_window_class_init(MyWindowClass *klass) {
  GtkWidgetClass *wc = GTK_WIDGET_CLASS(klass);
  gtk_widget_class_set_template_from_resource(
      wc, "/org/shybu8/streamripper/my-window.ui");
  gtk_widget_class_bind_template_child(wc, MyWindow, source_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, add_clip_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, output_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, file_label);
  gtk_widget_class_bind_template_child(wc, MyWindow, settings_mbtn);
  gtk_widget_class_bind_template_child(wc, MyWindow, sidebar_list_box);
  gtk_widget_class_bind_template_child(wc, MyWindow, start_ts_entry);
  gtk_widget_class_bind_template_child(wc, MyWindow, end_ts_entry);
  gtk_widget_class_bind_template_child(wc, MyWindow, start_ts_plus_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, end_ts_plus_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, start_ts_minus_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, end_ts_minus_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, start_ts_spin_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, end_ts_spin_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, start_ts_now_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, end_ts_now_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, total_label);
  gtk_widget_class_bind_template_child(wc, MyWindow, preview_start_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, preview_end_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, preview_start_spin_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, preview_end_spin_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, preview_full_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, render_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, reset_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, start_stop_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, timer_label);
}

static void my_window_init(MyWindow *self) {
  gtk_widget_init_template(GTK_WIDGET(self));
}

MyWindow *my_window_new(GtkApplication *app) {
  return g_object_new(MY_TYPE_WINDOW, "application", app, NULL);
}
