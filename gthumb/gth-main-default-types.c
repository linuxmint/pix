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
#include <glib/gi18n.h>
#include "dlg-preferences-browser.h"
#include "dlg-preferences-extensions.h"
#include "dlg-preferences-general.h"
#include "dlg-preferences-shortcuts.h"
#include "gth-file-comment.h"
#include "gth-file-details.h"
#include "gth-file-properties.h"
#include "gth-main.h"
#include "pixbuf-io.h"


static GthShortcutCategory shortcut_categories[] = {
	{ GTH_SHORTCUT_CATEGORY_HIDDEN, NULL, 0 },
	{ GTH_SHORTCUT_CATEGORY_GENERAL, N_("General"), 31 },
	{ GTH_SHORTCUT_CATEGORY_UI, N_("Show/Hide"), 30 },
	{ GTH_SHORTCUT_CATEGORY_NAVIGATION, N_("Navigation"), 12 },
	{ GTH_SHORTCUT_CATEGORY_FILE_MANAGER, N_("File Manager"), 10 },
	{ GTH_SHORTCUT_CATEGORY_VIEWER, N_("Viewer"), 20 },
	{ GTH_SHORTCUT_CATEGORY_FILTERS, N_("Filters"), 13 },
};


static void
gth_main_register_default_file_loader (void)
{
	GSList *formats;
	GSList *scan;

	formats = gdk_pixbuf_get_formats ();
	for (scan = formats; scan; scan = scan->next) {
		GdkPixbufFormat  *format = scan->data;
		char            **mime_types;
		int               i;

		if (gdk_pixbuf_format_is_disabled (format))
			continue;

		mime_types = gdk_pixbuf_format_get_mime_types (format);
		for (i = 0; mime_types[i] != NULL; i++)
			gth_main_register_image_loader_func (gth_pixbuf_animation_new_from_file,
							     (g_content_type_is_a (mime_types[i], "image/gif") ? GTH_IMAGE_FORMAT_GDK_PIXBUF_ANIMATION : GTH_IMAGE_FORMAT_GDK_PIXBUF),
							     mime_types[i],
							     NULL);

		g_strfreev (mime_types);
	}

	g_slist_free (formats);
}


void
gth_main_register_default_types (void)
{
	gth_main_register_type ("file-properties", GTH_TYPE_FILE_PROPERTIES);
	gth_main_register_type ("file-properties", GTH_TYPE_FILE_COMMENT);
	gth_main_register_type ("file-properties", GTH_TYPE_FILE_DETAILS);
	gth_main_register_default_file_loader ();
	gth_main_register_shortcut_category (shortcut_categories, G_N_ELEMENTS (shortcut_categories));
	gth_hook_add_callback ("dlg-preferences-construct", 1, G_CALLBACK (general__dlg_preferences_construct_cb), NULL);
	gth_hook_add_callback ("dlg-preferences-apply", 1, G_CALLBACK (general__dlg_preferences_apply), NULL);
	gth_hook_add_callback ("dlg-preferences-construct", 2, G_CALLBACK (browser__dlg_preferences_construct_cb), NULL);
	gth_hook_add_callback ("dlg-preferences-apply", 2, G_CALLBACK (browser__dlg_preferences_apply), NULL);
	gth_hook_add_callback ("dlg-preferences-construct", 50, G_CALLBACK (shortcuts__dlg_preferences_construct_cb), NULL);
	gth_hook_add_callback ("dlg-preferences-construct", 9999, G_CALLBACK (extensions__dlg_preferences_construct_cb), NULL);
	gth_hook_add_callback ("dlg-preferences-apply", 9999 /* Must be the last callback */, G_CALLBACK (extensions__dlg_preferences_apply), NULL);
}
