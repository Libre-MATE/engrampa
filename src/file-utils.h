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

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <gio/gio.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MIME_TYPE_DIRECTORY "folder"
#define MIME_TYPE_ARCHIVE "application/x-archive"

#define get_home_relative_path(x) g_strconcat(g_get_home_dir(), "/", (x), NULL)

gboolean uri_exists(const char *uri);
gboolean uri_is_file(const char *uri);
gboolean uri_is_dir(const char *uri);
gboolean path_is_dir(const char *path);
gboolean uri_is_local(const char *uri);
char *get_dir_content_if_unique(const char *uri);
gboolean path_in_path(const char *path_src, const char *path_dest);
goffset get_file_size(const char *uri);
goffset get_file_size_for_path(const char *path);
time_t get_file_mtime(const char *uri);
time_t get_file_mtime_for_path(const char *path);
gboolean make_directory_tree(GFile *dir, mode_t mode, GError **error);
gboolean ensure_dir_exists(const char *uri, mode_t mode, GError **error);
gboolean make_directory_tree_from_path(const char *path, mode_t mode,
                                       GError **error);
const char *file_name_from_path(const char *path);
char *dir_name_from_path(const char *path);
char *remove_level_from_path(const char *path);
char *remove_ending_separator(const char *path);
char *build_uri(const char *base, ...);
char *remove_extension_from_path(const char *path);
const char *get_file_extension(const char *filename);
gboolean file_extension_is(const char *filename, const char *ext);
gboolean is_mime_type(const char *type, const char *pattern);
const char *get_file_mime_type(const char *uri, gboolean fast_file_type);
const char *get_file_mime_type_for_path(const char *filename,
                                        gboolean fast_file_type);
guint64 get_dest_free_space(const char *path);
gboolean remove_directory(const char *uri);
gboolean remove_local_directory(const char *directory);
char *get_temp_work_dir(const char *parent_folder);
gboolean is_temp_work_dir(const char *dir);
gboolean is_temp_dir(const char *dir);

/* misc functions used to parse a command output lines. */

gboolean file_list__match_pattern(const char *line, const char *pattern);
int file_list__get_index_from_pattern(const char *line, const char *pattern);
char *file_list__get_prev_field(const char *line, int start_from, int field_n);
gboolean check_permissions(const char *path, int mode);
gboolean check_file_permissions(GFile *file, int mode);
gboolean is_program_in_path(const char *filename);
gboolean is_program_available(const char *filename, gboolean check);

/* URI utils */

const char *get_home_uri(void);
GFile *get_user_config_subdirectory(const char *child_name, gboolean create_);
char *get_alternative_uri(const char *folder, const char *name);
char *get_alternative_uri_for_uri(const char *uri);

void path_list_free(GList *path_list);
GList *path_list_dup(GList *path_list);

GList *gio_file_list_dup(GList *l);
void gio_file_list_free(GList *l);
void g_key_file_save(GKeyFile *key_file, GFile *file);

#endif /* FILE_UTILS_H */
