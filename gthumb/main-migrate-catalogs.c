/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
#include "dom.h"
#include "gio-utils.h"
#include "glib-utils.h"
#include "gth-time.h"
#include "gth-preferences.h"
#include "gth-user-dir.h"


#define MAX_LINE_LENGTH 4096


typedef struct {
	GSettings *settings;
	GFile     *collections_dir;
} MigrationData;


static void
migration_data_free (MigrationData *data)
{
	_g_object_unref (data->collections_dir);
	_g_object_unref (data->settings);
	g_free (data);
}


static void
migration_done (GError   *error,
		gpointer  user_data)
{
	MigrationData *data = user_data;

	g_settings_set_boolean (data->settings, PREF_DATA_MIGRATION_CATALOGS_2_10, TRUE);
	migration_data_free (data);
}


static void
copy_unquoted (char *unquoted,
	       char *line)
{
	int len = strlen (line);

	strncpy (unquoted, line + 1, len - 2);
	unquoted[len - 2] = 0;
}


static void
migration_for_each_file (GFile     *file,
			 GFileInfo *info,
			 gpointer   user_data)
{
	MigrationData *data = user_data;
	char          *buffer;
	gsize          buffer_size;
	GError        *error = NULL;
	DomDocument   *document;
	char          *extension;
	char          *new_buffer;
	gsize          new_buffer_size;
	GFile         *catalogs_dir;
	char          *catalogs_path;
	char          *relative_path;
	char          *tmp_path;
	char          *full_path_no_ext;
	char          *full_path;
	GFile         *catalog_file;
	GFile         *catalog_dir;
	char          *catalog_dir_path;

	if (g_file_info_get_file_type (info) != G_FILE_TYPE_REGULAR)
		return;

	if (! _g_file_load_in_buffer (file,
				      (void **) &buffer,
				      &buffer_size,
				      NULL,
				      &error))
	{
		g_warning ("%s", error->message);
		return;
	}

	if (buffer != NULL)
		buffer[buffer_size] = '\0';

	/*
	g_print ("* %s\n", g_file_get_path (file));
	g_print ("%s\n", buffer);
	*/

	document = dom_document_new ();
	extension = ".catalog";

	if ((buffer != NULL) && (*buffer != '\0')) {
		/*
		 * Catalog Format : A list of quoted filenames, one name per line :
		 *     # sort: name      Optional: possible values: none, name, path, size, time, manual
		 *     "filename_1"
		 *     "filename_2"
		 *     ...
		 *     "filename_n"
		 *
		 * Search Format : Search data followed by files list :
		 *    # Search           Line 1 : search data starts with SEARCH_HEADER
		 *    "/home/pippo"      Line 2 : directory to start from.
		 *    "FALSE"            Line 3 : recursive.
		 *    "*.jpg"            Line 4 : file pattern.
		 *    ""                 Line 5 : comment pattern.
		 *    "Rome; Paris"      Line 6 : place pattern.
		 *    0"Formula 1"       Line 7 : old versions have only a keywords
		 *                                pattern, new versions have a 0 or 1
		 *                                (0 = must match at leaset one
		 *                                keyword; 1 = must match all keywords)
		 *				  followed by the keywords pattern.
		 *    1022277600         Line 8 : date in time_t format.
		 *    0                  Line 9 : date scope (DATE_ANY      = 0,
		 *                                            DATE_BEFORE   = 1,
		 *                                            DATE_EQUAL_TO = 2,
		 *                                            DATE_AFTER    = 3)
		 *    "filename_1"       From Line 10 as catalog format.
		 *    "filename_2"
		 *    ...
		 *    "filename_n"
		 *
		 */

		gboolean     file_list = FALSE;
		char       **lines;
		int          n_line;
		DomElement  *root_node = NULL;
		DomElement  *files_node = NULL;
		DomElement  *folder_node = NULL;
		DomElement  *tests_node = NULL;

		lines = g_strsplit (buffer, "\n", -1);
		for (n_line = 0; lines[n_line] != NULL; n_line++) {
			char *line = lines[n_line];
			char *filename;
			char  unquoted [MAX_LINE_LENGTH];
			char *uri;

			if (! file_list && (strncmp (line, "# Search", 8) == 0)) {
				DomElement *node;
				char       *s;
				gboolean    b;
				int         line_ofs;
				/*gboolean    all_keywords;*/
				time_t      date;
				int         date_scope;

				extension = ".search";
				root_node = dom_document_create_element (document, "search", "version", "1.0", NULL);
				dom_element_append_child (DOM_ELEMENT (document), root_node);

				/* -- folder -- */

				/* line 2: start directory */

				n_line++;
				copy_unquoted (unquoted, lines[n_line]);
				s = g_strdup (unquoted);

				/* line 3: recursive */

				n_line++;
				copy_unquoted (unquoted, lines[n_line]);
				b = strcmp (unquoted, "TRUE") == 0;

				folder_node = dom_document_create_element (document, "folder",
									   "recursive", (b ? "true" : "false"),
									   "uri", s,
									   NULL);

				g_free (s);

				/* -- tests -- */

				tests_node = dom_document_create_element (document, "tests",
									  "match", "all",
									  NULL);

				/* line 4: filename */

				n_line++;
				copy_unquoted (unquoted, lines[n_line]);
				s = g_strdup (unquoted);
				if (strcmp (s, "") != 0) {
					char *op;

					if (strchr (s, '*') == NULL)
						op = "contains";
					else
						op = "matches";
					node = dom_document_create_element (document, "test",
									    "id", "file::name",
									    "op", op,
									    "value", s,
									    NULL);
					dom_element_append_child (tests_node, node);
				}

				g_free (s);

				/* line 5: comment */

				n_line++;
				copy_unquoted (unquoted, lines[n_line]);
				s = g_strdup (unquoted);
				if (strcmp (s, "") != 0) {
					node = dom_document_create_element (document, "test",
									    "id", "comment::note",
									    "op", "contains",
									    "value", s,
									    NULL);
					dom_element_append_child (tests_node, node);
				}

				g_free (s);

				/* line 6: place */

				n_line++;
				copy_unquoted (unquoted, lines[n_line]);
				s = g_strdup (unquoted);
				if (strcmp (s, "") != 0) {
					char **v;
					int    j;

					v = g_strsplit (s, ";", -1);
					for (j = 0; v[j] != NULL; j++) {
						node = dom_document_create_element (document, "test",
										    "id", "comment::place",
										    "op", "contains",
										    "value", g_strstrip (v[j]),
										    NULL);
						dom_element_append_child (tests_node, node);
					}

					g_strfreev (v);
				}

				g_free (s);

				/* line 7: keywords */

				n_line++;
				line_ofs = 0;
				/*all_keywords = FALSE;*/
				if (lines[n_line][0] != '"') {
					line_ofs = 1;
					/*all_keywords = (*line == '1');*/
				}
				copy_unquoted (unquoted, lines[n_line] + line_ofs);
				s = g_strdup (unquoted);
				if (strcmp (s, "") != 0) {
					char **v;
					int    j;

					v = g_strsplit (s, ";", -1);
					for (j = 0; v[j] != NULL; j++) {
						node = dom_document_create_element (document, "test",
										    "id", "comment::category",
										    "op", "equal",
										    "value", g_strstrip (v[j]),
										    NULL);
						dom_element_append_child (tests_node, node);
					}

					g_strfreev (v);
				}

				g_free (s);

				/* line 8: date */

				n_line++;
				sscanf (lines[n_line], "%ld", &date);

				/* line 9: date scope */

				n_line++;
				sscanf (line, "%d", &date_scope);

				if ((date > 0) && (date_scope >= 1) && (date_scope <= 3)) {
					struct tm   *tm;
					GthDateTime *dt;
					char        *exif_date;
					char        *op;

					tm = localtime (&date);
					dt = gth_datetime_new ();
					gth_datetime_from_struct_tm (dt, tm);
					exif_date = gth_datetime_to_exif_date (dt);

					switch (date_scope) {
					case 1:
						op = "before";
						break;
					case 2:
						op = "equal";
						break;
					case 3:
						op = "after";
						break;
					default:
						op = "";
						break;
					}

					node = dom_document_create_element (document, "test",
									    "id", "file::mtime",
									    "op", op,
									    "value", exif_date,
									    NULL);
					dom_element_append_child (tests_node, node);

					g_free (exif_date);
					gth_datetime_free (dt);
				}

				continue;
			}

			if (! file_list && (strncmp (line, "# sort: ", 8) == 0)) {
				char *sort_name[] = { "none", "name", "path", "size", "time", "exif-date", "comment", "manual", NULL };
				char *order_type[] = { "general::unsorted", "file::name", "file::name", "file::size", "file::mtime", "exif::photo::datetimeoriginal", "file::name", "general::unsorted" };
				char *sort_type;
				int   i;

				sort_type = line + 8;
				sort_type[strlen (sort_type)] = 0;

				for (i = 0; sort_name[i] != NULL; i++) {
					if (strcmp (sort_type, sort_name[i]) == 0) {
						DomElement *order_node;

						if (root_node == NULL) {
							root_node = dom_document_create_element (document, "catalog", "version", "1.0", NULL);
							dom_element_append_child (DOM_ELEMENT (document), root_node);
						}

						order_node = dom_document_create_element (document, "order", "inverse", "0", "type", order_type[i], NULL);
						dom_element_append_child (root_node, order_node);

						break;
					}
				}

				continue;
			}

			file_list  = TRUE;
			if (line[0] != '"')
				continue;
			if (strlen (line) <= 2)
				continue;
			filename = g_strndup (line + 1, strlen (line) - 2);

			if (root_node == NULL) {
				root_node = dom_document_create_element (document, "catalog", "version", "1.0", NULL);
				dom_element_append_child (DOM_ELEMENT (document), root_node);
			}

			if (files_node == NULL) {
				files_node = dom_document_create_element (document, "files", NULL);
				dom_element_append_child (root_node, files_node);
			}

			if (filename[0] == '/')
				uri = g_filename_to_uri (filename, NULL, NULL);
			else
				uri = g_strdup (filename);

			if (uri != NULL)
				dom_element_append_child (files_node, dom_document_create_element (document, "file", "uri", uri, NULL));

			g_free (uri);
			g_free (filename);
		}

		if (root_node == NULL) {
			root_node = dom_document_create_element (document, "catalog", "version", "1.0", NULL);
			dom_element_append_child (DOM_ELEMENT (document), root_node);
		}

		if (folder_node != NULL)
			dom_element_append_child (root_node, folder_node);
		if (tests_node != NULL)
			dom_element_append_child (root_node, tests_node);


		g_strfreev (lines);
	}
	else {
		DomElement *root_node;
		DomElement *files_node;

		root_node = dom_document_create_element (document, "catalog", "version", "1.0", NULL);
		dom_element_append_child (DOM_ELEMENT (document), root_node);

		files_node = dom_document_create_element (document, "files", NULL);
		dom_element_append_child (root_node, files_node);
	}

	new_buffer = dom_document_dump (document, &new_buffer_size);

	catalogs_dir = gth_user_dir_get_file_for_write (GTH_DIR_DATA, GTHUMB_DIR, "catalogs", NULL);
	catalogs_path = g_file_get_path (catalogs_dir);
	relative_path = g_file_get_relative_path (data->collections_dir, file);
	tmp_path = g_strconcat (catalogs_path, G_DIR_SEPARATOR_S, relative_path, NULL);
	full_path_no_ext = _g_uri_remove_extension (tmp_path);
	full_path = g_strconcat (full_path_no_ext, extension, NULL);
	catalog_file = g_file_new_for_path (full_path);
	catalog_dir = g_file_get_parent (catalog_file);
	catalog_dir_path = g_file_get_path (catalog_dir);
	g_mkdir_with_parents (catalog_dir_path, 0700);

	/*
	g_print ("==> %s\n", g_file_get_path (catalog_file));
	g_print ("%s\n", new_buffer);
	*/

	if (! g_file_query_exists (catalog_file, NULL)) {
		if (! _g_file_write (catalog_file,
				     FALSE,
				     G_FILE_CREATE_PRIVATE,
				     new_buffer,
				     new_buffer_size,
				     NULL,
				     &error))
		{
			g_warning ("%s", error->message);
		}
	}

	g_object_unref (catalog_dir);
	g_object_unref (catalog_file);
	g_free (full_path);
	g_free (full_path_no_ext);
	g_free (tmp_path);
	g_free (relative_path);
	g_free (catalogs_path);
	g_object_unref (catalogs_dir);
	g_free (new_buffer);
	g_object_unref (document);
	g_free (buffer);
}


static DirOp
migration_start_dir (GFile      *directory,
		     GFileInfo  *info,
		     GError    **error,
		     gpointer    user_data)
{
	return DIR_OP_CONTINUE;
}


void
migrate_catalogs_from_2_10 (void)
{
	MigrationData *data;
	GFile         *home_dir;

	data = g_new0 (MigrationData, 1);
	data->settings = g_settings_new (GTHUMB_DATA_MIGRATION_SCHEMA);

	if (g_settings_get_boolean (data->settings, PREF_DATA_MIGRATION_CATALOGS_2_10)) {
		migration_data_free (data);
		return;
	}

	home_dir = g_file_new_for_path (g_get_home_dir ());
	data->collections_dir = _g_file_get_child (home_dir, ".gnome2", "gthumb", "collections", NULL);

	g_directory_foreach_child (data->collections_dir,
				   TRUE,
				   TRUE,
				   "standard::name,standard::type",
				   NULL,
				   migration_start_dir,
				   migration_for_each_file,
				   migration_done,
				   data);

	g_object_unref (home_dir);
}
