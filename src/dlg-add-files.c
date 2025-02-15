/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001, 2003, 2004 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dlg-add-files.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>

#include "file-utils.h"
#include "fr-window.h"
#include "gtk-utils.h"
#include "preferences.h"

#define GET_WIDGET(x) (GTK_WIDGET(gtk_builder_get_object(builder, (x))))

typedef struct {
  FrWindow *window;
  GSettings *settings;
  GtkWidget *dialog;
  GtkWidget *add_if_newer_checkbutton;
} DialogData;

static void open_file_destroy_cb(GtkWidget *file_sel, DialogData *data) {
  g_object_unref(data->settings);
  g_free(data);
}

static int file_sel_response_cb(GtkWidget *widget, int response,
                                DialogData *data) {
  GtkFileChooser *file_sel = GTK_FILE_CHOOSER(widget);
  FrWindow *window = data->window;
  char *current_folder;
  char *uri;
  gboolean update;
  GSList *selections, *iter;
  GList *item_list = NULL;

  current_folder = gtk_file_chooser_get_current_folder_uri(file_sel);
  uri = gtk_file_chooser_get_uri(file_sel);

  if (current_folder != NULL) {
    g_settings_set_string(data->settings, PREF_ADD_CURRENT_FOLDER,
                          current_folder);
    fr_window_set_add_default_dir(window, current_folder);
  }

  if (uri != NULL) {
    g_settings_set_string(data->settings, PREF_ADD_FILENAME, uri);
    g_free(uri);
  }

  if ((response == GTK_RESPONSE_CANCEL) ||
      (response == GTK_RESPONSE_DELETE_EVENT)) {
    gtk_widget_destroy(data->dialog);
    g_free(current_folder);
    return TRUE;
  }

  if (response == GTK_RESPONSE_HELP) {
    show_help_dialog(GTK_WINDOW(data->dialog), "engrampa-add-options");
    g_free(current_folder);
    return TRUE;
  }

  /* check folder permissions. */

  if (uri_is_dir(current_folder) && !check_permissions(current_folder, R_OK)) {
    GtkWidget *d;
    char *utf8_path;

    utf8_path = g_filename_display_name(current_folder);

    d = _gtk_error_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL, NULL,
                              _("Could not add the files to the archive"),
                              _("You don't have the right permissions to read "
                                "files from folder \"%s\""),
                              utf8_path);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(GTK_WIDGET(d));

    g_free(utf8_path);
    g_free(current_folder);
    return FALSE;
  }

  update = gtk_toggle_button_get_active(
      GTK_TOGGLE_BUTTON(data->add_if_newer_checkbutton));

  /**/

  selections = gtk_file_chooser_get_uris(file_sel);
  for (iter = selections; iter != NULL; iter = iter->next)
    item_list = g_list_prepend(item_list, g_file_new_for_uri(iter->data));

  if (item_list != NULL) fr_window_archive_add_files(window, item_list, update);

  gio_file_list_free(item_list);
  g_slist_free_full(selections, g_free);
  g_free(current_folder);

  gtk_widget_destroy(data->dialog);

  return TRUE;
}

/* create the "add" dialog. */
void add_files_cb(GtkWidget *widget, void *callback_data) {
  GtkBuilder *builder;
  DialogData *data;
  char *folder;

  builder = gtk_builder_new_from_resource(
      ENGRAMPA_RESOURCE_UI_PATH G_DIR_SEPARATOR_S "dlg-add-files.ui");

  data = g_new0(DialogData, 1);
  data->window = callback_data;
  data->settings = g_settings_new(ENGRAMPA_SCHEMA_ADD);
  data->dialog = GET_WIDGET("dialog_add_files");
  data->add_if_newer_checkbutton = GET_WIDGET("add_if_newer_checkbutton");

  /* set data */
  folder = g_settings_get_string(data->settings, PREF_ADD_CURRENT_FOLDER);
  if ((folder == NULL) || (strcmp(folder, "") == 0))
    folder = g_strdup(fr_window_get_add_default_dir(data->window));
  gtk_file_chooser_set_current_folder_uri(
      GTK_FILE_CHOOSER(GTK_WIDGET(data->dialog)), folder);
  g_free(folder);

  /* signals */
  gtk_builder_add_callback_symbols(
      builder, "on_dlg_add_files_destroy", G_CALLBACK(open_file_destroy_cb),
      "on_dlg_add_files_response", G_CALLBACK(file_sel_response_cb), NULL);
  gtk_builder_connect_signals(builder, data);

  g_object_unref(builder);

  gtk_window_set_modal(GTK_WINDOW(data->dialog), TRUE);
  gtk_widget_show(data->dialog);
}
