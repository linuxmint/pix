/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 Free Software Foundation, Inc.
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

#ifndef _URI_UTILS_H
#define _URI_UTILS_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
	char *scheme;
	char *userinfo;
	char *host;
	char *port;
	char *path;
	char *query;
	char *fragment;
} UriParts;

typedef enum {
	URI_PART_SCHEME,
	URI_PART_USERINFO,
	URI_PART_HOST,
	URI_PART_PORT,
	URI_PART_PATH,
	URI_PART_QUERY,
	URI_PART_FRAGMENT
} UriPart;

gboolean	_g_uri_split			(const char	 *uri,
						 UriParts	 *parts);
char *		_g_uri_join			(UriParts	 *parts);
void		_g_uri_parts_init		(UriParts	 *parts);
void		_g_uri_parts_clear		(UriParts	 *parts);
char *		_g_uri_get_part		(const char	 *uri,
						 UriPart	  part);
char *		_g_uri_get_path		(const char	 *uri);
char *		_g_uri_get_basename		(const char	 *uri);
char *		_g_uri_get_extension		(const char	 *uri);
char *		_g_uri_get_scheme		(const char	 *uri);
char *		_g_uri_get_relative_path	(const char	 *uri,
						 const char	 *base);
gboolean	_g_uri_is_parent		(const char	 *uri,
						 const char	 *file);
char *		_g_uri_get_parent		(const char	 *uri);
char *		_g_uri_remove_extension	(const char	 *uri);
char *		_g_uri_from_path		(const char	 *path);
const char *	_g_uri_get_home		(void);
char *		_g_uri_append_path		(const char	 *uri,
						 const char	 *path);

/* Path utils */

char **	_g_path_split_components	(const char	 *path,
						 int		 *size);
char *		_g_path_join_components	(char		**comp_v);
char *		_g_path_join			(const char	 *first,
						 ...) G_GNUC_NULL_TERMINATED;
const char *	_g_path_get_basename		(const char	 *path);
char *		_g_path_get_parent		(const char	 *path);
const char *	_g_path_get_extension		(const char	 *path);
char *		_g_path_remove_extension	(const char	 *path);
gboolean	_g_path_is_parent		(const char	 *dir,
						 const char	 *file);
char *		_g_path_get_relative		(const char	 *path,
						 const char	 *base);

G_END_DECLS

#endif /* _URI_UTILS_H */
