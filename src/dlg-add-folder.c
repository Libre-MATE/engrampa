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

#include "dlg-add-folder.h"

#include <gio/gio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>

#include "file-utils.h"
#include "fr-window.h"
#include "gtk-utils.h"
#include "preferences.h"

#ifdef __GNUC__
#define UNUSED_VARIABLE __attribute__((unused))
#else
#define UNUSED_VARIABLE
#endif

#define GET_WIDGET(x) (GTK_WIDGET(gtk_builder_get_object(builder, (x))))

typedef struct {
  FrWindow *window;
  GSettings *settings;
  GtkWidget *dialog;
  GtkWidget *include_subfold_checkbutton;
  GtkWidget *exclude_symlinks;
  GtkWidget *add_if_newer_checkbutton;
  GtkWidget *include_files_entry;
  GtkWidget *exclude_files_entry;
  GtkWidget *exclude_folders_entry;
  char *last_options;
} DialogData;

static void open_file_destroy_cb(GtkWidget *widget, DialogData *data) {
  g_object_unref(data->settings);
  g_free(data->last_options);
  g_free(data);
}

static gboolean utf8_only_spaces(const char *text) {
  const char *scan;

  if (text == NULL) return TRUE;

  for (scan = text; *scan != 0; scan = g_utf8_next_char(scan)) {
    gunichar c = g_utf8_get_char(scan);
    if (!g_unichar_isspace(c)) return FALSE;
  }

  return TRUE;
}

static void dlg_add_folder_save_last_options(DialogData *data);

static int file_sel_response_cb(GtkWidget *widget, int response,
                                DialogData *data) {
  GtkFileChooser *file_sel = GTK_FILE_CHOOSER(widget);
  FrWindow *window = data->window;
  char *selected_folder;
  gboolean update, UNUSED_VARIABLE recursive, follow_links;
  const char *include_files;
  const char *exclude_files;
  const char *exclude_folders;
  char *dest_dir;
  char *local_filename;

  dlg_add_folder_save_last_options(data);

  if ((response == GTK_RESPONSE_CANCEL) ||
      (response == GTK_RESPONSE_DELETE_EVENT)) {
    gtk_widget_destroy(data->dialog);
    return TRUE;
  }

  if (response == GTK_RESPONSE_HELP) {
    show_help_dialog(GTK_WINDOW(data->dialog), "engrampa-add-options");
    return TRUE;
  }

  selected_folder = gtk_file_chooser_get_uri(file_sel);

  /* check folder permissions. */

  if (!check_permissions(selected_folder, R_OK)) {
    GtkWidget *d;
    char *utf8_path;

    utf8_path = g_filename_display_name(selected_folder);

    d = _gtk_error_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL, NULL,
                              _("Could not add the files to the archive"),
                              _("You don't have the right permissions to read "
                                "files from folder \"%s\""),
                              utf8_path);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(GTK_WIDGET(d));

    g_free(utf8_path);
    g_free(selected_folder);

    return FALSE;
  }

  update = gtk_toggle_button_get_active(
      GTK_TOGGLE_BUTTON(data->add_if_newer_checkbutton));
  recursive = gtk_toggle_button_get_active(
      GTK_TOGGLE_BUTTON(data->include_subfold_checkbutton));
  follow_links =
      !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->exclude_symlinks));

  include_files = gtk_entry_get_text(GTK_ENTRY(data->include_files_entry));
  if (utf8_only_spaces(include_files)) include_files = "*";

  exclude_files = gtk_entry_get_text(GTK_ENTRY(data->exclude_files_entry));
  if (utf8_only_spaces(exclude_files)) exclude_files = NULL;

  exclude_folders = gtk_entry_get_text(GTK_ENTRY(data->exclude_folders_entry));
  if (utf8_only_spaces(exclude_folders)) exclude_folders = NULL;

  local_filename = g_filename_from_uri(selected_folder, NULL, NULL);
  dest_dir = build_uri(fr_window_get_current_location(window),
                       file_name_from_path(local_filename), NULL);

  fr_window_archive_add_with_wildcard(window, include_files, exclude_files,
                                      exclude_folders, selected_folder,
                                      dest_dir, update, follow_links);

  g_free(local_filename);
  g_free(dest_dir);
  g_free(selected_folder);

  gtk_widget_destroy(data->dialog);

  return TRUE;
}

static int include_subfold_toggled_cb(GtkWidget *widget,
                                      gpointer callback_data) {
  DialogData *data = callback_data;

  gtk_widget_set_sensitive(
      data->exclude_symlinks,
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));

  return FALSE;
}

static void load_options_cb(GtkWidget *w, DialogData *data);
static void save_options_cb(GtkWidget *w, DialogData *data);
static void clear_options_cb(GtkWidget *w, DialogData *data);
static void dlg_add_folder_load_last_options(DialogData *data);

/* create the "add" dialog. */
void add_folder_cb(GtkWidget *widget, void *callback_data) {
  GtkBuilder *builder;
  DialogData *data;

  builder = gtk_builder_new_from_resource(
      ENGRAMPA_RESOURCE_UI_PATH G_DIR_SEPARATOR_S "dlg-add-folder.ui");

  data = g_new0(DialogData, 1);
  data->window = callback_data;
  data->settings = g_settings_new(ENGRAMPA_SCHEMA_ADD);
  data->dialog = GET_WIDGET("dialog_add_folder");
  data->add_if_newer_checkbutton = GET_WIDGET("add_if_newer_checkbutton");
  data->exclude_symlinks = GET_WIDGET("exclude_symlinks");
  data->include_subfold_checkbutton = GET_WIDGET("include_subfold_checkbutton");
  data->include_files_entry = GET_WIDGET("include_files_entry");
  data->exclude_files_entry = GET_WIDGET("exclude_files_entry");
  data->exclude_folders_entry = GET_WIDGET("exclude_folders_entry");

  /* set data */
  dlg_add_folder_load_last_options(data);

  /* signals */
  gtk_builder_add_callback_symbols(
      builder, "on_dialog_add_folder_destroy", G_CALLBACK(open_file_destroy_cb),
      "on_dialog_add_folder_response", G_CALLBACK(file_sel_response_cb),
      "on_include_subfold_checkbutton_toggled",
      G_CALLBACK(include_subfold_toggled_cb), "on_load_button_clicked",
      G_CALLBACK(load_options_cb), "on_save_button_clicked",
      G_CALLBACK(save_options_cb), "on_clear_button_clicked",
      G_CALLBACK(clear_options_cb), NULL);
  gtk_builder_connect_signals(builder, data);

  g_object_unref(builder);

  gtk_window_set_modal(GTK_WINDOW(data->dialog), TRUE);
  gtk_widget_show(data->dialog);
}

/* load/save the dialog options */

static void dlg_add_folder_save_last_used_options(DialogData *data,
                                                  const char *options_path) {
  g_free(data->last_options);
  data->last_options = g_strdup(file_name_from_path(options_path));
}

static void sync_widgets_with_options(DialogData *data, const char *base_dir,
                                      const char *filename,
                                      const char *include_files,
                                      const char *exclude_files,
                                      const char *exclude_folders,
                                      gboolean update, gboolean recursive,
                                      gboolean no_symlinks) {
  if ((base_dir == NULL) || (strcmp(base_dir, "") == 0))
    base_dir = fr_window_get_add_default_dir(data->window);

  if ((filename != NULL) && (strcmp(filename, base_dir) != 0))
    gtk_file_chooser_select_uri(GTK_FILE_CHOOSER(data->dialog), filename);
  else
    gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(data->dialog),
                                            base_dir);

  if (include_files != NULL)
    gtk_entry_set_text(GTK_ENTRY(data->include_files_entry), include_files);
  if (exclude_files != NULL)
    gtk_entry_set_text(GTK_ENTRY(data->exclude_files_entry), exclude_files);
  if (exclude_folders != NULL)
    gtk_entry_set_text(GTK_ENTRY(data->exclude_folders_entry), exclude_folders);
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(data->add_if_newer_checkbutton), update);
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(data->include_subfold_checkbutton), recursive);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->exclude_symlinks),
                               no_symlinks);
}

static void clear_options_cb(GtkWidget *w, DialogData *data) {
  sync_widgets_with_options(
      data,
      gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(data->dialog)),
      gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(data->dialog)), "", "", "",
      FALSE, TRUE, FALSE);
}

static gboolean dlg_add_folder_load_options(DialogData *data,
                                            const char *name) {
  GFile *options_dir;
  GFile *options_file;
  char *file_path;
  GKeyFile *key_file;
  GError *error = NULL;
  char *base_dir = NULL;
  char *filename = NULL;
  char *include_files = NULL;
  char *exclude_files = NULL;
  char *exclude_folders = NULL;
  gboolean update;
  gboolean recursive;
  gboolean no_symlinks;

  options_dir = get_user_config_subdirectory(ADD_FOLDER_OPTIONS_DIR, TRUE);
  options_file = g_file_get_child(options_dir, name);
  file_path = g_file_get_path(options_file);
  key_file = g_key_file_new();
  if (!g_key_file_load_from_file(key_file, file_path, G_KEY_FILE_KEEP_COMMENTS,
                                 &error)) {
    if (error->code != G_IO_ERROR_NOT_FOUND)
      g_warning("Could not load options file: %s\n", error->message);
    g_clear_error(&error);
    g_object_unref(options_file);
    g_object_unref(options_dir);
    g_key_file_free(key_file);
    return FALSE;
  }

  base_dir = g_key_file_get_string(key_file, "Options", "base_dir", NULL);
  filename = g_key_file_get_string(key_file, "Options", "filename", NULL);
  include_files =
      g_key_file_get_string(key_file, "Options", "include_files", NULL);
  exclude_files =
      g_key_file_get_string(key_file, "Options", "exclude_files", NULL);
  exclude_folders =
      g_key_file_get_string(key_file, "Options", "exclude_folders", NULL);
  update = g_key_file_get_boolean(key_file, "Options", "update", NULL);
  recursive = g_key_file_get_boolean(key_file, "Options", "recursive", NULL);
  no_symlinks =
      g_key_file_get_boolean(key_file, "Options", "no_symlinks", NULL);

  sync_widgets_with_options(data, base_dir, filename, include_files,
                            exclude_files, exclude_folders, update, recursive,
                            no_symlinks);

  dlg_add_folder_save_last_used_options(data, file_path);

  g_free(base_dir);
  g_free(filename);
  g_free(include_files);
  g_free(exclude_files);
  g_free(exclude_folders);
  g_key_file_free(key_file);
  g_free(file_path);
  g_object_unref(options_file);
  g_object_unref(options_dir);

  return TRUE;
}

static void dlg_add_folder_load_last_options(DialogData *data) {
  char *base_dir = NULL;
  char *filename = NULL;
  char *include_files = NULL;
  char *exclude_files = NULL;
  char *exclude_folders = NULL;
  gboolean update;
  gboolean recursive;
  gboolean no_symlinks;

  base_dir = g_settings_get_string(data->settings, PREF_ADD_CURRENT_FOLDER);
  filename = g_settings_get_string(data->settings, PREF_ADD_FILENAME);
  include_files = g_settings_get_string(data->settings, PREF_ADD_INCLUDE_FILES);
  exclude_files = g_settings_get_string(data->settings, PREF_ADD_EXCLUDE_FILES);
  exclude_folders =
      g_settings_get_string(data->settings, PREF_ADD_EXCLUDE_FOLDERS);
  update = g_settings_get_boolean(data->settings, PREF_ADD_UPDATE);
  recursive = g_settings_get_boolean(data->settings, PREF_ADD_RECURSIVE);
  no_symlinks = g_settings_get_boolean(data->settings, PREF_ADD_NO_SYMLINKS);

  sync_widgets_with_options(data, base_dir, filename, include_files,
                            exclude_files, exclude_folders, update, recursive,
                            no_symlinks);

  g_free(base_dir);
  g_free(filename);
  g_free(include_files);
  g_free(exclude_files);
  g_free(exclude_folders);
}

static void get_options_from_widgets(DialogData *data, char **base_dir,
                                     char **filename,
                                     const char **include_files,
                                     const char **exclude_files,
                                     const char **exclude_folders,
                                     gboolean *update, gboolean *recursive,
                                     gboolean *no_symlinks) {
  *base_dir =
      gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(data->dialog));
  *filename = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(data->dialog));
  *update = gtk_toggle_button_get_active(
      GTK_TOGGLE_BUTTON(data->add_if_newer_checkbutton));
  *recursive = gtk_toggle_button_get_active(
      GTK_TOGGLE_BUTTON(data->include_subfold_checkbutton));
  *no_symlinks =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->exclude_symlinks));

  *include_files = gtk_entry_get_text(GTK_ENTRY(data->include_files_entry));
  if (utf8_only_spaces(*include_files)) *include_files = "";

  *exclude_files = gtk_entry_get_text(GTK_ENTRY(data->exclude_files_entry));
  if (utf8_only_spaces(*exclude_files)) *exclude_files = "";

  *exclude_folders = gtk_entry_get_text(GTK_ENTRY(data->exclude_folders_entry));
  if (utf8_only_spaces(*exclude_folders)) *exclude_folders = "";
}

static void dlg_add_folder_save_current_options(DialogData *data,
                                                GFile *options_file) {
  char *base_dir;
  char *filename;
  const char *include_files;
  const char *exclude_files;
  const char *exclude_folders;
  gboolean update;
  gboolean recursive;
  gboolean no_symlinks;
  GKeyFile *key_file;

  get_options_from_widgets(data, &base_dir, &filename, &include_files,
                           &exclude_files, &exclude_folders, &update,
                           &recursive, &no_symlinks);

  fr_window_set_add_default_dir(data->window, base_dir);

  key_file = g_key_file_new();
  g_key_file_set_string(key_file, "Options", "base_dir", base_dir);
  g_key_file_set_string(key_file, "Options", "filename", filename);
  g_key_file_set_string(key_file, "Options", "include_files", include_files);
  g_key_file_set_string(key_file, "Options", "exclude_files", exclude_files);
  g_key_file_set_string(key_file, "Options", "exclude_folders",
                        exclude_folders);
  g_key_file_set_boolean(key_file, "Options", "update", update);
  g_key_file_set_boolean(key_file, "Options", "recursive", recursive);
  g_key_file_set_boolean(key_file, "Options", "no_symlinks", no_symlinks);

  g_key_file_save(key_file, options_file);

  g_key_file_free(key_file);
  g_free(base_dir);
  g_free(filename);
}

static void dlg_add_folder_save_last_options(DialogData *data) {
  char *base_dir;
  char *filename;
  const char *include_files;
  const char *exclude_files;
  const char *exclude_folders;
  gboolean update;
  gboolean recursive;
  gboolean no_symlinks;

  get_options_from_widgets(data, &base_dir, &filename, &include_files,
                           &exclude_files, &exclude_folders, &update,
                           &recursive, &no_symlinks);

  g_settings_set_string(data->settings, PREF_ADD_CURRENT_FOLDER, base_dir);
  g_settings_set_string(data->settings, PREF_ADD_FILENAME, filename);
  g_settings_set_string(data->settings, PREF_ADD_INCLUDE_FILES, include_files);
  g_settings_set_string(data->settings, PREF_ADD_EXCLUDE_FILES, exclude_files);
  g_settings_set_string(data->settings, PREF_ADD_EXCLUDE_FOLDERS,
                        exclude_folders);
  g_settings_set_boolean(data->settings, PREF_ADD_UPDATE, update);
  g_settings_set_boolean(data->settings, PREF_ADD_RECURSIVE, recursive);
  g_settings_set_boolean(data->settings, PREF_ADD_NO_SYMLINKS, no_symlinks);

  g_free(base_dir);
  g_free(filename);
}

typedef struct {
  DialogData *data;
  GtkWidget *dialog;
  GtkWidget *aod_treeview;
  GtkTreeModel *aod_model;
} LoadOptionsDialogData;

static void aod_destroy_cb(GtkWidget *widget, LoadOptionsDialogData *aod_data) {
  g_free(aod_data);
}

static void aod_apply_cb(GtkWidget *widget, gpointer callback_data) {
  LoadOptionsDialogData *aod_data = callback_data;
  DialogData *data = aod_data->data;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  char *options_name;

  selection =
      gtk_tree_view_get_selection(GTK_TREE_VIEW(aod_data->aod_treeview));
  if (!gtk_tree_selection_get_selected(selection, NULL, &iter)) return;

  gtk_tree_model_get(aod_data->aod_model, &iter, 1, &options_name, -1);

  dlg_add_folder_load_options(data, options_name);
  g_free(options_name);

  gtk_widget_destroy(aod_data->dialog);
}

static void aod_activated_cb(GtkTreeView *tree_view, GtkTreePath *path,
                             GtkTreeViewColumn *column,
                             gpointer callback_data) {
  aod_apply_cb(NULL, callback_data);
}

static void aod_update_option_list(LoadOptionsDialogData *aod_data) {
  GtkListStore *list_store = GTK_LIST_STORE(aod_data->aod_model);
  GFile *options_dir;
  GFileEnumerator *file_enum;
  GFileInfo *info;
  GError *err = NULL;

  gtk_list_store_clear(list_store);

  options_dir = get_user_config_subdirectory(ADD_FOLDER_OPTIONS_DIR, TRUE);
  make_directory_tree(options_dir, 0700, NULL);

  file_enum = g_file_enumerate_children(
      options_dir, G_FILE_ATTRIBUTE_STANDARD_NAME, 0, NULL, &err);
  if (err != NULL) {
    g_warning("Failed to enumerate children: %s", err->message);
    g_clear_error(&err);
    g_object_unref(options_dir);
    return;
  }

  while ((info = g_file_enumerator_next_file(file_enum, NULL, &err)) != NULL) {
    const char *name;
    char *display_name;
    GtkTreeIter iter;

    if (err != NULL) {
      g_warning("Failed to get info while enumerating: %s", err->message);
      g_clear_error(&err);
      continue;
    }

    name = g_file_info_get_name(info);
    display_name = g_filename_display_name(name);

    gtk_list_store_append(GTK_LIST_STORE(aod_data->aod_model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(aod_data->aod_model), &iter, 0, name, 1,
                       display_name, -1);

    g_free(display_name);
    g_object_unref(info);
  }

  if (err != NULL) {
    g_warning("Failed to get info after enumeration: %s", err->message);
    g_clear_error(&err);
  }

  g_object_unref(options_dir);
}

static void aod_remove_cb(GtkWidget *widget, LoadOptionsDialogData *aod_data) {
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  char *filename;
  GFile *options_dir;
  GFile *options_file;
  GError *error = NULL;

  selection =
      gtk_tree_view_get_selection(GTK_TREE_VIEW(aod_data->aod_treeview));
  if (!gtk_tree_selection_get_selected(selection, NULL, &iter)) return;

  gtk_tree_model_get(aod_data->aod_model, &iter, 1, &filename, -1);
  gtk_list_store_remove(GTK_LIST_STORE(aod_data->aod_model), &iter);

  options_dir = get_user_config_subdirectory(ADD_FOLDER_OPTIONS_DIR, TRUE);
  options_file = g_file_get_child(options_dir, filename);
  if (!g_file_delete(options_file, NULL, &error)) {
    g_warning("could not delete the options: %s", error->message);
    g_clear_error(&error);
  }

  g_object_unref(options_file);
  g_object_unref(options_dir);
  g_free(filename);
}

static void load_options_cb(GtkWidget *w, DialogData *data) {
  LoadOptionsDialogData *aod_data;
  GtkBuilder *builder;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  aod_data = g_new0(LoadOptionsDialogData, 1);

  aod_data->data = data;
  builder = gtk_builder_new_from_resource(
      ENGRAMPA_RESOURCE_UI_PATH G_DIR_SEPARATOR_S "add-options.ui");

  /* Get the widgets. */

  aod_data->dialog = GET_WIDGET("add_options_dialog");
  aod_data->aod_treeview = GET_WIDGET("aod_treeview");

  /* Set the signals handlers. */
  gtk_builder_add_callback_symbols(
      builder, "on_add_options_dialog_destroy", G_CALLBACK(aod_destroy_cb),
      "on_aod_treeview_row_activated", G_CALLBACK(aod_activated_cb),
      "on_aod_ok_button_clicked", G_CALLBACK(aod_apply_cb),
      "on_aod_cancel_button_clicked", G_CALLBACK(aod_remove_cb), NULL);
  gtk_builder_connect_signals(builder, aod_data);

  g_signal_connect_swapped(gtk_builder_get_object(builder, "aod_cancelbutton"),
                           "clicked", G_CALLBACK(gtk_widget_destroy),
                           G_OBJECT(aod_data->dialog));

  g_object_unref(builder);

  /* Set data. */

  aod_data->aod_model =
      GTK_TREE_MODEL(gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING));
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(aod_data->aod_model),
                                       0, GTK_SORT_ASCENDING);
  gtk_tree_view_set_model(GTK_TREE_VIEW(aod_data->aod_treeview),
                          aod_data->aod_model);
  g_object_unref(aod_data->aod_model);

  /**/

  renderer = gtk_cell_renderer_text_new();
  column =
      gtk_tree_view_column_new_with_attributes(NULL, renderer, "text", 0, NULL);
  gtk_tree_view_column_set_sort_column_id(column, 0);
  gtk_tree_view_append_column(GTK_TREE_VIEW(aod_data->aod_treeview), column);

  aod_update_option_list(aod_data);

  /* Run */

  gtk_window_set_transient_for(GTK_WINDOW(aod_data->dialog),
                               GTK_WINDOW(data->dialog));
  gtk_window_set_modal(GTK_WINDOW(aod_data->dialog), TRUE);
  gtk_widget_show(aod_data->dialog);
}

static void save_options_cb(GtkWidget *w, DialogData *data) {
  GFile *options_dir;
  GFile *options_file;
  char *opt_filename;

  options_dir = get_user_config_subdirectory(ADD_FOLDER_OPTIONS_DIR, TRUE);
  make_directory_tree(options_dir, 0700, NULL);

  opt_filename = _gtk_request_dialog_run(
      GTK_WINDOW(data->dialog), GTK_DIALOG_MODAL, _("Save Options"),
      _("_Options Name:"),
      (data->last_options != NULL) ? data->last_options : "", 1024,
      _("_Cancel"), _("_Save"));
  if (opt_filename == NULL) return;

  options_file =
      g_file_get_child_for_display_name(options_dir, opt_filename, NULL);
  dlg_add_folder_save_current_options(data, options_file);
  dlg_add_folder_save_last_used_options(data, opt_filename);

  g_free(opt_filename);
  g_object_unref(options_file);
  g_object_unref(options_dir);
}
