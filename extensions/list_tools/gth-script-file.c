/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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
#include <gthumb.h>
#include "gth-script.h"
#include "gth-script-file.h"


/* version 1.0: Original version
   version 1.1: Changed command special codes to a single character. */
#define SCRIPT_FORMAT "1.1"


enum {
	CHANGED,
	LAST_SIGNAL
};


struct _GthScriptFilePrivate
{
	gboolean  loaded;
	GList    *items;
};


static guint gth_script_file_signals[LAST_SIGNAL] = { 0 };


static GType gth_script_file_get_type (void);


G_DEFINE_TYPE_WITH_CODE (GthScriptFile,
			 gth_script_file,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (GthScriptFile))


static void
gth_script_file_finalize (GObject *object)
{
	GthScriptFile *self;

	self = GTH_SCRIPT_FILE (object);
	_g_object_list_unref (self->priv->items);

	G_OBJECT_CLASS (gth_script_file_parent_class)->finalize (object);
}


static void
gth_script_file_class_init (GthScriptFileClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass*) klass;
	object_class->finalize = gth_script_file_finalize;

	gth_script_file_signals[CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GthScriptFileClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
gth_script_file_init (GthScriptFile *self)
{
	self->priv = gth_script_file_get_instance_private (self);
	self->priv->items = NULL;
	self->priv->loaded = FALSE;
}


GthScriptFile *
gth_script_file_get (void)
{
	static GthScriptFile *script_file = NULL;

	if (script_file == NULL)
		script_file = g_object_new (GTH_TYPE_SCRIPT_FILE, NULL);

  	return script_file;
}


/* -- convert_command_attributes_1_0 -- */


static gboolean
convert_command_attributes_1_0_cb (const GMatchInfo *match_info,
				   GString          *result,
				   gpointer          user_data)
{
	char *match;

	g_string_append_c (result, '%');
	match = g_match_info_fetch (match_info, 0);
	if (strcmp (match, "%ask") == 0)
		g_string_append_c (result, GTH_SCRIPT_CODE_ASK_VALUE);
	else if (strcmp (match, "%quote") == 0)
		g_string_append_c (result, GTH_SCRIPT_CODE_QUOTE);
	if (strcmp (match, "%attr") == 0)
		g_string_append_c (result, GTH_SCRIPT_CODE_FILE_ATTRIBUTE);

	return FALSE;
}


static char *
convert_command_attributes_1_0 (const char *command)
{
	GRegex *re;
	char   *new_command;

	re = g_regex_new ("%ask|%quote|%attr", 0, 0, NULL);
	new_command = g_regex_replace_eval (re,
					    command,
					    -1,
					    0,
					    0,
					    convert_command_attributes_1_0_cb,
					    NULL,
					    NULL);

	g_regex_unref (re);

	return new_command;
}


static gboolean
gth_script_file_load_from_data (GthScriptFile  *self,
                                const char     *data,
                                gsize           length,
                                GError        **error)
{
	DomDocument *doc;
	gboolean     success;

	doc = dom_document_new ();
	success = dom_document_load (doc, data, length, error);
	if (success) {
		DomElement *scripts_node;
		DomElement *child;
		GList      *new_items = NULL;

		scripts_node = DOM_ELEMENT (doc)->first_child;
		if ((scripts_node != NULL) && (g_strcmp0 (scripts_node->tag_name, "scripts") == 0)) {
			gboolean convert_format_1_0;

			convert_format_1_0 = _g_str_equal (dom_element_get_attribute (DOM_ELEMENT (scripts_node), "version"), "1.0");

			for (child = scripts_node->first_child;
			     child != NULL;
			     child = child->next_sibling)
			{
				GthScript *script = NULL;

				if (strcmp (child->tag_name, "script") == 0) {
					script = gth_script_new ();
					dom_domizable_load_from_element (DOM_DOMIZABLE (script), child);
					if (convert_format_1_0) {
						char *new_command;

						new_command = convert_command_attributes_1_0 (gth_script_get_command (script));
						g_object_set (script, "command", new_command, NULL);

						g_free (new_command);
					}
				}

				if (script == NULL)
					continue;

				new_items = g_list_prepend (new_items, script);
			}

			new_items = g_list_reverse (new_items);
			self->priv->items = g_list_concat (self->priv->items, new_items);
		}
	}
	g_object_unref (doc);

	return success;
}


static gboolean
gth_script_file_load_from_file (GthScriptFile  *self,
                                GFile          *file,
                                GError        **error)
{
	char     *buffer;
	gsize     len;
	GError   *read_error;
	gboolean  retval;

	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (file != NULL, FALSE);

	read_error = NULL;
	_g_file_load_in_buffer (file, (void **) &buffer, &len, NULL, &read_error);
	if (read_error != NULL) {
		g_propagate_error (error, read_error);
		return FALSE;
	}

	read_error = NULL;
	retval = gth_script_file_load_from_data (self,
						 buffer,
                                           	 len,
                                           	 &read_error);
  	if (read_error != NULL) {
		g_propagate_error (error, read_error);
		g_free (buffer);
		return FALSE;
	}

  	g_free (buffer);

	return retval;
}


static void
_gth_script_file_load_if_needed (GthScriptFile *self)
{
	GFile *default_script_file;

	if (self->priv->loaded)
		return;

	default_script_file = gth_user_dir_get_file_for_read (GTH_DIR_CONFIG, GTHUMB_DIR, "scripts.xml", NULL);
	gth_script_file_load_from_file (self, default_script_file, NULL);
	self->priv->loaded = TRUE;

	g_object_unref (default_script_file);
}


static char *
gth_script_file_to_data (GthScriptFile  *self,
			 gsize          *len,
			 GError        **data_error)
{
	DomDocument *doc;
	DomElement  *root;
	char        *data;
	GList       *scan;

	doc = dom_document_new ();
	root = dom_document_create_element (doc, "scripts",
					    "version", SCRIPT_FORMAT,
					    NULL);
	dom_element_append_child (DOM_ELEMENT (doc), root);
	for (scan = self->priv->items; scan; scan = scan->next)
		dom_element_append_child (root, dom_domizable_create_element (DOM_DOMIZABLE (scan->data), doc));
	data = dom_document_dump (doc, len);

	g_object_unref (doc);

	return data;
}


static gboolean
gth_script_file_to_file (GthScriptFile  *self,
                         GFile          *file,
                         GError        **error)
{
	char     *data;
	GError   *data_error, *write_error;
	gsize     len;
	gboolean  retval;

	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (file != NULL, FALSE);

	data_error = NULL;
	data = gth_script_file_to_data (self, &len, &data_error);
	if (data_error) {
		g_propagate_error (error, data_error);
		return FALSE;
	}

	write_error = NULL;
	if (! _g_file_write (file, FALSE, 0, data, len, NULL, &write_error)) {
		g_propagate_error (error, write_error);
		retval = FALSE;
	}
	else
		retval = TRUE;

	g_free (data);

	return retval;
}


GthScript *
gth_script_file_get_script (GthScriptFile *self,
			    const char    *id)
{
	GList *scan;

	for (scan = self->priv->items; scan; scan = scan->next) {
		GthScript *script = scan->data;

		if (g_strcmp0 (gth_script_get_id (script), id) == 0)
			return script;
	}

	return NULL;
}


gboolean
gth_script_file_save (GthScriptFile  *self,
		      GError        **error)
{
	GFile    *default_script_file;
	gboolean  result;

	_gth_script_file_load_if_needed (self);

	default_script_file = gth_user_dir_get_file_for_write (GTH_DIR_CONFIG, GTHUMB_DIR, "scripts.xml", NULL);
	result = gth_script_file_to_file (self, default_script_file, error);
	if (result)
		g_signal_emit (G_OBJECT (self), gth_script_file_signals[CHANGED], 0);

	g_object_unref (default_script_file);

	return result;
}


GList *
gth_script_file_get_scripts (GthScriptFile *self)
{
	GList *list;
	GList *scan;

	_gth_script_file_load_if_needed (self);

	list = NULL;
	for (scan = self->priv->items; scan; scan = scan->next)
		list = g_list_prepend (list, gth_duplicable_duplicate (GTH_DUPLICABLE (scan->data)));

	return g_list_reverse (list);
}


static int
find_by_id (gconstpointer a,
            gconstpointer b)
{
	GthScript *script = (GthScript *) a;
	GthScript *script_to_find = (GthScript *) b;

	return g_strcmp0 (gth_script_get_id (script), gth_script_get_id (script_to_find));
}


gboolean
gth_script_file_has_script (GthScriptFile *self,
			    GthScript     *script)
{
	return g_list_find_custom (self->priv->items, script, find_by_id) != NULL;
}


void
gth_script_file_add (GthScriptFile *self,
		     GthScript     *script)
{
	GList *link;

	_gth_script_file_load_if_needed (self);

	g_object_ref (script);

	link = g_list_find_custom (self->priv->items, script, find_by_id);
	if (link != NULL) {
		g_object_unref (link->data);
		link->data = script;
	}
	else
		self->priv->items = g_list_append (self->priv->items, script);
}


void
gth_script_file_remove (GthScriptFile *self,
			GthScript     *script)
{
	GList *link;

	_gth_script_file_load_if_needed (self);

	link = g_list_find_custom (self->priv->items, script, find_by_id);
	if (link == NULL)
		return;
	self->priv->items = g_list_remove_link (self->priv->items, link);

	_g_object_list_unref (link);
}


void
gth_script_file_clear (GthScriptFile *self)
{
	_g_object_list_unref (self->priv->items);
	self->priv->items = NULL;
}
