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
#include <glib/gi18n.h>
#include <gthumb.h>
#include <extensions/flicker_utils/dlg-export-to-flickr.h>
#include <extensions/flicker_utils/dlg-import-from-flickr.h>
#include <extensions/flicker_utils/flickr-types.h>
#include "actions.h"


static FlickrServer www_flickr_com = {
	"Flickr",
	"flickr",
	"https://www.flickr.com",
	"https",

	"https://www.flickr.com/services/oauth/request_token",
	"https://www.flickr.com/services/oauth/authorize",
	"https://www.flickr.com/services/oauth/access_token",
	"8960706ee7f4151e893b11837e9c24ce",
	"1ff8d1e45c873423",

	"https://api.flickr.com/services/rest",
	"https://up.flickr.com/services/upload",
	"static.flickr.com",
	FALSE,
	TRUE
};


void
gth_browser_activate_export_flickr (GSimpleAction	*action,
				    GVariant		*parameter,
				    gpointer		 user_data)
{
	GthBrowser *browser = GTH_BROWSER (user_data);
	GList      *items;
	GList      *file_list;

	items = gth_file_selection_get_selected (GTH_FILE_SELECTION (gth_browser_get_file_list_view (browser)));
	file_list = gth_file_list_get_files (GTH_FILE_LIST (gth_browser_get_file_list (browser)), items);
	if (file_list == NULL)
		file_list = gth_file_store_get_visibles (gth_browser_get_file_store (browser));
	dlg_export_to_flickr (&www_flickr_com, browser, file_list);

	_g_object_list_unref (file_list);
	_gtk_tree_path_list_free (items);
}


void
gth_browser_activate_import_flickr (GSimpleAction	*action,
				    GVariant		*parameter,
				    gpointer		 user_data)
{
	dlg_import_from_flickr (&www_flickr_com, GTH_BROWSER (user_data));
}
