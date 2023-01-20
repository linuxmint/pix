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

#include <config.h>
#include <string.h>
#include <locale.h>
#include <gtk/gtk.h>
#include "gtk-utils.h"


static char *
test_gtk_tree_path_get_previous_or_parent (const char *path)
{
	GtkTreePath *tree_path;
	GtkTreePath *result;
	char        *result_s;

	tree_path = gtk_tree_path_new_from_string (path);
	if (tree_path == NULL)
		return NULL;

	result = _gtk_tree_path_get_previous_or_parent (tree_path);
	if (result != NULL) {
		result_s = gtk_tree_path_to_string (result);
		gtk_tree_path_free (result);
	}
	else
		result_s = NULL;

	gtk_tree_path_free (tree_path);

	return result_s;
}


static void
test_gtk_tree_path_get_previous_or_parent_all (void)
{
	g_assert_cmpstr (test_gtk_tree_path_get_previous_or_parent ("0"), ==, NULL);
	g_assert_cmpstr (test_gtk_tree_path_get_previous_or_parent ("1"), ==, "0");
	g_assert_cmpstr (test_gtk_tree_path_get_previous_or_parent ("2"), ==, "1");
	g_assert_cmpstr (test_gtk_tree_path_get_previous_or_parent ("0:0"), ==, "0");
	g_assert_cmpstr (test_gtk_tree_path_get_previous_or_parent ("0:1"), ==, "0:0");
	g_assert_cmpstr (test_gtk_tree_path_get_previous_or_parent ("0:2"), ==, "0:1");
	g_assert_cmpstr (test_gtk_tree_path_get_previous_or_parent ("0:0:0"), ==, "0:0");
	g_assert_cmpstr (test_gtk_tree_path_get_previous_or_parent ("0:0:1"), ==, "0:0:0");
	g_assert_cmpstr (test_gtk_tree_path_get_previous_or_parent ("0:0:2"), ==, "0:0:1");
	g_assert_cmpstr (test_gtk_tree_path_get_previous_or_parent ("4:10:0:3"), ==, "4:10:0:2");
	g_assert_cmpstr (test_gtk_tree_path_get_previous_or_parent ("4:10:0"), ==, "4:10");
}


int
main (int   argc,
      char *argv[])
{
	setlocale (LC_ALL, "");
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/gtk-utils/_gtk_tree_path_get_previous_or_parent", test_gtk_tree_path_get_previous_or_parent_all);

	return g_test_run ();
}
