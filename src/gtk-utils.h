/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA.
 */

#ifndef GTK_UTILS_H
#define GTK_UTILS_H

#include <gio/gio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

GtkWidget *_gtk_message_dialog_new(GtkWindow *parent, GtkDialogFlags flags,
                                   const char *icon_name, const char *message,
                                   const char *secondary_message,
                                   const char *first_button_text, ...);
gchar *_gtk_request_dialog_run(GtkWindow *parent, GtkDialogFlags flags,
                               const char *title, const char *message,
                               const char *default_value, int max_length,
                               const char *no_button_text,
                               const char *yes_button_text);
GtkWidget *_gtk_error_dialog_new(GtkWindow *parent, GtkDialogFlags flags,
                                 GList *row_output, const char *primary_text,
                                 const char *secondary_text, ...)
    G_GNUC_PRINTF(5, 6);
void _gtk_error_dialog_run(GtkWindow *parent, const gchar *main_message,
                           const gchar *format, ...);
void _gtk_entry_set_locale_text(GtkEntry *entry, const char *text);
char *_gtk_entry_get_locale_text(GtkEntry *entry);
void _gtk_entry_set_filename_text(GtkEntry *entry, const char *text);
GdkPixbuf *get_icon_pixbuf(GIcon *icon, int size, GtkIconTheme *icon_theme);
GdkPixbuf *get_mime_type_pixbuf(const char *mime_type, int icon_size,
                                GtkIconTheme *icon_theme);
void show_help_dialog(GtkWindow *parent, const char *section);

int _gtk_widget_lookup_for_size(GtkWidget *widget, GtkIconSize icon_size);
#endif
