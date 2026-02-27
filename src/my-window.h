#pragma once
#include <gtk/gtk.h>

#define MY_TYPE_WINDOW (my_window_get_type())
G_DECLARE_FINAL_TYPE(MyWindow, my_window, MY, WINDOW, GtkApplicationWindow)

MyWindow *my_window_new(GtkApplication *app);
