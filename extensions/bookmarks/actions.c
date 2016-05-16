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
#include <gthumb.h>
#include "dlg-bookmarks.h"


void
gth_browser_activate_action_bookmarks_add (GtkAction  *action,
					   GthBrowser *browser)
{
	GBookmarkFile *bookmarks;
	GFile         *location;
	char          *uri;

	location = gth_browser_get_location (browser);
	if (location == NULL)
		return;

	bookmarks = gth_main_get_default_bookmarks ();
	uri = g_file_get_uri (location);
	_g_bookmark_file_add_uri (bookmarks, uri);
	gth_main_bookmarks_changed ();

	g_free (uri);
}


void
gth_browser_activate_action_bookmarks_edit (GtkAction  *action,
					    GthBrowser *browser)
{
	dlg_bookmarks (browser);
}
