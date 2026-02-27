#include "my-window.h"
#include "clip-row.h"
#include "glib.h"
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

  // Service
  gint64 timer_start_us;
  gint64 timer_elapsed_us;
  guint decimal_timer_timout_id;
  ClipRow *current_row;
};

G_DEFINE_FINAL_TYPE(MyWindow, my_window, GTK_TYPE_APPLICATION_WINDOW)

static gboolean on_decimal_timer_timeout(void *data) {
  MyWindow *win = data;
  gint64 now_us = g_get_monotonic_time();
  gint64 us = now_us - win->timer_start_us;

  // tenths of a second
  guint64 t = (guint64)(us / 100000); // 0.1s = 100,000us
  guint64 sec = t / 10;
  guint64 dec = t % 10;
  guint64 min = sec / 60;
  guint64 hrs = min / 60;

  char buf[64];
  g_snprintf(buf, sizeof buf, "%02llu:%02llu:%02llu.%llu",
             (unsigned long long)hrs, (unsigned long long)min,
             (unsigned long long)sec, (unsigned long long)dec);
  gtk_label_set_text(GTK_LABEL(win->timer_label), buf);

  return G_SOURCE_CONTINUE;
}

static void on_start_stop_timer_clicked(GtkButton *btn, void *data) {
  MyWindow *win = data;

  if (win->decimal_timer_timout_id == 0) {
    win->timer_start_us = g_get_monotonic_time() - win->timer_elapsed_us;
    win->decimal_timer_timout_id =
        g_timeout_add(100, on_decimal_timer_timeout, win);
    gtk_button_set_label(btn, "Stop");
  } else {
    win->timer_elapsed_us = g_get_monotonic_time() - win->timer_start_us;
    g_source_remove(win->decimal_timer_timout_id);
    win->decimal_timer_timout_id = 0;
    gtk_button_set_label(btn, "Start");
  }
}

static void on_reset_clicked(GtkButton *btn, void *data) {
  (void)btn;
  MyWindow *win = data;

  win->timer_start_us = g_get_monotonic_time();
  win->timer_elapsed_us = 0;
  on_decimal_timer_timeout(win);
}

static void on_sidebar_row_selected(GtkListBox *box, GtkListBoxRow *row,
                                    MyWindow *win) {
  (void)box;
  (void)row;
  (void)win;
  g_debug("Row selected");
}

static void on_add_clip_clicked(GtkButton *btn, void *data) {
  (void)btn;
  MyWindow *win = data;

  gtk_list_box_append(GTK_LIST_BOX(win->sidebar_list_box),
                      GTK_WIDGET(clip_row_new()));
}

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

  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_start_stop_timer_clicked);
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_reset_clicked);
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_sidebar_row_selected);
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_add_clip_clicked);
}

static void my_window_init(MyWindow *self) {
  gtk_widget_init_template(GTK_WIDGET(self));
  ClipRow *initial_row = clip_row_new();
  gtk_list_box_append(GTK_LIST_BOX(self->sidebar_list_box),
                      GTK_WIDGET(initial_row));
  self->current_row = initial_row;
}

MyWindow *my_window_new(GtkApplication *app) {
  return g_object_new(MY_TYPE_WINDOW, "application", app, NULL);
}
