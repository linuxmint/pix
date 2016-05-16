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
#include "dlg-preferences-extensions.h"
#include "gth-file-properties.h"
#include "gth-main.h"
#include "pixbuf-io.h"


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
}


void
gth_main_register_default_types (void)
{
	gth_main_register_type ("file-properties", GTH_TYPE_FILE_PROPERTIES);
	gth_main_register_default_file_loader ();
	gth_hook_add_callback ("dlg-preferences-construct", 9999, G_CALLBACK (extensions__dlg_preferences_construct_cb), NULL);
	gth_hook_add_callback ("dlg-preferences-apply", 9999 /* Must be the last callback */, G_CALLBACK (extensions__dlg_preferences_apply), NULL);
}
