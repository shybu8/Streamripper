#include "ffmpeg.h"
#include "stdio.h"

char *get_extension(GFile *file) {
  char *basename = g_file_get_basename(file);
  if (!basename)
    return NULL;

  char *dot = strrchr(basename, '.');

  if (!dot || dot == basename || dot[1] == '\0') {
    g_free(basename);
    return NULL;
  }

  char *ext = g_strdup(dot + 1);
  g_free(basename);
  return ext; // caller must g_free()
}

char *alloc_as_str(guint64 target) {
  return g_strdup_printf("%" G_GUINT64_FORMAT, target);
}

void ffmpeg_render_finish(GObject *source_obj, GAsyncResult *res, void *data) {
  MyWindow *win = data;
  GSubprocess *proc = G_SUBPROCESS(source_obj);
  GError *error = NULL;

  GtkAlertDialog *alert;

  if (!g_subprocess_wait_check_finish(proc, res, &error)) {
    g_critical("ffmpeg failed: %s", error->message);
    alert = gtk_alert_dialog_new("Error rendering: %s", error->message);
    gtk_alert_dialog_show(alert, GTK_WINDOW(win));
    g_clear_error(&error);
    g_object_unref(alert);
    return;
  }

  alert = gtk_alert_dialog_new("Rendered successfully");
  gtk_alert_dialog_show(alert, GTK_WINDOW(win));
  g_object_unref(alert);

  g_info("ffmpeg finished successfully");
}

void ffmpeg_render(GFile *file, guint64 start_ts, guint64 end_ts,
                   GFile *output_dir, const char *output_name, MyWindow *win) {
  char *path = g_file_get_path(file);
  char *output_ext = get_extension(file);
  output_ext = output_ext ? output_ext : g_strdup("mkv");
  char *output_dir_str = g_file_get_path(output_dir);
  char *output_path =
      g_strdup_printf("%s/%s.%s", output_dir_str, output_name, output_ext);

  char *start_ts_str = alloc_as_str(start_ts);
  char *end_ts_str = alloc_as_str(end_ts);

  const char *argv[] = {"ffmpeg",     "-i",        path,       "-ss",
                        start_ts_str, "-to",       end_ts_str, "-c",
                        "copy",       output_path, NULL};

  GSubprocess *proc = g_subprocess_newv(argv,
                                        G_SUBPROCESS_FLAGS_STDOUT_SILENCE |
                                            G_SUBPROCESS_FLAGS_STDERR_SILENCE,
                                        NULL);

  if (!proc) {
    g_critical("Unable to spawn ffmpeg subprocess");
    return;
  }

  g_subprocess_wait_check_async(proc, NULL, ffmpeg_render_finish, win);
  g_object_unref(proc);

  free(start_ts_str);
  free(end_ts_str);
  free(output_path);
  g_free(output_dir_str);
  g_free(output_ext);
  g_free(path);
}
