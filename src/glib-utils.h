/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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

#ifndef _GLIB_UTILS_H
#define _GLIB_UTILS_H

#include <glib.h>

/* string utils */

gboolean strchrs(const char *str, const char *chars);
char *str_substitute(const char *str, const char *from_str, const char *to_str);
char *escape_str(const char *str, const char *meta_chars);
gboolean match_regexps(GRegex **regexps, const char *string,
                       GRegexMatchFlags match_options);
char **search_util_get_patterns(const char *pattern_string);
GRegex **search_util_get_regexps(const char *pattern_string,
                                 GRegexCompileFlags compile_options);
void free_regexps(GRegex **regexps);
const char *eat_spaces(const char *line);
char **split_line(const char *line, int n_fields);
const char *get_last_field(const char *line, int last_field);
const char *get_static_string(const char *s);
char *g_uri_display_basename(const char *uri);

/* path filename */

const char *_g_path_get_file_name(const char *path);
const char *_g_path_get_base_name(const char *path, const char *base_dir,
                                  gboolean junk_paths);

/**/

#ifndef __GNUC__
#define __FUNCTION__ ""
#endif

#define DEBUG_INFO __FILE__, __LINE__, __FUNCTION__

void debug(const char *file, int line, const char *function, const char *format,
           ...);

#endif /* _GLIB_UTILS_H */
