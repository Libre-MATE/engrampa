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

#include "dlg-password.h"

#include <gtk/gtk.h>
#include <string.h>

#include "fr-window.h"
#include "gtk-utils.h"
#include "preferences.h"

#define GET_WIDGET(x) (GTK_WIDGET(gtk_builder_get_object(builder, (x))))

typedef struct {
  FrWindow *window;
  GtkWidget *dialog;
  GtkWidget *pw_password_entry;
  GtkWidget *pw_encrypt_header_checkbutton;
} DialogData;

/* called when the main dialog is closed. */
static void destroy_cb(GtkWidget *widget, DialogData *data) { g_free(data); }

static void response_cb(GtkWidget *dialog, int response_id, DialogData *data) {
  char *password;
  gboolean encrypt_header;

  switch (response_id) {
    case GTK_RESPONSE_OK:
      password = _gtk_entry_get_locale_text(GTK_ENTRY(data->pw_password_entry));
      fr_window_set_password(data->window, password);
      g_free(password);

      encrypt_header = gtk_toggle_button_get_active(
          GTK_TOGGLE_BUTTON(data->pw_encrypt_header_checkbutton));
      {
        GSettings *settings;

        settings = g_settings_new(ENGRAMPA_SCHEMA_GENERAL);
        g_settings_set_boolean(settings, PREF_GENERAL_ENCRYPT_HEADER,
                               encrypt_header);
        g_object_unref(settings);
      }
      fr_window_set_encrypt_header(data->window, encrypt_header);
      break;
    default:
      break;
  }

  gtk_widget_destroy(data->dialog);
}

void dlg_password(GtkWidget *widget, gpointer callback_data) {
  GtkBuilder *builder;
  FrWindow *window = callback_data;
  DialogData *data;

  data = g_new0(DialogData, 1);
  builder = gtk_builder_new_from_resource(
      ENGRAMPA_RESOURCE_UI_PATH G_DIR_SEPARATOR_S "password.ui");
  data->window = window;

  /* Get the widgets. */

  data->dialog = GET_WIDGET("password_dialog");
  data->pw_password_entry = GET_WIDGET("pw_password_entry");
  data->pw_encrypt_header_checkbutton =
      GET_WIDGET("pw_encrypt_header_checkbutton");

  /* Set widgets data. */

  _gtk_entry_set_locale_text(GTK_ENTRY(data->pw_password_entry),
                             fr_window_get_password(window));
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(data->pw_encrypt_header_checkbutton),
      fr_window_get_encrypt_header(window));

  /* Set the signals handlers. */

  gtk_builder_add_callback_symbols(
      builder, "on_password_dialog_destroy", G_CALLBACK(destroy_cb),
      "on_password_dialog_response", G_CALLBACK(response_cb), NULL);

  gtk_builder_connect_signals(builder, data);
  g_object_unref(builder);

  /* Run dialog. */

  gtk_widget_grab_focus(data->pw_password_entry);
  if (gtk_widget_get_realized(GTK_WIDGET(window)))
    gtk_window_set_transient_for(GTK_WINDOW(data->dialog), GTK_WINDOW(window));
  gtk_window_set_modal(GTK_WINDOW(data->dialog), TRUE);

  gtk_widget_show(data->dialog);
}
