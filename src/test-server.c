/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2010 The Free Software Foundation, Inc.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gio/gio.h>

GMainLoop *loop;

static void engrampa_addtoarchive_ready_cb(GObject *source_object,
                                           GAsyncResult *res,
                                           gpointer user_data) {
  GDBusProxy *proxy;
  GVariant *values;
  GError *error = NULL;

  proxy = G_DBUS_PROXY(source_object);
  values = g_dbus_proxy_call_finish(proxy, res, &error);
  if (values == NULL) {
    g_error("%s\n", error->message);
  }

  if (values != NULL) g_variant_unref(values);
  g_object_unref(proxy);

  g_main_loop_quit(loop);
}

static void on_signal(GDBusProxy *proxy, char *sender_name, char *signal_name,
                      GVariant *parameters, gpointer user_data) {
  if (g_strcmp0(signal_name, "Progress") == 0) {
    double fraction;
    char *details;

    g_variant_get(parameters, "(ds)", &fraction, &details);
    g_print("Progress: %f (%s)\n", fraction, details);

    g_free(details);
  }
}

int main(int argc, char *argv[]) {
  GDBusConnection *connection;
  GError *error = NULL;

  connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
  if (connection != NULL) {
    GDBusProxy *proxy;

    proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE, NULL,
                                  "org.mate.Engrampa", "/org/mate/Engrampa",
                                  "org.mate.ArchiveManager", NULL, &error);

    if (proxy != NULL) {
      g_signal_connect(proxy, "g-signal", G_CALLBACK(on_signal), NULL);

      /* -- AddToArchive -- */

      char *archive;
      char **files;

      archive =
          g_strdup("file:///home/paolo/Scrivania/firefox-4.0b8pre.tar.gz");
      files = g_new0(char *, 2);
      files[0] = g_strdup("file:///home/paolo/Scrivania/firefox-4.0b8pre");
      files[1] = NULL;

      g_dbus_proxy_call(proxy, "AddToArchive",
                        g_variant_new("(s^asb)", archive, files, FALSE),
                        G_DBUS_CALL_FLAGS_NONE, G_MAXINT, NULL,
                        engrampa_addtoarchive_ready_cb, NULL);

      g_free(archive);
      g_strfreev(files);
    }
  }

  loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);

  return 0;
}
