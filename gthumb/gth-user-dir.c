/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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

#include <config.h>
#include <glib/gi18n.h>
#include <glib.h>
#include "gth-user-dir.h"


#define GTH_DIR_CONFIG_MODE 0755
#define GTH_DIR_CACHE_MODE  0700
#define GTH_DIR_DATA_MODE   0700


static int
_gth_user_dir_mkdir_with_parents (GthDir  dir_type,
				  GFile  *dir)
{
	char *pathname;
	int   mode;
	int   result;

	pathname = g_file_get_path (dir);

	switch (dir_type) {
	case GTH_DIR_CONFIG:
		mode = GTH_DIR_CONFIG_MODE;
		break;
	case GTH_DIR_CACHE:
		mode = GTH_DIR_CACHE_MODE;
		break;
	case GTH_DIR_DATA:
		mode = GTH_DIR_DATA_MODE;
		break;
	default:
		g_assert_not_reached ();
	}

	result = g_mkdir_with_parents (pathname, mode);

	g_free (pathname);

	return result;
}


static GFile *
_gth_user_dir_get_file_va_list (GthDir      dir_type,
			        const char *first_element,
                                va_list     var_args)
{
	const char *user_dir;
	char       *config_dir;
	GString    *path;
	const char *element;
	GFile      *file;

	user_dir = NULL;
	switch (dir_type) {
	case GTH_DIR_CONFIG:
		user_dir = g_get_user_config_dir ();
		break;
	case GTH_DIR_CACHE:
		user_dir = g_get_user_cache_dir ();
		break;
	case GTH_DIR_DATA:
		user_dir = g_get_user_data_dir ();
		break;
	}

	config_dir = g_build_path (G_DIR_SEPARATOR_S,
				   user_dir,
				   NULL);
	path = g_string_new (user_dir);

	element = first_element;
  	while (element != NULL) {
  		if (element[0] != '\0') {
  			g_string_append (path, G_DIR_SEPARATOR_S);
  			g_string_append (path, element);
  		}
		element = va_arg (var_args, const char *);
	}

	file = g_file_new_for_path (path->str);

	g_string_free (path, TRUE);
	g_free (config_dir);

	return file;
}


void
gth_user_dir_mkdir_with_parents (GthDir      dir_type,
			         const char *first_element,
                                 ...)
{
	va_list  var_args;
	GFile   *file;

	va_start (var_args, first_element);
	file = _gth_user_dir_get_file_va_list (dir_type, first_element, var_args);
	va_end (var_args);

	_gth_user_dir_mkdir_with_parents (dir_type, file);

	g_object_unref (file);
}


GFile *
gth_user_dir_get_dir_for_write (GthDir      dir_type,
			        const char *first_element,
                                ...)
{
	va_list  var_args;
	GFile   *file;

	va_start (var_args, first_element);
	file = _gth_user_dir_get_file_va_list (dir_type, first_element, var_args);
	va_end (var_args);

	_gth_user_dir_mkdir_with_parents (dir_type, file);

	return file;
}


GFile *
gth_user_dir_get_file_for_read (GthDir      dir_type,
				const char *first_element,
				...)
{
	va_list  var_args;
	GFile   *file;

	va_start (var_args, first_element);
	file = _gth_user_dir_get_file_va_list (dir_type, first_element, var_args);
	va_end (var_args);

	return file;
}


GFile *
gth_user_dir_get_file_for_write (GthDir      dir_type,
				 const char *first_element,
				 ...)
{
	va_list  var_args;
	GFile   *file;
	GFile   *parent;

	va_start (var_args, first_element);
	file = _gth_user_dir_get_file_va_list (dir_type, first_element, var_args);
	va_end (var_args);

	parent = g_file_get_parent (file);
	_gth_user_dir_mkdir_with_parents (dir_type, parent);
	g_object_unref (parent);

	return file;
}
