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
#include "gth-file-data.h"
#include "glib-utils.h"
#include "gth-main.h"


static int
gth_file_data_cmp_filename (GthFileData *a,
			    GthFileData *b)
{
	const char *key_a, *key_b;

	key_a = gth_file_data_get_filename_sort_key (a);
	key_b = gth_file_data_get_filename_sort_key (b);

	return strcmp (key_a, key_b);
}


static int
gth_file_data_cmp_uri (GthFileData *a,
		       GthFileData *b)
{
	GFile *parent_a;
	GFile *parent_b;
	int    result;

	parent_a = g_file_get_parent (a->file);
	parent_b = g_file_get_parent (b->file);
	if (! g_file_equal (parent_a, parent_b)) {
		char *uri_a;
		char *uri_b;

		uri_a = g_file_get_uri (a->file);
		uri_b = g_file_get_uri (b->file);
		result = strcmp (uri_a, uri_b);

		g_free (uri_b);
		g_free (uri_a);
	}
	else
		result = gth_file_data_cmp_filename (a, b);

	return result;
}


static int
gth_file_data_cmp_filesize (GthFileData *a,
			    GthFileData *b)
{
	goffset size_a, size_b;

	size_a = g_file_info_get_size (a->info);
	size_b = g_file_info_get_size (b->info);

	if (size_a < size_b)
		return -1;
	else if (size_a > size_b)
		return 1;
	else
		return 0;
}


static int
gth_file_data_cmp_modified_time (GthFileData *a,
			         GthFileData *b)
{
	GTimeVal *ta, *tb;
	int       result;

	ta = gth_file_data_get_modification_time (a);
	tb = gth_file_data_get_modification_time (b);

	result = _g_time_val_cmp (ta, tb);
	if (result == 0)
		result = gth_file_data_cmp_filename (a, b);

	return result;
}


static int
gth_general_data_cmp_dimensions (GthFileData *a,
				 GthFileData *b)
{
	int width_a;
	int height_a;
	int width_b;
	int height_b;
	int result;

	width_a = g_file_info_get_attribute_int32 (a->info, "frame::width");
	height_a = g_file_info_get_attribute_int32 (a->info, "frame::height");
	width_b = g_file_info_get_attribute_int32 (b->info, "frame::width");
	height_b = g_file_info_get_attribute_int32 (b->info, "frame::height");

	result = width_a * height_a - width_b * height_b;
	if (result == 0)
		result = gth_file_data_cmp_filename (a, b);

	return result;
}


GthFileDataSort default_sort_types[] = {
	{ "file::name", N_("file name"), "standard::display-name", gth_file_data_cmp_filename },
	{ "file::path", N_("file path"), "standard::display-name", gth_file_data_cmp_uri },
	{ "file::size", N_("file size"), "standard::size", gth_file_data_cmp_filesize },
	{ "file::mtime", N_("file modified date"), "time::modified,time::modified-usec", gth_file_data_cmp_modified_time },
	{ "general::unsorted", N_("no sorting"), "", NULL },
	{ "general::dimensions", N_("dimensions"), "frame::width,frame::height", gth_general_data_cmp_dimensions },
};


void
gth_main_register_default_sort_types (void)
{
	int i;

	for (i = 0; i < G_N_ELEMENTS (default_sort_types); i++)
		gth_main_register_sort_type (&default_sort_types[i]);
}
