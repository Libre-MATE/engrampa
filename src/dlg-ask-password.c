/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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

#include "dlg-ask-password.h"

#include <gtk/gtk.h>
#include <string.h>

#include "file-utils.h"
#include "fr-window.h"
#include "glib-utils.h"
#include "gtk-utils.h"

#define GET_LABEL(x) (GTK_LABEL(gtk_builder_get_object(builder, (x))))
#define GET_WIDGET(x) (GTK_WIDGET(gtk_builder_get_object(builder, (x))))

typedef enum {
  FR_PASSWORD_TYPE_MAIN,
  FR_PASSWORD_TYPE_PASTE_FROM
} FrPasswordType;

typedef struct {
  FrWindow *window;
  FrPasswordType pwd_type;
  GtkWidget *dialog;
  GtkWidget *pw_password_entry;
} DialogData;

/* called when the main dialog is closed. */
static void destroy_cb(GtkWidget *widget, DialogData *data) { g_free(data); }

static void ask_password__response_cb(GtkWidget *dialog, int response_id,
                                      DialogData *data) {
  char *password;

  switch (response_id) {
    case GTK_RESPONSE_OK:
      password = _gtk_entry_get_locale_text(GTK_ENTRY(data->pw_password_entry));
      if (data->pwd_type == FR_PASSWORD_TYPE_MAIN)
        fr_window_set_password(data->window, password);
      else if (data->pwd_type == FR_PASSWORD_TYPE_PASTE_FROM)
        fr_window_set_password_for_paste(data->window, password);
      g_free(password);
      if (fr_window_is_batch_mode(data->window))
        fr_window_resume_batch(data->window);
      else
        fr_window_restart_current_batch_action(data->window);
      break;

    default:
      if (fr_window_is_batch_mode(data->window))
        gtk_widget_destroy(GTK_WIDGET(data->window));
      else
        fr_window_reset_current_batch_action(data->window);
      break;
  }

  gtk_widget_destroy(data->dialog);
}

static void dlg_ask_password__common(FrWindow *window,
                                     FrPasswordType pwd_type) {
  GtkBuilder *builder;
  DialogData *data;
  char *text;
  char *name = NULL;

  data = g_new0(DialogData, 1);
  builder = gtk_builder_new_from_resource(
      ENGRAMPA_RESOURCE_UI_PATH G_DIR_SEPARATOR_S "batch-password.ui");
  data->window = window;
  data->pwd_type = pwd_type;

  /* Get the widgets. */

  data->dialog = GET_WIDGET("password_dialog");
  data->pw_password_entry = GET_WIDGET("pw_password_entry");

  /* Set widgets data. */

  if (data->pwd_type == FR_PASSWORD_TYPE_MAIN)
    name = g_uri_display_basename(fr_window_get_archive_uri(window));
  else if (data->pwd_type == FR_PASSWORD_TYPE_PASTE_FROM)
    name = g_uri_display_basename(fr_window_get_paste_archive_uri(window));
  g_assert(name != NULL);
  text = g_strdup_printf(_("Enter the password for the archive '%s'."), name);
  gtk_label_set_label(GET_LABEL("pw_password_label"), text);
  g_free(text);

  if (fr_window_get_password(window) != NULL)
    _gtk_entry_set_locale_text(GTK_ENTRY(data->pw_password_entry),
                               fr_window_get_password(window));

  /* Set the signals handlers. */

  gtk_builder_add_callback_symbols(builder, "on_password_dialog_destroy",
                                   G_CALLBACK(destroy_cb),
                                   "on_password_dialog_response",
                                   G_CALLBACK(ask_password__response_cb), NULL);

  gtk_builder_connect_signals(builder, data);

  g_object_unref(builder);

  /* Run dialog. */

  gtk_widget_grab_focus(data->pw_password_entry);
  if (gtk_widget_get_realized(GTK_WIDGET(window))) {
    gtk_window_set_transient_for(GTK_WINDOW(data->dialog), GTK_WINDOW(window));
    gtk_window_set_modal(GTK_WINDOW(data->dialog), TRUE);
  } else
    gtk_window_set_title(GTK_WINDOW(data->dialog), name);
  g_free(name);

  gtk_widget_show(data->dialog);
}

void dlg_ask_password(FrWindow *window) {
  dlg_ask_password__common(window, FR_PASSWORD_TYPE_MAIN);
}

void dlg_ask_password_for_paste_operation(FrWindow *window) {
  dlg_ask_password__common(window, FR_PASSWORD_TYPE_PASTE_FROM);
}
