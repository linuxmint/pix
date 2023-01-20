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
#include <gtk/gtk.h>
#include <gthumb.h>
#include "callbacks.h"
#include "gth-edit-general-page.h"


G_MODULE_EXPORT void
gthumb_extension_activate (void)
{
	/**
	 * Called to delete a file metadata
	 *
	 * @file (GFile *): the file
	 * @buffer (void **buffer): the file content
	 * @size (gsize *): the file size
	 **/
	gth_hook_register ("delete-metadata", 3);

	gth_main_register_type ("edit-comment-dialog-page", GTH_TYPE_EDIT_GENERAL_PAGE);
	gth_hook_add_callback ("gth-browser-construct", 7, G_CALLBACK (edit_metadata__gth_browser_construct_cb), NULL);
	gth_hook_add_callback ("gth-browser-selection-changed", 10, G_CALLBACK (edit_metadata__gth_browser_selection_changed_cb), NULL);
}


G_MODULE_EXPORT void
gthumb_extension_deactivate (void)
{
}


G_MODULE_EXPORT gboolean
gthumb_extension_is_configurable (void)
{
	return FALSE;
}


G_MODULE_EXPORT void
gthumb_extension_configure (GtkWindow *parent)
{
}
