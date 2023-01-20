/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2019 The Free Software Foundation, Inc.
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
#include "dom.h"
#include "gio-utils.h"
#include "glib-utils.h"
#include "gth-shortcut.h"
#include "gth-user-dir.h"


GthShortcut *
gth_shortcut_new (const char *action_name,
		  GVariant   *param)
{
	GthShortcut *shortcut;

	g_return_val_if_fail (action_name != NULL, NULL);

	shortcut = g_new (GthShortcut, 1);
	shortcut->action_name = g_strdup (action_name);
	shortcut->action_parameter = (param != NULL) ? g_variant_ref_sink (param) : NULL;
	shortcut->detailed_action = g_action_print_detailed_name (shortcut->action_name, shortcut->action_parameter);
	shortcut->description = NULL;
	shortcut->context = 0;
	shortcut->category = NULL;
	shortcut->default_accelerator = NULL;
	shortcut->accelerator = NULL;
	shortcut->label = NULL;
	shortcut->keyval = 0;
	shortcut->modifiers = 0;
	shortcut->viewer_context = NULL;

	return shortcut;
}


GthShortcut *
gth_shortcut_dup (const GthShortcut *shortcut)
{
	GthShortcut *new_shortcut;

	new_shortcut = gth_shortcut_new (shortcut->action_name, shortcut->action_parameter);
	new_shortcut->description = g_strdup (shortcut->description);
	new_shortcut->context = shortcut->context;
	new_shortcut->category = shortcut->category;
	new_shortcut->default_accelerator = g_strdup (shortcut->default_accelerator);
	new_shortcut->viewer_context = shortcut->viewer_context;
	gth_shortcut_set_accelerator (new_shortcut, shortcut->accelerator);

	return new_shortcut;
}


void
gth_shortcut_free (GthShortcut *shortcut)
{
	g_free (shortcut->action_name);
	if (shortcut->action_parameter != NULL)
		g_variant_unref (shortcut->action_parameter);
	g_free (shortcut->detailed_action);
	g_free (shortcut->description);
	g_free (shortcut->default_accelerator);
	g_free (shortcut->accelerator);
	g_free (shortcut->label);
	g_free (shortcut);
}


void
gth_shortcut_set_accelerator (GthShortcut *shortcut,
			      const char  *accelerator)
{
	guint           keyval;
	GdkModifierType modifiers;

	keyval = 0;
	modifiers = 0;

	if ((shortcut->context & GTH_SHORTCUT_CONTEXT_DOC) == 0) {
		if (accelerator != NULL)
			gtk_accelerator_parse (accelerator, &keyval, &modifiers);
		gth_shortcut_set_key (shortcut, keyval, modifiers);
	}
	else {
		shortcut->keyval = keyval;
		shortcut->modifiers = modifiers;
		shortcut->accelerator = g_strdup (accelerator);
		shortcut->label = g_strdup (accelerator);
	}
}


void
gth_shortcut_set_viewer_context (GthShortcut *shortcut,
				 const char  *viewer_context)
{
	shortcut->viewer_context = viewer_context;
}


void
gth_shortcut_set_key (GthShortcut       *shortcut,
		      guint              keyval,
		      GdkModifierType    modifiers)
{
	g_free (shortcut->accelerator);
	g_free (shortcut->label);

	shortcut->keyval = keyval;
	shortcut->modifiers = modifiers;
	shortcut->accelerator = gtk_accelerator_name (shortcut->keyval, shortcut->modifiers);
	shortcut->label = gtk_accelerator_get_label (shortcut->keyval, shortcut->modifiers);
}


gboolean
gth_shortcut_customizable (GthShortcut *shortcut)
{
	return ((shortcut->context & GTH_SHORTCUT_CONTEXT_FIXED) == 0)
		&& ((shortcut->context & GTH_SHORTCUT_CONTEXT_INTERNAL) == 0)
		&& ((shortcut->context & GTH_SHORTCUT_CONTEXT_DOC) == 0);
}


static gboolean
_gth_shortcut_for_different_viewer (GthShortcut *shortcut,
				    int          context,
				    const char  *viewer_context)
{
	/* Two shortcuts can be equal if they both belong to the viewer context
	 * but have different categories.
	 * For example the image viewer shortcuts do not collide with the video
	 * viewer shortcuts. */
	return ((context & GTH_SHORTCUT_CONTEXT_VIEWER) != 0)
			&& ((shortcut->context & GTH_SHORTCUT_CONTEXT_VIEWER) != 0)
			&& (viewer_context != NULL)
			&& (shortcut->viewer_context != NULL)
			&& ! _g_str_equal (shortcut->viewer_context, viewer_context);
}


GthShortcut *
gth_shortcut_array_find (GPtrArray       *shortcuts_v,
			 int              context,
			 const char      *viewer_context,
			 guint            keycode,
			 GdkModifierType  modifiers)
{
	int i;

	if (keycode == 0)
		return NULL;

	/* Remove flags not related to the window mode. */
	context = context & GTH_SHORTCUT_CONTEXT_ANY;
	keycode = gdk_keyval_to_lower (keycode);

	for (i = 0; i < shortcuts_v->len; i++) {
		GthShortcut *shortcut = g_ptr_array_index (shortcuts_v, i);

		if (((shortcut->context & context) != 0)
			&& (shortcut->keyval == keycode)
			&& (shortcut->modifiers == modifiers))
		{
			if (! _gth_shortcut_for_different_viewer (shortcut, context, viewer_context))
				return shortcut;
		}
	}

	return NULL;
}


GthShortcut *
gth_shortcut_array_find_by_accel (GPtrArray  *shortcuts_v,
				  int         context,
				  const char *viewer_context,
				  const char *accelerator)
{
	int i;

	if (accelerator == NULL)
		return NULL;

	/* Remove flags not related to the window mode. */
	context = context & GTH_SHORTCUT_CONTEXT_ANY;

	for (i = 0; i < shortcuts_v->len; i++) {
		GthShortcut *shortcut = g_ptr_array_index (shortcuts_v, i);

		if (((shortcut->context & context) != 0)
			&& (g_strcmp0 (shortcut->accelerator, accelerator) == 0))
		{
			if (! _gth_shortcut_for_different_viewer (shortcut, context, viewer_context))
				return shortcut;
		}
	}

	return NULL;
}


GthShortcut *
gth_shortcut_array_find_by_action (GPtrArray  *shortcuts_v,
				   const char *detailed_action)
{
	int i;

	if (detailed_action == NULL)
		return NULL;

	for (i = 0; i < shortcuts_v->len; i++) {
		GthShortcut *shortcut = g_ptr_array_index (shortcuts_v, i);

		if (g_strcmp0 (shortcut->detailed_action, detailed_action) == 0)
			return shortcut;
	}

	return NULL;
}


gboolean
gth_shortcut_valid (guint           keycode,
		    GdkModifierType modifiers)
{
	switch (keycode) {
	case GDK_KEY_Escape:
	case GDK_KEY_Tab:
		return FALSE;

	/* These shortcuts are valid for us but gtk_accelerator_valid
	 * considers them not valid, hence the are added here
	 * explicitly. */

	case GDK_KEY_Left:
	case GDK_KEY_Right:
	case GDK_KEY_Up:
	case GDK_KEY_Down:
	case GDK_KEY_KP_Left:
	case GDK_KEY_KP_Right:
	case GDK_KEY_KP_Up:
	case GDK_KEY_KP_Down:
		return TRUE;

	default:
		return gtk_accelerator_valid (keycode, modifiers);
	}

	return FALSE;
}


gboolean
gth_shortcuts_write_to_file (GPtrArray  *shortcuts_v,
			     GError    **error)
{
	DomDocument *doc;
	DomElement  *shortcuts;
	int          i;
	char        *buffer;
	gsize        size;
	GFile       *file;
	gboolean     result;

	doc = dom_document_new ();
	shortcuts = dom_document_create_element (doc, "shortcuts", NULL);
	for (i = 0; i < shortcuts_v->len; i++) {
		GthShortcut *shortcut = g_ptr_array_index (shortcuts_v, i);

		if ((shortcut->context & GTH_SHORTCUT_CONTEXT_INTERNAL) != 0)
			continue;

		if ((shortcut->context & GTH_SHORTCUT_CONTEXT_FIXED) != 0)
			continue;

		if ((shortcut->context & GTH_SHORTCUT_CONTEXT_DOC) != 0)
			continue;

		dom_element_append_child (shortcuts,
			dom_document_create_element (doc, "shortcut",
						     "action", shortcut->detailed_action,
						     "accelerator", shortcut->accelerator,
						     NULL));
	}
	dom_element_append_child (DOM_ELEMENT (doc), shortcuts);

	buffer = dom_document_dump (doc, &size);
	file = gth_user_dir_get_file_for_write (GTH_DIR_CONFIG, GTHUMB_DIR, SHORTCUTS_FILE, NULL);
	result = _g_file_write (file, FALSE, G_FILE_CREATE_NONE, buffer, size, NULL, error);

	g_object_unref (file);
	g_free (buffer);
	g_object_unref (doc);

	return result;
}


gboolean
gth_shortcuts_load_from_file (GPtrArray  *shortcuts_v,
			      GHashTable *shortcuts,
			      GError    **error)
{
	gboolean  success = FALSE;
	GFile    *file;
	void     *buffer;
	gsize     size;

	file = gth_user_dir_get_file_for_write (GTH_DIR_CONFIG, GTHUMB_DIR, SHORTCUTS_FILE, NULL);
	if (_g_file_load_in_buffer (file, &buffer, &size, NULL, error)) {
		DomDocument *doc;

		doc = dom_document_new ();
		if (dom_document_load (doc, buffer, size, error)) {
			DomElement *node;

			for (node = DOM_ELEMENT (doc)->first_child; node; node = node->next_sibling) {
				if (g_strcmp0 (node->tag_name, "shortcuts") == 0) {
					DomElement *shortcut_node;

					for (shortcut_node = node->first_child; shortcut_node; shortcut_node = shortcut_node->next_sibling) {
						if (g_strcmp0 (shortcut_node->tag_name, "shortcut") == 0) {
							const char  *detailed_action;
							const char  *accelerator;
							GthShortcut *shortcut;

							detailed_action = dom_element_get_attribute (shortcut_node, "action");
							accelerator = dom_element_get_attribute (shortcut_node, "accelerator");

							if (detailed_action == NULL)
								continue;

							shortcut = g_hash_table_lookup (shortcuts, detailed_action);
							if (shortcut != NULL)
								gth_shortcut_set_accelerator (shortcut, accelerator);
						}
					}

					success = TRUE;
				}
			}
		}

		if (! success && (error != NULL))
			*error = g_error_new_literal (DOM_ERROR, DOM_ERROR_INVALID_FORMAT, _("Invalid file format"));

		g_object_unref (doc);
		g_free (buffer);
	}

	g_object_unref (file);

	return success;
}
