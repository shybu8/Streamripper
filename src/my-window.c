#include "my-window.h"
#include "clip-row.h"
#include "ffmpeg.h"
#include "gio/gio.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "save_load.h"

#define MEDIA_TICK_MILLISEC 100

typedef enum {
  MEDIA_STATE_IDLE = 0,
  MEDIA_STATE_WAIT_PREPARED,
  MEDIA_STATE_WAIT_SEEK,
  MEDIA_STATE_PLAYING
} MediaState;

typedef enum {
  RANGE_KIND_FULL = 0,
  RANGE_KIND_START,
  RANGE_KIND_END
} RangeKind;

struct _MyWindow {
  GtkApplicationWindow parent_instance;
  // Upper bar
  // GtkLabel *label;
  // GtkButton *source_btn;
  // GtkButton *add_clip_btn;
  // GtkButton *output_btn;
  GtkLabel *file_label;
  // GtkMenuButton *settings_mbtn;
  // Center paned
  GtkListBox *sidebar_list_box;
  GtkSpinButton *start_ts_spin_btn;
  GtkSpinButton *end_ts_spin_btn;
  // GtkButton *start_ts_now_btn;
  // GtkButton *end_ts_now_btn;
  GtkButton *total_label;
  // GtkButton *preview_start_btn;
  // GtkButton *preview_end_btn;
  GtkSpinButton *preview_start_spin_btn;
  GtkSpinButton *preview_end_spin_btn;
  // GtkSpinButton *preview_full_btn;
  // GtkSpinButton *render_btn;
  GtkVideo *video;
  // Bottom bar
  GtkButton *reset_btn;
  GtkButton *start_stop_btn;
  GtkLabel *timer_label;

  // Service
  gint64 timer_start_us;
  gint64 timer_elapsed_us;
  guint decimal_timer_timout_id;
  ClipRow *current_row;
  GFile *source_file;
  GFile *output_dir;
  GPtrArray *clip_page_records;
  MediaState media_state;
  guint media_tick_id;
  GtkMediaStream *media_stream;
  RangeKind range_kind;
};

G_DEFINE_FINAL_TYPE(MyWindow, my_window, GTK_TYPE_APPLICATION_WINDOW)

static void init_clip_page_record(ClipPageRecord *cpr) {
  cpr->start_ts = 0;
  cpr->end_ts = 0;
}

static void save_clip_page_record(MyWindow *win, ClipPageRecord *cpr) {
  cpr->start_ts = gtk_spin_button_get_value_as_int(win->start_ts_spin_btn);
  cpr->end_ts = gtk_spin_button_get_value_as_int(win->end_ts_spin_btn);
}

static void save_current_clip_page_record(MyWindow *win) {
  int idx = gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(win->current_row));
  ClipPageRecord *cpr = g_ptr_array_index(win->clip_page_records, idx);
  save_clip_page_record(win, cpr);
}

static void load_clip_page_record(MyWindow *win, ClipPageRecord *cpr) {
  gtk_spin_button_set_value(win->start_ts_spin_btn, (double)cpr->start_ts);
  gtk_spin_button_set_value(win->end_ts_spin_btn, (double)cpr->end_ts);
}

static void recalc_total(MyWindow *win) {
  gint64 start = gtk_spin_button_get_value_as_int(win->start_ts_spin_btn);
  gint64 end = gtk_spin_button_get_value_as_int(win->end_ts_spin_btn);

  char buf[64];
  guint64 hours = 0, mins = 0, secs = 0;
  gint64 secs_total = end - start;
  if (secs_total > 0) {
    secs = secs_total % 60;
    guint64 mins_total = secs_total / 60;
    mins = mins_total % 60;
    hours = mins_total / 60;
  }
  g_snprintf(buf, sizeof(buf), "Total: %01llu:%01llu:%01llu",
             (unsigned long long)hours, (unsigned long long)mins,
             (unsigned long long)secs);
  gtk_label_set_text(GTK_LABEL(win->total_label), buf);
}

static void media_load_stream(MyWindow *win) {
  if (win->media_stream)
    g_object_unref(win->media_stream);
  gtk_video_set_autoplay(win->video, FALSE);
  gtk_video_set_loop(win->video, FALSE);
  win->media_stream = gtk_media_file_new_for_file(win->source_file);
  gtk_video_set_media_stream(win->video, GTK_MEDIA_STREAM(win->media_stream));
}

static gboolean media_tick(void *data) {
  MyWindow *win = data;
  switch (win->media_state) {
  case MEDIA_STATE_WAIT_PREPARED:
    if (!gtk_media_stream_is_prepared(win->media_stream))
      return G_SOURCE_CONTINUE;
    gtk_media_stream_pause(win->media_stream);
    gint64 start_ts_u;
    switch (win->range_kind) {
    case RANGE_KIND_FULL:
    case RANGE_KIND_START:
      start_ts_u =
          gtk_spin_button_get_value_as_int(win->start_ts_spin_btn) * 1000000;
      break;
    case RANGE_KIND_END:
      start_ts_u =
          MAX((gtk_spin_button_get_value_as_int(win->end_ts_spin_btn) -
               gtk_spin_button_get_value_as_int(win->preview_end_spin_btn)),
              gtk_spin_button_get_value_as_int(win->start_ts_spin_btn)) *
          1000000;
      break;
    }
    gtk_media_stream_seek(win->media_stream, start_ts_u);
    win->media_state = MEDIA_STATE_WAIT_SEEK;
    return G_SOURCE_CONTINUE;
  case MEDIA_STATE_WAIT_SEEK:
    if (!gtk_media_stream_is_seeking(win->media_stream)) {
      gtk_media_stream_play(win->media_stream);
      win->media_state = MEDIA_STATE_PLAYING;
    }
    return G_SOURCE_CONTINUE;
  case MEDIA_STATE_PLAYING: {
    gint64 ts = gtk_media_stream_get_timestamp(win->media_stream);
    gint64 stop_ts;
    switch (win->range_kind) {
    case RANGE_KIND_FULL:
    case RANGE_KIND_END:
      stop_ts =
          gtk_spin_button_get_value_as_int(win->end_ts_spin_btn) * 1000000;
      break;
    case RANGE_KIND_START:
      stop_ts =
          MIN((gtk_spin_button_get_value_as_int(win->start_ts_spin_btn) +
               gtk_spin_button_get_value_as_int(win->preview_start_spin_btn)),
              gtk_spin_button_get_value_as_int(win->end_ts_spin_btn)) *
          1000000;
    }
    if (gtk_media_stream_get_ended(win->media_stream) || ts >= stop_ts) {
      gtk_media_stream_pause(win->media_stream);
      win->media_tick_id = 0;
      win->media_state = MEDIA_STATE_IDLE;
      return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
  }
  case MEDIA_STATE_IDLE:
  default:
    win->media_tick_id = 0;
    return G_SOURCE_REMOVE;
  }
}

static void media_play_range(MyWindow *win, RangeKind range_kind) {
  media_load_stream(win);
  win->range_kind = range_kind;
  win->media_state = MEDIA_STATE_WAIT_PREPARED;
  win->media_tick_id = g_timeout_add(MEDIA_TICK_MILLISEC, media_tick, win);
}

static bool is_clip_tss_valid(MyWindow *win) {
  return gtk_spin_button_get_value_as_int(win->start_ts_spin_btn) <
         gtk_spin_button_get_value_as_int(win->end_ts_spin_btn);
}

static void on_preview_start_clicked(GtkButton *btn, void *data) {
  (void)btn;
  MyWindow *win = data;
  if (is_clip_tss_valid(win))
    media_play_range(win, RANGE_KIND_START);
}

static void on_preview_end_clicked(GtkButton *btn, void *data) {
  (void)btn;
  MyWindow *win = data;
  if (is_clip_tss_valid(win))
    media_play_range(win, RANGE_KIND_END);
}

static void on_preview_full_clicked(GtkButton *btn, void *data) {
  (void)btn;
  MyWindow *win = data;
  if (is_clip_tss_valid(win))
    media_play_range(win, RANGE_KIND_FULL);
}

static void on_spin_button_change(GtkSpinButton *btn, void *data) {
  (void)btn;
  recalc_total(data);
}

static void on_start_ts_preview_now_clicked(GtkButton *btn, void *data) {
  (void)btn;
  MyWindow *win = data;
  if (!win->media_stream)
    return;
  gint64 ts = gtk_media_stream_get_timestamp(win->media_stream);
  gtk_spin_button_set_value(win->start_ts_spin_btn, (double)(ts / 1000000));
}

static void on_end_ts_preview_now_clicked(GtkButton *btn, void *data) {
  (void)btn;
  MyWindow *win = data;
  if (!win->media_stream)
    return;
  gint64 ts = gtk_media_stream_get_timestamp(win->media_stream);
  gtk_spin_button_set_value(win->end_ts_spin_btn, (double)(ts / 1000000));
}

static void on_render_clicked(GtkButton *btn, void *data) {
  (void)btn;
  MyWindow *win = data;
  const char *output_name = clip_row_get_label_text(win->current_row);
  guint64 start_ts = gtk_spin_button_get_value_as_int(win->start_ts_spin_btn);
  guint64 end_ts = gtk_spin_button_get_value_as_int(win->end_ts_spin_btn);
  if (win->source_file)
    ffmpeg_render(win->source_file, start_ts, end_ts, win->output_dir,
                  output_name, win);
}

static gboolean on_decimal_timer_timeout(void *data) {
  MyWindow *win = data;
  gint64 now_us = g_get_monotonic_time();
  gint64 us = now_us - win->timer_start_us;

  // tenths of a second
  guint64 t = (guint64)(us / 100000); // 0.1s = 100,000us
  guint64 total_sec = t / 10;
  guint64 dec = t % 10;
  guint64 sec = total_sec % 60;
  guint64 total_min = total_sec / 60;
  guint64 min = total_min % 60;
  guint64 hrs = min / 60;

  char buf[64];
  g_snprintf(buf, sizeof(buf), "%02llu:%02llu:%02llu.%llu",
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

  if (win->current_row == CLIP_ROW(row))
    return;

  // Save current row
  save_current_clip_page_record(win);

  // Load selected row
  int idx = gtk_list_box_row_get_index(row);
  ClipPageRecord *cpr = g_ptr_array_index(win->clip_page_records, idx);
  load_clip_page_record(win, cpr);

  win->current_row = CLIP_ROW(row);
}

static void add_clip(MyWindow *win) {
  gtk_list_box_append(GTK_LIST_BOX(win->sidebar_list_box),
                      GTK_WIDGET(clip_row_new()));
  ClipPageRecord *cpr = g_new(ClipPageRecord, 1);
  init_clip_page_record(cpr);
  g_ptr_array_add(win->clip_page_records, cpr);
}

static void on_add_clip_clicked(GtkButton *btn, void *data) {
  (void)btn;
  MyWindow *win = data;
  add_clip(win);
}

static guint64 secs_on_timer(MyWindow *win) {
  guint64 us = win->decimal_timer_timout_id != 0
                   ? g_get_monotonic_time() - (guint64)win->timer_start_us
                   : (guint64)win->timer_elapsed_us;
  return us / 1000000;
}

static void on_start_ts_now_clicked(GtkButton *btn, void *data) {
  (void)btn;
  MyWindow *win = data;
  gtk_spin_button_set_value(win->start_ts_spin_btn, (double)secs_on_timer(win));
}

static void on_end_ts_now_clicked(GtkButton *btn, void *data) {
  (void)btn;
  MyWindow *win = data;
  gtk_spin_button_set_value(win->end_ts_spin_btn, (double)secs_on_timer(win));
}

char *get_display_name(GFile *file) {
  const char *unknown_filename = "Unknown filename";
  GFileInfo *info =
      g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                        G_FILE_QUERY_INFO_NONE, NULL, NULL);
  if (!info)
    return g_strdup(unknown_filename);

  const char *dn = g_file_info_get_display_name(info);
  char *res = dn ? g_strdup(dn) : g_strdup(unknown_filename);
  g_object_unref(info);
  return res; // Caller frees
}

static void on_source_open_finish(GObject *dlg, GAsyncResult *res, void *data) {
  MyWindow *win = data;
  GFile *file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(dlg), res, NULL);

  if (!file) {
    g_object_unref(win);
    return;
  }

  if (win->source_file)
    g_object_unref(win->source_file);
  win->source_file = file;

  char *dn = get_display_name(file);
  gtk_label_set_text(GTK_LABEL(win->file_label), dn);
  g_free(dn);

  g_object_unref(win);
}

static void on_source_clicked(GtkButton *btn, void *data) {
  (void)btn;
  GtkFileDialog *dlg = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dlg, "Open file");
  gtk_file_dialog_set_modal(dlg, TRUE);

  g_object_ref(data);
  gtk_file_dialog_open(dlg, GTK_WINDOW(data), NULL, on_source_open_finish,
                       data);

  g_object_unref(dlg);
}

static void on_output_select_folder_finish(GObject *dlg, GAsyncResult *res,
                                           void *data) {
  MyWindow *win = data;
  GFile *dir =
      gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(dlg), res, NULL);

  if (!dir) {
    g_object_unref(win);
    return;
  }

  if (win->output_dir)
    g_object_unref(win->output_dir);
  win->output_dir = dir;
  g_object_unref(win);
}

static void on_output_clicked(GtkButton *btn, void *data) {
  (void)btn;
  GtkFileDialog *dlg = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dlg, "Choose output directory");
  gtk_file_dialog_set_modal(dlg, TRUE);

  g_object_ref(data);
  gtk_file_dialog_select_folder(dlg, GTK_WINDOW(data), NULL,
                                on_output_select_folder_finish, data);
  g_object_unref(dlg);
}

static ClipData *get_clip_data(MyWindow *win, size_t *len) {
  save_current_clip_page_record(win);
  size_t i = 0;
  ClipData *res = NULL; // = malloc(sizeof(ClipData));
  GtkListBoxRow *row;

  while ((row = gtk_list_box_get_row_at_index(win->sidebar_list_box, i))) {
    res = realloc(res, sizeof(ClipData) * (i + 1));
    res[i] = (ClipData){
        .name = clip_row_get_label_text(CLIP_ROW(row)),
        .info = g_ptr_array_index(win->clip_page_records, i),
    };
    i++;
  }
  *len = i;
  return res;
}

static void on_save_select_file_finish(GObject *dlg, GAsyncResult *res,
                                       void *data) {
  MyWindow *win = data;
  GFile *file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(dlg), res, NULL);

  if (!file) {
    g_object_unref(win);
    return;
  }

  size_t data_len;
  ClipData *clip_data = get_clip_data(win, &data_len);
  save_into_file(clip_data, data_len, file);
  g_object_unref(file);
  g_object_unref(win);
}

static void on_save_clicked(GtkButton *btn, void *data) {
  (void)btn;
  MyWindow *win = data;

  // size_t data_len;
  // ClipData *clip_data = get_clip_data(win, &data_len);
  // serialize_clip_page_records(clip_data, data_len);
  // free(clip_data);

  GtkFileDialog *dlg = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dlg, "Choose file to save into");
  gtk_file_dialog_set_modal(dlg, TRUE);

  g_object_ref(data);
  gtk_file_dialog_save(dlg, GTK_WINDOW(data), NULL, on_save_select_file_finish,
                       win);
  g_object_unref(dlg);
}

static void my_window_dispose(GObject *obj) {
  MyWindow *self = MY_WINDOW(obj);

  if (self->decimal_timer_timout_id != 0) {
    g_source_remove(self->decimal_timer_timout_id);
    self->decimal_timer_timout_id = 0;
  }

  if (self->sidebar_list_box)
    g_signal_handlers_disconnect_by_data(self->sidebar_list_box, self);

  if (self->media_stream)
    g_object_unref(self->media_stream);

  self->current_row = NULL;

  g_clear_object(&self->source_file);
  g_clear_pointer(&self->clip_page_records, g_ptr_array_unref);

  gtk_widget_dispose_template(GTK_WIDGET(self), MY_TYPE_WINDOW);

  G_OBJECT_CLASS(my_window_parent_class)->dispose(obj);
}

static void my_window_class_init(MyWindowClass *klass) {
  GObjectClass *oc = G_OBJECT_CLASS(klass);
  oc->dispose = my_window_dispose;

  GtkWidgetClass *wc = GTK_WIDGET_CLASS(klass);
  gtk_widget_class_set_template_from_resource(
      wc, "/org/shybu8/streamripper/my-window.ui");
  // gtk_widget_class_bind_template_child(wc, MyWindow, source_btn);
  // gtk_widget_class_bind_template_child(wc, MyWindow, add_clip_btn);
  // gtk_widget_class_bind_template_child(wc, MyWindow, output_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, file_label);
  // gtk_widget_class_bind_template_child(wc, MyWindow, settings_mbtn);
  gtk_widget_class_bind_template_child(wc, MyWindow, sidebar_list_box);
  gtk_widget_class_bind_template_child(wc, MyWindow, start_ts_spin_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, end_ts_spin_btn);
  // gtk_widget_class_bind_template_child(wc, MyWindow, start_ts_now_btn);
  // gtk_widget_class_bind_template_child(wc, MyWindow, end_ts_now_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, total_label);
  // gtk_widget_class_bind_template_child(wc, MyWindow, preview_start_btn);
  // gtk_widget_class_bind_template_child(wc, MyWindow, preview_end_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, preview_start_spin_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, preview_end_spin_btn);
  // gtk_widget_class_bind_template_child(wc, MyWindow, preview_full_btn);
  // gtk_widget_class_bind_template_child(wc, MyWindow, render_btn);
  gtk_widget_class_bind_template_child(wc, MyWindow, video);
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
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_start_ts_now_clicked);
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_end_ts_now_clicked);
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_source_clicked);
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_output_clicked);
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_preview_start_clicked);
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_preview_end_clicked);
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_preview_full_clicked);
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_start_ts_preview_now_clicked);
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_end_ts_preview_now_clicked);
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_render_clicked);
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          on_save_clicked);
}

static void my_window_init(MyWindow *self) {
  gtk_widget_init_template(GTK_WIDGET(self));
  self->clip_page_records = g_ptr_array_new_with_free_func(g_free);
  add_clip(self);
  self->current_row =
      CLIP_ROW(gtk_list_box_get_row_at_index(self->sidebar_list_box, 0));
  g_signal_connect(self->start_ts_spin_btn, "value-changed",
                   G_CALLBACK(on_spin_button_change), self);
  g_signal_connect(self->end_ts_spin_btn, "value-changed",
                   G_CALLBACK(on_spin_button_change), self);
}

MyWindow *my_window_new(GtkApplication *app) {
  return g_object_new(MY_TYPE_WINDOW, "application", app, NULL);
}
